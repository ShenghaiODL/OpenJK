/*
===========================================================================
Non-lethal "neutralized" NPC state. See NPC_neutral.h.
===========================================================================
*/

#include "g_local.h"
#include "b_local.h"
#include "g_functions.h"
#include "anims.h"
#include "../cgame/cg_local.h"
#include "Q3_Interface.h"
#include "NPC_neutral.h"

extern cvar_t		*g_nonLethal;
extern cvar_t		*g_debugDamage;

extern gentity_t	*TossClientItems( gentity_t *self );
extern qboolean		PM_HasAnimation( gentity_t *ent, int animation );
extern int			eventClearTime;
extern void			G_DropKey( gentity_t *self );
extern void			G_CheckVictoryScript( gentity_t *self );
extern void			G_CheckCharmed( gentity_t *self );
extern void			OBJ_SetPendingObjectives( gentity_t *ent );
extern void			NPC_LeaveTroop( gentity_t *actor );
extern void			AI_DeleteSelfFromGroup( gentity_t *self );
extern qboolean		NPC_FreeCombatPoint( int combatPointID, qboolean failed );
extern qboolean		G_StandardHumanoid( gentity_t *self );
extern qboolean		NPC_CanSurrender( void );
extern void			NPC_Surrender( void );
extern void			NPC_BSRemove( void );
extern int			PM_PickAnim( gentity_t *self, int minAnim, int maxAnim );

static const char *neutralStateNames[NS_NUM] =
{
	"none", "shock", "surrendered", "fleeing", "out cold", "pacified", "maimed"
};

static const char *neutralHowNames[NH_NUM] =
{
	"none", "maim", "surrender", "knockout", "pacify", "disarm", "stealth"
};

const char *G_NeutralStateName( int state )
{
	if ( state < 0 || state >= NS_NUM )
	{
		return "?";
	}
	return neutralStateNames[state];
}

const char *G_NeutralHowName( int how )
{
	if ( how < 0 || how >= NH_NUM )
	{
		return "?";
	}
	return neutralHowNames[how];
}

qboolean G_IsNeutralized( const gentity_t *ent )
{
	return (qboolean)( ent && ent->NPC && ent->NPC->neutralState != NS_NONE );
}

// doors, movers and the key handover treat a downed body the same as a corpse
qboolean G_IsDownedBody( const gentity_t *ent )
{
	if ( !ent )
	{
		return qfalse;
	}
	if ( ent->health <= 0 )
	{
		return qtrue;
	}
	return (qboolean)( ent->NPC && (ent->NPC->neutralFlags&NF_DOWNED) );
}

// hook for the light/dark alignment system when that gets built: both this and real kills
// report here
void G_AlignmentEvent( gentity_t *attacker, gentity_t *victim, int how )
{
}

/*
-------------------------
G_FindDownedAllyNear / G_ReviveNeutralized

  A teammate who walks up to someone out cold shakes them awake: they get up (unarmed, the
  gun is wherever it fell), and since somebody obviously did this, both of them and their
  squads go looking for the player and never fully relax again (AI_AwarenessAlarm). The
  mission bookkeeping from the knockout (subdued count, targets, scripts) is not undone.
-------------------------
*/
gentity_t *G_FindDownedAllyNear( gentity_t *npc, float radius )
{
	gentity_t	*list[32];
	vec3_t		mins, maxs;
	for ( int k = 0; k < 3; k++ )
	{
		mins[k] = npc->currentOrigin[k] - radius;
		maxs[k] = npc->currentOrigin[k] + radius;
	}
	const int n = gi.EntitiesInBox( mins, maxs, list, 32 );
	for ( int k = 0; k < n; k++ )
	{
		gentity_t *e = list[k];
		if ( e == npc || !e->inuse || !e->client || !e->NPC || e->health <= 0 )
		{
			continue;
		}
		if ( e->NPC->neutralState != NS_OUT_COLD || e->client->playerTeam != npc->client->playerTeam )
		{
			continue;
		}
		if ( e->client->ps.heldByClient < ENTITYNUM_WORLD )
		{//being dragged off right now
			continue;
		}
		return e;
	}
	return NULL;
}

void G_ReviveNeutralized( gentity_t *npc, gentity_t *reviver )
{
	if ( !npc || !npc->client || !npc->NPC || npc->health <= 0 || npc->NPC->neutralState != NS_OUT_COLD )
	{
		return;
	}
	gNPC_t *ni = npc->NPC;

	ni->neutralState = NS_NONE;
	ni->neutralHow = NH_NONE;
	ni->neutralFlags &= ~NF_DOWNED;
	ni->neutralTime = 0;
	TIMER_Remove( npc, "CheckForWeaponToPickup" );
	TIMER_Remove( npc, "maimed" );
	TIMER_Remove( npc, "neutralGone" );
	TIMER_Remove( npc, "neutralShock" );

	npc->contents = CONTENTS_BODY;
	npc->maxs[2] = npc->client->standheight > 0 ? npc->client->standheight : DEFAULT_MAXS_2;
	gi.linkentity( npc );

	//which way they're lying decides the getup: the pelvis bolt's down axis points up when
	//they're on their back (same test the ragdoll settle uses)
	int anim = BOTH_GETUP2;//face down
	if ( npc->ghoul2.size() && npc->crotchBolt > -1 )
	{
		mdxaBone_t	matrix;
		vec3_t		dir;
		vec3_t		g2Angles = { 0, npc->client->ps.viewangles[YAW], 0 };
		gi.G2API_GetBoltMatrix( npc->ghoul2, npc->playerModel, npc->crotchBolt, &matrix, g2Angles, npc->currentOrigin, ( cg.time ? cg.time : level.time ), NULL, npc->s.modelScale );
		gi.G2API_GiveMeVectorFromMatrix( matrix, NEGATIVE_Z, dir );
		if ( dir[2] > 0.1f )
		{//on their back
			anim = BOTH_GETUP5;
		}
	}
	NPC_SetAnim( npc, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	npc->painDebounceTime = level.time + npc->client->ps.torsoAnimTimer;//stay down until the getup plays out
	npc->client->ps.weaponTime = npc->client->ps.torsoAnimTimer;

	//groggy and worried: shaken nerve, and the fists until they find a gun
	ni->nerveShock = Q_min( ni->nerveShock, -25 );
	if ( npc->client->ps.weapon == WP_NONE && npc->client->ps.weapons[WP_MELEE] )
	{
		NPC_ChangeWeapon( WP_MELEE );
	}

	if ( g_debugDamage && g_debugDamage->integer )
	{
		gi.Printf( "revived %s (%d) by %s (%d)\n", npc->NPC_type, npc->s.number, reviver ? reviver->NPC_type : "?", reviver ? reviver->s.number : -1 );
	}

	//everyone here knows now
	AI_AwarenessAlarm( npc, npc->currentOrigin );
	if ( reviver && reviver != npc )
	{
		AI_AwarenessAlarm( reviver, npc->currentOrigin );
	}
}

/*
-------------------------
G_CanNeutralize

The single gate for taking an NPC out of the fight alive. Story-critical NPCs, bosses,
saber users, droids and creatures always take normal lethal damage.
-------------------------
*/
qboolean G_CanNeutralize( gentity_t *npc, int how )
{
	if ( !npc || !npc->client || !npc->NPC )
	{
		return qfalse;
	}
	if ( npc->health <= 0 )
	{
		return qfalse;
	}
	if ( npc->NPC->neutralState != NS_NONE )
	{//already out of the fight
		return qfalse;
	}
	if ( g_nonLethal && !g_nonLethal->integer )
	{
		return qfalse;
	}
	if ( npc->nonlethal == NONLETHAL_NEVER )
	{//the map says this one has to die properly
		return qfalse;
	}
	if ( (npc->flags&FL_UNDYING) )
	{//scripted encounter that ends at the health threshold
		return qfalse;
	}
	if ( (npc->NPC->aiFlags&(NPCAI_BOSS_CHARACTER|NPCAI_SUBBOSS_CHARACTER|NPCAI_ROSH|NPCAI_HEAL_ROSH)) )
	{
		return qfalse;
	}
	if ( npc->NPC->behaviorState == BS_CINEMATIC )
	{//a designer is driving this one
		return qfalse;
	}
	if ( npc->client->ps.weapon == WP_SABER || npc->client->ps.weapons[WP_SABER] )
	{
		return qfalse;
	}
	switch ( npc->client->NPC_class )
	{
	case CLASS_JEDI:
	case CLASS_REBORN:
	case CLASS_SHADOWTROOPER:
	case CLASS_TAVION:
	case CLASS_ALORA:
	case CLASS_DESANN:
	case CLASS_LUKE:
	case CLASS_KYLE:
	case CLASS_BOBAFETT:
	case CLASS_PLAYER:
	case CLASS_VEHICLE:
		return qfalse;
	default:
		break;
	}
	if ( !G_StandardHumanoid( npc ) )
	{//droids, creatures, mechs
		return qfalse;
	}
	if ( how == NH_SURRENDER || how == NH_PACIFY )
	{//the surrender rules have their own exclusions (fist fighters, protocol droids...)
		qboolean ok;
		SaveNPCGlobals();
		SetNPCGlobals( npc );
		ok = NPC_CanSurrender();
		RestoreNPCGlobals();
		if ( !ok )
		{
			return qfalse;
		}
	}
	return qtrue;
}

/*
-------------------------
G_Neutralize

Everything player_die does for mission progress, without the dying. Safe to call more
than once: each part runs only the first time.
-------------------------
*/
void G_Neutralize( gentity_t *npc, gentity_t *attacker, int how )
{
	if ( !npc || !npc->client || !npc->NPC || npc->health <= 0 )
	{
		return;
	}

	gNPC_t		*ni = npc->NPC;
	const qboolean	first = (qboolean)( ni->neutralState == NS_NONE );

	if ( ni->neutralHow == NH_NONE )
	{
		ni->neutralHow = how;
	}

	// its target(s), once. A map can name a separate target for a live capture.
	if ( !(ni->neutralFlags&NF_TARGETS_FIRED) )
	{
		ni->neutralFlags |= NF_TARGETS_FIRED;
		if ( npc->NPC_subduetarget && npc->NPC_subduetarget[0] )
		{
			G_UseTargets2( npc, npc, npc->NPC_subduetarget );
		}
		else
		{
			G_UseTargets( npc, npc );
		}
		if ( (ni->scriptFlags&SCF_FFDEATH) )
		{
			G_UseTargets2( npc, npc, npc->target4 );
		}
		npc->target = NULL;
		npc->target2 = NULL;
		npc->target4 = NULL;
	}

	// its script, once: a subdue script if the map has one, otherwise the death script
	if ( !(ni->neutralFlags&NF_SCRIPT_RAN) )
	{
		ni->neutralFlags |= NF_SCRIPT_RAN;
		qboolean ranScript = qfalse;
		if ( npc->behaviorSet[BSET_SUBDUE] )
		{
			ranScript = G_ActivateBehavior( npc, BSET_SUBDUE );
		}
		else if ( npc->behaviorSet[BSET_DEATH] )
		{
			ranScript = G_ActivateBehavior( npc, BSET_DEATH );
		}
		if ( (ni->scriptFlags&SCF_FFDEATH) && npc->behaviorSet[BSET_FFDEATH] )
		{
			if ( G_ActivateBehavior( npc, BSET_FFDEATH ) )
			{
				ranScript = qtrue;
			}
		}
		// never again, not even on a real death later
		npc->behaviorSet[BSET_DEATH] = NULL;
		npc->behaviorSet[BSET_FFDEATH] = NULL;
		npc->behaviorSet[BSET_SUBDUE] = NULL;
		if ( !ranScript && !(npc->svFlags&SVF_KILLED_SELF) )
		{//nothing is scripting this one any more; same rule as player_die (don't free while a script is running)
			Quake3Game()->FreeEntity( npc );
			ni->neutralFlags |= NF_FREED_ICARUS;
		}
	}

	// out of the squad and off the nav/cover bookkeeping
	if ( ni->troop )
	{
		NPC_LeaveTroop( npc );
	}
	G_CheckCharmed( npc );
	NPC_FreeCombatPoint( ni->combatPoint, qfalse );
	if ( ni->group )
	{
		AI_DeleteSelfFromGroup( npc );
	}

	// a key can't be allowed to block progress
	if ( npc->message )
	{
		G_DropKey( npc );
	}

	// drop the gun and whatever else, exactly as a death would
	if ( !(ni->neutralFlags&NF_DROPPED_INV) )
	{
		ni->neutralFlags |= NF_DROPPED_INV;
		const int contents = gi.pointcontents( npc->currentOrigin, -1 );
		if ( !(contents&CONTENTS_NODROP) && npc->client->NPC_class != CLASS_GALAKMECH )
		{
			TossClientItems( npc );
		}
		if ( npc->s.weapon == WP_NONE && npc->client->ps.weapon != WP_NONE )
		{//TossClientItems took it out of the world state; take it out of the hand too
			npc->client->ps.weapon = WP_NONE;
			G_RemoveWeaponModels( npc );
		}
	}

	// stats: subdued, not killed
	if ( first && !(ni->neutralFlags&NF_STATS_COUNTED) )
	{
		ni->neutralFlags |= NF_STATS_COUNTED;
		if ( attacker && attacker->client && !attacker->s.number )
		{
			if ( npc->client->playerTeam == TEAM_ENEMY
				|| npc->client->playerTeam == TEAM_FREE
				|| ni->charmedTime > level.time
				|| ni->darkCharmedTime > level.time )
			{
				attacker->client->sess.missionStats.enemiesSubdued++;
			}
			G_CheckVictoryScript( attacker );
		}
		G_AlignmentEvent( attacker, npc, how );
		if ( how != NH_MAIM )
		{//(a maim already hit the witnesses through the dismemberment itself)
			AI_NerveWitness( npc, NE_ALLY_NEUTRALIZED, attacker );
		}
	}

	OBJ_SetPendingObjectives( npc );

	// it stays unarmed (the flee logic would otherwise go looking for a gun)
	TIMER_Set( npc, "CheckForWeaponToPickup", Q3_INFINITE );
	// the old marker, still read by player_die and the saber debug prints
	TIMER_Set( npc, "maimed", Q3_INFINITE );

	if ( attacker && attacker->client && attacker != npc && !npc->enemy )
	{
		G_SetEnemy( npc, attacker );
	}

	if ( g_debugDamage && g_debugDamage->integer )
	{
		gi.Printf( "neutralized %s (%d) by %s%s\n", npc->NPC_type, npc->s.number, G_NeutralHowName( how ), first ? "" : " (again)" );
	}

	if ( first )
	{
		int state;
		switch ( how )
		{
		case NH_MAIM:		state = NS_MAIMED;		break;
		case NH_KNOCKOUT:
		case NH_STEALTH:
		case NH_PACIFY:		state = NS_OUT_COLD;	break;//mind trick 3 puts them to sleep (walking them off needs flee pathing that isn't there yet)
		case NH_SURRENDER:	state = NS_SURRENDERED;	break;
		case NH_DISARM:
		default:			state = NS_SHOCK;		break;
		}
		G_NeutralEnterState( npc, state );
	}
}

// an ordinary death anim, frozen on its last frame: the same pose a corpse ends in, so the
// body lies on the floor properly (the crosshair still reads them as a live enemy, which is
// the tell that they're only out cold)
static void Neutral_SetDownedAnim( gentity_t *npc )
{
	const int anim = PM_PickAnim( npc, BOTH_DEATH1, BOTH_DEATH25 );
	NPC_SetAnim( npc, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	npc->client->ps.torsoAnimTimer = npc->client->ps.legsAnimTimer = Q3_INFINITE;
}

static qboolean Neutral_InDownedAnim( const gentity_t *npc )
{
	const int legs = npc->client->ps.legsAnim;
	if ( npc->client->ps.torsoAnim != legs )
	{
		return qfalse;
	}
	return (qboolean)( legs >= BOTH_DEATH1 && legs <= BOTH_DEATH25 );
}

/*
-------------------------
G_NeutralEnterState
-------------------------
*/
void G_NeutralEnterState( gentity_t *npc, int state )
{
	if ( !npc || !npc->NPC || !npc->client )
	{
		return;
	}
	gNPC_t *ni = npc->NPC;

	if ( state == NS_FLEEING || state == NS_PACIFIED )
	{//has to be able to start running, or it's hands-up instead
		TIMER_Set( npc, "meleeFallback", Q3_INFINITE );//never fights back with fists
		gentity_t *from = npc->enemy ? npc->enemy : player;
		if ( !G_StartFlee( npc, npc->enemy, from->currentOrigin, AEL_DANGER_GREAT, 6000, 10000 ) )
		{
			state = NS_SURRENDERED;
		}
	}

	ni->neutralState = state;
	ni->neutralTime = level.time;

	switch ( state )
	{
	case NS_SHOCK:
		{
			const int reactTime = Q_irand( 800, 1400 );
			TIMER_Set( npc, "neutralShock", reactTime );
			NPC_SetAnim( npc, SETANIM_BOTH, Q_irand( BOTH_PAIN1, BOTH_PAIN4 ), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			npc->client->ps.torsoAnimTimer = npc->client->ps.legsAnimTimer = reactTime;
		}
		break;
	case NS_OUT_COLD:
		ni->neutralFlags |= NF_DOWNED;
		//a corpse to doors and movers, but still a body the player bumps into (a low one, see the think)
		npc->contents = CONTENTS_BODY|CONTENTS_CORPSE;
		ni->desiredSpeed = 0;
		if ( npc->client->ps.weapon != WP_NONE )
		{
			npc->client->ps.weapon = npc->s.weapon = WP_NONE;
			G_RemoveWeaponModels( npc );
		}
		Neutral_SetDownedAnim( npc );
		gi.linkentity( npc );
		break;
	default:
		break;
	}
}

/*
-------------------------
NPC_NeutralThink

Runs at the top of NPC_RunBehavior for any neutralized NPC, ahead of every class
behaviour (so nothing else can pre-empt the pose). Uses the NPC globals.
-------------------------
*/
static void Neutral_HoldStill( qboolean faceEnemy )
{
	ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
	ucmd.buttons &= ~(BUTTON_ATTACK|BUTTON_ALT_ATTACK);
	if ( faceEnemy && NPC->enemy )
	{
		NPC_FaceEnemy();
	}
	NPC_UpdateAngles( qtrue, qtrue );
}

static qboolean Neutral_MaimedThink( void )
{//the hand-sever prototype, unchanged: shock, then run (hands up when caught) or hands up here
	if ( !TIMER_Done( NPC, "maimedReact" ) )
	{
		Neutral_HoldStill( qtrue );
		return qtrue;
	}

	if ( !TIMER_Exists( NPC, "maimedChoice" ) )
	{
		TIMER_Set( NPC, "maimedChoice", Q3_INFINITE );
		if ( NPC->enemy && Q_irand( 0, 1 ) )
		{
			TIMER_Set( NPC, "meleeFallback", Q3_INFINITE );
			if ( G_StartFlee( NPC, NPC->enemy, NPC->enemy->currentOrigin, AEL_DANGER_GREAT, 4000, 8000 ) )
			{
				TIMER_Set( NPC, "maimedFlee", Q3_INFINITE );
			}
		}
	}

	if ( TIMER_Exists( NPC, "maimedFlee" )
		&& NPC->enemy
		&& NPCInfo->tempBehavior == BS_FLEE
		&& !TIMER_Done( NPC, "flee" ) )
	{//still running away: let the normal flee behaviour drive it
		return qfalse;
	}

	Neutral_HoldStill( qtrue );
	NPC_Surrender();
	return qtrue;
}

static qboolean Neutral_FleeingThink( void )
{
	if ( NPCInfo->surrenderTime > level.time )
	{//the flee logic gave up and put its hands up
		NPCInfo->neutralState = NS_SURRENDERED;
		NPCInfo->neutralTime = level.time;
		return qfalse;
	}

	// gone: out of the player's sight for a while after the run is over
	if ( TIMER_Done( NPC, "flee" ) )
	{
		if ( gi.inPVS( player->currentOrigin, NPC->currentOrigin ) )
		{
			TIMER_Remove( NPC, "neutralGone" );
		}
		else if ( !TIMER_Exists( NPC, "neutralGone" ) )
		{
			TIMER_Set( NPC, "neutralGone", 5000 );
		}
		else if ( TIMER_Done( NPC, "neutralGone" ) )
		{//got away
			NPC_BSRemove();
			return qtrue;
		}
	}

	if ( NPCInfo->tempBehavior == BS_FLEE || NPCInfo->behaviorState == BS_FLEE )
	{//the normal flee behaviour drives it
		return qfalse;
	}
	// the flee ran out but we're still in view: keep running
	NPC_BSFlee();
	return qtrue;
}

qboolean NPC_NeutralThink( void )
{
	switch ( NPCInfo->neutralState )
	{
	case NS_MAIMED:
		return Neutral_MaimedThink();

	case NS_SHOCK:
		if ( !TIMER_Done( NPC, "neutralShock" ) )
		{
			Neutral_HoldStill( qtrue );
			return qtrue;
		}
		// resolve: run if there's somewhere to run, otherwise hands up
		G_NeutralEnterState( NPC, ( NPC->enemy && Q_irand( 0, 1 ) ) ? NS_FLEEING : NS_SURRENDERED );
		return qtrue;

	case NS_SURRENDERED:
		Neutral_HoldStill( qtrue );
		NPC_Surrender();
		return qtrue;

	case NS_FLEEING:
	case NS_PACIFIED:
		return Neutral_FleeingThink();

	case NS_OUT_COLD:
		G_BodyDragThink( NPC );//the player may be hauling us somewhere
		if ( eventClearTime == level.time + ALERT_CLEAR_TIME && !(NPC->client->ps.eFlags&EF_NODRAW) )
		{//like a corpse: something for a passing buddy to notice (and come and wake us)
			AddSightEvent( NPC, NPC->currentOrigin, 384, AEL_DISCOVERED );
		}
		ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
		ucmd.buttons = 0;
		//no steering, but gravity still applies (pmove runs with an empty command), so they settle on the floor
		NPC->client->ps.velocity[0] = NPC->client->ps.velocity[1] = 0;
		if ( !Neutral_InDownedAnim( NPC ) )
		{//something (pain, a push) knocked us out of the pose
			Neutral_SetDownedAnim( NPC );
		}
		NPC->client->ps.torsoAnimTimer = NPC->client->ps.legsAnimTimer = Q3_INFINITE;
		{//still a corpse to doors and movers; solid to people only once nobody is standing in us
		 //(a stealth takedown leaves the player right on top of the body)
			qboolean overlapped = qfalse;
			for ( int c = 0; c < MAX_CLIENTS && !overlapped; c++ )
			{
				const gentity_t *pl = &g_entities[c];
				if ( pl->inuse && pl->client && pl->health > 0
					&& pl->absmin[0] < NPC->absmax[0] && pl->absmax[0] > NPC->absmin[0]
					&& pl->absmin[1] < NPC->absmax[1] && pl->absmax[1] > NPC->absmin[1]
					&& pl->absmin[2] < NPC->absmax[2] && pl->absmax[2] > NPC->absmin[2] )
				{
					overlapped = qtrue;
				}
			}
			NPC->contents = overlapped ? CONTENTS_CORPSE : (CONTENTS_BODY|CONTENTS_CORPSE);
		}
		if ( level.time - NPCInfo->neutralTime > 1500 && NPC->maxs[2] > 16 )
		{//lying down now: a low box you step over, the way corpses shrink theirs
			NPC->maxs[2] = 16;
			gi.linkentity( NPC );
		}
		return qtrue;

	default:
		return qfalse;
	}
}
