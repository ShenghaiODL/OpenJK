/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include "g_local.h"
#include "anims.h"
#include "b_local.h"
#include "bg_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "g_vehicles.h"
#include "g_force.h"
#include "../qcommon/tri_coll_test.h"
#include "../cgame/cg_local.h"

extern qboolean WP_ForcePowerUsable( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
extern qboolean WP_ForceThrowable( gentity_t *ent, gentity_t *forwardEnt, gentity_t *self, qboolean pull, float cone, float radius, vec3_t forward );
extern void WP_ForcePowerStop( gentity_t *self, forcePowers_t forcePower );
extern void WP_ForceThrowHazardTrooper( gentity_t *self, gentity_t *trooper, qboolean pull );
extern void	WP_ResistForcePush( gentity_t *self, gentity_t *pusher, qboolean noPenalty );
extern int WP_AbsorbConversion(gentity_t *attacked, int atdAbsLevel, gentity_t *attacker, int atPower, int atPowerLevel, int atForceSpent);
extern qboolean ShouldPlayerResistForceThrow( gentity_t *player, gentity_t *attacker, qboolean pull );
extern qboolean Rosh_BeingHealed( gentity_t *self );
extern void G_KnockOffVehicle( gentity_t *pRider, gentity_t *self, qboolean bPull );
extern void WP_ForceKnockdown( gentity_t *self, gentity_t *pusher, qboolean pull, qboolean strongKnockdown, qboolean breakSaberLock );
extern qboolean Jedi_WaitingAmbush( gentity_t *self );
extern void G_ReflectMissile( gentity_t *ent, gentity_t *missile, vec3_t forward, qboolean perfect = qfalse );
extern qboolean InFront( vec3_t spot, vec3_t from, vec3_t fromAngles, float threshHold = 0.0f );
extern void WP_KnockdownTurret( gentity_t *self, gentity_t *pas );
extern void WP_SaberDrop( gentity_t *self, gentity_t *saber );
extern void WP_SaberReturn( gentity_t *self, gentity_t *saber );
extern qboolean PM_InKnockDown( playerState_t *ps );
extern void WP_ForcePowerDrain( gentity_t *self, forcePowers_t forcePower, int overrideAmt );

extern cvar_t	*g_timescale;
extern cvar_t	*g_forceStaggerLockoutScale;

extern int forcePowerNeeded[NUM_FORCE_POWERS];

extern qboolean	G_IsSaberBoss( const gentity_t *ent, qboolean includeElites );
extern int		WP_RepulseFullCharge( const gentity_t *self );

extern cvar_t	*g_debugForce;
extern void	G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );

//JKEnhanced 4B.4: the blast itself barely hurts (force.cfg repulse.damage, 0 by default) - the damage is
//in what they're thrown into (G_RepulseSlam)
static void RepulseDamage( gentity_t *self, gentity_t *enemy, float power )
{
	const int dmg = (int)( G_ForceTuning()->repulseDamage * power + 0.5f );
	if ( dmg > 0 )
	{
		G_Damage( enemy, self, self, NULL, enemy->currentOrigin, dmg, DAMAGE_NO_KNOCKBACK, MOD_UNKNOWN );
	}
}

//JKEnhanced: a shot coming at us goes back at whoever fired it (G_ReflectMissile aims by the saber
//defense rules and the crosshair, which suits a parry, not a blast all round)
static void RepulseReflectShot( gentity_t *self, gentity_t *missile, const vec3_t dir2Me )
{
	vec3_t bounceDir;
	if ( missile->owner && missile->owner->inuse && missile->owner != self )
	{
		VectorSubtract( missile->owner->currentOrigin, missile->currentOrigin, bounceDir );
	}
	else
	{
		VectorScale( dir2Me, -1.0f, bounceDir );
	}
	VectorNormalize( bounceDir );
	for ( int i = 0; i < 3; i++ )
	{
		bounceDir[i] += Q_flrand( -0.1f, 0.1f );
	}
	VectorNormalize( bounceDir );
	const float speed = VectorLength( missile->s.pos.trDelta );
	VectorScale( bounceDir, speed, missile->s.pos.trDelta );
	missile->s.pos.trTime = level.time - 10;	// move a bit on the very first frame
	VectorCopy( missile->currentOrigin, missile->s.pos.trBase );
	if ( !missile->lastEnemy )
	{//remember who originally shot this missile
		missile->lastEnemy = missile->owner;
	}
	missile->owner = self;
	if ( missile->s.weapon == WP_ROCKET_LAUNCHER )
	{//stop homing
		missile->e_ThinkFunc = thinkF_NULL;
	}
}

/*
Catching shots (JKEnhanced): while the player charges Repulse, shots coming at them within
REPULSE_CATCH_RADIUS stop dead in the air (TR_STATIONARY, their own timeout paused). When it fires they
all go back at whoever shot them (G_RepulseFlingShots); a charge that's cancelled lets them carry on
(G_RepulseReleaseShots). Not saved: a held shot from a loaded save just times out.
*/
#define REPULSE_CATCH_RADIUS	192.0f
static struct
{
	int		by;			// catcher + 1 (0 = not caught)
	int		stamp;		// s.pos.trTime when caught (a reused slot won't match)
	int		trType;		// to restore
	int		thinkLeft;	// ms left on its think when caught
} s_caught[MAX_GENTITIES];

static qboolean G_RepulseHolds( const gentity_t *self, const gentity_t *missile )
{
	const int n = missile->s.number;
	return (qboolean)( s_caught[n].by == self->s.number + 1 && missile->inuse && missile->s.eType == ET_MISSILE
		&& missile->s.pos.trType == TR_STATIONARY && missile->s.pos.trTime == s_caught[n].stamp );
}

static void RepulseReflectShot( gentity_t *self, gentity_t *missile, const vec3_t dir2Me );

void G_RepulseCatchShots( gentity_t *self )
{
	if ( !self || !self->client || self->s.number >= MAX_CLIENTS )
	{
		return;
	}
	gentity_t	*list[MAX_GENTITIES];
	vec3_t		mins, maxs;
	for ( int i = 0; i < 3; i++ )
	{
		mins[i] = self->currentOrigin[i] - REPULSE_CATCH_RADIUS;
		maxs[i] = self->currentOrigin[i] + REPULSE_CATCH_RADIUS;
	}
	const int num = gi.EntitiesInBox( mins, maxs, list, MAX_GENTITIES );
	for ( int i = 0; i < num; i++ )
	{
		gentity_t *m = list[i];
		if ( !m->inuse || m->s.eType != ET_MISSILE )
		{
			continue;
		}
		const int n = m->s.number;
		if ( G_RepulseHolds( self, m ) )
		{//still held: its timeout stays paused
			if ( m->nextthink )
			{
				m->nextthink = level.time + s_caught[n].thinkLeft;
			}
			continue;
		}
		if ( m->owner == self || m->s.weapon == WP_SABER
			|| ( m->s.pos.trType != TR_LINEAR && m->s.pos.trType != TR_GRAVITY )
			|| ( m->s.powerups & ( 1 << PW_FORCE_PROJECTILE ) ) )
		{
			continue;
		}
		vec3_t toMe;
		VectorSubtract( self->currentOrigin, m->currentOrigin, toMe );
		if ( VectorLengthSquared( toMe ) > REPULSE_CATCH_RADIUS*REPULSE_CATCH_RADIUS
			|| DotProduct( m->s.pos.trDelta, toMe ) <= 0 )
		{//not coming at us
			continue;
		}
		s_caught[n].by = self->s.number + 1;
		s_caught[n].trType = m->s.pos.trType;
		s_caught[n].thinkLeft = m->nextthink ? Q_max( FRAMETIME, m->nextthink - level.time ) : 0;
		VectorCopy( m->currentOrigin, m->s.pos.trBase );
		m->s.pos.trType = TR_STATIONARY;	//(trDelta kept: its speed for the way back)
		m->s.pos.trTime = level.time;
		s_caught[n].stamp = m->s.pos.trTime;
		if ( g_debugForce && g_debugForce->integer )
		{
			gi.Printf( "  REPULSE caught shot %d (%s)\n", n, m->classname ? m->classname : "?" );
		}
	}
}

//fling: back at whoever fired them (it went off); otherwise they carry on as they were (cancelled)
static void G_RepulseLetGo( gentity_t *self, qboolean fling )
{
	if ( !self || self->s.number >= MAX_CLIENTS )
	{
		return;
	}
	for ( int n = MAX_CLIENTS; n < globals.num_entities; n++ )
	{
		gentity_t *m = &g_entities[n];
		if ( !s_caught[n].by )
		{
			continue;
		}
		if ( s_caught[n].by != self->s.number + 1 )
		{
			continue;
		}
		const qboolean held = G_RepulseHolds( self, m );
		s_caught[n].by = 0;
		if ( !held )
		{
			continue;
		}
		m->s.pos.trType = (trType_t)s_caught[n].trType;
		VectorCopy( m->currentOrigin, m->s.pos.trBase );
		m->s.pos.trTime = level.time;
		if ( m->nextthink )
		{
			m->nextthink = level.time + s_caught[n].thinkLeft;
		}
		if ( fling )
		{
			vec3_t dir2Me;
			VectorSubtract( self->currentOrigin, m->currentOrigin, dir2Me );
			RepulseReflectShot( self, m, dir2Me );
		}
	}
}

void G_RepulseReleaseShots( gentity_t *self )
{
	G_RepulseLetGo( self, qfalse );
}
static qboolean G_RepulseCanBlock( gentity_t *self, gentity_t *victim )
{
	return (qboolean)( victim->client->ps.weapon == WP_SABER
		&& victim->client->ps.SaberActive()
		&& victim->client->ps.groundEntityNum != ENTITYNUM_NONE
		&& !PM_InKnockDown( &victim->client->ps )
		&& victim->client->ps.forceRageRecoveryTime < level.time
		&& !( victim->client->ps.eFlags & EF_FORCE_GRIPPED )
		&& InFront( self->currentOrigin, victim->currentOrigin, victim->client->ps.viewangles, 0.3f ) );
}

/*
Repulse slams: someone thrown by Repulse who hits a wall / prop / another person in the next
REPULSE_SLAM_MS takes damage by how fast they hit (force.cfg repulse.slamMin / slamFull / slamDamage);
a person they hit takes most of it too and goes down. Once per throw; landing on a floor doesn't count.
Called from DoImpact (g_active.cpp).
*/
#define REPULSE_SLAM_MS		1200
static struct
{
	int		until;
	int		pusher;
} s_repulseSlam[MAX_GENTITIES];

static void G_RepulseMarkSlam( gentity_t *victim, gentity_t *pusher )
{
	s_repulseSlam[victim->s.number].until = level.time + REPULSE_SLAM_MS;
	s_repulseSlam[victim->s.number].pusher = pusher->s.number;
}

void G_RepulseSlam( gentity_t *self, gentity_t *other, trace_t *trace, float speed )
{
	if ( !self || !self->client || !other || self->s.number < 0 || self->s.number >= MAX_GENTITIES )
	{
		return;
	}
	const int until = s_repulseSlam[self->s.number].until;
	if ( until <= level.time || until > level.time + REPULSE_SLAM_MS )//(over, or stale from a loaded save)
	{
		return;
	}
	const int pusherNum = s_repulseSlam[self->s.number].pusher;
	if ( other->s.number == pusherNum || other == self )
	{
		return;
	}
	if ( !other->client && trace && trace->plane.normal[2] > 0.7f )
	{//landing on a floor
		return;
	}
	const forceTuning_t *ft = G_ForceTuning();
	if ( speed < ft->repulseSlamMin )
	{
		return;
	}
	s_repulseSlam[self->s.number].until = 0;	//once
	gentity_t *pusher = ( pusherNum >= 0 && pusherNum < ENTITYNUM_WORLD ) ? &g_entities[pusherNum] : NULL;
	const float frac = Q_min( 1.0f, ( speed - ft->repulseSlamMin ) / (float)( ft->repulseSlamFull - ft->repulseSlamMin ) );
	const int dmg = Q_max( 1, (int)( ft->repulseSlamDamage * frac + 0.5f ) );

	if ( trace )
	{
		G_PlayEffect( G_EffectIndex( "env/impact_dustonly" ), trace->endpos, trace->plane.normal );
	}
	G_Sound( self, G_SoundIndex( va( "sound/weapons/melee/punch%d", Q_irand( 1, 4 ) ) ) );
	G_Damage( self, pusher, pusher, NULL, self->currentOrigin, dmg, DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK, MOD_IMPACT );
	if ( other->client && other->health > 0 )
	{//thrown into someone: they take most of it and go down
		vec3_t dir;
		VectorSubtract( other->currentOrigin, self->currentOrigin, dir );
		VectorNormalize( dir );
		G_Damage( other, pusher, pusher, dir, other->currentOrigin, dmg * 3 / 4, DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK, MOD_IMPACT );
		if ( other->health > 0 && !G_IsSaberBoss( other, qfalse )
			&& !( pusher && pusher->client && other->client->playerTeam == pusher->client->playerTeam ) )
		{//(not the pusher's own side: a body thrown into your companion bowled them over)
			G_Knockdown( other, pusher ? pusher : self, dir, 300, qtrue );
		}
	}
	if ( g_debugForce && g_debugForce->integer )
	{
		gi.Printf( "  REPULSE slam: %d (%s) hit %s at %.0f u/s: %d damage%s\n", self->s.number,
			self->NPC_type ? self->NPC_type : "player", other->client ? ( other->NPC_type ? other->NPC_type : "player" ) : "something solid",
			speed, dmg, other->client ? " (and most of it to them)" : "" );
	}
}

void ForceRepulseThrow( gentity_t *self, int chargeTime )
{
	//shove things around you away
	qboolean fake = qfalse;
	float		dist;
	gentity_t	*ent, *forwardEnt = NULL;
	gentity_t	*entityList[MAX_GENTITIES];
	gentity_t	*push_list[MAX_GENTITIES];
	int			numListedEntities = 0;
	vec3_t		mins, maxs;
	vec3_t		v;
	int			i, e;
	int			ent_count = 0;
	int			radius;
	vec3_t		center, ent_org, size, forward, right, end, dir, fwdangles = {0};
	trace_t		tr;
	int			anim, hold, soundIndex, cost;
	const forceTuning_t *ft = G_ForceTuning();
	
	if ( self->health <= 0 )
	{
		return;
	}
	if ( self->client->ps.leanofs )
	{//can't force-throw while leaning
		return;
	}
	if ( self->client->ps.forcePowerDebounce[FP_REPULSE] > level.time )
	{//already pushing- now you can't haul someone across the room, sorry
		return;
	}
	if ( !self->s.number && (cg.zoomMode || in_camera) )
	{//can't force throw/pull when zoomed in or in cinematic
		return;
	}
	extern qboolean WP_RepulseHeld( const gentity_t *self );
	if ( WP_RepulseHeld( self ) )
	{//JKEnhanced 4B.4: not out of a saber lock / Grip / Drain (Push is the way out of those)
		return;
	}

	if ( self->client->ps.legsAnim == BOTH_KNOCKDOWN3
		|| (self->client->ps.torsoAnim == BOTH_FORCE_GETUP_F1 && self->client->ps.torsoAnimTimer > 400)
		|| (self->client->ps.torsoAnim == BOTH_FORCE_GETUP_F2 && self->client->ps.torsoAnimTimer > 900)
		|| (self->client->ps.torsoAnim == BOTH_GETUP3 && self->client->ps.torsoAnimTimer > 500)
		|| (self->client->ps.torsoAnim == BOTH_GETUP4 && self->client->ps.torsoAnimTimer > 300)
		|| (self->client->ps.torsoAnim == BOTH_GETUP5 && self->client->ps.torsoAnimTimer > 500) )
	{//we're face-down, so we'd only be force-push/pulling the floor
		return;
	}
	
	radius = forcePushPullRadius[self->client->ps.forcePowerLevel[FP_REPULSE]];
	
	if ( !radius )
	{//no ability to do this yet
		return;
	}
	
	//JKEnhanced 4B.4: charge -> strength 0-1 (a tap still throws at repulse.minPct)
	const int fullCharge = WP_RepulseFullCharge( self );
	float power = ( fullCharge > 0 ) ? (float)chargeTime / fullCharge : 1.0f;
	power = Com_Clamp( ft->repulseMinPct / 100.0f, 1.0f, power );
	
	extern int WP_RepulseCost( const gentity_t *self );
	cost = WP_RepulseCost( self );//JKEnhanced: NPCs pay here; the player paid while charging (WP_RepulseChargeDrain)
	if ( self->s.number >= MAX_CLIENTS && !WP_ForcePowerUsable( self, FP_REPULSE, cost ) )
	{//(the player's charge already passed this when it started - an empty bar at the end still fires)
		return;
	}
	//make sure this plays and that you cannot press fire for about 1 second after this
	anim = BOTH_FORCE_2HANDEDLIGHTNING_RELEASE;//JKEnhanced: the release brings the hands back (the hold loops forever)
	soundIndex = G_SoundIndex( "sound/weapons/force/repulse.wav" );
	hold = 650;
	
	int parts = SETANIM_TORSO;
	NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART );
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;
	if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
	{
		hold = floor( hold*g_timescale->value );
	}
	self->client->ps.weaponTime = hold;//was 1000, but want to swing sooner
	//do effect... FIXME: build-up or delay this until in proper part of anim
	self->client->ps.powerups[PW_FORCE_REPULSE] = level.time + self->client->ps.torsoAnimTimer + 500;
	//reset to 0 in case it's still > 0 from a previous push
	self->client->pushEffectFadeTime = 0;
	
	G_Sound( self, soundIndex );
	
	VectorCopy( self->client->ps.viewangles, fwdangles );
	//fwdangles[1] = self->client->ps.viewangles[1];
	AngleVectors( fwdangles, forward, right, NULL );
	VectorCopy( self->currentOrigin, center );
	
	if ( !numListedEntities )
	{
		for ( i = 0 ; i < 3 ; i++ )
		{
			mins[i] = center[i] - radius;
			maxs[i] = center[i] + radius;
		}
		
		numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );
		
		for ( e = 0 ; e < numListedEntities ; e++ )
		{
			ent = entityList[ e ];
			
			//JKEnhanced: every shot in flight, blaster bolts included (Push only takes the heavy ones)
			const qboolean shot = (qboolean)( ent->inuse && ent->s.eType == ET_MISSILE && ent->owner != self
				&& ent->s.weapon != WP_SABER && ent->s.pos.trType != TR_STATIONARY
				&& !( ent->s.powerups & ( 1 << PW_FORCE_PROJECTILE ) ) );
			if ( !shot && !WP_ForceThrowable( ent, forwardEnt, self, qfalse, 0.0f, radius, forward ) )
			{
				continue;
			}

			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for ( i = 0 ; i < 3 ; i++ )
			{
				if ( center[i] < ent->absmin[i] )
				{
					v[i] = ent->absmin[i] - center[i];
				} else if ( center[i] > ent->absmax[i] )
				{
					v[i] = center[i] - ent->absmax[i];
				} else
				{
					v[i] = 0;
				}
			}
			
			VectorSubtract( ent->absmax, ent->absmin, size );
			VectorMA( ent->absmin, 0.5, size, ent_org );
			
			//see if they're in front of me
			VectorSubtract( ent_org, center, dir );
			VectorNormalize( dir );
			
			dist = VectorLength( v );
			
			if ( dist >= radius )
			{
				continue;
			}
			
			//in PVS?
			if ( !ent->bmodel && !gi.inPVS( ent_org, self->client->renderInfo.eyePoint ) )
			{//must be in PVS
				continue;
			}
			
			if ( ent != forwardEnt )
			{//don't need to trace against forwardEnt again
				//really should have a clear LOS to this thing...
				gi.trace( &tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_FORCE_PUSH, (EG2_Collision)0, 0 );//was MASK_SHOT, but changed to match above trace and crosshair trace
				if ( tr.fraction < 1.0f && tr.entityNum != ent->s.number )
				{//must have clear LOS
					continue;
				}
			}
			
			// ok, we are within the radius, add us to the incoming list
			push_list[ent_count] = ent;
			ent_count++;
		}
	}
	
	int maxVictimRecovery = 0;//longest recovery inflicted on any target this throw, used below to keep our own follow-up lockout honest
	for ( int x = 0; x < ent_count; x++ )
	{
		if ( push_list[x]->client )
		{//JKEnhanced 4B.4: clears space - harder the closer they are; a saber user facing it can block
		 //(and stops attacking to); the damage comes from what they're thrown into (G_RepulseSlam)
			gentity_t	*victim = push_list[x];
			vec3_t		pushDir;

			if ( Rosh_BeingHealed( victim ) )
			{
				continue;
			}
			if ( victim->client->NPC_class == CLASS_HAZARD_TROOPER
				&& victim->health > 0 )
			{//living hazard troopers resist push/pull
				WP_ForceThrowHazardTrooper( self, victim, qfalse );
				continue;
			}
			if ( fake )
			{//always resist
				WP_ResistForcePush( victim, self, qfalse );
				continue;
			}

			int modPowerLevel = WP_AbsorbConversion( victim, victim->client->ps.forcePowerLevel[FP_ABSORB], self, FP_REPULSE, self->client->ps.forcePowerLevel[FP_REPULSE], forcePowerNeeded[FP_REPULSE] );//(was indexed by LEVEL)
			if ( victim->client->NPC_class == CLASS_ASSASSIN_DROID )
			{
				modPowerLevel = 0;	// devides throw by 10
			}
			if ( modPowerLevel != -1
				&& InFront( self->currentOrigin, victim->client->renderInfo.eyePoint, victim->client->ps.viewangles, 0.3f ) )
			{//absorbed and I'm in front of them
				if ( victim->client->ps.forcePowerLevel[FP_ABSORB] <= FORCE_LEVEL_2 )
				{
					WP_ResistForcePush( victim, self, qfalse );
					victim->client->ps.saberMove = victim->client->ps.saberBounceMove = LS_READY;
					victim->client->ps.saberBlocked = BLOCKED_NONE;
				}
				continue;
			}
			if ( !victim->s.number && ShouldPlayerResistForceThrow( victim, self, qfalse ) )
			{//an NPC's Repulse on the player: the player can still brace against it (no attacking meanwhile)
				WP_ResistForcePush( victim, self, qfalse );
				victim->client->ps.saberMove = victim->client->ps.saberBounceMove = LS_READY;
				victim->client->ps.saberBlocked = BLOCKED_NONE;
				victim->client->ps.weaponTime = Q_max( victim->client->ps.weaponTime, 700 );
				continue;
			}
			if ( Jedi_WaitingAmbush( victim ) )
			{
				WP_ForceKnockdown( victim, self, qfalse, qtrue, qfalse );
				RepulseDamage( self, victim, power );
				continue;
			}

			G_KnockOffVehicle( victim, self, qfalse );

			if ( victim->NPC && victim->NPC->jumpState == JS_JUMPING )
			{//don't interrupt a scripted jump
				victim->forcePushTime = level.time + 600;
				continue;
			}
			if ( victim->s.number && ( victim->message || (victim->flags&FL_NO_KNOCKBACK) ) )
			{//an NPC who has a key / can't be moved: just knock them down
				WP_ForceKnockdown( victim, self, qfalse, qfalse, qfalse );
				RepulseDamage( self, victim, power );
				continue;
			}

			//strength: charge, and closer = harder (the edge of the radius gets half)
			VectorSubtract( victim->currentOrigin, self->currentOrigin, pushDir );
			const float d = VectorNormalize( pushDir );
			float knockback = ft->repulseKnockMin + ( ft->repulseKnockMax - ft->repulseKnockMin ) * power;
			knockback *= 1.0f - 0.5f * Q_min( 1.0f, d / radius );
			if ( modPowerLevel != -1 )
			{//absorbed from behind
				knockback /= ( modPowerLevel <= 0 ) ? 10.0f : ( modPowerLevel == 1 ) ? 6.0f : 2.0f;
			}
			if ( victim->health > 0 && victim->s.number >= MAX_CLIENTS && G_RepulseCanBlock( self, victim ) )
			{//a saber user facing it blocks: holds their ground (slides back a little), but blocking
			 //stops their attack (the player blocks through ShouldPlayerResistForceThrow above)
				WP_ResistForcePush( victim, self, qfalse );
				victim->client->ps.saberMove = victim->client->ps.saberBounceMove = LS_READY;
				victim->client->ps.saberBlocked = BLOCKED_NONE;
				victim->client->ps.weaponTime = Q_max( victim->client->ps.weaponTime, 700 );
				G_Throw( victim, pushDir, knockback * 0.25f );
				victim->forcePuller = self->s.number;
				if ( g_debugForce && g_debugForce->integer )
				{
					gi.Printf( "  REPULSE: %s (%d) blocked it\n", victim->NPC_type ? victim->NPC_type : "?", victim->s.number );
				}
				continue;
			}
			const qboolean boss = G_IsSaberBoss( victim, qfalse );
			if ( boss )
			{//shoved back, not off their feet
				knockback *= 0.5f;
			}
			G_Throw( victim, pushDir, knockback );
			victim->forcePuller = self->s.number;//so they don't hurt me if thrown into me
			G_RepulseMarkSlam( victim, self );
			if ( victim->client->ps.groundEntityNum != ENTITYNUM_NONE && victim->client->ps.velocity[2] < knockback * 0.5f )
			{//off the ground a little
				victim->client->ps.velocity[2] = knockback * 0.5f;
			}

			if ( victim->health > 0 )
			{
				//whatever they were doing, it stops: a swing, a parry, a held power
				victim->client->ps.saberMove = victim->client->ps.saberBounceMove = LS_READY;
				victim->client->ps.saberBlocked = BLOCKED_NONE;
				if ( !boss && !PM_InKnockDown( &victim->client->ps )
					&& ( self->client->ps.forcePowerLevel[FP_REPULSE] > FORCE_LEVEL_1 || power >= 1.0f ) )
				{//L2+ (or a full L1 charge) puts them down
					WP_ForceKnockdown( victim, self, qfalse, (qboolean)( knockback > 150 ), qfalse );
				}
				else
				{//a brief stun: no attacking for a moment
					victim->client->ps.weaponTime = Q_max( victim->client->ps.weaponTime, 500 );
					if ( victim->NPC )
					{
						victim->painDebounceTime = Q_max( victim->painDebounceTime, level.time + 500 );
					}
				}
				RepulseDamage( self, victim, power );
			}
			victim->forcePushTime = level.time + 600; // let the push effect last for 600 ms
			if ( victim->client->ps.torsoAnimTimer > maxVictimRecovery )
			{//track the longest recovery we just inflicted, so our own lockout below can respect it
				maxVictimRecovery = victim->client->ps.torsoAnimTimer;
			}
		}
		else if ( !fake )
		{//not a fake push/pull
			if ( push_list[x]->s.weapon == WP_SABER && (push_list[x]->contents&CONTENTS_LIGHTSABER) )
			{//a thrown saber, just send it back
				/*
				 if ( pull )
				 {//steal it?
				 }
				 else */if ( push_list[x]->owner && push_list[x]->owner->client && push_list[x]->owner->client->ps.SaberActive() && push_list[x]->s.pos.trType == TR_LINEAR && push_list[x]->owner->client->ps.saberEntityState != SES_RETURNING )
				 {//it's on and being controlled
					 //FIXME: prevent it from damaging me?
					 if ( self->s.number == 0 || Q_irand( 0, 2 ) )
					 {//certain chance of throwing it aside and turning it off?
						 //give it some velocity away from me
						 //FIXME: maybe actually push or pull it?
						 if ( Q_irand( 0, 1 ) )
						 {
							 VectorScale( right, -1, right );
						 }
						 G_ReflectMissile( self, push_list[x], right );
						 //FIXME: isn't turning off!!!
						 WP_SaberDrop( push_list[x]->owner, push_list[x] );
					 }
					 else
					 {
						 WP_SaberReturn( push_list[x]->owner, push_list[x] );
					 }
					 //different effect?
				 }
			}
			else if ( push_list[x]->s.eType == ET_MISSILE
					 && push_list[x]->s.pos.trType != TR_STATIONARY
					 && (push_list[x]->s.pos.trType != TR_INTERPOLATE||push_list[x]->s.weapon != WP_THERMAL) )//rolling and stationary thermal detonators are dealt with below
			{
				vec3_t dir2Me;
				VectorSubtract( self->currentOrigin, push_list[x]->currentOrigin, dir2Me );
				float dot = DotProduct( push_list[x]->s.pos.trDelta, dir2Me );
				
				if ( push_list[x]->s.eFlags&EF_MISSILE_STICK )
				{//caught a sticky in-air
					push_list[x]->s.eType = ET_MISSILE;
					push_list[x]->s.eFlags &= ~EF_MISSILE_STICK;
					push_list[x]->s.eFlags |= EF_BOUNCE_HALF;
					push_list[x]->splashDamage /= 3;
					push_list[x]->splashRadius /= 3;
					push_list[x]->e_ThinkFunc = thinkF_WP_Explode;
					push_list[x]->nextthink = level.time + Q_irand( 500, 3000 );
				}
				if ( dot >= 0 )
				{//it's heading towards me: back out the way it came (all round, not just ahead)
					RepulseReflectShot( self, push_list[x], dir2Me );
				}
				else
				{
					VectorScale( push_list[x]->s.pos.trDelta, 1.25f, push_list[x]->s.pos.trDelta );
				}
				//deflect sound
				//G_Sound( push_list[x], G_SoundIndex( va("sound/weapons/blaster/reflect%d.wav", Q_irand( 1, 3 ) ) ) );
				//push_list[x]->forcePushTime = level.time + 600; // let the push effect last for 600 ms
				
				if ( push_list[x]->s.eType == ET_MISSILE
					&& push_list[x]->s.weapon == WP_ROCKET_LAUNCHER
					&& push_list[x]->damage < 60 )
				{//pushing away a rocket raises it's damage to the max for NPCs
					push_list[x]->damage = 60;
				}
			}
			else if ( push_list[x]->svFlags & SVF_GLASS_BRUSH )
			{//break the glass
				trace_t tr;
				vec3_t	pushDir;
				float	damage = 800;
				
				AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
				VectorNormalize( forward );
				VectorMA( self->client->renderInfo.eyePoint, radius, forward, end );
				gi.trace( &tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT, (EG2_Collision)0, 0 );
				if ( tr.entityNum != push_list[x]->s.number || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
				{//must be pointing right at it
					continue;
				}
				
				
				VectorSubtract( tr.endpos, self->client->renderInfo.eyePoint, pushDir );
				
				damage -= VectorNormalize( pushDir );
				if ( damage < 200 )
				{
					damage = 200;
				}
				VectorScale( pushDir, damage, pushDir );
				
				G_Damage( push_list[x], self, self, pushDir, tr.endpos, damage, 0, MOD_UNKNOWN );
			}
			else if ( push_list[x]->s.eType == ET_MISSILE/*thermal resting on ground*/
					 || push_list[x]->s.eType == ET_ITEM
					 || push_list[x]->e_ThinkFunc == thinkF_G_RunObject || Q_stricmp( "limb", push_list[x]->classname ) == 0 )
			{//general object, toss it
				vec3_t	pushDir, kvel;
				float	knockback = 200;
				float	mass = 200;
				
				if ( self->enemy //I have an enemy
					&& push_list[x]->s.eType != ET_ITEM //not an item
					&& self->client->ps.forcePowerLevel[FP_REPULSE] > FORCE_LEVEL_2 //have push 3 or greater
					&& InFront(push_list[x]->currentOrigin, self->currentOrigin, self->currentAngles, 0.25f)//object is generally in front of me
					&& InFront(self->enemy->currentOrigin, self->currentOrigin, self->currentAngles, 0.75f)//enemy is pretty much right in front of me
					&& InFront(push_list[x]->currentOrigin, self->enemy->currentOrigin, self->enemy->currentAngles, 0.25f)//object is generally in front of enemy
					//FIXME: check dist to enemy and clear LOS to enemy and clear Path between object and enemy?
					&& ( (self->NPC&&(Q_irand(0,RANK_CAPTAIN)<self->NPC->rank) )//NPC with enough skill
						||( self->s.number<MAX_CLIENTS ) )
					)
				{//if I have an auto-enemy & he's in front of me, push it toward him!
					/*
					 if ( targetedObjectMassTotal + push_list[x]->mass > TARGETED_OBJECT_PUSH_MASS_MAX )
					 {//already pushed too many things
						//FIXME: pick closest?
						continue;
					 }
					 targetedObjectMassTotal += push_list[x]->mass;
					 */
					VectorSubtract( self->enemy->currentOrigin, push_list[x]->currentOrigin, pushDir );
				}
				else
				{
					VectorSubtract( push_list[x]->currentOrigin, self->currentOrigin, pushDir );
				}
				knockback -= VectorNormalize( pushDir );
				if ( knockback < 100 )
				{
					knockback = 100;
				}
				if ( Q_stricmp( "limb", push_list[x]->classname ) == 0 )
				{//spread multiple gibs out instead of letting them all converge on the exact same point
					pushDir[0] += Q_flrand( -0.3f, 0.3f );
					pushDir[1] += Q_flrand( -0.3f, 0.3f );
					pushDir[2] += Q_flrand( -0.1f, 0.3f );//slight upward bias so they don't just clip into the floor together
					VectorNormalize( pushDir );
				}

				//FIXME: if pull a FL_FORCE_PULLABLE_ONLY, clear the flag, assuming it's no longer in solid?  or check?
				VectorCopy( push_list[x]->currentOrigin, push_list[x]->s.pos.trBase );
				push_list[x]->s.pos.trTime = level.time;								// move a bit on the very first frame
				if ( push_list[x]->s.pos.trType != TR_INTERPOLATE )
				{//don't do this to rolling missiles
					push_list[x]->s.pos.trType = TR_GRAVITY;
				}
				
				if ( push_list[x]->e_ThinkFunc == thinkF_G_RunObject && push_list[x]->physicsBounce )
				{//it's a pushable misc_model_breakable, use it's mass instead of our one-size-fits-all mass
					mass = push_list[x]->physicsBounce;//same as push_list[x]->mass, right?
				}
				if ( mass < 50 )
				{//???
					mass = 50;
				}
				if ( g_gravity->value > 0 )
				{
					VectorScale( pushDir, g_knockback->value * knockback / mass * 0.8, kvel );
					kvel[2] = pushDir[2] * g_knockback->value * knockback / mass * 1.5;
				}
				else
				{
					VectorScale( pushDir, g_knockback->value * knockback / mass, kvel );
				}
				VectorAdd( push_list[x]->s.pos.trDelta, kvel, push_list[x]->s.pos.trDelta );
				if ( g_gravity->value > 0 )
				{
					if ( push_list[x]->s.pos.trDelta[2] < knockback )
					{
						push_list[x]->s.pos.trDelta[2] = knockback;
					}
				}
				//no trDuration?
				if ( push_list[x]->e_ThinkFunc != thinkF_G_RunObject )
				{//objects spin themselves?
					//spin it
					//FIXME: messing with roll ruins the rotational center???
					push_list[x]->s.apos.trTime = level.time;
					push_list[x]->s.apos.trType = TR_LINEAR;
					VectorClear( push_list[x]->s.apos.trDelta );
					push_list[x]->s.apos.trDelta[1] = Q_irand( -800, 800 );
					if ( Q_stricmp( "limb", push_list[x]->classname ) == 0 )
					{//restore pitch spin like a freshly-spawned gib gets (g_combat.cpp) -- fixes gibs
					//freezing mid-air at whatever pitch they had at the moment of the push impulse.
					//Roll is deliberately left alone, per the rotational-center warning just above.
						push_list[x]->s.apos.trDelta[0] = Q_irand( -300, 300 );
					}
				}
				
				if ( Q_stricmp( "limb", push_list[x]->classname ) == 0 )
				{//make sure it runs it's physics
					push_list[x]->e_ThinkFunc = thinkF_LimbThink;
					push_list[x]->nextthink = level.time + FRAMETIME;
				}
				push_list[x]->forcePushTime = level.time + 600; // let the push effect last for 600 ms
				push_list[x]->forcePuller = self->s.number;//remember this regardless
				if ( push_list[x]->item && push_list[x]->item->giTag == INV_SECURITY_KEY )
				{
					AddSightEvent( player, push_list[x]->currentOrigin, 128, AEL_DISCOVERED );//security keys are more important
				}
				else
				{
					AddSightEvent( player, push_list[x]->currentOrigin, 128, AEL_SUSPICIOUS );//hmm... or should this always be discovered?
				}
			}
			else if ( push_list[x]->s.weapon == WP_TURRET
					 && !Q_stricmp( "PAS", push_list[x]->classname )
					 && push_list[x]->s.apos.trType == TR_STATIONARY )
			{//a portable turret
				WP_KnockdownTurret( self, push_list[x] );
			}
		}
	}

	G_RepulseLetGo( self, qtrue );//JKEnhanced: the shots it caught while charging go back out
	if ( cost > 0 )
	{
		WP_ForcePowerDrain( self, FP_REPULSE, cost );
	}

	if ( self->NPC )
	{//NPCs can push more often
		//FIXME: vary by rank and game skill?
		self->client->ps.forcePowerDebounce[FP_REPULSE] = level.time + 200;
	}
	else
	{
		self->client->ps.forcePowerDebounce[FP_REPULSE] = level.time + self->client->ps.torsoAnimTimer + 500;
	}
	if ( ent_count && maxVictimRecovery > 0 )
	{//don't let us get back to attacking well before whoever we just knocked down can defend themselves
		int scaledHold = (int)( maxVictimRecovery * g_forceStaggerLockoutScale->value );
		self->client->ps.weaponTime = Q_max( self->client->ps.weaponTime, scaledHold );
	}
}

/*
G_NPCRepulse (JKEnhanced)

An NPC casts Repulse at once with the given charge (ms; ForceRepulseThrow scales radius/strength by
it), skipping the player's hold-to-charge loop. Bosses get it at spawn,
any other NPC through its .npc (FP_REPULSE). Returns qtrue if it fired (ForceRepulseThrow re-arms
the debounce only when it does).
*/
qboolean G_NPCRepulse( gentity_t *npc, int chargeMs )
{
	if ( !npc || !npc->client || npc->health <= 0
		|| npc->client->ps.forcePowerLevel[FP_REPULSE] <= FORCE_LEVEL_0
		|| npc->client->ps.forcePowerDebounce[FP_REPULSE] > level.time
		|| PM_InKnockDown( &npc->client->ps ) )
	{
		return qfalse;
	}
	extern void WP_StopShieldsForAttack( gentity_t *self );
	WP_StopShieldsForAttack( npc );	//L1-2 drop; an L3 toggle stays up (4B.3)
	const int before = npc->client->ps.forcePowerDebounce[FP_REPULSE];
	ForceRepulseThrow( npc, chargeMs );
	const qboolean fired = (qboolean)( npc->client->ps.forcePowerDebounce[FP_REPULSE] != before );
	if ( fired )
	{//skips WP_ForcePowerStart, so log it here for g_debugForce / forcetally
		extern void WP_ForceDebugCast( const gentity_t *self, int power, int cost );
		WP_ForceDebugCast( npc, FP_REPULSE, forcePowerNeeded[FP_REPULSE] );
	}
	return fired;
}
