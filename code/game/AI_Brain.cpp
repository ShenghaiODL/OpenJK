/*
===========================================================================
Utility brain for gun-wielding NPCs. See AI_Brain.h.

Runs on the NPC globals (NPC, NPCInfo, ucmd) like the rest of the trooper code.
===========================================================================
*/

#include "b_local.h"
#include "g_navigator.h"
#include "Q3_Interface.h"
#include "AI_Brain.h"

extern cvar_t	*d_aiLog;

// trooper-side primitives the executors drive (AI_Stormtrooper.cpp)
extern qboolean	ST_SpotHiddenFromEnemy( void );
extern qboolean	ST_FindGeoCover( vec3_t outPos );
extern void		ST_TrackEnemy( gentity_t *self, vec3_t enemyPos );
extern void		ST_StartThermalThrow( void );
extern void		ST_SayOutflank( void );
extern void		ST_SayCover( void );
extern qboolean	ST_IsMeleeWeapon( int weapon );

#define AAF_MOVES		0x0001	// sets a movement goal
#define AAF_NEEDS_SLOT	0x0002	// counts against the squad's "how many may move" cap
#define AAF_EMERGENCY	0x0004	// may break another action's commitment

typedef struct aiActionDef_s
{
	const char	*name;
	int			minHoldMs;
	int			flags;
} aiActionDef_t;

static const aiActionDef_t actionDefs[AA_NUM] =
{
	{ "none",			0,		0 },
	{ "hold cover",		3000,	0 },
	{ "relocate",		4000,	AAF_MOVES|AAF_NEEDS_SLOT },
	{ "advance",		4000,	AAF_MOVES|AAF_NEEDS_SLOT },
	{ "flank",			6000,	AAF_MOVES|AAF_NEEDS_SLOT },
	{ "stand & fire",	1500,	0 },
	{ "strafe",			700,	0 },
	{ "grenade",		6500,	0 },
	{ "suppress",		3000,	0 },
	{ "retreat",		4000,	AAF_MOVES|AAF_EMERGENCY },
	{ "flee",			4000,	AAF_MOVES|AAF_EMERGENCY },
	{ "seek weapon",	2000,	AAF_MOVES },
	{ "track",			5000,	AAF_MOVES },
	{ "regroup",		3000,	AAF_MOVES },
};

static aiDebug_t s_aiDebug[MAX_GENTITIES];

// everything a score function may look at; rebuilt on each rescore
typedef struct aiContext_s
{
	float		enemyDist;
	float		distNorm;		// enemyDist / profile rangeMax (>1 = farther than we'd like)
	qboolean	tooClose;		// inside the profile's rangeMin
	qboolean	enemyLOS, enemyCS, enemyInFOV, hitAlly;
	qboolean	enemySaber;
	qboolean	enemyLost;		// not seen for a while
	qboolean	inCover;		// crouched profile hidden from the enemy's eyes (cached 1 s)
	qboolean	hasThermal;		// a thermal to lob, and we're not holding it already
	qboolean	weaponless;
	qboolean	seekingWeapon;	// already running to a pickup
	qboolean	explosiveWeapon;
	qboolean	meleeWeapon;
	qboolean	canFlee;
	// companions: the one they're following
	qboolean	hasLeader;
	qboolean	leaderInPVS;
	float		leaderDist;
	float		leash;
	qboolean	lone;
	qboolean	coverRequested;	// a squadmate asked for covering fire
	float		health;			// fraction
	float		underFire;		// 1 hit recently, 0.5 near miss recently
	int			nerve;
	int			nerveTier;
	int			slotsFree;
	int			msSinceSeen;
	int			flankers;
	int			intelligence;
} aiContext_t;

const char *AI_Brain_ActionName( int action )
{
	if ( action < 0 || action >= AA_NUM )
	{
		return "?";
	}
	return actionDefs[action].name;
}

const aiDebug_t *AI_Brain_GetDebug( int entNum )
{
	if ( entNum < 0 || entNum >= MAX_GENTITIES )
	{
		return NULL;
	}
	return &s_aiDebug[entNum];
}

void AI_Brain_Interrupt( gentity_t *npc )
{
	if ( !npc || !npc->NPC )
	{
		return;
	}
	TIMER_Set( npc, "aiRescore", 0 );
	npc->NPC->aiActionHoldUntil = level.time;
}

static void Brain_Exit( int action );

void AI_Brain_Reset( gentity_t *npc )
{
	if ( !npc || !npc->NPC )
	{
		return;
	}
	if ( npc == NPC && npc->NPC->aiAction != AA_NONE )
	{
		Brain_Exit( npc->NPC->aiAction );
	}
	npc->NPC->aiAction = AA_NONE;
	npc->NPC->aiActionHoldUntil = 0;
	TIMER_Remove( npc, "aiRescore" );
}

/*
-------------------------
Context
-------------------------
*/
static qboolean Brain_IsExplosive( int weapon )
{
	switch ( weapon )
	{
	case WP_ROCKET_LAUNCHER:
	case WP_THERMAL:
	case WP_DET_PACK:
	case WP_TRIP_MINE:
		return qtrue;
	case WP_CONCUSSION:
		return (qboolean)!( NPCInfo->scriptFlags & SCF_ALT_FIRE );
	case WP_FLECHETTE:
		return (qboolean)!!( NPCInfo->scriptFlags & SCF_ALT_FIRE );
	default:
		return qfalse;
	}
}

static void Brain_BuildContext( aiContext_t *ctx, const aiProfile_t *prof, qboolean enemyLOS, qboolean enemyCS, qboolean enemyInFOV, qboolean hitAlly, float enemyDistSq )
{
	memset( ctx, 0, sizeof( *ctx ) );

	ctx->enemyDist	= sqrtf( enemyDistSq );
	ctx->distNorm	= prof->rangeMax > 0 ? ctx->enemyDist / prof->rangeMax : 1.0f;
	ctx->tooClose	= (qboolean)( ctx->enemyDist < prof->rangeMin );
	ctx->enemyLOS	= enemyLOS;
	ctx->enemyCS	= enemyCS;
	ctx->enemyInFOV	= enemyInFOV;
	ctx->hitAlly	= hitAlly;
	ctx->enemySaber	= (qboolean)( NPC->enemy && NPC->enemy->client && NPC->enemy->client->ps.weapon == WP_SABER && NPC->enemy->client->ps.SaberActive() );
	ctx->msSinceSeen= level.time - NPCInfo->enemyLastSeenTime;
	ctx->enemyLost	= (qboolean)( ctx->msSinceSeen > 7000 );

	// cover is a trace; remember the answer for a second
	if ( TIMER_Done( NPC, "coverCache" ) )
	{
		TIMER_Set( NPC, "coverCache", 1000 );
		if ( ST_SpotHiddenFromEnemy() )
		{
			TIMER_Set( NPC, "inCover", Q3_INFINITE );
		}
		else
		{
			TIMER_Remove( NPC, "inCover" );
		}
	}
	ctx->inCover = TIMER_Exists( NPC, "inCover" );

	ctx->weaponless		= (qboolean)( NPC->client->ps.weapon == WP_NONE );
	ctx->seekingWeapon	= (qboolean)( ctx->weaponless && NPCInfo->goalEntity && NPCInfo->goalEntity == NPCInfo->tempGoal
							&& NPCInfo->goalEntity->enemy && NPCInfo->goalEntity->enemy->s.eType == ET_ITEM );
	ctx->hasThermal		= (qboolean)( NPC->client->ps.weapon != WP_THERMAL
							&& NPC->client->ps.weapons[WP_THERMAL]
							&& NPC->client->ps.ammo[weaponData[WP_THERMAL].ammoIndex] > 0
							&& TIMER_Done( NPC, "thermalCooldown" ) );
	ctx->explosiveWeapon= Brain_IsExplosive( NPC->client->ps.weapon );
	ctx->meleeWeapon	= ST_IsMeleeWeapon( NPC->client->ps.weapon );
	ctx->canFlee		= (qboolean)( !( NPCInfo->scriptFlags & SCF_DONT_FLEE ) && TIMER_Done( NPC, "flee" ) );

	// following someone (companions, scripted SET_LEADER squads): fight, but on a leash
	{
		const gentity_t *leader = NPC->client->leader;
		const qboolean following = (qboolean)( NPCInfo->behaviorState == BS_FOLLOW_LEADER
			|| NPCInfo->defaultBehavior == BS_FOLLOW_LEADER || NPCInfo->tempBehavior == BS_FOLLOW_LEADER );
		if ( following && leader && leader != NPC && leader->inuse && leader->health > 0 )
		{
			ctx->hasLeader	= qtrue;
			ctx->leaderDist	= Distance( NPC->currentOrigin, leader->currentOrigin );
			ctx->leaderInPVS= gi.inPVS( NPC->currentOrigin, leader->currentOrigin );
			ctx->leash		= NPCInfo->leash > 0 ? NPCInfo->leash : 384.0f;
		}
	}
	ctx->health			= NPC->max_health > 0 ? (float)NPC->health / (float)NPC->max_health : 1.0f;
	ctx->underFire		= !TIMER_Done( NPC, "underFire" ) ? 1.0f : ( !TIMER_Done( NPC, "nrv5" ) ? 0.5f : 0.0f );//nrv5 = the near-miss debounce
	ctx->nerve			= AI_NerveEffective( NPC );
	ctx->nerveTier		= AI_NerveTier( NPC );
	ctx->intelligence	= NPCInfo->stats.intelligence;

	AIGroupInfo_t *group = NPCInfo->group;
	ctx->lone		= (qboolean)( !group || group->numGroup <= 1 );
	ctx->slotsFree	= AI_SquadSlotsFree( group );
	if ( !ctx->lone && ( NPCInfo->squadState == SQUAD_TRANSITION || NPCInfo->squadState == SQUAD_SCOUT ) )
	{//we're one of the ones already moving: our own slot is ours
		ctx->slotsFree++;
	}
	ctx->coverRequested = (qboolean)( group && group->coverRequestNum != ENTITYNUM_NONE
							&& group->coverRequestNum != NPC->s.number && group->coverRequestTime > level.time );
	ctx->flankers = AI_SquadFlankers( group, NPC );
}

/*
-------------------------
Scores: each returns 0..~1 before the profile weight
-------------------------
*/
static float NerveDrive( const aiContext_t *ctx )
{//how willing they are to press: 1 composed, down to 0 breaking
	switch ( ctx->nerveTier )
	{
	case NT_STEADY:		return 1.0f;
	case NT_SHAKEN:		return 0.5f;
	default:			return 0.0f;
	}
}

// how free a companion is to wander off: 1 near the leader, down to 0.2 past the leash
static float LeashFactor( const aiContext_t *ctx )
{
	if ( !ctx->hasLeader )
	{
		return 1.0f;
	}
	if ( ctx->leaderDist > ctx->leash )
	{
		return 0.2f;
	}
	if ( ctx->leaderDist > ctx->leash * 0.75f )
	{
		return 0.6f;
	}
	return 1.0f;
}

static float Score_HoldCover( const aiContext_t *ctx )
{
	if ( !ctx->inCover || ctx->weaponless || ctx->enemyDist < 128.0f )
	{
		return 0;
	}
	float s = 0.45f + 0.35f * ctx->underFire;
	if ( ctx->nerveTier >= NT_SHAKEN )
	{
		s += 0.2f;
	}
	if ( ctx->enemyCS )
	{
		s += 0.1f;
	}
	return s;
}

static float Score_Relocate( const aiContext_t *ctx )
{
	if ( ctx->weaponless || ctx->slotsFree <= 0 || !TIMER_Done( NPC, "relocateFail" ) )
	{
		return 0;
	}
	float s = ctx->inCover ? 0.1f : 0.5f;
	s += 0.3f * ctx->underFire;
	if ( ctx->health < 0.5f )
	{
		s += 0.2f;
	}
	if ( !ctx->enemyLOS && ctx->msSinceSeen > 3000 && !ctx->enemyLost )
	{//lost the line on them: go find one
		s += 0.2f;
	}
	if ( ctx->enemySaber && ctx->enemyDist < 300.0f )
	{
		s += 0.3f;
	}
	if ( ctx->tooClose )
	{
		s += 0.2f;
	}
	return s * LeashFactor( ctx );
}

static float Score_Advance( const aiContext_t *ctx )
{
	if ( ctx->weaponless || ctx->enemyLost )
	{
		return 0;
	}
	if ( ctx->meleeWeapon )
	{//fists and staffs have nothing to do until they're in reach
		return ctx->enemyCS ? 0 : 0.9f;
	}
	if ( ctx->slotsFree <= 0 )
	{
		return 0;
	}
	if ( ctx->enemySaber )
	{
		return 0.05f;
	}
	if ( ctx->distNorm <= 1.0f )
	{//close enough already
		return 0;
	}
	float s = Q_min( 1.0f, 0.3f + ( ctx->distNorm - 1.0f ) * 0.5f );
	s *= NerveDrive( ctx );
	if ( !ctx->enemyLOS )
	{
		s += 0.2f;
	}
	return s * LeashFactor( ctx );
}

static float Score_Flank( const aiContext_t *ctx )
{
	if ( ctx->weaponless || ctx->slotsFree <= 0 || ctx->enemyLost || ctx->intelligence < 2 )
	{
		return 0;
	}
	if ( ctx->nerve < 60 || ctx->flankers >= 2 || !TIMER_Done( NPC, "flankFail" ) )
	{
		return 0;
	}
	float s = 0.35f + ( ctx->enemyLOS ? 0.0f : 0.3f ) + ( ctx->enemyCS ? -0.15f : 0.15f );
	if ( ctx->lone )
	{
		s *= 0.5f;
	}
	return s * LeashFactor( ctx );
}

static float Score_StandFire( const aiContext_t *ctx )
{
	if ( ctx->weaponless )
	{
		return 0;
	}
	float s = ctx->enemyCS ? 0.55f : 0.2f;
	s -= 0.25f * ctx->underFire;
	if ( ctx->inCover )
	{
		s -= 0.1f;
	}
	return Q_max( 0.1f, s );
}

static float Score_Strafe( const aiContext_t *ctx )
{
	if ( ctx->weaponless || ctx->inCover || !TIMER_Done( NPC, "strafeDebounce" ) )
	{
		return 0;
	}
	float s = 0.15f + 0.45f * ctx->underFire;
	if ( !ctx->enemyCS )
	{
		s *= 0.5f;
	}
	if ( ctx->enemyDist < 100.0f )
	{
		s += 0.2f;
	}
	return s;
}

static float Score_Grenade( const aiContext_t *ctx )
{
	if ( !ctx->hasThermal || !ctx->enemyLOS || ctx->hitAlly )
	{
		return 0;
	}
	if ( ctx->enemyDist < 200.0f || ctx->enemyDist > 600.0f )
	{
		return 0;
	}
	return 0.75f + ( ctx->inCover ? 0.1f : 0.0f );
}

static float Score_Suppress( const aiContext_t *ctx )
{
	if ( ctx->weaponless || !ctx->coverRequested || ctx->enemyCS || ctx->msSinceSeen > 3000 )
	{
		return 0;
	}
	return 0.7f;
}

static float Score_Retreat( const aiContext_t *ctx )
{
	if ( ctx->nerveTier >= NT_BREAKING )
	{
		return 0.8f + ( ctx->nerveTier == NT_BROKEN ? 0.15f : 0.0f );
	}
	if ( ctx->nerveTier == NT_SHAKEN && ctx->health < 0.4f )
	{
		return 0.55f;
	}
	if ( ctx->explosiveWeapon && ctx->enemyDist < 160.0f )
	{//too close to use what we're holding
		return 0.6f;
	}
	return 0;
}

static float Score_Flee( const aiContext_t *ctx )
{
	if ( !ctx->canFlee )
	{
		return 0;
	}
	if ( ctx->nerveTier == NT_BROKEN )
	{
		return 0.95f;
	}
	if ( ctx->weaponless && !ctx->seekingWeapon && ctx->enemyDist < 400.0f )
	{
		return 0.9f;
	}
	return 0;
}

static float Score_SeekWeapon( const aiContext_t *ctx )
{
	return ctx->seekingWeapon ? 1.0f : 0.0f;
}

static float Score_Track( const aiContext_t *ctx )
{
	if ( !ctx->enemyLost || ctx->weaponless || !NPC->enemy )
	{
		return 0;
	}
	if ( !NAV::InSameRegion( NPC, NPC->enemy->currentOrigin ) )
	{
		return 0.2f;
	}
	return 0.8f;
}

static float Score_Regroup( const aiContext_t *ctx )
{
	if ( !ctx->hasLeader )
	{
		return 0;
	}
	if ( ctx->leaderDist > ctx->leash * 1.5f )
	{//way off: nothing else matters
		return 1.0f;
	}
	if ( !ctx->leaderInPVS )
	{//lost sight of them
		return 0.9f;
	}
	if ( ctx->leaderDist > ctx->leash )
	{
		return 0.6f;
	}
	return 0;
}

typedef float (*scoreFn_t)( const aiContext_t *ctx );
static const scoreFn_t scoreFns[AA_NUM] =
{
	NULL, Score_HoldCover, Score_Relocate, Score_Advance, Score_Flank, Score_StandFire, Score_Strafe,
	Score_Grenade, Score_Suppress, Score_Retreat, Score_Flee, Score_SeekWeapon, Score_Track, Score_Regroup
};

/*
-------------------------
Executors
-------------------------
*/
static qboolean Brain_GoToCombatPoint( const aiContext_t *ctx, int cpFlags, float avoidDist )
{
	vec3_t	searchFrom;
	VectorCopy( NPC->currentOrigin, searchFrom );
	if ( ctx->hasLeader && ctx->leaderDist > ctx->leash * 0.5f )
	{//a companion picks its spots around the leader, not around wherever it has drifted to
		VectorCopy( NPC->client->leader->currentOrigin, searchFrom );
	}
	int cp = NPC_FindCombatPointRetry( searchFrom, NPC->currentOrigin, searchFrom, &cpFlags, avoidDist, NPCInfo->lastFailedCombatPoint );
	if ( cp == -1 )
	{
		return qfalse;
	}
	NPC_SetCombatPoint( cp );
	NPC_SetMoveGoal( NPC, level.combatPoints[cp].origin, 8, qtrue, cp );
	TIMER_Set( NPC, "roamTime", Q3_INFINITE );
	NPCInfo->aiFlags &= ~NPCAI_STOP_AT_LOS;
	if ( !( cpFlags & CP_FLANK ) && !( ( cpFlags & CP_COVER ) && ( cpFlags & CP_CLEAR ) ) && !Q_irand( 0, 3 ) )
	{
		NPCInfo->aiFlags |= NPCAI_STOP_AT_LOS;
	}
	return qtrue;
}

static qboolean Brain_Enter( int action, const aiContext_t *ctx )
{
	AIGroupInfo_t *group = NPCInfo->group;

	switch ( action )
	{
	case AA_HOLD_COVER:
		{
			static const int holdMin[NT_NUM] = { 4000, 6000, 8000, 8000 };
			static const int holdMax[NT_NUM] = { 7000, 10000, 12000, 12000 };
			AI_GroupUpdateSquadstates( group, NPC, SQUAD_COVER );
			TIMER_Set( NPC, "coverHold", Q_irand( holdMin[ctx->nerveTier], holdMax[ctx->nerveTier] ) );
			const int duckTime = Q_irand( 1200, 2500 );
			TIMER_Set( NPC, "duck", duckTime );
			TIMER_Set( NPC, "coverPop", duckTime + Q_irand( 800, 1800 ) );
			if ( !Q3_TaskIDPending( NPC, TID_MOVE_NAV ) )
			{
				NPCInfo->goalEntity = NULL;
			}
		}
		return qtrue;

	case AA_RELOCATE:
		{
			int cpFlags = CP_CLEAR|CP_COVER|CP_AVOID_ENEMY|CP_HAS_ROUTE|CP_TRYFAR;
			if ( ctx->health < 0.5f )
			{
				cpFlags |= CP_DUCK;
			}
			if ( ctx->enemySaber )
			{
				cpFlags |= CP_AVOID|CP_SAFE;
			}
			if ( !Brain_GoToCombatPoint( ctx, cpFlags, 200.0f ) )
			{
				vec3_t pos;
				if ( !ST_FindGeoCover( pos ) )
				{
					TIMER_Set( NPC, "relocateFail", 3000 );
					AI_NerveEvent( NPC, NE_NO_COVER, 1.0f, NPC->enemy );
					if ( ctx->lone )
					{//nowhere to go: fight from here
						TIMER_Set( NPC, "cornered", 3000 );
						AI_NerveEvent( NPC, NE_CORNERED, 1.0f, NPC->enemy );
					}
					return qfalse;
				}
				NPC_SetMoveGoal( NPC, pos, 16, qtrue, -1, NULL );
			}
			TIMER_Set( NPC, "cornered", -1 );
			AI_GroupUpdateSquadstates( group, NPC, SQUAD_TRANSITION );
			AI_SquadRequestCover( NPC );
			if ( !ctx->enemyLOS )
			{
				ST_SayCover();
			}
		}
		return qtrue;

	case AA_ADVANCE:
		if ( ctx->meleeWeapon )
		{//straight at them, stop just inside reach
			NPC_FreeCombatPoint( NPCInfo->combatPoint, qfalse );
			NPCInfo->goalEntity = NPC->enemy;
			NPCInfo->goalRadius = NPC->maxs[0] + NPC->enemy->maxs[0] + ( NPC->client->ps.weapon == WP_TUSKEN_STAFF ? 48.0f : 12.0f );
			AI_GroupUpdateSquadstates( group, NPC, SQUAD_SCOUT );
			return qtrue;
		}
		if ( !Brain_GoToCombatPoint( ctx, CP_CLEAR|CP_COVER|CP_APPROACH_ENEMY|CP_AVOID_ENEMY|CP_HAS_ROUTE, 128.0f ) )
		{
			NPC_SetMoveGoal( NPC, NPCInfo->enemyLastSeenLocation, 64, qfalse, -1, NULL );
			NPCInfo->aiFlags |= NPCAI_STOP_AT_LOS;
		}
		AI_GroupUpdateSquadstates( group, NPC, SQUAD_SCOUT );
		AI_SquadRequestCover( NPC );
		return qtrue;

	case AA_FLANK:
		if ( !Brain_GoToCombatPoint( ctx, CP_FLANK|CP_CLEAR|CP_APPROACH_ENEMY|CP_AVOID_ENEMY|CP_HAS_ROUTE, 128.0f ) )
		{
			TIMER_Set( NPC, "flankFail", 5000 );
			return qfalse;
		}
		AI_GroupUpdateSquadstates( group, NPC, SQUAD_TRANSITION );
		AI_SquadRequestCover( NPC );
		if ( group )
		{
			if ( group->flankLeftNum == ENTITYNUM_NONE )
			{
				group->flankLeftNum = NPC->s.number;
				NPCInfo->aiFlankSide = -1;
			}
			else
			{
				group->flankRightNum = NPC->s.number;
				NPCInfo->aiFlankSide = 1;
			}
		}
		ST_SayOutflank();
		return qtrue;

	case AA_STAND_FIRE:
	case AA_SUPPRESS:
		if ( !Q3_TaskIDPending( NPC, TID_MOVE_NAV ) )
		{
			NPC_FreeCombatPoint( NPCInfo->combatPoint, qfalse );
			NPCInfo->goalEntity = NULL;
		}
		AI_GroupUpdateSquadstates( group, NPC, SQUAD_STAND_AND_SHOOT );
		if ( action == AA_SUPPRESS )
		{
			TIMER_Set( NPC, "suppressFire", 3000 );
		}
		return qtrue;

	case AA_STRAFE:
		{
			const int strafeTime = Q_irand( 400, 900 );
			TIMER_Set( NPC, Q_irand( 0, 1 ) ? "strafeLeft" : "strafeRight", strafeTime );
			TIMER_Set( NPC, "strafeDebounce", strafeTime + Q_irand( 1000, 2000 ) );
		}
		return qtrue;

	case AA_GRENADE:
		ST_StartThermalThrow();
		return qtrue;

	case AA_RETREAT:
		if ( !Brain_GoToCombatPoint( ctx, CP_COVER|CP_AVOID|CP_SAFE|CP_RETREAT|CP_AVOID_ENEMY|CP_HAS_ROUTE, 200.0f ) )
		{
			vec3_t pos;
			if ( ST_FindGeoCover( pos ) || ( NPC->enemy && NPC_FindFleeSpot( NPC->enemy->currentOrigin, pos ) ) )
			{
				NPC_SetMoveGoal( NPC, pos, 16, qtrue, -1, NULL );
			}
			else
			{
				TIMER_Set( NPC, "relocateFail", 3000 );
				return qfalse;
			}
		}
		AI_GroupUpdateSquadstates( group, NPC, SQUAD_RETREAT );
		TIMER_Set( NPC, "duck", -1 );
		ST_SayCover();
		return qtrue;

	case AA_FLEE:
		if ( !NPC->enemy )
		{
			return qfalse;
		}
		return NPC_StartFlee( NPC->enemy, NPC->enemy->currentOrigin, AEL_DANGER_GREAT, 4000, 8000 );

	case AA_SEEK_WEAPON:
		return qtrue;	// NPC_CheckGetNewWeapon already set the pickup goal

	case AA_TRACK:
		if ( !NPC->enemy )
		{
			return qfalse;
		}
		ST_TrackEnemy( NPC, NPC->enemy->currentOrigin );
		AI_GroupUpdateSquadstates( group, NPC, SQUAD_SCOUT );
		return qtrue;

	case AA_REGROUP:
		if ( !ctx->hasLeader )
		{
			return qfalse;
		}
		//back to the leader; the follow code's own distance is the stopping radius
		NPC_FreeCombatPoint( NPCInfo->combatPoint, qfalse );
		NPCInfo->goalEntity = NPC->client->leader;
		NPCInfo->goalRadius = Q_max( 96.0f, NPCInfo->followDist );
		NPCInfo->aiFlags &= ~NPCAI_STOP_AT_LOS;
		AI_GroupUpdateSquadstates( group, NPC, SQUAD_TRANSITION );
		return qtrue;

	default:
		return qtrue;
	}
}

static void Brain_Exit( int action )
{
	AIGroupInfo_t *group = NPCInfo->group;
	switch ( action )
	{
	case AA_HOLD_COVER:
		TIMER_Set( NPC, "coverHold", -1 );
		TIMER_Set( NPC, "coverPop", -1 );
		TIMER_Set( NPC, "duck", -1 );
		if ( NPCInfo->squadState == SQUAD_COVER )
		{
			AI_GroupUpdateSquadstates( group, NPC, SQUAD_STAND_AND_SHOOT );
		}
		break;
	case AA_SUPPRESS:
		TIMER_Set( NPC, "suppressFire", -1 );
		break;
	case AA_FLANK:
		if ( group )
		{
			if ( group->flankLeftNum == NPC->s.number )
			{
				group->flankLeftNum = ENTITYNUM_NONE;
			}
			if ( group->flankRightNum == NPC->s.number )
			{
				group->flankRightNum = ENTITYNUM_NONE;
			}
		}
		break;
	default:
		break;
	}
}

// an action has run its course (arrived, gave up, timer ended): drop it so the next rescore
// starts fresh. Just interrupting isn't enough: if the same action scores best again it has
// to be *entered* again (a chase whose goal was cleared would otherwise stand there forever
// with ADVANCE still "current").
static void Brain_Finish( void )
{
	if ( NPCInfo->aiAction != AA_NONE )
	{
		Brain_Exit( NPCInfo->aiAction );
	}
	NPCInfo->aiAction = AA_NONE;
	AI_Brain_Interrupt( NPC );
}

// per think while an action is current: notice when it has run its course
static void Brain_Update( int action )
{
	switch ( action )
	{
	case AA_HOLD_COVER:
		if ( TIMER_Done( NPC, "coverHold" ) )
		{
			Brain_Finish();
		}
		break;
	case AA_ADVANCE:
	case AA_REGROUP:
		if ( NPCInfo->goalEntity && ( NPCInfo->goalEntity == NPC->enemy || NPCInfo->goalEntity == NPC->client->leader ) )
		{//chasing someone directly: done once we're in reach
			if ( DistanceSquared( NPC->currentOrigin, NPCInfo->goalEntity->currentOrigin ) < NPCInfo->goalRadius * NPCInfo->goalRadius )
			{
				NPCInfo->goalEntity = NULL;
				Brain_Finish();
			}
			break;
		}
		//fall through
	case AA_RELOCATE:
	case AA_FLANK:
	case AA_RETREAT:
	case AA_TRACK:
		if ( !NPCInfo->goalEntity && !Q3_TaskIDPending( NPC, TID_MOVE_NAV ) )
		{//got there (or gave up)
			Brain_Finish();
		}
		else if ( level.time - NPCInfo->aiActionStart > 1000 && level.time - NPC->lastMoveTime > 800 )
		{//been trying to go somewhere and haven't actually moved: wedged against something
			TIMER_Set( NPC, "relocateFail", 2000 );
			NPC_FreeCombatPoint( NPCInfo->combatPoint, qfalse );
			NPCInfo->goalEntity = NULL;
			Brain_Finish();
		}
		break;
	case AA_FLEE:
		if ( TIMER_Done( NPC, "flee" ) )
		{
			Brain_Finish();
		}
		break;
	default:
		break;
	}
}

/*
-------------------------
AI_Brain_Think
-------------------------
*/
static void Brain_Log( gentity_t *npc, const float *scores, int from, int to )
{
	if ( !d_aiLog || !d_aiLog->integer )
	{
		return;
	}
	if ( d_aiLog->integer > 0 && d_aiLog->integer != npc->s.number )
	{
		return;
	}
	char line[512];
	Com_sprintf( line, sizeof(line), "BRAIN %s (%d): %s -> %s |", npc->NPC_type, npc->s.number, AI_Brain_ActionName( from ), AI_Brain_ActionName( to ) );
	for ( int a = 1; a < AA_NUM; a++ )
	{
		if ( scores[a] > 0 )
		{
			Q_strcat( line, sizeof(line), va( " %s %.2f", actionDefs[a].name, scores[a] ) );
		}
	}
	gi.Printf( "%s\n", line );
}

void AI_Brain_Think( gentity_t *npc, qboolean enemyLOS, qboolean enemyCS, qboolean enemyInFOV, qboolean hitAlly, float enemyDistSq )
{
	if ( !npc || npc != NPC || !NPCInfo || !NPC->enemy )
	{
		return;
	}
	if ( Q3_TaskIDPending( NPC, TID_MOVE_NAV ) || !( NPCInfo->scriptFlags & SCF_CHASE_ENEMIES ) )
	{//a script owns the movement, or this one isn't allowed to move on its own
		return;
	}

	const int cur = NPCInfo->aiAction;

	if ( !TIMER_Done( NPC, "aiRescore" ) )
	{
		Brain_Update( cur );
		return;
	}
	TIMER_Set( NPC, "aiRescore", Q_irand( 300, 600 ) - NPCInfo->stats.reactions * 40 );

	const aiProfile_t	*prof = AI_Brain_ProfileFor( NPC );
	aiContext_t			ctx;
	float				scores[AA_NUM];
	int					best = AA_NONE;
	float				bestScore = 0.0f;

	Brain_BuildContext( &ctx, prof, enemyLOS, enemyCS, enemyInFOV, hitAlly, enemyDistSq );

	// a little noise keeps squads from all picking the same thing; dimmer NPCs get more of it
	const float noise = ( 5 - Com_Clamp( 1, 5, ctx.intelligence ) ) * 0.04f;

	scores[AA_NONE] = 0;
	for ( int a = 1; a < AA_NUM; a++ )
	{
		float s = 0;
		if ( ( prof->allowMask & ( 1 << a ) ) && scoreFns[a] )
		{
			s = scoreFns[a]( &ctx ) * prof->weight[a];
			if ( s > 0 )
			{
				s += Q_flrand( -noise, noise );
				if ( a == cur )
				{//commitment: the thing we're doing gets the benefit of the doubt
					s = ( s + 0.15f ) * 1.25f;
				}
			}
		}
		scores[a] = Q_max( 0.0f, s );
		if ( scores[a] > bestScore )
		{
			bestScore = scores[a];
			best = a;
		}
	}

	aiDebug_t *dbg = &s_aiDebug[NPC->s.number];
	memcpy( dbg->score, scores, sizeof( scores ) );

	if ( best != cur && best != AA_NONE )
	{
		qboolean canSwitch = (qboolean)( cur == AA_NONE || level.time >= NPCInfo->aiActionHoldUntil );
		if ( !canSwitch && ( actionDefs[best].flags & AAF_EMERGENCY ) && bestScore > scores[cur] * 1.5f )
		{
			canSwitch = qtrue;
		}
		if ( canSwitch )
		{
			if ( cur != AA_NONE )
			{
				Brain_Exit( cur );
			}
			if ( Brain_Enter( best, &ctx ) )
			{
				Brain_Log( NPC, scores, cur, best );
				NPCInfo->aiAction = best;
				NPCInfo->aiActionStart = level.time;
				NPCInfo->aiActionHoldUntil = level.time + actionDefs[best].minHoldMs;
				dbg->lastSwitchTime = level.time;
			}
			else
			{//couldn't start it; fall back to standing and shooting rather than freezing
				if ( cur == AA_NONE || cur == best )
				{
					Brain_Enter( AA_STAND_FIRE, &ctx );
					NPCInfo->aiAction = AA_STAND_FIRE;
					NPCInfo->aiActionStart = level.time;
					NPCInfo->aiActionHoldUntil = level.time + actionDefs[AA_STAND_FIRE].minHoldMs;
				}
				else
				{//keep doing what we were doing
					Brain_Enter( cur, &ctx );
				}
			}
		}
	}

	dbg->action = NPCInfo->aiAction;
	dbg->holdUntil = NPCInfo->aiActionHoldUntil;
	Brain_Update( NPCInfo->aiAction );
}
