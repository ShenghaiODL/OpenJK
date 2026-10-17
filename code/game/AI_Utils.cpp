/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

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

// Squad (AIGroupInfo_t) bookkeeping shared by the AI_ files. Squads are formed and kept
// current in AI_Squad.cpp; this file holds the member-list primitives.

#include "b_local.h"
#include "g_nav.h"
#include "g_navigator.h"

extern cvar_t		*d_noGroupAI;

void AI_SetClosestBuddy( AIGroupInfo_t *group )
{
	int	i, j;
	int	dist, bestDist;

	for ( i = 0; i < group->numGroup; i++ )
	{
		group->member[i].closestBuddy = ENTITYNUM_NONE;

		bestDist = Q3_INFINITE;
		for ( j = 0; j < group->numGroup; j++ )
		{
			if ( j == i )
			{//you are not your own buddy
				continue;
			}
			dist = DistanceSquared( g_entities[group->member[i].number].currentOrigin, g_entities[group->member[j].number].currentOrigin );
			if ( dist < bestDist )
			{
				bestDist = dist;
				group->member[i].closestBuddy = group->member[j].number;
			}
		}
	}
}


// the leader: highest rank, first one in on a tie, and it stays put while they live
void AI_SetNewGroupCommander( AIGroupInfo_t *group )
{
	gentity_t *member = NULL;
	group->commander = NULL;
	group->leaderNum = ENTITYNUM_NONE;
	for ( int i = 0; i < group->numGroup; i++ )
	{
		member = &g_entities[group->member[i].number];
		if ( !member->NPC )
		{
			continue;
		}
		if ( !group->commander || !group->commander->NPC || member->NPC->rank > group->commander->NPC->rank )
		{
			group->commander = member;
			group->leaderNum = member->s.number;
		}
	}
}

void AI_InsertGroupMember( AIGroupInfo_t *group, gentity_t *member )
{
	int i;
	//okay, you know what?  Check this damn group and make sure we're not already in here!
	for ( i = 0; i < group->numGroup; i++ )
	{
		if ( group->member[i].number == member->s.number )
		{//already in here
			break;
		}
	}
	if ( i < group->numGroup )
	{//found him in group already
	}
	else
	{//add him in
		group->member[group->numGroup++].number = member->s.number;
		group->numState[member->NPC->squadState]++;
	}
	if ( !group->commander || !group->commander->NPC || (member->NPC->rank > group->commander->NPC->rank) )
	{//keep track of highest rank
		group->commander = member;
		group->leaderNum = member->s.number;
	}
	member->NPC->group = group;
	member->NPC->squadNum = group->squadId;

	if ( group->enemy && group->numGroup > 1 )
	{//help arrived mid-fight (debounced per NPC, so the constant regrouping doesn't spam it)
		for ( i = 0; i < group->numGroup; i++ )
		{
			gentity_t *other = &g_entities[group->member[i].number];
			if ( other != member )
			{
				AI_NerveEvent( other, NE_REINFORCED, 1.0f, member );
			}
		}
	}
}

void AI_DeleteGroupMember( AIGroupInfo_t *group, int memberNum )
{
	gentity_t *member = &g_entities[group->member[memberNum].number];
	if ( group->commander && group->commander->s.number == group->member[memberNum].number )
	{
		group->commander = NULL;
	}
	if ( member->NPC && member->NPC->group == group )
	{//(a member whose pointer already moved to another squad keeps that one)
		member->NPC->group = NULL;
		member->NPC->squadNum = 0;
	}
	for ( int i = memberNum; i < (group->numGroup-1); i++ )
	{
		memcpy( &group->member[i], &group->member[i+1], sizeof( group->member[i] ) );
	}
	group->numGroup--;
	if ( group->numGroup < 0 )
	{
		group->numGroup = 0;
	}
	if ( !group->commander )
	{
		AI_SetNewGroupCommander( group );
	}
}

void AI_DeleteSelfFromGroup( gentity_t *self )
{
	for ( int i = 0; i < self->NPC->group->numGroup; i++ )
	{
		if ( self->NPC->group->member[i].number == self->s.number )
		{
			AI_DeleteGroupMember( self->NPC->group, i );
			return;
		}
	}
}

void AI_GroupUpdateEnemyLastSeen( AIGroupInfo_t *group, vec3_t spot )
{
	if ( !group )
	{
		return;
	}
	group->lastSeenEnemyTime = level.time;
	VectorCopy( spot, group->enemyLastSeenPos );
	VectorCopy( spot, group->lastKnownEnemyPos );
	group->lastKnownEnemyTime = level.time;
}

void AI_GroupUpdateSquadstates( AIGroupInfo_t *group, gentity_t *member, int newSquadState )
{
	if ( !group )
	{
		member->NPC->squadState = newSquadState;
		return;
	}

	for ( int i = 0; i < group->numGroup; i++ )
	{
		if ( group->member[i].number == member->s.number )
		{
			group->numState[member->NPC->squadState]--;
			member->NPC->squadState = newSquadState;
			group->numState[member->NPC->squadState]++;
			return;
		}
	}
}

// once a frame for every live squad
void AI_UpdateGroups( void )
{
	if ( d_noGroupAI->integer )
	{
		return;
	}
	for ( int i = 0; i < MAX_FRAME_GROUPS; i++ )
	{
		if ( !level.groups[i].numGroup || AI_SquadUpdate( &level.groups[i] ) == qfalse )
		{
			memset( &level.groups[i], 0, sizeof( level.groups[i] ) );
		}
	}
}

qboolean AI_GroupContainsEntNum( AIGroupInfo_t *group, int entNum )
{
	if ( !group )
	{
		return qfalse;
	}
	for ( int i = 0; i < group->numGroup; i++ )
	{
		if ( group->member[i].number == entNum )
		{
			return qtrue;
		}
	}
	return qfalse;
}
