/*
===========================================================================
Squads.

A squad is the NPCs that were placed together (a shared "squadname" spawn key) plus
whoever is close by when the shooting starts. There's no map-wide merging by shared
enemy any more: a squad is a handful of people who can actually see each other. Morale
lives in each member's nerve (AI_Nerve.cpp); the squad only coordinates: who's allowed
to be moving right now, who's covering whom, where the enemy was last seen, and who
leads. Storage is still level.groups[] / AIGroupInfo_t so the save game and the pointer
fixups don't change.
===========================================================================
*/

#include "b_local.h"
#include "g_navigator.h"
#include "AI_Brain.h"

extern cvar_t	*d_noGroupAI;
extern void		AI_DissolveGroup( AIGroupInfo_t *group );

#define SQUAD_MAX_MEMBERS		8
#define SQUAD_JOIN_RADIUS		768.0f
#define SQUAD_LOST_ENEMY_MS		60000

/*
-------------------------
AI_SquadEligible

Who can be in a squad at all. Mechs, droids and creatures run their own AI.
-------------------------
*/
qboolean AI_SquadEligible( gentity_t *ent )
{
	if ( !ent || !ent->inuse || !ent->client || !ent->NPC )
	{
		return qfalse;
	}
	if ( ent->health <= 0 || G_IsNeutralized( ent ) )
	{
		return qfalse;
	}
	if ( (ent->NPC->scriptFlags&SCF_NO_GROUPS) )
	{
		return qfalse;
	}
	if ( ent->NPC->confusionTime > level.time || ent->NPC->insanityTime > level.time )
	{
		return qfalse;
	}
	if ( (ent->s.eFlags&(EF_LOCKED_TO_WEAPON|EF_HELD_BY_RANCOR|EF_HELD_BY_SAND_CREATURE|EF_HELD_BY_WAMPA)) )
	{
		return qfalse;
	}
	switch ( ent->client->ps.weapon )
	{
	case WP_EMPLACED_GUN:
	case WP_BOT_LASER:
	case WP_TURRET:
	case WP_ATST_MAIN:
	case WP_ATST_SIDE:
	case WP_TIE_FIGHTER:
		return qfalse;
	default:
		break;
	}
	switch ( ent->client->NPC_class )
	{
	case CLASS_ATST:
	case CLASS_PROBE:
	case CLASS_SEEKER:
	case CLASS_REMOTE:
	case CLASS_SENTRY:
	case CLASS_INTERROGATOR:
	case CLASS_MINEMONSTER:
	case CLASS_HOWLER:
	case CLASS_RANCOR:
	case CLASS_WAMPA:
	case CLASS_SAND_CREATURE:
	case CLASS_MARK1:
	case CLASS_MARK2:
	case CLASS_GALAKMECH:
	case CLASS_VEHICLE:
		return qfalse;
	default:
		break;
	}
	return qtrue;
}

static const char *Squad_NameOf( const AIGroupInfo_t *group )
{
	if ( !group->numGroup )
	{
		return NULL;
	}
	const gentity_t *first = &g_entities[group->member[0].number];
	return ( first->squadname && first->squadname[0] ) ? first->squadname : NULL;
}

static qboolean Squad_HasMemberNear( const AIGroupInfo_t *group, const gentity_t *ent, float *outDist2 )
{
	float best = SQUAD_JOIN_RADIUS*SQUAD_JOIN_RADIUS;
	qboolean found = qfalse;
	for ( int i = 0; i < group->numGroup; i++ )
	{
		const gentity_t *member = &g_entities[group->member[i].number];
		const float d2 = DistanceSquared( member->currentOrigin, ent->currentOrigin );
		if ( d2 < best && gi.inPVS( member->currentOrigin, ent->currentOrigin ) )
		{
			best = d2;
			found = qtrue;
		}
	}
	if ( outDist2 )
	{
		*outDist2 = best;
	}
	return found;
}

static AIGroupInfo_t *Squad_New( gentity_t *self )
{
	for ( int i = 0; i < MAX_FRAME_GROUPS; i++ )
	{
		AIGroupInfo_t *group = &level.groups[i];
		if ( group->numGroup )
		{
			continue;
		}
		memset( group, 0, sizeof( *group ) );
		group->squadId		= i + 1;
		group->team			= self->client->playerTeam;
		group->formedTime	= level.time;
		group->leaderNum	= ENTITYNUM_NONE;
		group->flankLeftNum = group->flankRightNum = group->coverRequestNum = group->duelLeadNum = ENTITYNUM_NONE;
		group->enemy		= self->enemy;
		if ( group->enemy )
		{
			group->lastSeenEnemyTime = level.time;
			VectorCopy( group->enemy->currentOrigin, group->enemyLastSeenPos );
			VectorCopy( group->enemy->currentOrigin, group->lastKnownEnemyPos );
			group->lastKnownEnemyTime = level.time;
		}
		return group;
	}
	return NULL;
}

/*
-------------------------
AI_SquadJoin

Put this NPC in a squad: the one that shares its squadname, else the nearest one with a
member in sight, else (once it has an enemy) a new one of its own.
-------------------------
*/
void AI_SquadJoin( gentity_t *self )
{
	if ( !self || !self->NPC )
	{
		return;
	}
	if ( d_noGroupAI->integer || !AI_SquadEligible( self ) )
	{
		self->NPC->group = NULL;
		return;
	}
	if ( self->NPC->group && self->NPC->group->numGroup && AI_GroupContainsEntNum( self->NPC->group, self->s.number ) )
	{//already in one
		return;
	}
	self->NPC->group = NULL;

	const char		*myName = ( self->squadname && self->squadname[0] ) ? self->squadname : NULL;
	AIGroupInfo_t	*best = NULL;
	float			bestD2 = Q3_INFINITE;

	for ( int i = 0; i < MAX_FRAME_GROUPS; i++ )
	{
		AIGroupInfo_t *group = &level.groups[i];
		if ( !group->numGroup || group->team != self->client->playerTeam || group->numGroup >= SQUAD_MAX_MEMBERS )
		{
			continue;
		}
		const char *theirName = Squad_NameOf( group );
		if ( myName )
		{//placed as part of a named squad: only that squad will do
			if ( theirName && !Q_stricmp( myName, theirName ) )
			{
				best = group;
				break;
			}
			continue;
		}
		float d2;
		if ( Squad_HasMemberNear( group, self, &d2 ) && d2 < bestD2 )
		{
			bestD2 = d2;
			best = group;
		}
	}

	if ( !best )
	{
		if ( !myName && !self->enemy )
		{//nobody near and nothing to fight yet: no point holding a slot
			return;
		}
		best = Squad_New( self );
		if ( !best )
		{
			return;
		}
	}

	AI_InsertGroupMember( best, self );
	if ( !best->enemy && self->enemy )
	{
		best->enemy = self->enemy;
		best->lastSeenEnemyTime = level.time;
	}
	AI_SortGroupByPathCostToEnemy( best );
	AI_SetClosestBuddy( best );
}

/*
-------------------------
AI_SquadUpdate

Once a frame per squad: prune, recount, keep the leader and the shared enemy current,
expire coordination requests. Returns qfalse when the squad is empty and can be dropped.
-------------------------
*/
qboolean AI_SquadUpdate( AIGroupInfo_t *group )
{
	int	i;

	for ( i = 0; i < group->numGroup; i++ )
	{
		gentity_t *member = &g_entities[group->member[i].number];
		if ( !member->inuse || !member->client || !member->NPC
			|| member->health <= 0
			|| G_IsNeutralized( member )
			|| member->client->playerTeam != group->team
			|| member->NPC->group != group )
		{
			AI_DeleteGroupMember( group, i );
			i--;
		}
	}
	if ( !group->numGroup )
	{
		return qfalse;
	}

	for ( i = 0; i < NUM_SQUAD_STATES; i++ )
	{
		group->numState[i] = 0;
	}
	int nerveSum = 0;
	for ( i = 0; i < group->numGroup; i++ )
	{
		gentity_t *member = &g_entities[group->member[i].number];
		group->numState[member->NPC->squadState]++;
		nerveSum += member->NPC->nerveBase + member->NPC->nerveShock;
	}
	group->avgNerve = nerveSum / group->numGroup;

	// the leader stays the leader until they're gone
	if ( group->leaderNum == ENTITYNUM_NONE || !AI_GroupContainsEntNum( group, group->leaderNum ) )
	{
		AI_SetNewGroupCommander( group );
	}
	else
	{
		group->commander = &g_entities[group->leaderNum];
	}

	// the shared enemy: whoever the leader is fighting, else the first member who has one
	if ( group->enemy && ( !group->enemy->inuse || group->enemy->health <= 0 ) )
	{
		group->enemy = NULL;
	}
	if ( !group->enemy )
	{
		gentity_t *enemy = group->commander ? group->commander->enemy : NULL;
		for ( i = 0; !enemy && i < group->numGroup; i++ )
		{
			enemy = g_entities[group->member[i].number].enemy;
		}
		if ( enemy && enemy->inuse && enemy->health > 0 )
		{
			group->enemy = enemy;
			group->lastSeenEnemyTime = level.time;
		}
	}
	else if ( group->lastSeenEnemyTime < level.time - SQUAD_LOST_ENEMY_MS )
	{//nobody's seen them in a minute: everyone goes looking, and the squad stands down
		AI_DissolveGroup( group );
	}

	if ( group->coverRequestNum != ENTITYNUM_NONE
		&& ( group->coverRequestTime < level.time || !AI_GroupContainsEntNum( group, group->coverRequestNum ) ) )
	{
		group->coverRequestNum = ENTITYNUM_NONE;
	}
	if ( group->flankLeftNum != ENTITYNUM_NONE
		&& ( !AI_GroupContainsEntNum( group, group->flankLeftNum ) || g_entities[group->flankLeftNum].NPC->aiAction != AA_FLANK ) )
	{
		group->flankLeftNum = ENTITYNUM_NONE;
	}
	if ( group->flankRightNum != ENTITYNUM_NONE
		&& ( !AI_GroupContainsEntNum( group, group->flankRightNum ) || g_entities[group->flankRightNum].NPC->aiAction != AA_FLANK ) )
	{
		group->flankRightNum = ENTITYNUM_NONE;
	}
	group->relocatingCount = group->numState[SQUAD_TRANSITION] + group->numState[SQUAD_SCOUT];

	//mark this group as not having been run this frame
	group->processed = qfalse;
	return qtrue;
}

// how many more members may be on the move right now (at least one, never more than half)
int AI_SquadSlotsFree( const AIGroupInfo_t *group )
{
	if ( !group || group->numGroup <= 1 )
	{
		return 1;
	}
	const int maxMoving = Q_max( 1, group->numGroup / 2 );
	return maxMoving - ( group->numState[SQUAD_TRANSITION] + group->numState[SQUAD_SCOUT] );
}

// a member about to move asks the squad for covering fire
void AI_SquadRequestCover( gentity_t *mover )
{
	if ( !mover || !mover->NPC || !mover->NPC->group )
	{
		return;
	}
	mover->NPC->group->coverRequestNum = mover->s.number;
	mover->NPC->group->coverRequestTime = level.time + 3000;
}

// how many squadmates are already flanking
int AI_SquadFlankers( const AIGroupInfo_t *group, const gentity_t *except )
{
	if ( !group )
	{
		return 0;
	}
	int count = 0;
	for ( int i = 0; i < group->numGroup; i++ )
	{
		const gentity_t *member = &g_entities[group->member[i].number];
		if ( member != except && member->NPC && member->NPC->aiAction == AA_FLANK )
		{
			count++;
		}
	}
	return count;
}
