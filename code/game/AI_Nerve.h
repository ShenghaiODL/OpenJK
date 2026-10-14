/*
===========================================================================
Per-NPC nerve: fear/confidence that replaces the old group morale number.

Each NPC has a base (rank or the .npc "nerve" key), a shock value that events push around
and that decays back to zero, and situational modifiers computed fresh each time (officer
present, numbers, squad contagion, what the enemy is holding, own wounds). The effective
value (0-100) is banded into tiers with hysteresis. Events reach the NPC they happen to
through AI_NerveEvent, and nearby allies who can see it through AI_NerveWitness.
===========================================================================
*/
#pragma once

typedef enum
{
	NT_STEADY = 0,	// >= 60
	NT_SHAKEN,		// 35-60
	NT_BREAKING,	// 15-35
	NT_BROKEN,		// < 15
	NT_NUM
} nerveTier_t;

typedef enum
{
	NE_ALLY_KILLED = 0,		// witnessed
	NE_LEADER_KILLED,		// witnessed: the squad commander or an officer
	NE_ALLY_DISMEMBERED,	// witnessed
	NE_ALLY_NEUTRALIZED,	// witnessed: taken alive
	NE_HIT,					// took damage (scaled by fraction of max health)
	NE_NEAR_MISS,			// a shot passed close
	NE_EXPLOSION_NEAR,		// in the radius of a blast
	NE_GRENADE_NEAR,		// a danger alert (live grenade, incoming rocket)
	NE_FORCE_ON_ME,			// pushed, pulled, gripped, zapped, drained (witnesses get the "seen" delta)
	NE_ENEMY_SABER_SEEN,	// first contact with a saber-wielding enemy
	NE_CORNERED,			// no cover anywhere
	NE_NO_COVER,			// a cover search came up empty
	NE_DISARMED,			// lost the weapon
	NE_HURT_ENEMY,			// landed a hit on the enemy (witnesses get a smaller boost)
	NE_ALLY_KILLS_ENEMY,	// the killer and anyone who saw it
	NE_REINFORCED,			// someone joined the squad mid-fight
	NE_LEADER_HURT,			// witnessed: the leader (the player, for companions) took damage
	NE_NUM
} nerveEvent_t;

void		AI_NerveInit( gentity_t *npc );
void		AI_NerveEvent( gentity_t *npc, int ev, float scale, gentity_t *source );
void		AI_NerveWitness( gentity_t *victim, int ev, gentity_t *source );
void		AI_NerveNearMiss( const vec3_t pos, gentity_t *owner, gentity_t *hit, float radius );
void		AI_NerveUpdate( gentity_t *npc );
int			AI_NerveEffective( gentity_t *npc, int *outSituational = NULL );
int			AI_NerveSituational( gentity_t *npc, int *officer, int *numbers, int *contagion, int *threat, int *wounds );
int			AI_NerveTier( gentity_t *npc );
qboolean	AI_NerveImmune( gentity_t *npc );
int			AI_GetNerveMoraleTier( gentity_t *npc );	// shim: 0 ROUTED .. 4 RUSH for the old readers
const char	*AI_NerveTierName( int tier );
const char	*AI_NerveEventName( int ev );
