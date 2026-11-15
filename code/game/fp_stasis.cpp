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

/*
JKEnhanced 4B.7: Stasis is a forward cone (force.cfg stasis.range / cone / duration by level).
- Anyone without a saber in it is frozen (the stock freeze: ps.stasisTime, frozen anim, no AI).
- Saber users, Reborn and bosses are slowed instead (move and animate at stasis.slowPct /
  bossSlowPct) - they used to be immune.
- Shots in it hang in the air and carry on when it wears off; thrown sabers in it drop.
Absorb still takes it (per target). The slows and held shots aren't saved: they end on a load.
*/

#include "g_local.h"
#include "anims.h"
#include "b_local.h"
#include "bg_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "g_force.h"
#include "../cgame/cg_local.h"

extern qboolean WP_ForcePowerUsable( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
extern void		WP_ForcePowerStart( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
extern int		WP_AbsorbConversion( gentity_t *attacked, int atdAbsLevel, gentity_t *attacker, int atPower, int atPowerLevel, int atForceSpent );
extern void		WP_SaberDrop( gentity_t *self, gentity_t *saber );
extern qboolean	PM_InKnockDown( playerState_t *ps );
extern qboolean	G_IsSaberBoss( const gentity_t *ent, qboolean includeElites );

extern cvar_t	*g_timescale;
extern cvar_t	*g_debugForce;
extern int		forcePowerNeeded[NUM_FORCE_POWERS];

#define STASIS_MAX_MS	20000	// anything set further out than this is stale (from before a load)

static const char *StasisWho( const gentity_t *ent )
{
	return ( ent->s.number < MAX_CLIENTS ) ? "player" : ( ent->NPC_type ? ent->NPC_type : ent->classname );
}

//---------------------------------------------------------------------------------------------------
// slowed saber users
//---------------------------------------------------------------------------------------------------
static struct
{
	int		until;
	int		pct;
} s_slow[MAX_GENTITIES];

float G_StasisSlowScale( const gentity_t *ent )
{
	if ( !ent || ent->s.number < 0 || ent->s.number >= MAX_GENTITIES || ent->health <= 0 )
	{
		return 1.0f;
	}
	const int until = s_slow[ent->s.number].until;
	if ( until <= level.time || until > level.time + STASIS_MAX_MS )
	{
		return 1.0f;
	}
	return s_slow[ent->s.number].pct / 100.0f;
}

static void G_StasisSlow( gentity_t *victim, int ms )
{
	const forceTuning_t *ft = G_ForceTuning();
	s_slow[victim->s.number].until = level.time + ms;
	s_slow[victim->s.number].pct = G_IsSaberBoss( victim, qfalse ) ? ft->stasisBossSlowPct : ft->stasisSlowPct;
}

//---------------------------------------------------------------------------------------------------
// frozen (the stock Stasis freeze)
//---------------------------------------------------------------------------------------------------
static void G_StasisFreeze( gentity_t *victim, int ms )
{
	float	currentFrame, animSpeed;
	int		junk;

	victim->client->ps.stasisTime = level.time + ms;
	VectorClear( victim->client->ps.velocity );
	if ( gi.G2API_HaveWeGhoul2Models( victim->ghoul2 ) )
	{
		gi.G2API_GetBoneAnimIndex( &victim->ghoul2[victim->playerModel], victim->rootBone,
								  level.time, &currentFrame, &junk, &junk, &junk, &animSpeed, NULL );
		gi.G2API_SetBoneAnimIndex( &victim->ghoul2[victim->playerModel], victim->rootBone,
								  currentFrame, currentFrame+1,
								  BONE_ANIM_OVERRIDE_FREEZE, animSpeed, level.time, currentFrame, 100 );
		if ( victim->headModel > 0 )
		{
			gi.G2API_SetBoneAnimIndex( &victim->ghoul2[victim->headModel], victim->headRootBone,
									  currentFrame, currentFrame+1,
									  BONE_ANIM_OVERRIDE_FREEZE, animSpeed, level.time, currentFrame, 100 );
		}
	}
}

//---------------------------------------------------------------------------------------------------
// shots held in the air
//---------------------------------------------------------------------------------------------------
static struct
{
	int		until;		// 0 = not held
	int		stamp;		// s.pos.trTime when held (a reused slot won't match)
	int		trType;
	int		thinkLeft;
} s_heldShot[MAX_GENTITIES];

static void G_StasisHoldShot( gentity_t *m, int ms )
{
	const int n = m->s.number;
	s_heldShot[n].until = level.time + ms;
	s_heldShot[n].trType = m->s.pos.trType;
	s_heldShot[n].thinkLeft = m->nextthink ? Q_max( FRAMETIME, m->nextthink - level.time ) : 0;
	VectorCopy( m->currentOrigin, m->s.pos.trBase );
	m->s.pos.trType = TR_STATIONARY;	//(trDelta kept for when it goes on)
	m->s.pos.trTime = level.time;
	s_heldShot[n].stamp = m->s.pos.trTime;
}

//G_RunMissile: qtrue = still held, skip its move this frame (its timeout stays paused); when the
//Stasis wears off it carries on the way it was going
qboolean G_StasisMissileHeld( gentity_t *m )
{
	const int n = m->s.number;
	if ( !s_heldShot[n].until )
	{
		return qfalse;
	}
	if ( m->s.pos.trType != TR_STATIONARY || m->s.pos.trTime != s_heldShot[n].stamp
		|| s_heldShot[n].until > level.time + STASIS_MAX_MS )
	{//something else moved it (a Push, Repulse), the slot was reused, or it's stale: let it be
		s_heldShot[n].until = 0;
		return qfalse;
	}
	if ( level.time < s_heldShot[n].until )
	{
		if ( m->nextthink )
		{
			m->nextthink = level.time + s_heldShot[n].thinkLeft;
		}
		return qtrue;
	}
	m->s.pos.trType = (trType_t)s_heldShot[n].trType;
	VectorCopy( m->currentOrigin, m->s.pos.trBase );
	m->s.pos.trTime = level.time;
	if ( m->nextthink )
	{
		m->nextthink = level.time + s_heldShot[n].thinkLeft;
	}
	s_heldShot[n].until = 0;
	return qfalse;
}

//---------------------------------------------------------------------------------------------------
// the cast
//---------------------------------------------------------------------------------------------------
void ForceStasis( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}
	if ( !WP_ForcePowerUsable( self, FP_STASIS, 0 ) )
	{
		return;
	}
	if ( self->client->ps.forcePowerDebounce[FP_STASIS] > level.time )
	{
		return;
	}
	if ( !self->s.number && (cg.zoomMode || in_camera) )
	{//can't when zoomed in or in a cinematic
		return;
	}
	if ( self->client->ps.saberLockTime > level.time )
	{
		return;
	}

	const forceTuning_t	*ft = G_ForceTuning();
	const int			casterLevel = Q_max( FORCE_LEVEL_1, Q_min( FORCE_LEVEL_3, self->client->ps.forcePowerLevel[FP_STASIS] ) );
	const float			range = (float)ft->stasisRange[casterLevel - 1];
	const float			minDot = cos( DEG2RAD( (float)ft->stasisCone[casterLevel - 1] ) );
	vec3_t				forward, mins, maxs, eye;
	gentity_t			*list[MAX_GENTITIES];
	int					frozen = 0, slowed = 0, shots = 0;

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorCopy( self->client->renderInfo.eyePoint, eye );
	for ( int i = 0; i < 3; i++ )
	{
		mins[i] = eye[i] - range;
		maxs[i] = eye[i] + range;
	}
	const int num = gi.EntitiesInBox( mins, maxs, list, MAX_GENTITIES );
	for ( int e = 0; e < num; e++ )
	{
		gentity_t *ent = list[e];
		if ( !ent || ent == self || !ent->inuse )
		{
			continue;
		}
		vec3_t center, dir;
		VectorAdd( ent->absmin, ent->absmax, center );
		VectorScale( center, 0.5f, center );
		VectorSubtract( center, eye, dir );
		if ( VectorNormalize( dir ) > range || DotProduct( dir, forward ) < minDot )
		{
			continue;
		}
		trace_t los;
		gi.trace( &los, eye, vec3_origin, vec3_origin, center, self->s.number, MASK_OPAQUE, (EG2_Collision)0, 0 );
		if ( los.fraction < 1.0f && los.entityNum != ent->s.number )
		{
			continue;
		}

		if ( ent->client )
		{
			if ( ent->health <= 0
				|| ( ent->NPC && (ent->NPC->scriptFlags&SCF_NO_FORCE) )
				|| ent->client->playerTeam == self->client->playerTeam )
			{
				continue;
			}
			int lvl = WP_AbsorbConversion( ent, ent->client->ps.forcePowerLevel[FP_ABSORB], self, FP_STASIS, casterLevel, forcePowerNeeded[FP_STASIS] );
			if ( lvl == -1 )
			{
				lvl = casterLevel;
			}
			if ( lvl <= 0 )
			{//absorbed
				if ( g_debugForce && g_debugForce->integer )
				{
					gi.Printf( "  STASIS %s -> %s: absorbed\n", StasisWho( self ), StasisWho( ent ) );
				}
				continue;
			}
			lvl = Q_min( FORCE_LEVEL_3, lvl );
			const int ms = ft->stasisDuration[lvl - 1];
			const qboolean slow = (qboolean)( ent->s.weapon == WP_SABER || ent->client->NPC_class == CLASS_REBORN
				|| G_IsSaberBoss( ent, qfalse ) );
			if ( slow )
			{
				G_StasisSlow( ent, ms );
				slowed++;
			}
			else
			{
				G_StasisFreeze( ent, ms );
				frozen++;
			}
			if ( ent->NPC )
			{
				AI_NerveEvent( ent, NE_FORCE_ON_ME, 1.0f, self );
			}
			if ( g_debugForce && g_debugForce->integer )
			{
				gi.Printf( "  STASIS %s -> %s: %s for %.1f s\n", StasisWho( self ), StasisWho( ent ),
					slow ? va( "slowed to %d%%", (int)( G_StasisSlowScale( ent ) * 100.0f + 0.5f ) ) : "frozen", ms / 1000.0f );
			}
		}
		else if ( ent->s.eType == ET_MISSILE && ent->owner != self && ent->s.pos.trType != TR_STATIONARY )
		{
			if ( ent->s.weapon == WP_SABER || (ent->contents&CONTENTS_LIGHTSABER) )
			{//a thrown saber: it drops out of the air
				if ( ent->owner && ent->owner->client && ent->owner->client->ps.saberInFlight )
				{
					WP_SaberDrop( ent->owner, ent );
					shots++;
				}
			}
			else if ( !( ent->s.powerups & ( 1 << PW_FORCE_PROJECTILE ) ) )
			{
				G_StasisHoldShot( ent, ft->stasisDuration[casterLevel - 1] );
				shots++;
			}
		}
	}

	if ( g_debugForce && g_debugForce->integer )
	{
		gi.Printf( "  STASIS %s L%d (%.0f units, %d deg): %d frozen, %d slowed, %d shots held\n", StasisWho( self ),
			casterLevel, range, ft->stasisCone[casterLevel - 1], frozen, slowed, shots );
	}

	int parts = SETANIM_TORSO;
	if ( !PM_InKnockDown( &self->client->ps ) )
	{
		if ( !VectorLengthSquared( self->client->ps.velocity ) && !(self->client->ps.pm_flags&PMF_DUCKED) )
		{
			parts = SETANIM_BOTH;
		}
	}
	NPC_SetAnim( self, parts, BOTH_FORCEPUSH, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART );
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;
	G_Sound( self, G_SoundIndex( "sound/weapons/force/protecthit.wav" ) );
	if ( self->client->renderInfo.handRPoint[0] || self->client->renderInfo.handRPoint[1] || self->client->renderInfo.handRPoint[2] )
	{//a ripple out the way it went
		G_PlayEffect( G_EffectIndex( "force/pushblur" ), self->client->renderInfo.handRPoint, forward );
	}

	WP_ForcePowerStart( self, FP_STASIS, 0 );

	self->client->ps.weaponTime = 1000;
	if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
	{
		self->client->ps.weaponTime = floor( self->client->ps.weaponTime * g_timescale->value );
	}
	self->client->ps.forcePowerDebounce[FP_STASIS] = level.time + self->client->ps.torsoAnimTimer + 500;
}
