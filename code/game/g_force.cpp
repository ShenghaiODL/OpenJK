/*
===========================================================================
JKEnhanced: Force tuning (Step 4B)

ext_data/force.cfg:

	cost
	{
		Push		15			// forcePowerNeeded, by the names in WP_ForcePowerName
		SaberThrow	20
		...
	}
	multiTarget
	{
		extra	5				// Push/Pull L3 cost = base + extra per extra target, capped
		cap		30
	}
	npc
	{
		costScale		1.0		// x on what NPCs pay (g_npcForcePool 1)
		bossPoolScale	1.5		// saber bosses: x max Force, x regen speed (at spawn)
		bossRegenScale	1.5
	}
	protect
	{
		duration1		8000	// L1 / L2: ms (pay the cost up front, regen keeps running)
		duration2		12000
		toggleCost		5		// L3: a toggle; on costs this, then...
		damagePct		40		// ...lets 40% of a hit through...
		hitCostDiv		3		// ...for 1 Force per 3 damage it takes off
		dodgeChance		20		// L3: % a ranged shot is dodged outright
		dodgeCost		10
		dodgeCooldown	1500
	}

Anything left out keeps its built-in value (the cost table is the code's forcePowerNeeded[]).
===========================================================================
*/

#include "g_local.h"
#include "g_force.h"
#include "wp_saber.h"

extern int			forcePowerNeeded[NUM_FORCE_POWERS];
extern const char	*WP_ForcePowerName( int power );

static const forceTuning_t s_defaultTuning =
{
	5,		// multiTargetExtra
	30,		// multiTargetCap
	1.0f,	// npcCostScale
	1.5f,	// bossPoolScale
	1.5f,	// bossRegenScale
	{ 8000, 12000 },	// protectDuration
	5,		// protectToggleCost
	40,		// protectDamagePct
	3,		// protectHitCostDiv
	20,		// protectDodgeChance
	10,		// protectDodgeCost
	1500,	// protectDodgeCooldown
	2000,	// repulseCharge
	35,		// repulseMinPct
	150,	// repulseKnockMin
	400,	// repulseKnockMax
	0,		// repulseDamage
	400,	// repulseSlamMin
	1400,	// repulseSlamFull
	40,		// repulseSlamDamage
	15,		// repulseStartCost
	100,	// repulseTotalCost
	{ 4000, 7000, 10000 },	// speedDuration
	125,	// speedRunPct
	50,		// speedLandPct
	150,	// speedRecoveryPct
	150,	// speedCancelBonus
	150,	// speedParryPct
	1,		// speedDodges
	{ 512, 768, 1024 },		// stasisRange
	{ 25, 35, 45 },			// stasisCone
	{ 3000, 4500, 6000 },	// stasisDuration
	50,		// stasisSlowPct
	70,		// stasisBossSlowPct
	{ 512, 768, 1024 },		// insanityRange
	{ 30, 40, 50 },			// insanityCone
	{ 20, 35, 50 },			// insanityNerve
	{ 3000, 4500, 6000 },	// insanityCower
	{ 800, 1200, 1600 },	// insanityWaver
	6000,	// insanityHallucinate
	25,		// insanityHallucinateYaw
	10,		// insanityExtraCost
	100,	// insanityCostCap
	4000,	// insanityCooldown
};

static int	s_shieldFlash[MAX_GENTITIES];	// Protect L3: level.time of its last block

static forceTuning_t	s_tuning;
static qboolean			s_tuningLoaded = qfalse;
static int				s_defaultCosts[NUM_FORCE_POWERS];
static qboolean			s_defaultCostsSaved = qfalse;

static qboolean G_ForceOpenBlock( const char **p, const char *name )
{
	const char *token = COM_ParseExt( p, qtrue );
	if ( Q_stricmp( token, "{" ) )
	{
		Com_Printf( S_COLOR_YELLOW"force.cfg: expected { after %s\n", name );
		return qfalse;
	}
	return qtrue;
}

static void G_ParseForceCosts( const char **p )
{
	if ( !G_ForceOpenBlock( p, "cost" ) )
	{
		return;
	}
	while ( 1 )
	{
		const char *token = COM_ParseExt( p, qtrue );
		if ( !token[0] || !Q_stricmp( token, "}" ) )
		{
			return;
		}
		int power = -1;
		for ( int i = 0; i < NUM_FORCE_POWERS; i++ )
		{
			if ( !Q_stricmp( token, WP_ForcePowerName( i ) ) )
			{
				power = i;
				break;
			}
		}
		int n = 0;
		if ( power < 0 )
		{
			Com_Printf( S_COLOR_YELLOW"force.cfg: unknown power '%s' in cost\n", token );
			SkipRestOfLine( p );
		}
		else if ( !COM_ParseInt( p, &n ) )
		{
			forcePowerNeeded[power] = Q_max( 0, n );
		}
	}
}

static void G_ParseForceMultiTarget( const char **p )
{
	if ( !G_ForceOpenBlock( p, "multiTarget" ) )
	{
		return;
	}
	while ( 1 )
	{
		const char *token = COM_ParseExt( p, qtrue );
		if ( !token[0] || !Q_stricmp( token, "}" ) )
		{
			return;
		}
		int n = 0;
		if ( !Q_stricmp( token, "extra" ) )			{ if ( !COM_ParseInt( p, &n ) ) s_tuning.multiTargetExtra = Q_max( 0, n ); }
		else if ( !Q_stricmp( token, "cap" ) )		{ if ( !COM_ParseInt( p, &n ) ) s_tuning.multiTargetCap = Q_max( 0, n ); }
		else
		{
			Com_Printf( S_COLOR_YELLOW"force.cfg: unknown key '%s' in multiTarget\n", token );
			SkipRestOfLine( p );
		}
	}
}

static void G_ParseForceNPC( const char **p )
{
	if ( !G_ForceOpenBlock( p, "npc" ) )
	{
		return;
	}
	while ( 1 )
	{
		const char *token = COM_ParseExt( p, qtrue );
		if ( !token[0] || !Q_stricmp( token, "}" ) )
		{
			return;
		}
		float f = 0.0f;
		if ( !Q_stricmp( token, "costScale" ) )				{ if ( !COM_ParseFloat( p, &f ) ) s_tuning.npcCostScale = Q_max( 0.0f, f ); }
		else if ( !Q_stricmp( token, "bossPoolScale" ) )	{ if ( !COM_ParseFloat( p, &f ) ) s_tuning.bossPoolScale = Q_max( 0.1f, f ); }
		else if ( !Q_stricmp( token, "bossRegenScale" ) )	{ if ( !COM_ParseFloat( p, &f ) ) s_tuning.bossRegenScale = Q_max( 0.1f, f ); }
		else
		{
			Com_Printf( S_COLOR_YELLOW"force.cfg: unknown key '%s' in npc\n", token );
			SkipRestOfLine( p );
		}
	}
}

typedef struct
{
	const char	*key;
	int			*field;
	int			min;
} forceIntKey_t;

static void G_ParseForceIntBlock( const char **p, const char *name, const forceIntKey_t *keys )
{
	if ( !G_ForceOpenBlock( p, name ) )
	{
		return;
	}
	while ( 1 )
	{
		const char *token = COM_ParseExt( p, qtrue );
		if ( !token[0] || !Q_stricmp( token, "}" ) )
		{
			return;
		}
		const forceIntKey_t *k = keys;
		while ( k->key && Q_stricmp( token, k->key ) )
		{
			k++;
		}
		int n = 0;
		if ( !k->key )
		{
			Com_Printf( S_COLOR_YELLOW"force.cfg: unknown key '%s' in %s\n", token, name );
			SkipRestOfLine( p );
		}
		else if ( !COM_ParseInt( p, &n ) )
		{
			*k->field = Q_max( k->min, n );
		}
	}
}

static void G_ParseForceProtect( const char **p )
{
	const forceIntKey_t keys[] =
	{
		{ "duration1",		&s_tuning.protectDuration[0],	100 },
		{ "duration2",		&s_tuning.protectDuration[1],	100 },
		{ "toggleCost",		&s_tuning.protectToggleCost,	1 },	//0 would mean "full cost" to WP_ForcePowerStart
		{ "damagePct",		&s_tuning.protectDamagePct,		0 },
		{ "hitCostDiv",		&s_tuning.protectHitCostDiv,	1 },
		{ "dodgeChance",	&s_tuning.protectDodgeChance,	0 },
		{ "dodgeCost",		&s_tuning.protectDodgeCost,		0 },
		{ "dodgeCooldown",	&s_tuning.protectDodgeCooldown,	0 },
		{ NULL, NULL, 0 }
	};
	G_ParseForceIntBlock( p, "protect", keys );
	s_tuning.protectDamagePct = Q_min( 100, s_tuning.protectDamagePct );
}

static void G_ParseForceSpeed( const char **p )
{
	const forceIntKey_t keys[] =
	{
		{ "duration1",		&s_tuning.speedDuration[0],		500 },
		{ "duration2",		&s_tuning.speedDuration[1],		500 },
		{ "duration3",		&s_tuning.speedDuration[2],		500 },
		{ "runPct",			&s_tuning.speedRunPct,			100 },
		{ "landPct",		&s_tuning.speedLandPct,			10 },
		{ "recoveryPct",	&s_tuning.speedRecoveryPct,		100 },
		{ "cancelBonus",	&s_tuning.speedCancelBonus,		0 },
		{ "parryPct",		&s_tuning.speedParryPct,		100 },
		{ "dodges",			&s_tuning.speedDodges,			0 },
		{ NULL, NULL, 0 }
	};
	G_ParseForceIntBlock( p, "speed", keys );
	s_tuning.speedLandPct = Q_min( 100, s_tuning.speedLandPct );
}

static void G_ParseForceStasis( const char **p )
{
	const forceIntKey_t keys[] =
	{
		{ "range1",			&s_tuning.stasisRange[0],		64 },
		{ "range2",			&s_tuning.stasisRange[1],		64 },
		{ "range3",			&s_tuning.stasisRange[2],		64 },
		{ "cone1",			&s_tuning.stasisCone[0],		5 },
		{ "cone2",			&s_tuning.stasisCone[1],		5 },
		{ "cone3",			&s_tuning.stasisCone[2],		5 },
		{ "duration1",		&s_tuning.stasisDuration[0],	250 },
		{ "duration2",		&s_tuning.stasisDuration[1],	250 },
		{ "duration3",		&s_tuning.stasisDuration[2],	250 },
		{ "slowPct",		&s_tuning.stasisSlowPct,		10 },
		{ "bossSlowPct",	&s_tuning.stasisBossSlowPct,	10 },
		{ NULL, NULL, 0 }
	};
	G_ParseForceIntBlock( p, "stasis", keys );
	for ( int i = 0; i < 3; i++ )
	{
		s_tuning.stasisCone[i] = Q_min( 90, s_tuning.stasisCone[i] );
	}
	s_tuning.stasisSlowPct = Q_min( 100, s_tuning.stasisSlowPct );
	s_tuning.stasisBossSlowPct = Q_min( 100, s_tuning.stasisBossSlowPct );
}

static void G_ParseForceInsanity( const char **p )
{
	const forceIntKey_t keys[] =
	{
		{ "range1",			&s_tuning.insanityRange[0],			64 },
		{ "range2",			&s_tuning.insanityRange[1],			64 },
		{ "range3",			&s_tuning.insanityRange[2],			64 },
		{ "cone1",			&s_tuning.insanityCone[0],			5 },
		{ "cone2",			&s_tuning.insanityCone[1],			5 },
		{ "cone3",			&s_tuning.insanityCone[2],			5 },
		{ "nerve1",			&s_tuning.insanityNerve[0],			0 },
		{ "nerve2",			&s_tuning.insanityNerve[1],			0 },
		{ "nerve3",			&s_tuning.insanityNerve[2],			0 },
		{ "cower1",			&s_tuning.insanityCower[0],			0 },
		{ "cower2",			&s_tuning.insanityCower[1],			0 },
		{ "cower3",			&s_tuning.insanityCower[2],			0 },
		{ "waver1",			&s_tuning.insanityWaver[0],			0 },
		{ "waver2",			&s_tuning.insanityWaver[1],			0 },
		{ "waver3",			&s_tuning.insanityWaver[2],			0 },
		{ "hallucinate",	&s_tuning.insanityHallucinate,		0 },
		{ "hallucinateYaw",	&s_tuning.insanityHallucinateYaw,	0 },
		{ "extraCost",		&s_tuning.insanityExtraCost,		0 },
		{ "costCap",		&s_tuning.insanityCostCap,			0 },
		{ "cooldown",		&s_tuning.insanityCooldown,		0 },
		{ NULL, NULL, 0 }
	};
	G_ParseForceIntBlock( p, "insanity", keys );
	for ( int i = 0; i < 3; i++ )
	{
		s_tuning.insanityCone[i] = Q_min( 90, s_tuning.insanityCone[i] );
		s_tuning.insanityNerve[i] = Q_min( 60, s_tuning.insanityNerve[i] );	//the nerve shock floor
	}
	s_tuning.insanityHallucinate = Q_min( 15000, s_tuning.insanityHallucinate );
	s_tuning.insanityHallucinateYaw = Q_min( 60, s_tuning.insanityHallucinateYaw );
}

static void G_ParseForceRepulse( const char **p )
{
	const forceIntKey_t keys[] =
	{
		{ "charge",			&s_tuning.repulseCharge,		50 },
		{ "minPct",			&s_tuning.repulseMinPct,		0 },
		{ "knockMin",		&s_tuning.repulseKnockMin,		0 },
		{ "knockMax",		&s_tuning.repulseKnockMax,		0 },
		{ "damage",			&s_tuning.repulseDamage,		0 },
		{ "slamMin",		&s_tuning.repulseSlamMin,		0 },
		{ "slamFull",		&s_tuning.repulseSlamFull,		1 },
		{ "slamDamage",		&s_tuning.repulseSlamDamage,	0 },
		{ "startCost",		&s_tuning.repulseStartCost,		0 },
		{ "totalCost",		&s_tuning.repulseTotalCost,		0 },
		{ NULL, NULL, 0 }
	};
	G_ParseForceIntBlock( p, "repulse", keys );
	s_tuning.repulseMinPct = Q_min( 100, s_tuning.repulseMinPct );
	s_tuning.repulseKnockMax = Q_max( s_tuning.repulseKnockMin, s_tuning.repulseKnockMax );
	s_tuning.repulseSlamFull = Q_max( s_tuning.repulseSlamMin + 1, s_tuning.repulseSlamFull );
	s_tuning.repulseTotalCost = Q_max( s_tuning.repulseStartCost, s_tuning.repulseTotalCost );
}

void G_ForceShieldFlash( const gentity_t *ent, int power )
{
	if ( power == FP_PROTECT && ent && ent->s.number >= 0 && ent->s.number < MAX_GENTITIES )
	{
		s_shieldFlash[ent->s.number] = level.time;
	}
}

int G_ForceShieldFlashTime( int entNum, int power )
{
	if ( power != FP_PROTECT || entNum < 0 || entNum >= MAX_GENTITIES )
	{
		return 0;
	}
	return s_shieldFlash[entNum];
}

qboolean G_ForceShieldIsToggle( const gentity_t *ent, int power )
{
	return (qboolean)( power == FP_PROTECT && ent && ent->client
		&& ( ent->client->ps.forcePowersActive & ( 1 << power ) )
		&& ent->client->ps.forcePowerLevel[power] >= FORCE_LEVEL_3 );
}

//Speed (4B.6): the level of a Speed in use, 0 when off. Everyone gets the perks of their level and below.
int G_SpeedLevelPS( const playerState_t *ps )
{
	if ( !ps || !( ps->forcePowersActive & ( 1 << FP_SPEED ) ) )
	{
		return 0;
	}
	return Q_max( FORCE_LEVEL_1, Q_min( FORCE_LEVEL_3, ps->forcePowerLevel[FP_SPEED] ) );
}

int G_SpeedLevel( const gentity_t *ent )
{
	return ( ent && ent->client ) ? G_SpeedLevelPS( &ent->client->ps ) : 0;
}

//Focus's reflex dodges, per cast (not saved: a loaded save's Speed just has none left)
static int	s_speedDodgesLeft[MAX_GENTITIES];
static int	s_speedCastTime[MAX_GENTITIES];

void G_SpeedCastStart( const gentity_t *ent )
{
	if ( ent && ent->s.number >= 0 && ent->s.number < MAX_GENTITIES )
	{
		s_speedDodgesLeft[ent->s.number] = s_tuning.speedDodges;
		s_speedCastTime[ent->s.number] = level.time;
	}
}

void G_SpeedCastEnd( const gentity_t *ent )
{
	if ( ent && ent->s.number >= 0 && ent->s.number < MAX_GENTITIES )
	{
		s_speedDodgesLeft[ent->s.number] = 0;
	}
}

qboolean G_SpeedTakeDodge( const gentity_t *ent )
{
	if ( G_SpeedLevel( ent ) < FORCE_LEVEL_3 )
	{
		return qfalse;
	}
	const int n = ent->s.number;
	if ( s_speedCastTime[n] > level.time || s_speedDodgesLeft[n] <= 0 )
	{//(later than now: stale, from before a load)
		return qfalse;
	}
	s_speedDodgesLeft[n]--;
	return qtrue;
}

void G_LoadForceTuning( void )
{
	//flash times are level.time stamps: a new map or a loaded save starts clean
	memset( s_shieldFlash, 0, sizeof( s_shieldFlash ) );
	memset( s_speedDodgesLeft, 0, sizeof( s_speedDodgesLeft ) );
	memset( s_speedCastTime, 0, sizeof( s_speedCastTime ) );

	//forcePowerNeeded[] is the code's table: keep a copy so a reload (new map) starts from it,
	//not from the last file's values
	if ( !s_defaultCostsSaved )
	{
		memcpy( s_defaultCosts, forcePowerNeeded, sizeof( s_defaultCosts ) );
		s_defaultCostsSaved = qtrue;
	}
	memcpy( forcePowerNeeded, s_defaultCosts, sizeof( s_defaultCosts ) );
	s_tuning = s_defaultTuning;
	s_tuningLoaded = qtrue;

	char *buffer = NULL;
	const int len = gi.FS_ReadFile( "ext_data/force.cfg", (void **)&buffer );
	if ( len <= 0 || !buffer )
	{
		Com_Printf( S_COLOR_YELLOW"G_LoadForceTuning: no ext_data/force.cfg, using built-in values\n" );
		return;
	}
	COM_BeginParseSession( "force.cfg" );
	const char *p = buffer;
	while ( 1 )
	{
		const char *token = COM_ParseExt( &p, qtrue );
		if ( !token[0] )
		{
			break;
		}
		if ( !Q_stricmp( token, "cost" ) )				{ G_ParseForceCosts( &p ); }
		else if ( !Q_stricmp( token, "multiTarget" ) )	{ G_ParseForceMultiTarget( &p ); }
		else if ( !Q_stricmp( token, "npc" ) )			{ G_ParseForceNPC( &p ); }
		else if ( !Q_stricmp( token, "protect" ) )		{ G_ParseForceProtect( &p ); }
		else if ( !Q_stricmp( token, "repulse" ) )		{ G_ParseForceRepulse( &p ); }
		else if ( !Q_stricmp( token, "speed" ) )		{ G_ParseForceSpeed( &p ); }
		else if ( !Q_stricmp( token, "stasis" ) )		{ G_ParseForceStasis( &p ); }
		else if ( !Q_stricmp( token, "insanity" ) )	{ G_ParseForceInsanity( &p ); }
		else
		{
			Com_Printf( S_COLOR_YELLOW"force.cfg: expected cost / multiTarget / npc / protect / repulse / speed / stasis / insanity, got '%s'\n", token );
			SkipRestOfLine( &p );
		}
	}
	COM_EndParseSession();
	gi.FS_FreeFile( buffer );
}

const forceTuning_t *G_ForceTuning( void )
{
	return s_tuningLoaded ? &s_tuning : &s_defaultTuning;
}
