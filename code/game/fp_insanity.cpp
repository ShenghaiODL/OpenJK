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
JKEnhanced 4B.8: Insanity is Terror, a forward cone (force.cfg insanity.range / cone by level).
Everyone in it takes a nerve shock (insanity.nerve, AI_Nerve's NE_TERROR); what's left decides:
- broken (nerve < 15): they give up on the spot - drop the gun, then flee or surrender (the
  non-lethal system, the same as a nerve break);
- breaking (< 30): they cower (the stock insanity: clutching their head, enemy dropped);
- steadier: they only flinch (insanity.waver), keeping their enemy.
Saber users and Reborn can't be broken: they flinch for half as long. Bosses only shrug.
Allies who see someone hit lose a little nerve too.
L3 adds hallucinations: whoever isn't taken out aims at a phantom beside you for
insanity.hallucinate ms (insanity.hallucinateYaw degrees off) - their shots can find their allies.
Replaces the old single-target confusion and the L3 charm (that was Mind Trick L3's code).
The phantoms aren't saved: they end on a load. The cowering is (NPC->insanityTime).
*/

#include "g_local.h"
#include "anims.h"
#include "b_local.h"
#include "bg_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "g_force.h"
#include "NPC_neutral.h"
#include "../cgame/cg_local.h"

extern qboolean WP_ForcePowerUsable( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
extern void		WP_ForcePowerStart( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
extern int		WP_AbsorbConversion( gentity_t *attacked, int atdAbsLevel, gentity_t *attacker, int atPower, int atPowerLevel, int atForceSpent );
extern qboolean	PM_InKnockDown( playerState_t *ps );
extern qboolean	PM_HasAnimation( gentity_t *ent, int animation );
extern qboolean	G_IsSaberBoss( const gentity_t *ent, qboolean includeElites );
extern void		NPC_Jedi_PlayConfusionSound( gentity_t *self );

extern cvar_t	*g_timescale;
extern cvar_t	*g_debugForce;
extern int		forcePowerNeeded[NUM_FORCE_POWERS];

#define TERROR_MAX_TARGETS	32
#define TERROR_MAX_MS		20000	// a phantom set further out than this is stale (from before a load)

static const char *TerrorWho( const gentity_t *ent )
{
	return ( ent->s.number < MAX_CLIENTS ) ? "player" : ( ent->NPC_type ? ent->NPC_type : ent->classname );
}

//---------------------------------------------------------------------------------------------------
// L3 phantoms
//---------------------------------------------------------------------------------------------------
static struct
{
	int		until;
	float	yaw;
} s_phantom[MAX_GENTITIES];

//NPC_UpdateAngles: they think you're over there
float G_InsanityAimYaw( const gentity_t *npc )
{
	if ( !npc || npc->s.number < 0 || npc->s.number >= MAX_GENTITIES || npc->health <= 0 || !npc->enemy )
	{
		return 0.0f;
	}
	const int until = s_phantom[npc->s.number].until;
	if ( until <= level.time || until > level.time + TERROR_MAX_MS || G_IsNeutralized( npc ) )
	{
		return 0.0f;
	}
	return s_phantom[npc->s.number].yaw;
}

//---------------------------------------------------------------------------------------------------
// what it does to one of them
//---------------------------------------------------------------------------------------------------
static void Terror_Cower( gentity_t *victim, int ms, qboolean dropEnemy )
{
	if ( ms <= 0 )
	{
		return;
	}
	victim->NPC->insanityTime = Q_max( victim->NPC->insanityTime, level.time + ms );
	if ( PM_HasAnimation( victim, BOTH_SONICPAIN_HOLD ) )
	{//G_CheckInsanity keeps them in it for the rest
		NPC_SetAnim( victim, SETANIM_LEGS, BOTH_SONICPAIN_HOLD, SETANIM_FLAG_NORMAL );
		NPC_SetAnim( victim, SETANIM_TORSO, BOTH_SONICPAIN_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		victim->client->ps.torsoAnimTimer = Q_max( victim->client->ps.torsoAnimTimer, ms );
		victim->client->ps.weaponTime = victim->client->ps.torsoAnimTimer;
	}
	if ( dropEnemy && victim->enemy )
	{
		G_ClearEnemy( victim );
	}
	if ( victim->ghoul2.size() && victim->headBolt != -1 )
	{
		G_PlayEffect( G_EffectIndex( "force/drain_hand" ), victim->playerModel, victim->headBolt, victim->s.number, victim->currentOrigin, ms, qtrue );
	}
}

static qboolean Terror_CanAffect( const gentity_t *ent )
{
	switch ( ent->client->NPC_class )
	{
	case CLASS_GALAKMECH://in armor
	case CLASS_ATST:
	case CLASS_PROBE:
	case CLASS_GONK:
	case CLASS_R2D2:
	case CLASS_R5D2:
	case CLASS_MARK1:
	case CLASS_MARK2:
	case CLASS_MOUSE:
	case CLASS_SEEKER:
	case CLASS_REMOTE:
	case CLASS_PROTOCOL:
	case CLASS_ASSASSIN_DROID:
	case CLASS_SABER_DROID:
	case CLASS_BOBAFETT:
		return qfalse;
	case CLASS_RANCOR:
		return (qboolean)!(ent->spawnflags&1);
	default:
		return qtrue;
	}
}

//---------------------------------------------------------------------------------------------------
// the cast
//---------------------------------------------------------------------------------------------------
void ForceInsanity( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}
	if ( !WP_ForcePowerUsable( self, FP_INSANITY, 0 ) )
	{
		return;
	}
	if ( self->client->ps.weaponTime >= 800 )
	{//just did one!
		return;
	}
	if ( self->client->ps.forcePowerDebounce[FP_INSANITY] > level.time )
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
	const int			casterLevel = Q_max( FORCE_LEVEL_1, Q_min( FORCE_LEVEL_3, self->client->ps.forcePowerLevel[FP_INSANITY] ) );
	const float			range = (float)ft->insanityRange[casterLevel - 1];
	const float			minDot = cos( DEG2RAD( (float)ft->insanityCone[casterLevel - 1] ) );
	vec3_t				forward, mins, maxs, eye;
	gentity_t			*list[MAX_GENTITIES];
	gentity_t			*hit[TERROR_MAX_TARGETS];
	int					numHit = 0, broken = 0, cowered = 0, wavered = 0, shrugged = 0, phantoms = 0;

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorCopy( self->client->renderInfo.eyePoint, eye );
	for ( int i = 0; i < 3; i++ )
	{
		mins[i] = eye[i] - range;
		maxs[i] = eye[i] + range;
	}
	//who's in it
	gentity_t	*cand[TERROR_MAX_TARGETS];
	int			numCand = 0;
	const int num = gi.EntitiesInBox( mins, maxs, list, MAX_GENTITIES );
	for ( int e = 0; e < num && numCand < TERROR_MAX_TARGETS; e++ )
	{
		gentity_t *ent = list[e];
		if ( !ent || ent == self || !ent->inuse || !ent->client || !ent->NPC || ent->health <= 0 )
		{
			continue;
		}
		if ( (ent->NPC->scriptFlags&SCF_NO_FORCE)
			|| ent->client->playerTeam == self->client->playerTeam
			|| G_IsNeutralized( ent )
			|| !Terror_CanAffect( ent ) )
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
		cand[numCand++] = ent;
	}
	for ( int a = 1; a < numCand; a++ )
	{//nearest first, so a short pool reaches the closest
		gentity_t	*ent = cand[a];
		const float	d = DistanceSquared( ent->currentOrigin, eye );
		int			b = a - 1;
		while ( b >= 0 && DistanceSquared( cand[b]->currentOrigin, eye ) > d )
		{
			cand[b + 1] = cand[b];
			b--;
		}
		cand[b + 1] = ent;
	}

	//the first costs the base, each one after that insanity.extraCost more (up to costCap); short of
	//Force, it reaches only as many as you can pay for
	const int	baseCost = forcePowerNeeded[FP_INSANITY];
	int			cost = baseCost;
	if ( numCand > 1 )
	{
		cost = Q_min( Q_max( baseCost, ft->insanityCostCap ), baseCost + ft->insanityExtraCost * ( numCand - 1 ) );
		if ( ft->insanityExtraCost > 0 && cost > self->client->ps.forcePower )
		{
			const int affordable = 1 + Q_max( 0, ( self->client->ps.forcePower - baseCost ) / ft->insanityExtraCost );
			numCand = Q_max( 1, Q_min( numCand, affordable ) );
			cost = Q_min( cost, baseCost + ft->insanityExtraCost * ( numCand - 1 ) );
		}
	}

	for ( int c = 0; c < numCand; c++ )
	{
		gentity_t *ent = cand[c];
		int lvl = WP_AbsorbConversion( ent, ent->client->ps.forcePowerLevel[FP_ABSORB], self, FP_INSANITY, casterLevel, forcePowerNeeded[FP_INSANITY] );
		if ( lvl == -1 )
		{
			lvl = casterLevel;
		}
		if ( lvl <= 0 )
		{//absorbed
			if ( g_debugForce && g_debugForce->integer )
			{
				gi.Printf( "  TERROR %s -> %s: absorbed\n", TerrorWho( self ), TerrorWho( ent ) );
			}
			continue;
		}
		lvl = Q_min( FORCE_LEVEL_3, lvl );

		if ( (ent->NPC->scriptFlags&SCF_NO_MIND_TRICK) || G_IsSaberBoss( ent, qfalse ) )
		{//they don't scare
			NPC_Jedi_PlayConfusionSound( ent );
			shrugged++;
			if ( g_debugForce && g_debugForce->integer )
			{
				gi.Printf( "  TERROR %s -> %s: shrugged it off\n", TerrorWho( self ), TerrorWho( ent ) );
			}
			continue;
		}

		const int before = AI_NerveEffective( ent );
		AI_NerveEvent( ent, NE_TERROR, (float)ft->insanityNerve[lvl - 1], self );
		const int after = AI_NerveEffective( ent );
		const qboolean saberUser = (qboolean)( ent->s.weapon == WP_SABER || ent->client->NPC_class == CLASS_REBORN );
		const char *result;

		if ( saberUser )
		{//Jedi aren't easily scared: a flinch
			Terror_Cower( ent, ft->insanityWaver[lvl - 1] / 2, qfalse );
			wavered++;
			result = "flinched";
		}
		else if ( after < 15 && G_CanNeutralize( ent, NH_SURRENDER ) )
		{//nerve gone: drop the gun, run or put their hands up
			ent->NPC->nerveTier = NT_BROKEN;
			TIMER_Set( ent, "nerveBrokenHold", 4000 );//as AI_NerveUpdate does on entering BROKEN (NERVE_BROKEN_HOLD_MS)
			G_Neutralize( ent, self, NH_DISARM );
			broken++;
			result = "broke";
		}
		else if ( after < 30 )
		{
			Terror_Cower( ent, ft->insanityCower[lvl - 1], qtrue );
			cowered++;
			result = "cowering";
		}
		else
		{
			Terror_Cower( ent, ft->insanityWaver[lvl - 1], qfalse );
			wavered++;
			result = "flinched";
		}

		if ( casterLevel >= FORCE_LEVEL_3 && lvl >= FORCE_LEVEL_3 && !saberUser && !G_IsNeutralized( ent ) && ft->insanityHallucinate > 0 )
		{//they'll see you where you aren't
			const float yawMax = (float)ft->insanityHallucinateYaw;
			s_phantom[ent->s.number].until = level.time + ft->insanityHallucinate;
			s_phantom[ent->s.number].yaw = Q_flrand( yawMax * 0.5f, yawMax ) * ( Q_irand( 0, 1 ) ? 1.0f : -1.0f );
			phantoms++;
		}

		hit[numHit++] = ent;
		if ( g_debugForce && g_debugForce->integer )
		{
			gi.Printf( "  TERROR %s -> %s: nerve %d -> %d, %s%s\n", TerrorWho( self ), TerrorWho( ent ), before, after, result,
				G_InsanityAimYaw( ent ) != 0.0f ? va( ", phantom %+.0f deg", G_InsanityAimYaw( ent ) ) : "" );
		}
	}

	//allies who saw it - after everyone's own hit, so a witness delta can't use up a target's debounce first
	for ( int h = 0; h < numHit; h++ )
	{
		AI_NerveWitness( hit[h], NE_TERROR, self );
	}

	if ( g_debugForce && g_debugForce->integer )
	{
		gi.Printf( "  TERROR %s L%d (%.0f units, %d deg): %d broke, %d cowering, %d flinched, %d shrugged, %d phantoms; cost %d, %.1f s cooldown\n", TerrorWho( self ),
			casterLevel, range, ft->insanityCone[casterLevel - 1], broken, cowered, wavered, shrugged, phantoms, cost, ft->insanityCooldown / 1000.0f );
	}

	NPC_SetAnim( self, SETANIM_TORSO, BOTH_MINDTRICK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_RESTART|SETANIM_FLAG_HOLD );
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;
	if ( self->client->renderInfo.handLPoint[0] || self->client->renderInfo.handLPoint[1] || self->client->renderInfo.handLPoint[2] )
	{
		G_PlayEffect( G_EffectIndex( "force/drain_hand" ), self->client->renderInfo.handLPoint, forward );
	}

	WP_ForcePowerStart( self, FP_INSANITY, cost );

	self->client->ps.weaponTime = 1000;
	if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
	{
		self->client->ps.weaponTime = floor( self->client->ps.weaponTime * g_timescale->value );
	}
	self->client->ps.forcePowerDebounce[FP_INSANITY] = level.time + Q_max( self->client->ps.torsoAnimTimer + 500, ft->insanityCooldown );
}
