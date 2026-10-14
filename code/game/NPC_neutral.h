/*
===========================================================================
Non-lethal "neutralized" NPC state.

An NPC that has been taken out of the fight without being killed: maimed, knocked out,
surrendered, pacified. For mission purposes it counts as dead (its targets and death
script fire once, it's removed from its squad, it drops its key and inventory, it counts
on the stats screen) but the body stays alive and targetable, and can still be finished
off later without anything firing twice.

See G_Neutralize for the rules, NPC_NeutralThink for the per-state behaviour.
===========================================================================
*/
#pragma once

typedef enum
{
	NS_NONE = 0,
	NS_SHOCK,		// just happened; a moment of standing stunned before resolving
	NS_SURRENDERED,	// hands up, stays that way
	NS_FLEEING,		// running away; despawns once out of sight for a while
	NS_OUT_COLD,	// lying unconscious; never wakes
	NS_PACIFIED,	// mind-tricked into walking away (Phase 7)
	NS_MAIMED,		// lost a hand to a saber: shock, then surrender and/or flee (the original prototype)
	NS_NUM
} neutralState_t;

typedef enum
{
	NH_NONE = 0,
	NH_MAIM,
	NH_SURRENDER,
	NH_KNOCKOUT,
	NH_PACIFY,
	NH_DISARM,
	NH_STEALTH,
	NH_NUM
} neutralHow_t;

// gNPC_t::neutralFlags
#define NF_TARGETS_FIRED	0x00000001	// target / subduetarget already fired
#define NF_SCRIPT_RAN		0x00000002	// death/subdue script already run
#define NF_STATS_COUNTED	0x00000004	// already counted as subdued (so a later kill isn't counted again)
#define NF_DROPPED_INV		0x00000008	// weapon/items already dropped
#define NF_DOWNED			0x00000010	// lying down: doors, movers and keys treat it like a corpse
#define NF_FREED_ICARUS		0x00000020	// script name already released

// spawner "nonlethal" key (gentity_t::nonlethal)
#define NONLETHAL_DEFAULT	0
#define NONLETHAL_NEVER		1
#define NONLETHAL_PREFER	2

qboolean	G_CanNeutralize( gentity_t *npc, int how );
void		G_Neutralize( gentity_t *npc, gentity_t *attacker, int how );
void		G_NeutralEnterState( gentity_t *npc, int state );
qboolean	G_IsNeutralized( const gentity_t *ent );
qboolean	G_IsDownedBody( const gentity_t *ent );
// a teammate shaking a knocked-out NPC awake: back into the fight, unarmed and alarmed
gentity_t	*G_FindDownedAllyNear( gentity_t *npc, float radius );
void		G_ReviveNeutralized( gentity_t *npc, gentity_t *reviver );
const char	*G_NeutralStateName( int state );
const char	*G_NeutralHowName( int how );
void		G_AlignmentEvent( gentity_t *attacker, gentity_t *victim, int how );

// NPC_RunBehavior hook: returns qtrue when it has handled this think
qboolean	NPC_NeutralThink( void );
