/*
===========================================================================
Awareness and stealth. See AI_Awareness.h.
===========================================================================
*/

#include "b_local.h"
#include "g_functions.h"
#include "anims.h"
#include "Q3_Interface.h"
#include "AI_Awareness.h"

cvar_t	*g_stealth;
cvar_t	*g_stealthLightScale;
cvar_t	*g_stealthNoiseScale;

extern cvar_t	*g_spskill;
extern qboolean	G_ValidEnemy( gentity_t *self, gentity_t *enemy );
extern qboolean	G_StandardHumanoid( gentity_t *self );
extern void		G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void		NPC_TempLookTarget( gentity_t *self, int lookEntNum, int minLookTime, int maxLookTime );
extern qboolean	G_ReleaseEntity( gentity_t *grabber );

// the sight model (was the top of AI_Stormtrooper.cpp)
#define	MAX_VIEW_DIST		1024
#define MAX_VIEW_SPEED		250
#define	MAX_LIGHT_INTENSITY 255
#define	MIN_LIGHT_THRESHOLD	0.1
#define	DISTANCE_THRESHOLD	0.075f

#define	DISTANCE_SCALE		0.35f	//These first three get your base detection rating, ideally add up to 1
#define	FOV_SCALE			0.40f	//
#define	LIGHT_SCALE			0.25f	//
#define	SPEED_SCALE			0.25f	//These next two are bonuses
#define	TURNING_SCALE		0.25f	//

#define	REALIZE_THRESHOLD	0.6f
#define CAUTIOUS_THRESHOLD	( REALIZE_THRESHOLD * 0.75f )

#define ST_LSTATE_INVESTIGATE	2	//AI_Stormtrooper.cpp's LSTATE_INVESTIGATE (each AI file has its own private LSTATE enum)

static const char *awareNames[AW_NUM] = { "unaware", "suspicious", "searching", "alert" };

const char *AI_AwareStateName( int state )
{
	return ( state >= 0 && state < AW_NUM ) ? awareNames[state] : "?";
}

static qboolean Aware_Eligible( gentity_t *npc )
{
	if ( !npc || !npc->client || !npc->NPC || npc->health <= 0 )
	{
		return qfalse;
	}
	if ( G_IsNeutralized( npc ) )
	{
		return qfalse;
	}
	if ( npc->NPC->behaviorState == BS_CINEMATIC || npc->NPC->tempBehavior == BS_CINEMATIC )
	{
		return qfalse;
	}
	return G_StandardHumanoid( npc );
}

static qboolean Aware_SaberUser( const gentity_t *npc )
{
	return (qboolean)( npc->client->ps.weapon == WP_SABER || npc->client->ps.weapons[WP_SABER] );
}

/*
-------------------------
AI_SightRating
-------------------------
*/
float AI_SightRating( gentity_t *npc, gentity_t *target, qboolean *instant )
{
	*instant = qfalse;
	if ( !npc || !npc->NPC || !target || !target->client || target->health <= 0 )
	{
		return 0;
	}
	if ( (target->flags&FL_NOTARGET) || (target->s.eFlags&EF_NODRAW) )
	{
		return 0;
	}
	gNPC_t *ni = npc->NPC;

	float minDist = 40;//any closer than 40 and we definitely notice
	if ( target->client->ps.weapon == WP_SABER && target->client->ps.SaberActive() && !target->client->ps.saberInFlight )
	{//a lit saber in hand: noticed from further
		minDist = 100;
	}

	const float	distSq		= DistanceSquared( target->currentOrigin, npc->currentOrigin );
	const float	maxViewDist	= ni->stats.visrange > 0 ? ni->stats.visrange : MAX_VIEW_DIST;
	if ( distSq > maxViewDist * maxViewDist )
	{
		return 0;
	}
	// in front of them and nothing in the way, always (the old "within 40 u" didn't care about
	// either, which made sneaking up behind someone impossible)
	if ( !InFOV( target, npc, ni->stats.hfov, ni->stats.vfov ) )
	{
		return 0;
	}
	qboolean clearLOS;
	{
		const qboolean swapped = (qboolean)( npc != NPC );
		if ( swapped ) { SaveNPCGlobals(); SetNPCGlobals( npc ); }
		clearLOS = target->client->ps.leanofs ? NPC_ClearLOS( target->client->renderInfo.eyePoint ) : NPC_ClearLOS( target );
		if ( swapped ) { RestoreNPCGlobals(); }
	}
	if ( !clearLOS )
	{
		return 0;
	}

	if ( target->client->NPC_class == CLASS_ATST )
	{//can't miss 'em!
		*instant = qtrue;
		return 2.0f;
	}
	const qboolean crouched = (qboolean)( (target->client->ps.pm_flags&PMF_DUCKED) || target->client->usercmd.upmove < 0 );
	if ( distSq < minDist * minDist && !crouched )
	{
		*instant = qtrue;
		return 2.0f;
	}

	vec3_t	targ_org = { target->currentOrigin[0], target->currentOrigin[1], target->currentOrigin[2] + target->maxs[2] - 4 };
	float	hAngle_perc = NPC_GetHFOVPercentage( targ_org, npc->client->renderInfo.eyePoint, npc->client->renderInfo.eyeAngles, ni->stats.hfov );
	float	vAngle_perc = NPC_GetVFOVPercentage( targ_org, npc->client->renderInfo.eyePoint, npc->client->renderInfo.eyeAngles, ni->stats.vfov );

	//Scale them vertically some, and horizontally pretty harshly
	vAngle_perc *= vAngle_perc;
	hAngle_perc *= ( hAngle_perc * hAngle_perc );

	const float	target_dist		= sqrtf( distSq );
	const float	target_speed	= VectorLength( target->client->ps.velocity );
	const float	dist_rating		= ( target_dist / maxViewDist );
	float		speed_rating	= ( target_speed / MAX_VIEW_SPEED );
	const float	turning_rating	= AngleDelta( target->client->ps.viewangles[PITCH], target->lastAngles[PITCH] )/180.0f + AngleDelta( target->client->ps.viewangles[YAW], target->lastAngles[YAW] )/180.0f;
	const float	lightScale		= g_stealthLightScale ? g_stealthLightScale->value : 1.0f;
	const float	light_level		= ( target->lightLevel / MAX_LIGHT_INTENSITY ) * lightScale;
	const float	FOV_perc		= 1.0f - ( hAngle_perc + vAngle_perc ) * 0.5f;
	float		vis_rating		= 0.0f;

	//Too dark
	if ( light_level < MIN_LIGHT_THRESHOLD )
	{
		return 0;
	}
	//Too close?
	if ( dist_rating < DISTANCE_THRESHOLD )
	{
		*instant = qtrue;
		return 2.0f;
	}
	//Out of range
	if ( dist_rating > 1.0f )
	{
		return 0;
	}
	if ( speed_rating > 1.0f )
	{
		speed_rating = 1.0f;
	}

	//...Visibilty linearly wanes over distance
	const float dist_influence	= DISTANCE_SCALE * ( 1.0f - dist_rating );
	//...As the percentage out of the FOV increases, straight perception suffers on an exponential scale
	const float fov_influence	= FOV_SCALE * ( 1.0f - FOV_perc );
	//...Lack of light hides, abundance of light exposes
	const float light_influence	= ( light_level - 0.5f ) * LIGHT_SCALE;

	float target_rating = dist_influence + fov_influence + light_influence;

	//water and fog hide you some
	const int contents = gi.pointcontents( targ_org, target->s.number );
	if ( contents&CONTENTS_WATER )
	{
		const int myContents = gi.pointcontents( npc->client->renderInfo.eyePoint, npc->s.number );
		if ( !(myContents&CONTENTS_WATER) )
		{
			vis_rating = ( npc->client->NPC_class == CLASS_SWAMPTROOPER ) ? 0.10f : 0.35f;
		}
		else if ( npc->client->NPC_class != CLASS_SWAMPTROOPER )
		{
			vis_rating = 0.15f;
		}
	}
	else if ( contents&CONTENTS_FOG )
	{
		vis_rating = 0.15f;
	}
	target_rating *= ( 1.0f - vis_rating );

	//...Motion draws the eye quickly
	target_rating += speed_rating * SPEED_SCALE;
	target_rating += turning_rating * TURNING_SCALE;

	//...Smaller targets are harder to identify (was 0.9: crouching barely mattered)
	if ( crouched )
	{
		target_rating *= 0.6f;
	}
	if ( npc->client->NPC_class == CLASS_SWAMPTROOPER )
	{//swamptroopers can see much better
		target_rating *= 1.33f;
	}
	//difficulty: easy 0.8, medium 1.0, hard 1.2
	target_rating *= 0.8f + 0.2f * Com_Clamp( 0, 2, g_spskill->integer );

	return Q_max( 0.0f, target_rating );
}

/*
-------------------------
state changes
-------------------------
*/
static void Aware_Say( gentity_t *npc, int state )
{
	if ( npc->NPC->scriptFlags & SCF_NO_COMBAT_TALK )
	{
		return;
	}
	if ( Aware_SaberUser( npc ) )
	{
		switch ( state )
		{
		case AW_SUSPICIOUS:
		case AW_SEARCHING:	G_AddVoiceEvent( npc, Q_irand( EV_JDETECTED1, EV_JDETECTED3 ), 3000 );	break;
		case AW_ALERT:		G_AddVoiceEvent( npc, Q_irand( EV_ANGER1, EV_ANGER3 ), 2000 );			break;
		default:			break;
		}
		return;
	}
	switch ( state )
	{
	case AW_SUSPICIOUS:	G_AddVoiceEvent( npc, Q_irand( EV_SIGHT1, EV_SIGHT3 ), 3000 );			break;
	case AW_SEARCHING:	G_AddVoiceEvent( npc, Q_irand( EV_SUSPICIOUS1, EV_SUSPICIOUS3 ), 3000 );	break;
	case AW_ALERT:		G_AddVoiceEvent( npc, Q_irand( EV_DETECTED1, EV_DETECTED3 ), 2000 );		break;
	case AW_UNAWARE:	G_AddVoiceEvent( npc, Q_irand( EV_GIVEUP1, EV_GIVEUP4 ), 3000 );			break;
	default:			break;
	}
}

// go and look where it was: the trooper investigate behaviour walks to investigateGoal
static void Aware_GoLook( gentity_t *npc )
{
	gNPC_t *ni = npc->NPC;
	if ( !(ni->scriptFlags&SCF_CHASE_ENEMIES) || Q3_TaskIDPending( npc, TID_MOVE_NAV ) )
	{//not allowed to wander: just face it
		return;
	}
	if ( ni->tempBehavior != BS_DEFAULT && ni->tempBehavior != BS_INVESTIGATE )
	{//scripted into something else
		return;
	}
	if ( Aware_SaberUser( npc ) )
	{//Jedi don't use the trooper investigate loop; they face it and light up (Jedi_Patrol)
		return;
	}
	VectorCopy( ni->lastKnownEnemyPos, ni->investigateGoal );
	NPC_SetMoveGoal( npc, ni->investigateGoal, 32, qtrue );
	ni->localState = ST_LSTATE_INVESTIGATE;
	ni->tempBehavior = BS_INVESTIGATE;
	ni->investigateDebounceTime = 8000;
	ni->pauseTime = level.time;
}

static void Aware_Enter( gentity_t *npc, int state, gentity_t *target )
{
	gNPC_t *ni = npc->NPC;
	const int old = ni->awareState;
	ni->awareState = state;

	switch ( state )
	{
	case AW_SUSPICIOUS:
		if ( old < AW_SUSPICIOUS )
		{
			if ( target )
			{
				NPC_TempLookTarget( npc, target->s.number, 2000, 4000 );
			}
			Aware_Say( npc, AW_SUSPICIOUS );
		}
		break;
	case AW_SEARCHING:
		if ( old < AW_SEARCHING )
		{
			Aware_Say( npc, AW_SEARCHING );
			if ( Aware_SaberUser( npc ) && !npc->client->ps.saberInFlight && !npc->client->ps.SaberActive() )
			{
				npc->client->ps.SaberActivate();
			}
			Aware_GoLook( npc );
		}
		break;
	case AW_ALERT:
		ni->awareLevel = 1.0f;
		if ( target && !npc->enemy && (ni->scriptFlags&SCF_LOOK_FOR_ENEMIES) )
		{
			G_SetEnemy( npc, target );
			ni->enemyLastSeenTime = level.time;
			VectorCopy( target->currentOrigin, ni->enemyLastSeenLocation );
			TIMER_Set( npc, "attackDelay", Q_irand( 500, 2500 ) );
			if ( old < AW_ALERT )
			{
				Aware_Say( npc, AW_ALERT );
			}
		}
		break;
	case AW_UNAWARE:
		if ( old >= AW_SEARCHING )
		{
			Aware_Say( npc, AW_UNAWARE );
		}
		break;
	default:
		break;
	}
}

// apply the thresholds to awareLevel; target is whoever (if anyone) is responsible
static void Aware_Apply( gentity_t *npc, gentity_t *target )
{
	gNPC_t *ni = npc->NPC;
	ni->awareLevel = Com_Clamp( 0.0f, 1.0f, ni->awareLevel );

	int state = ni->awareState;
	if ( ni->awareLevel >= AWARE_ALERT )
	{
		state = AW_ALERT;
	}
	else if ( ni->awareLevel >= AWARE_SEARCHING )
	{
		state = Q_max( state, AW_SEARCHING );
	}
	else if ( ni->awareLevel >= AWARE_SUSPICIOUS )
	{
		state = Q_max( state, AW_SUSPICIOUS );
	}
	//coming down: a little hysteresis so they don't flicker
	if ( state == AW_ALERT && ni->awareLevel < AWARE_ALERT )
	{
		state = AW_SEARCHING;
	}
	if ( state == AW_SEARCHING && ni->awareLevel < AWARE_SEARCHING - 0.1f )
	{
		state = AW_SUSPICIOUS;
	}
	if ( state == AW_SUSPICIOUS && ni->awareLevel < AWARE_SUSPICIOUS - 0.1f )
	{
		state = AW_UNAWARE;
	}
	if ( state != ni->awareState )
	{
		Aware_Enter( npc, state, target );
	}
}

/*
-------------------------
AI_AwarenessUpdate
-------------------------
*/
void AI_AwarenessUpdate( gentity_t *npc )
{
	if ( !g_stealth || !g_stealth->integer || !Aware_Eligible( npc ) )
	{
		return;
	}
	gNPC_t *ni = npc->NPC;

	if ( npc->enemy )
	{//fighting: we know exactly where they are whenever we can see them
		ni->awareState = AW_ALERT;
		ni->awareLevel = 1.0f;
		ni->awareLastRise = level.time;
		if ( ni->enemyLastSeenTime == level.time )
		{
			VectorCopy( npc->enemy->currentOrigin, ni->lastKnownEnemyPos );
			ni->lastKnownEnemyTime = level.time;
		}
		return;
	}

	const int	dt	= ( ni->awareLastRise > 0 && level.time > ni->awareLastRise ) ? Q_min( 500, level.time - ni->awareLastRise ) : 100;
	const float	dts	= dt * 0.001f;
	ni->awareLastRise = level.time;

	gentity_t	*bestEnt	= NULL;
	float		best		= 0;
	qboolean	instant		= qfalse;

	if ( (ni->scriptFlags&SCF_LOOK_FOR_ENEMIES)
		&& !(npc->svFlags&SVF_IGNORE_ENEMIES)
		&& npc->client->enemyTeam != TEAM_NEUTRAL
		&& ni->confusionTime < level.time && ni->insanityTime < level.time )
	{//who's out there?  the player and anyone fighting for them
		for ( int i = 0; i < ENTITYNUM_WORLD; i++ )
		{
			gentity_t *ent = &g_entities[i];
			if ( !ent->inuse || !ent->client || ent == npc )
			{
				continue;
			}
			if ( !G_ValidEnemy( npc, ent ) || !gi.inPVS( npc->currentOrigin, ent->currentOrigin ) )
			{
				continue;
			}
			qboolean	inst;
			const float	r = AI_SightRating( npc, ent, &inst );
			if ( inst )
			{
				instant = qtrue;
				bestEnt = ent;
				best = 2.0f;
				break;
			}
			if ( r > best )
			{
				best = r;
				bestEnt = ent;
			}
		}
	}

	if ( instant )
	{
		ni->awareLevel = 1.0f;
	}
	else if ( best > 0 )
	{
		float rate;
		if ( best >= REALIZE_THRESHOLD )
		{//obvious: a second or so
			rate = 1.0f + ( best - REALIZE_THRESHOLD ) * 2.0f;
		}
		else if ( best >= CAUTIOUS_THRESHOLD )
		{//something's there: a few seconds of looking
			rate = 0.4f;
		}
		else
		{//barely: a long stare
			rate = 0.15f * ( best / CAUTIOUS_THRESHOLD );
		}
		ni->awareLevel += rate * dts;
		TIMER_Set( npc, "awareHold", 3000 );
	}
	else if ( TIMER_Done( npc, "awareHold" ) )
	{//nothing to see: it fades (slower once they're actively searching)
		ni->awareLevel -= ( ni->awareState >= AW_SEARCHING ? 0.04f : 0.08f ) * dts;
	}
	if ( ni->searchPhase && ni->awareLevel < AWARE_SEARCHING )
	{//once they know for certain someone is here (a knocked-out buddy), they never relax past searching
		ni->awareLevel = AWARE_SEARCHING;
	}

	if ( bestEnt && best > 0 )
	{
		VectorCopy( bestEnt->currentOrigin, ni->lastKnownEnemyPos );
		ni->lastKnownEnemyTime = level.time;
	}

	Aware_Apply( npc, bestEnt );
}

/*
-------------------------
AI_AwarenessNoise

  Something was heard or seen (an alert event). The event level says how much.
-------------------------
*/
void AI_AwarenessNoise( gentity_t *npc, int alertLevel, const vec3_t pos, gentity_t *owner )
{
	if ( !g_stealth || !g_stealth->integer || !Aware_Eligible( npc ) || npc->enemy )
	{
		return;
	}
	gNPC_t *ni = npc->NPC;
	float add;
	switch ( alertLevel )
	{
	case AEL_MINOR:			add = 0.15f;	break;
	case AEL_SUSPICIOUS:	add = 0.35f;	break;
	default:				add = 0.7f;		break;//discovered, danger
	}
	ni->awareLevel += add;
	VectorCopy( pos, ni->lastKnownEnemyPos );
	ni->lastKnownEnemyTime = level.time;
	TIMER_Set( npc, "awareHold", 3000 );

	gentity_t *target = ( owner && owner->client && G_ValidEnemy( npc, owner ) ) ? owner : NULL;
	Aware_Apply( npc, target );
}

void AI_AwarenessSetAlert( gentity_t *npc )
{
	if ( !npc || !npc->NPC )
	{
		return;
	}
	npc->NPC->awareState = AW_ALERT;
	npc->NPC->awareLevel = 1.0f;
	npc->NPC->lastKnownEnemyTime = level.time;
	if ( npc->enemy )
	{
		VectorCopy( npc->enemy->currentOrigin, npc->NPC->lastKnownEnemyPos );
	}
}

void AI_AwarenessLostEnemy( gentity_t *npc )
{
	if ( !npc || !npc->NPC || npc->NPC->awareState != AW_ALERT )
	{
		return;
	}
	npc->NPC->awareState = AW_SEARCHING;
	npc->NPC->awareLevel = 0.8f;
	VectorCopy( npc->NPC->enemyLastSeenLocation, npc->NPC->lastKnownEnemyPos );
	TIMER_Set( npc, "awareHold", 10000 );
}

void AI_AwarenessReset( gentity_t *npc, float level )
{
	if ( !npc || !npc->NPC )
	{
		return;
	}
	if ( npc->NPC->searchPhase && level < AWARE_SEARCHING )
	{//they know someone's here: searching is as calm as they get
		level = AWARE_SEARCHING;
	}
	npc->NPC->awareLevel = Com_Clamp( 0.0f, 1.0f, level );
	npc->NPC->awareState = level >= AWARE_SEARCHING ? AW_SEARCHING : ( level >= AWARE_SUSPICIOUS ? AW_SUSPICIOUS : AW_UNAWARE );
	TIMER_Remove( npc, "awareHold" );
}

/*
-------------------------
AI_AwarenessAlarm

  Someone is definitely here (a buddy found out cold): this NPC and its squad go to ALERT
  on the player if they're a valid enemy, and never drop below SEARCHING again.
-------------------------
*/
static void Aware_AlarmOne( gentity_t *npc, const vec3_t where )
{
	if ( !npc || !npc->inuse || !npc->NPC || !npc->client || npc->health <= 0 || G_IsNeutralized( npc ) )
	{
		return;
	}
	npc->NPC->searchPhase = 1;
	VectorCopy( where, npc->NPC->lastKnownEnemyPos );
	npc->NPC->lastKnownEnemyTime = level.time;
	VectorCopy( where, npc->NPC->enemyLastSeenLocation );
	if ( !npc->enemy && G_ValidEnemy( npc, player ) && (npc->NPC->scriptFlags&SCF_LOOK_FOR_ENEMIES) )
	{
		G_SetEnemy( npc, player );
	}
	else
	{
		AI_AwarenessReset( npc, AWARE_SEARCHING + 0.2f );
	}
}

void AI_AwarenessAlarm( gentity_t *npc, const vec3_t where )
{
	Aware_AlarmOne( npc, where );
	if ( npc && npc->NPC && npc->NPC->group )
	{
		AIGroupInfo_t *group = npc->NPC->group;
		for ( int i = 0; i < group->numGroup; i++ )
		{
			Aware_AlarmOne( &g_entities[group->member[i].number], where );
		}
	}
}

/*
-------------------------
AI_CanBeSnuckUpOn
-------------------------
*/
qboolean AI_CanBeSnuckUpOn( gentity_t *npc, gentity_t *by )
{
	if ( !npc || !by || !npc->NPC || !npc->client || npc->health <= 0 )
	{
		return qfalse;
	}
	if ( npc->enemy || G_IsNeutralized( npc ) )
	{
		return qfalse;
	}
	if ( g_stealth && g_stealth->integer && npc->NPC->awareState >= AW_SEARCHING )
	{//already looking for you
		return qfalse;
	}
	if ( !G_CanNeutralize( npc, NH_STEALTH ) )
	{
		return qfalse;
	}
	if ( npc->client->ps.forcePowersKnown & ~( 1 << FP_LEVITATION ) )
	{//anyone with the Force (beyond the jump every NPC gets) feels you coming
		return qfalse;
	}
	if ( DistanceSquared( npc->currentOrigin, by->currentOrigin ) > 80.0f * 80.0f )
	{
		return qfalse;
	}
	//from behind: they're facing away from you
	vec3_t fwd, dir;
	AngleVectors( npc->client->ps.viewangles, fwd, NULL, NULL );
	VectorSubtract( by->currentOrigin, npc->currentOrigin, dir );
	dir[2] = 0;
	VectorNormalize( dir );
	return (qboolean)( DotProduct( fwd, dir ) < -0.2f );
}

/*
-------------------------
The old instant check: droids, mechs, turrets, and everyone when g_stealth is off
-------------------------
*/
qboolean NPC_CheckEnemyStealth( gentity_t *target )
{
	//In case we aquired one some other way
	if ( NPC->enemy != NULL )
	{
		return qtrue;
	}
	if ( !(NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES) )
	{
		return qfalse;
	}
	qboolean	instant;
	const float	rating = AI_SightRating( NPC, target, &instant );
	if ( instant || rating > REALIZE_THRESHOLD )
	{
		G_SetEnemy( NPC, target );
		NPCInfo->enemyLastSeenTime = level.time;
		TIMER_Set( NPC, "attackDelay", Q_irand( 500, 2500 ) );
		return qtrue;
	}
	return qfalse;
}

qboolean NPC_CheckPlayerTeamStealth( void )
{
	if ( g_stealth && g_stealth->integer && G_StandardHumanoid( NPC ) )
	{//awareness already ran this think (NPC_RunBehavior); it set the enemy if it got that far
		return (qboolean)( NPC->enemy != NULL );
	}
	for ( int i = 0; i < ENTITYNUM_WORLD; i++ )
	{
		if ( !PInUse( i ) )
		{
			continue;
		}
		gentity_t *enemy = &g_entities[i];
		if ( enemy && enemy->client && NPC_ValidEnemy( enemy ) )
		{
			if ( NPC_CheckEnemyStealth( enemy ) )
			{
				return qtrue;
			}
		}
	}
	return qfalse;
}

/*
-------------------------
G_BodyDragThink

  The player grabbed a body with +use (TryUse). Keep it just behind them while they walk;
  let go if they get away from it or die.
-------------------------
*/
void G_BodyDragThink( gentity_t *body )
{
	if ( !body || !body->client || body->client->ps.heldByClient >= ENTITYNUM_WORLD )
	{
		return;
	}
	gentity_t *dragger = &g_entities[body->client->ps.heldByClient];
	if ( !dragger->inuse || !dragger->client || dragger->health <= 0 || dragger->client->ps.heldClient != body->s.number )
	{
		body->client->ps.heldByClient = ENTITYNUM_NONE;
		body->owner = NULL;
		return;
	}
	if ( DistanceSquared( dragger->currentOrigin, body->currentOrigin ) > 160.0f * 160.0f )
	{//lost our grip
		G_ReleaseEntity( dragger );
		return;
	}

	vec3_t	fwd, want, mins, maxs;
	trace_t	tr;
	const vec3_t yawOnly = { 0, dragger->client->ps.viewangles[YAW], 0 };
	AngleVectors( yawOnly, fwd, NULL, NULL );
	VectorMA( dragger->currentOrigin, -40.0f, fwd, want );
	want[2] = dragger->currentOrigin[2];
	VectorSet( mins, -12, -12, body->mins[2] );
	VectorSet( maxs, 12, 12, Q_min( body->maxs[2], 16.0f ) );
	gi.trace( &tr, body->currentOrigin, mins, maxs, want, body->s.number, MASK_PLAYERSOLID & ~CONTENTS_BODY, (EG2_Collision)0, 0 );
	if ( tr.startsolid || tr.allsolid )
	{
		return;
	}
	G_SetOrigin( body, tr.endpos );
	VectorClear( body->client->ps.velocity );
	gi.linkentity( body );
}
