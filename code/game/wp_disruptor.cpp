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
#include "b_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "w_local.h"

//---------------------
//	Tenloss Disruptor
//---------------------

//---------------------------------------------------------
// Reworked into a real travel-time bolt (like a blaster, just harder-hitting and slower) instead of
// an instant hitscan trace -- damage/effects are now resolved by the generic missile-impact pipeline
// in g_missile.cpp (G_MissileImpacted), same as any other projectile weapon.
//---------------------------------------------------------
static void WP_DisruptorMainFire( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t	start;
	int		damage = weaponData[WP_DISRUPTOR].damage;

	if ( ent->NPC )
	{
		switch ( g_spskill->integer )
		{
		case 0:
			damage = DISRUPTOR_NPC_MAIN_DAMAGE_EASY;
			break;
		case 1:
			damage = DISRUPTOR_NPC_MAIN_DAMAGE_MEDIUM;
			break;
		case 2:
		default:
			damage = DISRUPTOR_NPC_MAIN_DAMAGE_HARD;
			break;
		}
	}

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );

	WP_MissileTargetHint(ent, start, forwardVec);

	gentity_t *missile = CreateMissile( start, forwardVec, DISRUPTOR_VELOCITY, 10000, ent );

	missile->classname = "disruptor_proj";
	missile->s.weapon = WP_DISRUPTOR;
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_DISRUPTOR;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
}

//---------------------------------------------------------
// Charged shot: also a real travel-time bolt now, damage scales with how long the trigger was held
// (same charge/count math as before), but no longer routes a full-charge kill through the
// disintegration finisher (MOD_SNIPER) -- just a bigger, harder-hitting version of the same bolt.
// The old multi-target penetration-through-N-enemies mechanic is dropped as part of this -- it
// doesn't map cleanly onto a real projectile, so a fully-charged shot is now one very hard-hitting
// bolt rather than a beam punching through several targets.
//---------------------------------------------------------
void WP_DisruptorAltFire( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t	start, dir;
	int		damage = weaponData[WP_DISRUPTOR].altDamage;

	if ( ent->NPC )
	{
		switch ( g_spskill->integer )
		{
		case 0:
			damage = DISRUPTOR_NPC_ALT_DAMAGE_EASY;
			break;
		case 1:
			damage = DISRUPTOR_NPC_ALT_DAMAGE_MEDIUM;
			break;
		case 2:
		default:
			damage = DISRUPTOR_NPC_ALT_DAMAGE_HARD;
			break;
		}
		VectorCopy( muzzle, start );
		VectorCopy( forwardVec, dir );
	}
	else
	{
		VectorCopy( ent->client->renderInfo.eyePoint, start );
		AngleVectors( ent->client->renderInfo.eyeAngles, dir, NULL, NULL );

		int count = ( level.time - ent->client->ps.weaponChargeTime - 50 ) / DISRUPTOR_CHARGE_UNIT;

		if ( count < 1 )
		{
			count = 1;
		}
		else if ( count > 10 )
		{
			count = 10;
		}

		damage = damage * count + weaponData[WP_DISRUPTOR].damage * 0.5f; // give a boost to low charge shots
	}

	gentity_t *missile = CreateMissile( start, dir, DISRUPTOR_VELOCITY, 10000, ent, qtrue );

	missile->classname = "disruptor_alt_proj";
	missile->s.weapon = WP_DISRUPTOR;
	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_DISRUPTOR;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
}

//---------------------------------------------------------
void WP_FireDisruptor( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	if ( alt_fire )
	{
		WP_DisruptorAltFire( ent );
	}
	else
	{
		WP_DisruptorMainFire( ent );
	}

	G_PlayEffect( G_EffectIndex( "disruptor/line_cap" ), muzzle, forwardVec );
}