/*
===========================================================================
Awareness: how much an NPC has noticed of the player (or any enemy) before it is actually
fighting them. Every humanoid runs it. Replaces the trooper-only stealth check with its
"cautious/realize" dice and the investigateCount counter.

  UNAWARE    nothing seen; patrolling / standing around
  SUSPICIOUS something caught their eye or ear: look that way, a line
  SEARCHING  enough to go and check: walk to where it was, saber users light up
  ALERT      an enemy; combat (brain, Jedi AI) takes over

awareLevel 0..1 is fed by the sight rating (distance, FOV, light, speed, crouch) and by
sound/sight alert events, and drains when nothing feeds it. Saved on gNPC_t (Phase 2).
===========================================================================
*/
#pragma once

enum
{
	AW_UNAWARE = 0,
	AW_SUSPICIOUS,
	AW_SEARCHING,
	AW_ALERT,
	AW_NUM
};

// thresholds on awareLevel going up (coming down has a little hysteresis)
#define AWARE_SUSPICIOUS	0.3f
#define AWARE_SEARCHING		0.6f
#define AWARE_ALERT			1.0f

// how clearly they can see this target right now: 0 = not at all, ~0.6+ = obvious;
// *instant is set when it's point blank / unmissable. Uses the NPC globals (runs for NPC)
float		AI_SightRating( gentity_t *npc, gentity_t *target, qboolean *instant );

// per think, before the behaviours (NPC_RunBehavior); does nothing with an enemy set
void		AI_AwarenessUpdate( gentity_t *npc );
// an alert event (sound or sight) reached them: level is an AEL_* value
void		AI_AwarenessNoise( gentity_t *npc, int alertLevel, const vec3_t pos, gentity_t *owner );
// hooks from G_SetEnemy / G_ClearEnemy
void		AI_AwarenessSetAlert( gentity_t *npc );
void		AI_AwarenessLostEnemy( gentity_t *npc );
// mind trick, scripts: forget what you saw
void		AI_AwarenessReset( gentity_t *npc, float level );
// a buddy found out cold: this NPC and its squad go on the player and never relax past SEARCHING
void		AI_AwarenessAlarm( gentity_t *npc, const vec3_t where );

// a stealth takedown is possible on them from where "by" stands (unaware, behind, close)
qboolean	AI_CanBeSnuckUpOn( gentity_t *npc, gentity_t *by );

const char	*AI_AwareStateName( int state );

// the old instant check, still used by droids/mechs and when g_stealth is off
qboolean	NPC_CheckEnemyStealth( gentity_t *target );
qboolean	NPC_CheckPlayerTeamStealth( void );

// dragging a body (corpse or knocked out) by the player's +use
void		G_BodyDragThink( gentity_t *body );

// the takedown itself (g_active.cpp): a BOTH_MELEE5 punch, ends in G_Neutralize(NH_STEALTH)
void		G_StartSpyTakedown( gentity_t *ent, gentity_t *victim );
qboolean	G_SpyTakedownUpdate( gentity_t *ent, usercmd_t *ucmd );

extern cvar_t	*g_stealth;				// 1 = awareness states; 0 = the old see-you-shoot-you
extern cvar_t	*g_stealthLightScale;	// how much light counts (1.0)
extern cvar_t	*g_stealthNoiseScale;	// player noise radii (1.0)
