/*
===========================================================================
JKEnhanced: Force tuning (Step 4B)

ext_data/force.cfg overrides the Force cost table and holds the NPC Force pool settings
(built-in defaults if the file or a key is missing). Per-level duration / damage / radius tables
move in here as the power reworks touch them.
===========================================================================
*/

#ifndef __G_FORCE_H__
#define __G_FORCE_H__

typedef struct forceTuning_s
{
	int		multiTargetExtra;	// Push/Pull L3: cost = base + this per extra target...
	int		multiTargetCap;		// ...capped here
	float	npcCostScale;		// x on what an NPC pays for a power (g_npcForcePool on)
	float	bossPoolScale;		// x on a saber boss's forcePowerMax at spawn
	float	bossRegenScale;		// x on a saber boss's regen speed at spawn

	//Protect (4B.3): L1-2 timed, L3 a toggle that pays per block (Absorb stays stock)
	int		protectDuration[2];	// L1, L2 (ms)
	int		protectToggleCost;	// L3: turning it on
	int		protectDamagePct;	// L3: % of the damage that still gets through
	int		protectHitCostDiv;	// L3: 1 Force per this much damage it takes off
	int		protectDodgeChance;	// L3: % a ranged shot is dodged outright...
	int		protectDodgeCost;	// ...for this much Force...
	int		protectDodgeCooldown;// ...at most once per this many ms

	//Repulse (4B.4): clears space around you; the hurt comes from what they slam into
	int		repulseCharge;		// ms to a full charge (every level; the level sets the radius)
	int		repulseMinPct;		// a tap still throws at this % strength
	int		repulseKnockMin;	// knockback at the edge of the radius / weakest charge...
	int		repulseKnockMax;	// ...up close at full charge
	int		repulseDamage;		// direct damage at full charge (scaled by charge)
	int		repulseSlamMin;		// thrown into a wall / someone at this speed starts to hurt...
	int		repulseSlamFull;	// ...and does repulseSlamDamage at this speed
	int		repulseSlamDamage;
	int		repulseStartCost;	// the player's Repulse: this to start charging...
	int		repulseTotalCost;	// ...draining to this much in all by a full charge (out of Force = it goes off early; NPCs pay cost.repulse)

	//Speed (4B.6): Burst (L1) / Flow (L2) / Focus (L3); each level has the ones below it
	int		speedDuration[3];	// real ms at L1 / L2 / L3 (the slow-mo no longer stretches it)
	int		speedRunPct;		// Burst: run speed %
	int		speedLandPct;		// Burst: landings / rolls / jump recovery take this % of the time
	int		speedRecoveryPct;	// Flow: saber bounce / deflect / return anim speed %
	int		speedCancelBonus;	// Flow: extra ms on g_blockCancelWindow
	int		speedParryPct;		// Focus: perfect-parry window %
	int		speedDodges;		// Focus: saber hits dodged per cast

	//Stasis (4B.7): a forward cone; no saber = frozen, saber users / Reborn slowed
	int		stasisRange[3];		// L1 / L2 / L3 reach
	int		stasisCone[3];		// half-angle, degrees
	int		stasisDuration[3];	// ms
	int		stasisSlowPct;		// a slowed saber user moves / animates at this %...
	int		stasisBossSlowPct;	// ...a boss at this %

	//Insanity (4B.8): Terror - a forward cone that hits nerve; broken flee or give up, shaken cower
	int		insanityRange[3];	// L1 / L2 / L3 reach
	int		insanityCone[3];	// half-angle, degrees
	int		insanityNerve[3];	// nerve shock dealt
	int		insanityCower[3];	// ms a breaking target cowers (drops its enemy)
	int		insanityWaver[3];	// ms a steadier target flinches (keeps its enemy; saber users get half)
	int		insanityHallucinate;	// L3: ms they aim at a phantom beside you
	int		insanityHallucinateYaw;	// ...this many degrees off (half to all of it)
	int		insanityExtraCost;	// per target past the first (cost.insanity is the first)...
	int		insanityCostCap;		// ...up to this much a cast
	int		insanityCooldown;	// ms before you can cast it again
} forceTuning_t;

void				G_LoadForceTuning( void );
const forceTuning_t	*G_ForceTuning( void );

//L3 Protect shows only when it stops something: game marks it, cgame fades it.
//(power is kept so another toggle shield can use it; only FP_PROTECT does now)
void		G_ForceShieldFlash( const gentity_t *ent, int power );
int			G_ForceShieldFlashTime( int entNum, int power );	// level.time of the last block, 0 = never
qboolean	G_ForceShieldIsToggle( const gentity_t *ent, int power );	// Protect on, at L3

#endif // __G_FORCE_H__

//Speed (4B.6): Burst (L1) / Flow (L2) / Focus (L3) perks go by the level in use (0 = off)
int			G_SpeedLevelPS( const playerState_t *ps );
int			G_SpeedLevel( const gentity_t *ent );
void		G_SpeedCastStart( const gentity_t *ent );	// a new cast: Focus's dodges refill
void		G_SpeedCastEnd( const gentity_t *ent );		// it ended: none left (an AI evasion burst flips Speed on without a cast)
qboolean	G_SpeedTakeDodge( const gentity_t *ent );	// Focus has a reflex dodge left: uses it

//Stasis (4B.7, fp_stasis.cpp): the cone slows saber users and holds shots in the air
float		G_StasisSlowScale( const gentity_t *ent );	// 1 = not slowed
qboolean	G_StasisMissileHeld( gentity_t *missile );	// G_RunMissile: qtrue = held this frame

//Insanity (4B.8, fp_insanity.cpp): L3 Terror's phantom - degrees to add to the NPC's aim (0 = none)
float		G_InsanityAimYaw( const gentity_t *npc );
