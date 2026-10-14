/*
===========================================================================
The saber fighter's tactical layer: what a Jedi/Reborn is trying to do in a duel right now.
The low-level block/evade/flip code in AI_Jedi.cpp is unchanged; this decides when to press,
when to hang back, and when to back off and get their Force back, and AI_Jedi.cpp reads
the answer (Jedi_TacticApply, the attack gate, the taunt gate). Stored in gNPC_t::duelRole.
===========================================================================
*/
#pragma once

enum
{
	JT_NONE = 0,
	JT_PRESS,			// attack: close in, string swings together
	JT_DEFEND,			// guard up, wait for them to over-commit, then counter
	JT_KEEP_DISTANCE,	// circle at the edge of reach (someone else has the lead, or shaken)
	JT_RECOVER,			// back off and let Force / guard come back
	JT_FORCE_OPENER,	// push a staggered enemy, or pull one who's out of reach
	JT_SABER_THROW,		// throw at someone standing off
	JT_TAUNT,			// gloat at a distance
	JT_NUM
};

// rescores when due and returns the tactic to run this think (JT_NONE = vanilla behaviour:
// not a saber duel, or a class that runs its own show)
int			AI_JediTactic( gentity_t *npc, float enemyDist );
void		AI_JediTacticDone( gentity_t *npc );	// executor finished a one-shot (opener, throw, taunt): rescore now
const char	*AI_JediTacticName( int tactic );

// 1 (grunt) .. 5 (master): how well a saber fighter reads and meets a swing (.npc "blockSkill",
// else by rank/class)
int			AI_JediBlockSkill( const gentity_t *npc );

// the enemy is open: recovering from a swing, bounced, broken, knocked down, or guard-broken
qboolean	AI_JediEnemyOpen( const gentity_t *npc );

// an NPC landing a timed parry (the player has WP_InPerfectParryWindow): skill-based, with a
// cooldown, decided once per frame so every check in one collision agrees
qboolean	WP_NPCPerfectParry( gentity_t *self );
