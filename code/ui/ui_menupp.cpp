/*
===========================================================================
Copyright (C) 2026 JKEnhanced

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// ui_menupp.cpp -- a small text preprocessor for .menu files (JKEnhanced)
//
// The SP menu parser has no #include / #define. Every file read through PC_StartParseSession
// (menus, menu lists, the HUD) passes through here first. Files without a line starting with '#'
// are left alone, so stock menus parse exactly as before.
//
// Supported, C-style:
//   #include "ui/common.menu"      (path from the game root; a bare name is also tried next to
//                                   the including file)
//   #define NAME text...           object-like; '\' at the end of a line continues it
//   #define NAME(a, b) text...     function-like; #a turns an argument into a "string",
//                                   a ## b pastes two pieces together
//   #undef NAME
//   #ifdef NAME / #ifndef NAME / #else / #endif
//
// Nothing is replaced inside "strings" or comments. Arguments may contain { } blocks with commas
// in them. Names are case-sensitive; UPPERCASE names avoid clashing with menu keywords.

// leave this at the top of all UI_xxxx files for PCH reasons...
#include "../server/exe_headers.h"

#include "ui_local.h"

#include <string>
#include <vector>
#include <map>

namespace
{
	const int MAX_INCLUDE_DEPTH	= 8;
	const int MAX_EXPAND_DEPTH	= 64;
	const int MAX_TRACKED		= 16;

	struct menuMacro_t
	{
		bool						function;
		std::vector<std::string>	params;
		std::string					body;
	};

	struct menuPP_t
	{
		std::map<std::string, menuMacro_t>	macros;
		std::vector<std::string>			expanding;	// macros currently being expanded (no self-recursion)
		int									warnings;
	};

	// buffers handed out by UI_PreprocessMenuFile, freed by UI_FreePreprocessedMenu
	char *s_tracked[MAX_TRACKED];

	inline bool IsIdentStart( char c ) { return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || c == '_'; }
	inline bool IsIdentChar( char c ) { return IsIdentStart( c ) || ( c >= '0' && c <= '9' ); }
	inline bool IsSpace( char c ) { return c == ' ' || c == '\t' || c == '\r'; }

	void PP_Warning( menuPP_t &pp, const char *file, int line, const char *msg )
	{
		pp.warnings++;
		Com_Printf( S_COLOR_YELLOW "WARNING: %s (%s line %d)\n", msg, file, line );
	}

	// copy a "string" starting at p (on the opening quote)
	void CopyString( const char *&p, const char *end, std::string &out )
	{
		out += *p++;
		while ( p < end && *p != '"' && *p != '\n' )
		{
			out += *p++;
		}
		if ( p < end && *p == '"' )
		{
			out += *p++;
		}
	}

	// copy a comment starting at p (on the '/'); returns false if p isn't a comment
	bool CopyComment( const char *&p, const char *end, std::string &out )
	{
		if ( p + 1 >= end || p[0] != '/' )
		{
			return false;
		}
		if ( p[1] == '/' )
		{
			while ( p < end && *p != '\n' )
			{
				out += *p++;
			}
			return true;
		}
		if ( p[1] == '*' )
		{
			out += *p++;
			out += *p++;
			while ( p < end && !( p[0] == '*' && p + 1 < end && p[1] == '/' ) )
			{
				out += *p++;
			}
			if ( p < end )
			{
				out += *p++;
				out += *p++;
			}
			return true;
		}
		return false;
	}

	std::string Trim( const std::string &s )
	{
		size_t a = 0, b = s.size();
		while ( a < b && ( IsSpace( s[a] ) || s[a] == '\n' ) ) a++;
		while ( b > a && ( IsSpace( s[b - 1] ) || s[b - 1] == '\n' ) ) b--;
		return s.substr( a, b - a );
	}

	// collect "( a, b, ... )" starting at p (on the '('). Commas only split at depth 0; ( ) { } [ ]
	// nest, and strings/comments are kept whole.
	bool ReadArgs( const char *&p, const char *end, std::vector<std::string> &args )
	{
		int depth = 0;
		std::string cur;

		p++;	// '('
		while ( p < end )
		{
			const char c = *p;
			if ( c == '"' )
			{
				CopyString( p, end, cur );
				continue;
			}
			if ( c == '/' && CopyComment( p, end, cur ) )
			{
				continue;
			}
			if ( c == '(' || c == '{' || c == '[' )
			{
				depth++;
			}
			else if ( c == ')' || c == '}' || c == ']' )
			{
				if ( depth == 0 && c == ')' )
				{
					p++;
					args.push_back( Trim( cur ) );
					return true;
				}
				depth--;
			}
			else if ( c == ',' && depth == 0 )
			{
				args.push_back( Trim( cur ) );
				cur.clear();
				p++;
				continue;
			}
			cur += c;
			p++;
		}
		return false;
	}

	// #param -> "param text" (an argument that's already quoted is used as it is)
	std::string Stringize( const std::string &arg )
	{
		if ( arg.size() >= 2 && arg[0] == '"' && arg[arg.size() - 1] == '"' )
		{
			return arg;
		}
		std::string s = "\"";
		for ( size_t i = 0; i < arg.size(); i++ )
		{
			const char c = arg[i];
			s += ( c == '"' ) ? '\'' : ( c == '\n' ? ' ' : c );
		}
		s += "\"";
		return s;
	}

	void Expand( menuPP_t &pp, const char *file, const char *p, const char *end, std::string &out, int depth );

	// substitute the arguments into a function-like macro's body
	std::string Substitute( const menuMacro_t &m, const std::vector<std::string> &args )
	{
		std::string out;
		const char *p = m.body.c_str();
		const char *end = p + m.body.size();

		while ( p < end )
		{
			const char c = *p;
			if ( c == '"' )
			{
				CopyString( p, end, out );
				continue;
			}
			if ( c == '#' && p + 1 < end && p[1] == '#' )
			{	// paste: drop the whitespace on both sides
				while ( !out.empty() && IsSpace( out[out.size() - 1] ) ) out.erase( out.size() - 1 );
				p += 2;
				while ( p < end && IsSpace( *p ) ) p++;
				continue;
			}
			if ( c == '#' && p + 1 < end && IsIdentStart( p[1] ) )
			{	// stringize a parameter
				const char *s = p + 1;
				const char *e = s;
				while ( e < end && IsIdentChar( *e ) ) e++;
				const std::string name( s, e - s );
				size_t i;
				for ( i = 0; i < m.params.size() && m.params[i] != name; i++ ) {}
				if ( i < m.params.size() )
				{
					out += Stringize( args[i] );
					p = e;
					continue;
				}
			}
			if ( IsIdentStart( c ) )
			{
				const char *s = p;
				while ( p < end && IsIdentChar( *p ) ) p++;
				const std::string name( s, p - s );
				size_t i;
				for ( i = 0; i < m.params.size() && m.params[i] != name; i++ ) {}
				out += ( i < m.params.size() ) ? args[i] : name;
				continue;
			}
			out += c;
			p++;
		}
		return out;
	}

	// expand macros in [p, end) into out (no directives here)
	void Expand( menuPP_t &pp, const char *file, const char *p, const char *end, std::string &out, int depth )
	{
		while ( p < end )
		{
			const char c = *p;
			if ( c == '"' )
			{
				CopyString( p, end, out );
				continue;
			}
			if ( c == '/' && CopyComment( p, end, out ) )
			{
				continue;
			}
			if ( c >= '0' && c <= '9' )
			{	// numbers and things like 2x: never start a macro name mid-word
				while ( p < end && ( IsIdentChar( *p ) || *p == '.' ) ) out += *p++;
				continue;
			}
			if ( !IsIdentStart( c ) )
			{
				out += c;
				p++;
				continue;
			}

			const char *s = p;
			while ( p < end && IsIdentChar( *p ) ) p++;
			const std::string name( s, p - s );

			std::map<std::string, menuMacro_t>::const_iterator it = pp.macros.find( name );
			bool busy = false;
			for ( size_t i = 0; i < pp.expanding.size(); i++ )
			{
				if ( pp.expanding[i] == name ) busy = true;
			}
			if ( it == pp.macros.end() || busy || depth >= MAX_EXPAND_DEPTH )
			{
				out += name;
				continue;
			}

			const menuMacro_t &m = it->second;
			std::string text;
			if ( m.function )
			{
				const char *q = p;
				while ( q < end && ( IsSpace( *q ) || *q == '\n' ) ) q++;
				if ( q >= end || *q != '(' )
				{	// a function-like name without arguments is left as it is
					out += name;
					continue;
				}
				std::vector<std::string> args;
				if ( !ReadArgs( q, end, args ) )
				{
					PP_Warning( pp, file, 0, va( "macro %s: missing ')'", name.c_str() ) );
					out += name;
					continue;
				}
				if ( args.size() == 1 && args[0].empty() && m.params.empty() )
				{
					args.clear();
				}
				if ( args.size() != m.params.size() )
				{
					PP_Warning( pp, file, 0, va( "macro %s takes %d arguments, got %d", name.c_str(), (int)m.params.size(), (int)args.size() ) );
					out += name;
					continue;
				}
				p = q;
				text = Substitute( m, args );
			}
			else
			{
				text = m.body;
			}

			pp.expanding.push_back( name );
			Expand( pp, file, text.c_str(), text.c_str() + text.size(), out, depth + 1 );
			pp.expanding.pop_back();
		}
	}

	bool ProcessFile( menuPP_t &pp, const char *file, const char *text, int len, std::string &out, int includeDepth );

	// read a directive's whole logical line (with '\' continuations); p is just past the '#'
	std::string ReadDirectiveLine( const char *&p, const char *end, int &lines )
	{
		std::string line;
		while ( p < end && *p != '\n' )
		{
			if ( *p == '\\' )
			{	// continuation: backslash, optional spaces / \r, newline
				const char *q = p + 1;
				while ( q < end && IsSpace( *q ) ) q++;
				if ( q < end && *q == '\n' )
				{
					line += '\n';
					lines++;
					p = q + 1;
					continue;
				}
			}
			line += *p++;
		}
		if ( p < end )
		{
			p++;	// '\n'
			lines++;
		}
		return line;
	}

	void ParseDefine( menuPP_t &pp, const char *file, int lineNo, const std::string &rest )
	{
		const char *p = rest.c_str();
		const char *end = p + rest.size();
		while ( p < end && IsSpace( *p ) ) p++;
		if ( p >= end || !IsIdentStart( *p ) )
		{
			PP_Warning( pp, file, lineNo, "#define without a name" );
			return;
		}
		const char *s = p;
		while ( p < end && IsIdentChar( *p ) ) p++;
		const std::string name( s, p - s );

		menuMacro_t m;
		m.function = false;
		if ( p < end && *p == '(' )
		{	// parameters (no space between the name and '(')
			m.function = true;
			p++;
			std::string cur;
			while ( p < end && *p != ')' )
			{
				if ( *p == ',' )
				{
					m.params.push_back( Trim( cur ) );
					cur.clear();
				}
				else
				{
					cur += *p;
				}
				p++;
			}
			if ( p >= end )
			{
				PP_Warning( pp, file, lineNo, va( "#define %s: missing ')'", name.c_str() ) );
				return;
			}
			p++;
			cur = Trim( cur );
			if ( !cur.empty() || !m.params.empty() )
			{
				m.params.push_back( cur );
			}
		}

		// body: drop // comments (the body may span several lines through '\')
		std::string body;
		while ( p < end )
		{
			if ( *p == '"' )
			{
				CopyString( p, end, body );
				continue;
			}
			if ( p[0] == '/' && p + 1 < end && p[1] == '/' )
			{
				while ( p < end && *p != '\n' ) p++;
				continue;
			}
			body += *p++;
		}
		m.body = Trim( body );
		pp.macros[name] = m;
	}

	bool IncludeFile( menuPP_t &pp, const char *file, int lineNo, const std::string &rest, std::string &out, int includeDepth )
	{
		std::string arg = Trim( rest );
		if ( arg.size() < 2 || !( ( arg[0] == '"' && arg[arg.size() - 1] == '"' ) || ( arg[0] == '<' && arg[arg.size() - 1] == '>' ) ) )
		{
			PP_Warning( pp, file, lineNo, "#include needs a \"path\"" );
			return false;
		}
		std::string path = arg.substr( 1, arg.size() - 2 );
		if ( includeDepth >= MAX_INCLUDE_DEPTH )
		{
			PP_Warning( pp, file, lineNo, va( "#include \"%s\": nested too deep", path.c_str() ) );
			return false;
		}

		char *buf = NULL;
		int len = ui.FS_ReadFile( path.c_str(), (void **)&buf );
		if ( len <= 0 && path.find( '/' ) == std::string::npos )
		{	// a bare name: try next to the including file
			std::string dir( file );
			const size_t slash = dir.find_last_of( "/\\" );
			if ( slash != std::string::npos )
			{
				path = dir.substr( 0, slash + 1 ) + path;
				len = ui.FS_ReadFile( path.c_str(), (void **)&buf );
			}
		}
		if ( len <= 0 || !buf )
		{
			PP_Warning( pp, file, lineNo, va( "#include \"%s\": file not found", path.c_str() ) );
			return false;
		}
		ProcessFile( pp, path.c_str(), buf, len, out, includeDepth + 1 );
		ui.FS_FreeFile( buf );
		return true;
	}

	// directives + expansion over a whole file
	bool ProcessFile( menuPP_t &pp, const char *file, const char *text, int len, std::string &out, int includeDepth )
	{
		const char *p = text;
		const char *end = text + len;
		int lineNo = 1;
		// conditional stack: each entry is whether that level is active, plus whether its #else ran
		std::vector<bool> active, sawElse;

		while ( p < end )
		{
			// one line at a time
			const char *lineStart = p;
			const char *q = p;
			while ( q < end && IsSpace( *q ) ) q++;

			bool live = true;
			for ( size_t i = 0; i < active.size(); i++ )
			{
				if ( !active[i] ) live = false;
			}

			if ( q < end && *q == '#' )
			{
				const int at = lineNo;
				p = q + 1;
				int lines = 0;
				const std::string line = ReadDirectiveLine( p, end, lines );
				lineNo += lines;
				const char *d = line.c_str();
				while ( *d && IsSpace( *d ) ) d++;
				const char *de = d;
				while ( *de && IsIdentChar( *de ) ) de++;
				const std::string directive( d, de - d );
				const std::string rest( de );
				const std::string restName = Trim( rest );

				if ( directive == "ifdef" || directive == "ifndef" )
				{
					const bool defined = pp.macros.find( restName ) != pp.macros.end();
					active.push_back( directive == "ifdef" ? defined : !defined );
					sawElse.push_back( false );
				}
				else if ( directive == "else" )
				{
					if ( active.empty() || sawElse.back() )
					{
						PP_Warning( pp, file, at, "#else without #ifdef" );
					}
					else
					{
						active.back() = !active.back();
						sawElse.back() = true;
					}
				}
				else if ( directive == "endif" )
				{
					if ( active.empty() )
					{
						PP_Warning( pp, file, at, "#endif without #ifdef" );
					}
					else
					{
						active.pop_back();
						sawElse.pop_back();
					}
				}
				else if ( !live )
				{
					// skipped
				}
				else if ( directive == "define" )
				{
					ParseDefine( pp, file, at, rest );
				}
				else if ( directive == "undef" )
				{
					pp.macros.erase( restName );
				}
				else if ( directive == "include" )
				{
					IncludeFile( pp, file, at, rest, out, includeDepth );
				}
				else
				{
					PP_Warning( pp, file, at, va( "unknown directive #%s", directive.c_str() ) );
				}

				// keep the line count of the outer file roughly right for parse warnings
				if ( includeDepth == 0 )
				{
					out.append( lines, '\n' );
				}
				continue;
			}

			if ( !live )
			{
				while ( p < end && *p != '\n' ) p++;
				if ( p < end ) { p++; lineNo++; }
				if ( includeDepth == 0 ) out += '\n';
				continue;
			}

			// an ordinary line. A function-like macro's arguments may run over several lines, so
			// expansion is given the rest of the file and reports how far it got: expand one line,
			// unless a macro call starting on it continues past the newline.
			const char *eol = lineStart;
			while ( eol < end && *eol != '\n' ) eol++;
			const char *stop = ( eol < end ) ? eol + 1 : eol;

			// a '(' left open on this line (a macro call whose arguments continue below): only parentheses
			// count, so a menuDef { ... } block is still processed line by line (directives inside work)
			{
				const char *s = lineStart;
				int depth = 0;
				bool open = false;
				while ( s < eol )
				{
					if ( *s == '"' ) { std::string dummy; CopyString( s, eol, dummy ); continue; }
					if ( *s == '/' && s + 1 < eol && s[1] == '/' ) break;
					if ( *s == '(' ) depth++;
					else if ( *s == ')' ) depth--;
					s++;
				}
				open = depth > 0;
				if ( open )
				{	// run on until the brackets balance (or the file ends)
					const char *t = eol;
					while ( t < end && depth > 0 )
					{
						if ( *t == '"' ) { std::string dummy; CopyString( t, end, dummy ); continue; }
						if ( *t == '/' && t + 1 < end && t[1] == '/' ) { while ( t < end && *t != '\n' ) t++; continue; }
						if ( *t == '/' && t + 1 < end && t[1] == '*' ) { std::string dummy; CopyComment( t, end, dummy ); continue; }
						if ( *t == '(' ) depth++;
						else if ( *t == ')' ) depth--;
						t++;
					}
					while ( t < end && *t != '\n' ) t++;
					stop = ( t < end ) ? t + 1 : t;
				}
			}

			Expand( pp, file, lineStart, stop, out, 0 );
			for ( const char *c = lineStart; c < stop; c++ )
			{
				if ( *c == '\n' ) lineNo++;
			}
			p = stop;
		}

		if ( !active.empty() )
		{
			PP_Warning( pp, file, lineNo, "missing #endif" );
		}
		return true;
	}

	bool NeedsPreprocess( const char *text, int len )
	{
		bool lineStart = true;
		for ( int i = 0; i < len; i++ )
		{
			const char c = text[i];
			if ( c == '\n' )
			{
				lineStart = true;
			}
			else if ( lineStart && c == '#' )
			{
				return true;
			}
			else if ( !IsSpace( c ) )
			{
				lineStart = false;
			}
		}
		return false;
	}
}

/*
===============
UI_PreprocessMenuFile

Returns a new buffer (free it with UI_FreePreprocessedMenu) and its length, or NULL when the file
has no directives and should be used as it is.
===============
*/
char *UI_PreprocessMenuFile( const char *fileName, const char *text, int len, int *outLen )
{
	if ( !text || len <= 0 || !NeedsPreprocess( text, len ) )
	{
		return NULL;
	}

	menuPP_t pp;
	pp.warnings = 0;
	std::string out;
	out.reserve( len * 2 );
	ProcessFile( pp, fileName, text, len, out, 0 );

	int slot;
	for ( slot = 0; slot < MAX_TRACKED && s_tracked[slot]; slot++ ) {}
	if ( slot == MAX_TRACKED )
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: UI_PreprocessMenuFile: too many open files, %s not preprocessed\n", fileName );
		return NULL;
	}

	char *buf = (char *)Z_Malloc( (int)out.size() + 1, TAG_TEMP_WORKSPACE, qfalse );
	memcpy( buf, out.c_str(), out.size() );
	buf[out.size()] = '\0';
	s_tracked[slot] = buf;
	if ( outLen )
	{
		*outLen = (int)out.size();
	}
	return buf;
}

/*
===============
UI_FreePreprocessedMenu

Frees a buffer from UI_PreprocessMenuFile. Returns qfalse if it isn't one (free it as a file).
===============
*/
qboolean UI_FreePreprocessedMenu( char *buffer )
{
	for ( int i = 0; i < MAX_TRACKED; i++ )
	{
		if ( buffer && s_tracked[i] == buffer )
		{
			Z_Free( buffer );
			s_tracked[i] = NULL;
			return qtrue;
		}
	}
	return qfalse;
}
