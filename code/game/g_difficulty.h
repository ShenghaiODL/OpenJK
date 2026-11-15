/*
===========================================================================
JKEnhanced: difficulty profile (Step 3F)

One place for what difficulty changes in our own systems, loaded from ext_data/difficulty.cfg
(built-in defaults if the file or a key is missing). Profiles 0-2 are g_spskill 0-2; profile 3 is
Jedi Master, which the menu sets as g_spskill 2 + handicap 50 (G_IsJediMaster). Stock g_spskill
checks elsewhere in the game are untouched.
===========================================================================
*/

#ifndef __G_DIFFICULTY_H__
#define __G_DIFFICULTY_H__

typedef struct difficultyProfile_s
{
	float	npcBoltDamage;			// NPC blaster bolt damage = the player's damage x this
	int		weaponSkillShift;		// added to every NPC's weapon skill (1-5)
	float	saberDamageToPlayer;	// x on the tiered NPC saber hit on the player
	float	parryWindowScale;		// x on the player's perfect-parry window
	int		nerveShift;				// added to enemy nerve (0-100; bosses / no-flee untouched)
	float	guardScale;				// x on NPC saber guard pools
	float	heavyCooldownScale;		// x on g_heavyAttackCooldown (time between NPC heavy attacks)
	float	coreCutToPlayer;		// saber rules: share of max health a solid torso/head cut takes off the player
} difficultyProfile_t;

void						G_LoadDifficultyProfiles( void );
const difficultyProfile_t	*G_Difficulty( void );
// Jedi Master: g_spskill 2 + handicap 50 (g_spskill 3+ also counts); g_spskill alone can't tell it from Jedi Knight
qboolean					G_IsJediMaster( void );
// 0-3: g_spskill 0-2, Jedi Master = 3
int							G_DifficultyLevel( void );

#endif // __G_DIFFICULTY_H__
