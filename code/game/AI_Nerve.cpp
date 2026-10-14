/*
===========================================================================
Per-NPC nerve. See AI_Nerve.h.
===========================================================================
*/

#include "g_local.h"
#include "b_local.h"
#include "AI_Nerve.h"

extern cvar_t	*d_nerveLog;

typedef struct nerveEventDef_s
{
	const char	*name;
	int			delta;			// to the NPC it happened to
	int			witnessDelta;	// to allies who saw it
	float		radius;			// how far a witness can be
	int			debounceMs;		// per NPC, per event
} nerveEventDef_t;

static const nerveEventDef_t nerveEvents[NE_NUM] =
{
	{ "ally killed",		  0, -12, 1024, 1000 },
	{ "leader killed",		  0, -20, 1024, 1000 },
	{ "ally dismembered",	  0, -15, 1024, 1000 },
	{ "ally neutralized",	  0,  -8, 1024, 1000 },
	{ "hit",				-30,   0,    0,  200 },
	{ "near miss",			 -2,   0,    0, 1000 },
	{ "explosion near",		 -8,   0,    0, 1000 },
	{ "grenade near",		 -6,   0,    0, 1000 },
	{ "force used on",		-10,  -4,  768, 1000 },
	{ "enemy saber",		 -6,   0,    0,30000 },
	{ "cornered",			 -5,   0,    0, 5000 },
	{ "no cover",			 -2,   0,    0,10000 },
	{ "disarmed",			-25,  -3,  512, 1000 },
	{ "hurt enemy",			  3,   1, 1024,  500 },
	{ "ally killed enemy",	  6,   6, 1024, 1000 },
	{ "reinforced",			  5,   0,    0,10000 },
	{ "leader hurt",		  0,  -4, 1024, 2000 },
};

static const char *nerveTierNames[NT_NUM] = { "STEADY", "SHAKEN", "BREAKING", "BROKEN" };

// the bands: STEADY >= 50, SHAKEN 30-50, BREAKING 15-30, BROKEN < 15 (+/-5 hysteresis).
// Rank bases sit inside STEADY so a squad starts composed; it takes real events to crack them.
#define NERVE_SHOCK_MIN		-60
#define NERVE_SHOCK_MAX		30
#define NERVE_MAX_WITNESS_TRACES	6
#define NERVE_BROKEN_HOLD_MS	4000	// how long they have to stay BROKEN before they can give up

const char *AI_NerveTierName( int tier )
{
	if ( tier < 0 || tier >= NT_NUM )
	{
		return "?";
	}
	return nerveTierNames[tier];
}

const char *AI_NerveEventName( int ev )
{
	if ( ev < 0 || ev >= NE_NUM )
	{
		return "?";
	}
	return nerveEvents[ev].name;
}

// bosses and NPCs scripted never to flee don't get rattled
qboolean AI_NerveImmune( gentity_t *npc )
{
	if ( !npc || !npc->NPC )
	{
		return qtrue;
	}
	if ( (npc->NPC->aiFlags&(NPCAI_BOSS_CHARACTER|NPCAI_SUBBOSS_CHARACTER)) )
	{
		return qtrue;
	}
	if ( (npc->NPC->scriptFlags&SCF_DONT_FLEE) )
	{
		return qtrue;
	}
	return qfalse;
}

/*
-------------------------
AI_NerveInit

Base nerve at spawn: the .npc "nerve" key wins, otherwise rank.
-------------------------
*/
void AI_NerveInit( gentity_t *npc )
{
	if ( !npc || !npc->NPC || !npc->client )
	{
		return;
	}
	gNPC_t *ni = npc->NPC;
	int base;

	if ( ni->stats.nerve > 0 )
	{
		base = ni->stats.nerve;
	}
	else
	{
		switch ( ni->rank )
		{
		case RANK_CIVILIAN:	base = 40;	break;
		case RANK_CREWMAN:	base = 50;	break;
		case RANK_ENSIGN:	base = 55;	break;
		case RANK_LT_JG:
		case RANK_LT:		base = 60;	break;
		case RANK_LT_COMM:
		case RANK_COMMANDER:base = 65;	break;
		default:			base = 75;	break;	// captain and up
		}
		if ( npc->client->playerTeam == TEAM_PLAYER && base < 70 )
		{//allies don't crack easily
			base = 70;
		}
	}
	if ( (ni->aiFlags&(NPCAI_BOSS_CHARACTER|NPCAI_SUBBOSS_CHARACTER)) && base < 90 )
	{
		base = 90;
	}
	if ( (ni->scriptFlags&SCF_DONT_FLEE) )
	{
		base = 100;
	}

	ni->nerveBase = Com_Clamp( 0, 100, base );
	ni->nerveShock = 0;
	ni->nerveTier = NT_STEADY;
	ni->nerveLastEvent = -1;
	ni->nerveLastEventTime = 0;
}

/*
-------------------------
AI_NerveSituational

The part that's never stored: computed from what's around the NPC right now.
-------------------------
*/
static int Nerve_EnemyThreat( gentity_t *enemy )
{
	if ( !enemy )
	{
		return 0;
	}
	int threat = 0;
	// the old group-morale enemy table, rescaled to the 0-100 nerve range
	switch ( enemy->s.weapon )
	{
	case WP_SABER:			threat = -5;	break;	//(a lit saber also fires the one-off "enemy saber" event)
	case WP_BRYAR_PISTOL:
	case WP_BLASTER_PISTOL:	threat = 4;		break;
	case WP_DISRUPTOR:		threat = 3;		break;
	case WP_REPEATER:		threat = -2;	break;
	case WP_FLECHETTE:		threat = -3;	break;
	case WP_ROCKET_LAUNCHER:threat = -15;	break;
	case WP_CONCUSSION:		threat = -18;	break;
	case WP_THERMAL:		threat = -8;	break;
	case WP_TRIP_MINE:		threat = -4;	break;
	case WP_DET_PACK:		threat = -15;	break;
	case WP_MELEE:			threat = 20;	break;
	case WP_STUN_BATON:		threat = 10;	break;
	case WP_EMPLACED_GUN:	threat = -12;	break;
	case WP_ATST_MAIN:		threat = -12;	break;
	case WP_ATST_SIDE:		threat = -20;	break;
	default:				threat = 0;		break;
	}
	// a wounded enemy is less frightening
	if ( enemy->health < 10 )
	{
		threat += 10;
	}
	else if ( enemy->health < 25 )
	{
		threat += 5;
	}
	else if ( enemy->health < 50 )
	{
		threat += 2;
	}
	return Com_Clamp( -20, 20, threat );
}

int AI_NerveSituational( gentity_t *npc, int *officer, int *numbers, int *contagion, int *threat, int *wounds )
{
	int o = 0, n = 0, c = 0, t = 0, w = 0;
	gNPC_t *ni = npc->NPC;

	if ( ni->group )
	{
		AIGroupInfo_t *group = ni->group;
		// an officer in charge steadies everyone else
		if ( group->commander && group->commander != npc && group->commander->health > 0
			&& group->commander->NPC && group->commander->NPC->rank >= RANK_LT_JG )
		{
			o = 8;
		}
		// numbers: us vs. however many the enemy brought
		int enemyCount = 1;
		if ( group->enemy && group->enemy->NPC && group->enemy->NPC->group )
		{
			enemyCount = group->enemy->NPC->group->numGroup;
		}
		n = Com_Clamp( -10, 10, ( group->numGroup - enemyCount ) * 3 );
		// contagion: drift toward the squad average
		c = (int)( ( group->avgNerve - ( ni->nerveBase + ni->nerveShock ) ) * 0.25f );
	}

	t = Nerve_EnemyThreat( npc->enemy );

	if ( npc->max_health > 0 && npc->health < npc->max_health )
	{
		w = -(int)( 15.0f * ( 1.0f - (float)npc->health / (float)npc->max_health ) );
	}

	if ( officer )	*officer = o;
	if ( numbers )	*numbers = n;
	if ( contagion )*contagion = c;
	if ( threat )	*threat = t;
	if ( wounds )	*wounds = w;
	return o + n + c + t + w;
}

int AI_NerveEffective( gentity_t *npc, int *outSituational )
{
	if ( !npc || !npc->NPC )
	{
		if ( outSituational ) *outSituational = 0;
		return 100;
	}
	const int sit = AI_NerveSituational( npc, NULL, NULL, NULL, NULL, NULL );
	if ( outSituational )
	{
		*outSituational = sit;
	}
	return Com_Clamp( 0, 100, npc->NPC->nerveBase + npc->NPC->nerveShock + sit );
}

int AI_NerveTier( gentity_t *npc )
{
	if ( !npc || !npc->NPC )
	{
		return NT_STEADY;
	}
	return npc->NPC->nerveTier;
}

// the old 0 ROUTED .. 4 RUSH scale the trooper code still reads. A composed squad sits at
// ADVANCE, like a healthy squad did under the old morale sum; it takes high nerve to flank or
// rush, and anyone BREAKING or worse falls back to cover.
int AI_GetNerveMoraleTier( gentity_t *npc )
{
	switch ( AI_NerveTier( npc ) )
	{
	case NT_STEADY:
		{
			const int e = AI_NerveEffective( npc );
			return e >= 85 ? 4 : ( e >= 70 ? 3 : 2 );
		}
	case NT_SHAKEN:		return 1;
	default:			return 0;
	}
}

/*
-------------------------
AI_NerveUpdate

Once per think: shock decays toward zero, and the tier follows the effective value with a
little hysteresis so it doesn't flicker at the boundaries.
-------------------------
*/
void AI_NerveUpdate( gentity_t *npc )
{
	if ( !npc || !npc->NPC )
	{
		return;
	}
	gNPC_t *ni = npc->NPC;

	if ( ni->nerveShock != 0 && TIMER_Done( npc, "nerveDecay" ) )
	{
		// 4 steps a second normally, half that while the enemy is in sight; the step grows
		// with how far from composed they are, so a deep shock fades back toward the middle
		// instead of pinning them at the floor
		const qboolean enemyInSight = (qboolean)( npc->enemy && level.time - ni->enemyLastSeenTime < 1000 );
		TIMER_Set( npc, "nerveDecay", enemyInSight ? 500 : 250 );
		const int step = 1 + abs( ni->nerveShock ) / 30;
		if ( ni->nerveShock > 0 )
		{
			ni->nerveShock = Q_max( 0, ni->nerveShock - step );
		}
		else
		{
			ni->nerveShock = Q_min( 0, ni->nerveShock + step );
		}
	}

	static const int bands[3] = { 50, 30, 15 };	// STEADY/SHAKEN, SHAKEN/BREAKING, BREAKING/BROKEN
	const int e = AI_NerveEffective( npc );
	int tier = ni->nerveTier;
	if ( tier > NT_STEADY && e >= bands[tier-1] + 5 )
	{
		tier--;
	}
	else if ( tier < NT_BROKEN && e < bands[tier] - 5 )
	{
		tier++;
	}
	if ( tier != ni->nerveTier )
	{
		if ( d_nerveLog && d_nerveLog->integer && ( d_nerveLog->integer < 0 || d_nerveLog->integer == npc->s.number ) )
		{
			gi.Printf( "NERVE %s (%d): %s -> %s (%d)\n", npc->NPC_type, npc->s.number, AI_NerveTierName( ni->nerveTier ), AI_NerveTierName( tier ), e );
		}
		if ( tier == NT_BROKEN )
		{//they have to stay broken a while before they'll actually give up
			TIMER_Set( npc, "nerveBrokenHold", NERVE_BROKEN_HOLD_MS );
		}
		else if ( ni->nerveTier == NT_BROKEN )
		{
			TIMER_Remove( npc, "nerveBrokenHold" );
		}
		ni->nerveTier = tier;
	}
}

/*
-------------------------
AI_NerveEvent
-------------------------
*/
static void Nerve_Apply( gentity_t *npc, int ev, int delta, gentity_t *source )
{
	gNPC_t *ni = npc->NPC;
	if ( delta == 0 )
	{
		return;
	}
	if ( AI_NerveImmune( npc ) )
	{
		return;
	}
	char timerName[24];
	Com_sprintf( timerName, sizeof(timerName), "nrv%d", ev );
	if ( !TIMER_Done( npc, timerName ) )
	{
		return;
	}
	TIMER_Set( npc, timerName, nerveEvents[ev].debounceMs );

	ni->nerveShock = Com_Clamp( NERVE_SHOCK_MIN, NERVE_SHOCK_MAX, ni->nerveShock + delta );
	ni->nerveLastEvent = ev;
	ni->nerveLastEventTime = level.time;

	if ( d_nerveLog && d_nerveLog->integer && ( d_nerveLog->integer < 0 || d_nerveLog->integer == npc->s.number ) )
	{
		gi.Printf( "NERVE %s (%d): %s %+d -> shock %d, effective %d\n", npc->NPC_type, npc->s.number,
			nerveEvents[ev].name, delta, ni->nerveShock, AI_NerveEffective( npc ) );
	}
}

void AI_NerveEvent( gentity_t *npc, int ev, float scale, gentity_t *source )
{
	if ( !npc || !npc->NPC || !npc->client || npc->health <= 0 || ev < 0 || ev >= NE_NUM )
	{
		return;
	}
	int delta = (int)( nerveEvents[ev].delta * scale );
	if ( delta == 0 && nerveEvents[ev].delta != 0 )
	{//a tiny scaled hit still counts for something
		delta = nerveEvents[ev].delta > 0 ? 1 : -1;
	}
	Nerve_Apply( npc, ev, delta, source );
}

/*
-------------------------
AI_NerveWitness

Everyone on the victim's team nearby who can see it. One bounded box query; traces only
for the closest few, and only when they're far enough that a wall is plausible.
-------------------------
*/
void AI_NerveWitness( gentity_t *victim, int ev, gentity_t *source )
{
	if ( !victim || !victim->client || ev < 0 || ev >= NE_NUM )
	{
		return;
	}
	const nerveEventDef_t *def = &nerveEvents[ev];
	if ( def->witnessDelta == 0 || def->radius <= 0 )
	{
		return;
	}

	gentity_t	*entityList[MAX_GENTITIES];
	vec3_t		mins, maxs;
	int			traces = 0;

	for ( int i = 0; i < 3; i++ )
	{
		mins[i] = victim->currentOrigin[i] - def->radius;
		maxs[i] = victim->currentOrigin[i] + def->radius;
	}
	const int num = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );
	for ( int e = 0; e < num; e++ )
	{
		gentity_t *ent = entityList[e];
		if ( ent == victim || !ent->inuse || !ent->NPC || !ent->client || ent->health <= 0 )
		{
			continue;
		}
		if ( ent->client->playerTeam != victim->client->playerTeam )
		{
			continue;
		}
		if ( G_IsNeutralized( ent ) )
		{
			continue;
		}
		if ( !gi.inPVS( ent->currentOrigin, victim->currentOrigin ) )
		{
			continue;
		}
		if ( !InFOV( victim, ent, ent->NPC->stats.hfov, ent->NPC->stats.vfov ) )
		{
			continue;
		}
		if ( DistanceSquared( ent->currentOrigin, victim->currentOrigin ) > 256*256 )
		{//far enough that something could be in the way: trace, but only so many
			if ( traces >= NERVE_MAX_WITNESS_TRACES )
			{
				continue;
			}
			traces++;
			if ( !G_ClearLOS( ent, victim ) )
			{
				continue;
			}
		}
		Nerve_Apply( ent, ev, def->witnessDelta, source );
	}
}

/*
-------------------------
AI_NerveNearMiss

A shot passed close to whoever's in the box around pos and hostile to the shooter.
-------------------------
*/
void AI_NerveNearMiss( const vec3_t pos, gentity_t *owner, gentity_t *hit, float radius )
{
	if ( !owner || !owner->client )
	{
		return;
	}
	gentity_t	*entityList[MAX_GENTITIES];
	vec3_t		mins, maxs;

	for ( int i = 0; i < 3; i++ )
	{
		mins[i] = pos[i] - radius;
		maxs[i] = pos[i] + radius;
	}
	const int num = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );
	for ( int e = 0; e < num; e++ )
	{
		gentity_t *ent = entityList[e];
		if ( ent == owner || ent == hit || !ent->inuse || !ent->NPC || !ent->client || ent->health <= 0 )
		{
			continue;
		}
		if ( ent->client->playerTeam == owner->client->playerTeam )
		{
			continue;
		}
		AI_NerveEvent( ent, NE_NEAR_MISS, 1.0f, owner );
	}
}
