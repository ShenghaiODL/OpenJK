/*
===========================================================================
JKEnhanced: difficulty profile (Step 3F)

ext_data/difficulty.cfg:

	skill 0
	{
		npcBoltDamage		0.4
		weaponSkillShift	-1
		...
	}
	skill 1 { ... }
	skill 2 { ... }		// Jedi Knight
	skill 3 { ... }		// Jedi Master (g_spskill 2 + handicap 50); missing = built-in Master defaults

Any key or block left out keeps its built-in default (the numbers below, which match what the
code did before the profile existed, plus the new parry/nerve/guard/heavy factors).
===========================================================================
*/

#include "g_local.h"
#include "g_difficulty.h"

extern cvar_t	*g_spskill;

#define NUM_DIFFICULTY_PROFILES	4

static const difficultyProfile_t s_defaultProfiles[NUM_DIFFICULTY_PROFILES] =
{
	//	bolt	wpnSkill	saber	parry	nerve	guard	heavyCD	coreCut
	{	0.4f,	-1,			0.75f,	1.2f,	-10,	0.85f,	1.4f,	0.5f	},	// 0: easy
	{	0.6f,	0,			1.0f,	1.0f,	0,		1.0f,	1.0f,	0.7f	},	// 1: normal
	{	0.8f,	1,			1.25f,	0.9f,	5,		1.15f,	0.85f,	0.85f	},	// 2: hard (Jedi Knight)
	{	0.9f,	1,			1.35f,	0.85f,	8,		1.25f,	0.75f,	1.0f	},	// 3: Jedi Master (g_spskill 2 + handicap 50)
};

static cvar_t	*s_handicap = NULL;

qboolean G_IsJediMaster( void )
{
	if ( !g_spskill )
	{
		return qfalse;
	}
	if ( g_spskill->integer >= 3 )
	{
		return qtrue;
	}
	if ( g_spskill->integer < 2 )
	{
		return qfalse;
	}
	if ( !s_handicap )
	{
		s_handicap = gi.cvar( "handicap", "100", CVAR_USERINFO|CVAR_SAVEGAME );
	}
	return (qboolean)( s_handicap && s_handicap->integer > 0 && s_handicap->integer <= 50 );
}

int G_DifficultyLevel( void )
{
	if ( G_IsJediMaster() )
	{
		return 3;
	}
	const int skill = g_spskill ? g_spskill->integer : 1;
	return skill <= 0 ? 0 : ( skill >= 2 ? 2 : skill );
}

static difficultyProfile_t	s_profiles[NUM_DIFFICULTY_PROFILES];
static qboolean				s_profilesLoaded = qfalse;

static void G_ParseDifficultyBlock( const char **p, difficultyProfile_t *prof )
{
	const char *token = COM_ParseExt( p, qtrue );
	if ( Q_stricmp( token, "{" ) )
	{
		Com_Printf( S_COLOR_YELLOW"difficulty.cfg: expected { after skill N\n" );
		return;
	}
	while ( 1 )
	{
		token = COM_ParseExt( p, qtrue );
		if ( !token[0] || !Q_stricmp( token, "}" ) )
		{
			return;
		}
		float f = 0.0f;
		int n = 0;
		if ( !Q_stricmp( token, "npcBoltDamage" ) )				{ if ( !COM_ParseFloat( p, &f ) ) prof->npcBoltDamage = f; }
		else if ( !Q_stricmp( token, "weaponSkillShift" ) )		{ if ( !COM_ParseInt( p, &n ) ) prof->weaponSkillShift = n; }
		else if ( !Q_stricmp( token, "saberDamageToPlayer" ) )	{ if ( !COM_ParseFloat( p, &f ) ) prof->saberDamageToPlayer = f; }
		else if ( !Q_stricmp( token, "parryWindowScale" ) )		{ if ( !COM_ParseFloat( p, &f ) ) prof->parryWindowScale = f; }
		else if ( !Q_stricmp( token, "nerveShift" ) )			{ if ( !COM_ParseInt( p, &n ) ) prof->nerveShift = n; }
		else if ( !Q_stricmp( token, "guardScale" ) )			{ if ( !COM_ParseFloat( p, &f ) ) prof->guardScale = f; }
		else if ( !Q_stricmp( token, "heavyCooldownScale" ) )	{ if ( !COM_ParseFloat( p, &f ) ) prof->heavyCooldownScale = f; }
		else if ( !Q_stricmp( token, "coreCutToPlayer" ) )		{ if ( !COM_ParseFloat( p, &f ) ) prof->coreCutToPlayer = f; }
		else
		{
			Com_Printf( S_COLOR_YELLOW"difficulty.cfg: unknown key '%s'\n", token );
			SkipRestOfLine( p );
		}
	}
}

void G_LoadDifficultyProfiles( void )
{
	memcpy( s_profiles, s_defaultProfiles, sizeof( s_profiles ) );
	s_profilesLoaded = qtrue;

	char *buffer = NULL;
	const int len = gi.FS_ReadFile( "ext_data/difficulty.cfg", (void **)&buffer );
	if ( len <= 0 || !buffer )
	{
		Com_Printf( S_COLOR_YELLOW"G_LoadDifficultyProfiles: no ext_data/difficulty.cfg, using built-in values\n" );
		return;
	}
	COM_BeginParseSession( "difficulty.cfg" );
	const char *p = buffer;
	while ( 1 )
	{
		const char *token = COM_ParseExt( &p, qtrue );
		if ( !token[0] )
		{
			break;
		}
		if ( Q_stricmp( token, "skill" ) )
		{
			Com_Printf( S_COLOR_YELLOW"difficulty.cfg: expected 'skill N', got '%s'\n", token );
			SkipRestOfLine( &p );
			continue;
		}
		int skill = -1;
		if ( COM_ParseInt( &p, &skill ) || skill < 0 || skill >= NUM_DIFFICULTY_PROFILES )
		{
			Com_Printf( S_COLOR_YELLOW"difficulty.cfg: skill must be 0-%d\n", NUM_DIFFICULTY_PROFILES - 1 );
			break;
		}
		G_ParseDifficultyBlock( &p, &s_profiles[skill] );
	}
	COM_EndParseSession();
	gi.FS_FreeFile( buffer );
}

const difficultyProfile_t *G_Difficulty( void )
{
	const int index = G_DifficultyLevel();
	return s_profilesLoaded ? &s_profiles[index] : &s_defaultProfiles[index];
}
