/*
===========================================================================
JKEnhanced 4B.9: boss bar and stage events (g_combat.cpp). The game side picks the boss to show;
the cgame (same DLL) draws it.
===========================================================================
*/
#pragma once

typedef struct bossBarInfo_s
{
	int			entNum;
	char		name[64];
	int			health;
	int			maxHealth;
	int			stages;			// 1 = no stage lines
	int			guard;
	int			guardMax;		// 0 = no guard bar
	qboolean	guardBroken;
	qboolean	recovering;		// mid stage break: untouchable
} bossBarInfo_t;

qboolean	G_BossBarInfo( bossBarInfo_t *out );	// qfalse = no bar this frame
void		G_BossBarNoteHit( const gentity_t *targ, const gentity_t *attacker );
int			G_BossStage( const gentity_t *ent );	// 1 = first stage, 0 = no stages
