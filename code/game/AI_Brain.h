/*
===========================================================================
Utility brain for gun-wielding NPCs.

Every few hundred milliseconds the NPC scores a handful of candidate actions from what it
can see right now (distance, line of sight, cover, how rattled it is, what the squad is
doing) and commits to the best one for a while. The executors reuse the existing movement
primitives (combat points, geometry cover, the cover-hold cycle, strafe timers, the
thermal swap, flee), so this layer decides *what* and the old plumbing still does *how*.
===========================================================================
*/
#pragma once

typedef enum
{
	AA_NONE = 0,
	AA_HOLD_COVER,		// stay behind cover, duck and pop out to shoot
	AA_RELOCATE,		// find a better spot (cover, a line of fire)
	AA_ADVANCE,			// close the distance
	AA_FLANK,			// get around them
	AA_STAND_FIRE,		// stand here and shoot
	AA_STRAFE,			// sidestep under fire
	AA_GRENADE,			// swap to the thermal and lob one
	AA_SUPPRESS,		// covering fire at the last known position while a squadmate moves
	AA_RETREAT,			// fall back to cover away from them
	AA_FLEE,			// run
	AA_SEEK_WEAPON,		// go get a gun
	AA_TRACK,			// go find them where they were last seen
	AA_REGROUP,			// companions: get back to the leader
	AA_NUM
} aiAction_t;

typedef enum
{
	AIPROF_NONE = 0,
	AIPROF_TROOPER,
	AIPROF_OFFICER,
	AIPROF_ALLY,
	AIPROF_SNIPER,		// hold, relocate, hide after shots; never advances
	AIPROF_GRENADIER,	// lobs, goes to fists up close
	AIPROF_MELEE,		// Tusken staff, fists: close in and strike
	AIPROF_ROCKET,		// rocket trooper on the ground: keeps its distance
	AIPROF_MANDO,		// non-boss Mandalorians when Boba_Tactics hands off
	AIPROF_HAZARD,		// hazard troopers when CTroop hands off: armored, never runs
	AIPROF_NUM
} aiProfileId_t;

int					AI_Brain_ProfileIdByName( const char *name );

typedef struct aiProfile_s
{
	const char	*name;
	float		weight[AA_NUM];		// per-action multiplier
	float		rangeMin;			// preferred engagement band
	float		rangeMax;
	int			allowMask;			// 1<<action for each action this profile may take
} aiProfile_t;

typedef struct aiDebug_s
{
	float	score[AA_NUM];
	int		action;
	int		holdUntil;
	int		lastSwitchTime;
} aiDebug_t;

// called from the trooper attack loop after perception, before movement
void				AI_Brain_Think( gentity_t *npc, qboolean enemyLOS, qboolean enemyCS, qboolean enemyInFOV, qboolean hitAlly, float enemyDistSq );
// something happened (hit, arrived, grenade): re-decide at the next think
void				AI_Brain_Interrupt( gentity_t *npc );
// drop the current action (enemy lost, behaviour change)
void				AI_Brain_Reset( gentity_t *npc );
const aiProfile_t	*AI_Brain_ProfileFor( gentity_t *npc );
const char			*AI_Brain_ActionName( int action );
const aiDebug_t		*AI_Brain_GetDebug( int entNum );
