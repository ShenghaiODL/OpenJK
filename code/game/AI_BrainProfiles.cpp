/*
===========================================================================
Brain profiles: what kind of fighter this is. Same brain, different weights and range.
===========================================================================
*/

#include "b_local.h"
#include "AI_Brain.h"

#define ALLOW( a )		( 1 << (a) )
#define ALLOW_ALL		( ( 1 << AA_NUM ) - 1 )

//                             NONE HOLD  RELOC ADV   FLANK STAND STRAFE GREN  SUPP  RETR  FLEE  SEEK  TRACK REGRP
static const aiProfile_t aiProfiles[AIPROF_NUM] =
{
	{ "none",		{ 0,   1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f }, 256.0f, 700.0f, ALLOW_ALL },
	// the line trooper: does a bit of everything
	{ "trooper",	{ 0,   1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f }, 256.0f, 700.0f, ALLOW_ALL },
	// officers hang back, hold and direct fire; they're the last to break
	{ "officer",	{ 0,   1.2f, 1.0f, 0.6f, 0.5f, 1.0f, 0.8f, 1.0f, 1.3f, 1.1f, 0.8f, 1.0f, 0.8f, 1.0f }, 384.0f, 900.0f, ALLOW_ALL },
	// the player's side: never runs off or gives up
	{ "ally",		{ 0,   1.0f, 1.0f, 0.8f, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f, 0.8f, 0.0f, 1.0f, 1.0f, 1.0f }, 256.0f, 700.0f, ALLOW_ALL & ~ALLOW( AA_FLEE ) },
	// snipers sit, shoot, hide, and find another perch; they never close the distance
	{ "sniper",		{ 0,   1.4f, 1.0f, 0.0f, 0.0f, 1.0f, 0.3f, 0.5f, 0.6f, 1.2f, 1.0f, 1.0f, 0.5f, 1.0f }, 512.0f, 1600.0f, ALLOW_ALL & ~( ALLOW( AA_ADVANCE ) | ALLOW( AA_FLANK ) ) },
	// grenadiers want lobbing distance and love the grenade action
	{ "grenadier",	{ 0,   0.8f, 1.0f, 1.0f, 0.6f, 0.8f, 0.8f, 1.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f }, 256.0f, 600.0f, ALLOW_ALL },
	// melee: close in, no cover games
	{ "melee",		{ 0,   0.0f, 0.3f, 2.0f, 0.0f, 1.0f, 0.4f, 0.0f, 0.0f, 0.8f, 1.0f, 1.0f, 1.2f, 1.0f }, 48.0f, 64.0f, ALLOW_ALL & ~( ALLOW( AA_HOLD_COVER ) | ALLOW( AA_FLANK ) | ALLOW( AA_GRENADE ) | ALLOW( AA_SUPPRESS ) ) },
	// rocket troopers on the ground: stand-off weapon, backs off when crowded
	{ "rocket",		{ 0,   1.0f, 1.0f, 0.4f, 0.3f, 1.0f, 0.5f, 0.8f, 0.8f, 1.3f, 0.8f, 1.0f, 0.8f, 1.0f }, 384.0f, 1200.0f, ALLOW_ALL },
	// Mandalorians: aggressive, mobile
	{ "mando",		{ 0,   0.8f, 1.0f, 1.2f, 1.2f, 1.0f, 1.0f, 1.0f, 0.8f, 0.8f, 0.5f, 1.0f, 1.0f, 1.0f }, 256.0f, 800.0f, ALLOW_ALL },
	// hazard troopers: armored, slow, never run
	{ "hazard",		{ 0,   1.2f, 0.8f, 0.8f, 0.4f, 1.2f, 0.4f, 1.0f, 1.0f, 0.6f, 0.0f, 1.0f, 0.8f, 1.0f }, 256.0f, 800.0f, ALLOW_ALL & ~ALLOW( AA_FLEE ) },
};

int AI_Brain_ProfileIdByName( const char *name )
{
	if ( !name || !name[0] )
	{
		return AIPROF_NONE;
	}
	for ( int i = AIPROF_TROOPER; i < AIPROF_NUM; i++ )
	{
		if ( !Q_stricmp( name, aiProfiles[i].name ) )
		{
			return i;
		}
	}
	return AIPROF_NONE;
}

// what kind of fighter this is, from class and the weapon it was handed at spawn
static int Brain_DefaultProfile( gentity_t *npc )
{
	const int weapon = npc->NPC->spawnWeapon ? npc->NPC->spawnWeapon : npc->client->ps.weapon;

	switch ( npc->client->NPC_class )
	{
	case CLASS_ROCKETTROOPER:	return AIPROF_ROCKET;
	case CLASS_MANDO:			return AIPROF_MANDO;
	case CLASS_HAZARD_TROOPER:	return AIPROF_HAZARD;
	default:					break;
	}
	if ( npc->client->playerTeam == TEAM_PLAYER )
	{//the player's side stays loyal whatever it carries (melee allies still chase via the context flag)
		return AIPROF_ALLY;
	}
	if ( npc->client->ps.weapons[WP_THERMAL] )
	{//carries thermals: a gunner with a sidearm thermal (stormtrooper_grenadier) or a thermal-and-fists
	 //thrower (Gran, who spawns holding the fists); checked before melee so Gran still lobs
		return AIPROF_GRENADIER;
	}
	if ( weapon == WP_MELEE || weapon == WP_TUSKEN_STAFF || weapon == WP_STUN_BATON )
	{
		return AIPROF_MELEE;
	}
	if ( weapon == WP_DISRUPTOR || ( weapon == WP_TUSKEN_RIFLE && (npc->NPC->scriptFlags&SCF_ALT_FIRE) ) )
	{//(a disruptor user goes to alt fire past 256 u anyway, so they're a sniper whatever the .npc says)
		return AIPROF_SNIPER;
	}
	if ( npc->NPC->rank >= RANK_ENSIGN )
	{
		return AIPROF_OFFICER;
	}
	return AIPROF_TROOPER;
}

const aiProfile_t *AI_Brain_ProfileFor( gentity_t *npc )
{
	if ( !npc || !npc->NPC || !npc->client )
	{
		return &aiProfiles[AIPROF_TROOPER];
	}
	int id = npc->NPC->aiProfile;
	if ( id <= AIPROF_NONE || id >= AIPROF_NUM )
	{
		id = Brain_DefaultProfile( npc );
		npc->NPC->aiProfile = id;
	}
	return &aiProfiles[id];
}
