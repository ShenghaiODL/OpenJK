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

#ifndef __AI__
#define __AI__

//Distance ratings
enum distance_e
{
	DIST_MELEE,
	DIST_LONG,
};

//Attack types
enum attack_e
{
	ATTACK_MELEE,
	ATTACK_RANGE,
};

enum
{
	SQUAD_IDLE,					//No target found, waiting
	SQUAD_STAND_AND_SHOOT,		//Standing in position and shoot (no cover)
	SQUAD_RETREAT,				//Running away from combat
	SQUAD_COVER,				//Under protective cover
	SQUAD_TRANSITION,			//Moving between points, not firing
	SQUAD_POINT,				//On point, laying down suppressive fire
	SQUAD_SCOUT,				//Poking out to draw enemy
	NUM_SQUAD_STATES,
};

//sigh... had to move in here for groupInfo
typedef enum //# rank_e
{
	RANK_CIVILIAN,
	RANK_CREWMAN,
	RANK_ENSIGN,
	RANK_LT_JG,
	RANK_LT,
	RANK_LT_COMM,
	RANK_COMMANDER,
	RANK_CAPTAIN,
    RANK_MAX
} rank_t;

qboolean NPC_CheckPlayerTeamStealth( void );

//AI_STORMTROOPER (snipers, grenadiers and Tusken use it too, with a brain profile + AI_FireModules.cpp)
void Saboteur_Decloak( gentity_t *self, int uncloakTime = 8000 );
void NPC_BSST_Investigate( void );
void NPC_BSST_Default( void );
void NPC_BSST_Sleep( void );

//AI_JEDI
void NPC_BSJedi_Investigate( void );
void NPC_BSJedi_Default( void );
void NPC_BSJedi_FollowLeader( void );

// AI_DROID
void NPC_BSDroid_Default( void );

// AI_ImperialProbe
void NPC_BSImperialProbe_Default( void );

// AI_atst
void NPC_BSATST_Default( void );

void NPC_BSInterrogator_Default( void );

// AI Mark 1
void NPC_BSMark1_Default( void );

// AI Mark 2
void NPC_BSMark2_Default( void );

//monsters
void NPC_BSMineMonster_Default( void );
void NPC_BSHowler_Default( void );
void NPC_BSRancor_Default( void );
void NPC_BSWampa_Default( void );
void NPC_BSSandCreature_Default( void );

// animals
void NPC_BSAnimal_Default( void );

//Utilities
//Group AI
#define	MAX_FRAME_GROUPS	32
// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!
class AIGroupMember_t
{
public:
	int	number;
	int waypoint;
	int	closestBuddy;


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<int32_t>(number);
		saved_game.write<int32_t>(waypoint);
		saved_game.write<int32_t>(closestBuddy);
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game, int32_t version)
	{
		saved_game.read<int32_t>(number);
		saved_game.read<int32_t>(waypoint);
		saved_game.read<int32_t>(closestBuddy);
	}
}; // AIGroupMember_t

#define MAX_GROUP_MEMBERS 32
// !!!!!!!!!! LOADSAVE-affecting structure !!!!!!!!!!
class AIGroupInfo_t
{
public:
	int			numGroup;
	qboolean	processed;
	team_t		team;
	gentity_t	*enemy;
	int			enemyWP;
	int			speechDebounceTime;
	int			lastSeenEnemyTime;
	gentity_t	*commander;
	vec3_t		enemyLastSeenPos;
	int			numState[ NUM_SQUAD_STATES ];
	AIGroupMember_t member[ MAX_GROUP_MEMBERS ];
	// squad coordination (AI_Squad.cpp)
	int			squadId;
	int			leaderNum;
	int			formedTime;
	int			avgNerve;
	int			relocatingCount;
	int			flankLeftNum;
	int			flankRightNum;
	int			coverRequestNum;
	int			coverRequestTime;
	int			duelLeadNum;
	vec3_t		lastKnownEnemyPos;
	int			lastKnownEnemyTime;


	void sg_export(
		ojk::SavedGameHelper& saved_game) const
	{
		saved_game.write<int32_t>(numGroup);
		saved_game.write<int32_t>(processed);
		saved_game.write<int32_t>(team);
		saved_game.write<int32_t>(enemy);
		saved_game.write<int32_t>(enemyWP);
		saved_game.write<int32_t>(speechDebounceTime);
		saved_game.write<int32_t>(lastSeenEnemyTime);
		saved_game.write<int32_t>(commander);
		saved_game.write<float>(enemyLastSeenPos);
		saved_game.write<int32_t>(numState);
		saved_game.write<>(member);
		saved_game.write<int32_t>(squadId);
		saved_game.write<int32_t>(leaderNum);
		saved_game.write<int32_t>(formedTime);
		saved_game.write<int32_t>(avgNerve);
		saved_game.write<int32_t>(relocatingCount);
		saved_game.write<int32_t>(flankLeftNum);
		saved_game.write<int32_t>(flankRightNum);
		saved_game.write<int32_t>(coverRequestNum);
		saved_game.write<int32_t>(coverRequestTime);
		saved_game.write<int32_t>(duelLeadNum);
		saved_game.write<float>(lastKnownEnemyPos);
		saved_game.write<int32_t>(lastKnownEnemyTime);
	}

	void sg_import(
		ojk::SavedGameHelper& saved_game, int32_t version)
	{
		saved_game.read<int32_t>(numGroup);
		saved_game.read<int32_t>(processed);
		saved_game.read<int32_t>(team);
		saved_game.read<int32_t>(enemy);
		saved_game.read<int32_t>(enemyWP);
		saved_game.read<int32_t>(speechDebounceTime);
		saved_game.read<int32_t>(lastSeenEnemyTime);
		saved_game.read<int32_t>(commander);
		saved_game.read<float>(enemyLastSeenPos);
		saved_game.read<int32_t>(numState);
		saved_game.read<>(member);
		saved_game.read<int32_t>(squadId);
		saved_game.read<int32_t>(leaderNum);
		saved_game.read<int32_t>(formedTime);
		saved_game.read<int32_t>(avgNerve);
		saved_game.read<int32_t>(relocatingCount);
		saved_game.read<int32_t>(flankLeftNum);
		saved_game.read<int32_t>(flankRightNum);
		saved_game.read<int32_t>(coverRequestNum);
		saved_game.read<int32_t>(coverRequestTime);
		saved_game.read<int32_t>(duelLeadNum);
		saved_game.read<float>(lastKnownEnemyPos);
		saved_game.read<int32_t>(lastKnownEnemyTime);
	}
}; // AIGroupInfo_t


// squads (AI_Squad.cpp)
qboolean	AI_SquadEligible( gentity_t *ent );
void		AI_SquadJoin( gentity_t *self );
qboolean	AI_SquadUpdate( AIGroupInfo_t *group );
int			AI_SquadSlotsFree( const AIGroupInfo_t *group );
void		AI_SquadRequestCover( gentity_t *mover );
int			AI_SquadFlankers( const AIGroupInfo_t *group, const gentity_t *except );

// member-list primitives (AI_Utils.cpp)
void		AI_InsertGroupMember( AIGroupInfo_t *group, gentity_t *member );
void		AI_DeleteGroupMember( AIGroupInfo_t *group, int memberNum );
void		AI_DeleteSelfFromGroup( gentity_t *self );
void		AI_SetNewGroupCommander( AIGroupInfo_t *group );
void		AI_SetClosestBuddy( AIGroupInfo_t *group );
void		AI_GroupUpdateEnemyLastSeen( AIGroupInfo_t *group, vec3_t spot );
void		AI_GroupUpdateSquadstates( AIGroupInfo_t *group, gentity_t *member, int newSquadState );
qboolean	AI_GroupContainsEntNum( AIGroupInfo_t *group, int entNum );
void		AI_UpdateGroups( void );

#endif	//__AI__