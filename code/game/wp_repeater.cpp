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

//-------------------
//	Heavy Repeater
//-------------------

//---------------------------------------------------------
static void WP_RepeaterMainFire( gentity_t *ent, vec3_t dir )
//---------------------------------------------------------
{
	vec3_t	start;
	int		damage	= weaponData[WP_REPEATER].damage;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t *missile = CreateMissile( start, dir, REPEATER_VELOCITY, 10000, ent );

	missile->classname = "repeater_proj";
	missile->s.weapon = WP_REPEATER;

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = REPEATER_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = REPEATER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = REPEATER_NPC_DAMAGE_HARD;
		}
	}
	
	if ( ent->s.weapon == WP_Z6_ROTARY )
	{
		missile->s.weapon = WP_Z6_ROTARY;
		damage = weaponData[WP_Z6_ROTARY].damage;
	}

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		missile->flags |= FL_OVERCHARGED;
//		damage *= 2;
//	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_REPEATER;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

#define REPEATER_ALT_CHARGE_UNIT 250.0f // ms held per extra distance step; longer hold throws the blob farther

//---------------------------------------------------------
static void WP_RepeaterAltFire( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t	start;
	int		damage	= weaponData[WP_REPEATER].altDamage;
	gentity_t *missile = NULL;
	float	chargeScale = 1.0f;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	if ( ent->client && ent->client->NPC_class == CLASS_GALAKMECH )
	{
		missile = CreateMissile( start, ent->client->hiddenDir, ent->client->hiddenDist, 10000, ent, qtrue );
	}
	else
	{
		WP_MissileTargetHint(ent, start, forwardVec);
		if ( ent->client )
		{
			int chargeCount = (int)( ( level.time - ent->client->ps.weaponChargeTime ) / REPEATER_ALT_CHARGE_UNIT );
			if ( chargeCount < 0 )
			{
				chargeCount = 0;
			}
			else if ( chargeCount > 5 )
			{
				chargeCount = 5;
			}
			chargeScale = 0.55f + chargeCount * 0.186f;//0.55x on an instant tap (was floored to 1.0x, i.e. no weak state at all), up to 1.48x at a full charge
		}
		missile = CreateMissile( start, forwardVec, (int)(REPEATER_ALT_VELOCITY * chargeScale), 10000, ent, qtrue );
	}

	missile->classname = "repeater_alt_proj";
	missile->s.weapon = WP_REPEATER;
	missile->mass = 10;

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = REPEATER_ALT_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = REPEATER_ALT_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = REPEATER_ALT_NPC_DAMAGE_HARD;
		}
	}

	VectorSet( missile->maxs, REPEATER_ALT_SIZE, REPEATER_ALT_SIZE, REPEATER_ALT_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );
	missile->s.pos.trType = TR_GRAVITY;
	missile->s.pos.trDelta[2] += 40.0f * chargeScale; //give a slight boost in the upward direction, more with a longer charge

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		missile->flags |= FL_OVERCHARGED;
//		damage *= 2;
//	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_REPEATER_ALT;
	missile->splashMethodOfDeath = MOD_REPEATER_ALT;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = weaponData[WP_REPEATER].altSplashDamage;
	missile->splashRadius = weaponData[WP_REPEATER].altSplashRadius;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
void WP_FireRepeater( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	vec3_t	dir, angs;

	vectoangles( forwardVec, angs );

	if ( alt_fire )
	{
		WP_RepeaterAltFire( ent );
	}
	else
	{
		if ( !(ent->client->ps.forcePowersActive&(1<<FP_SEE))
			|| ent->client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2 )
		{//force sight 2+ gives perfect aim
			//FIXME: maybe force sight level 3 autoaims some?
			// Troopers use their aim values as well as the gun's inherent inaccuracy
			// so check for all classes of stormtroopers and anyone else that has aim error
			if ( ent->client && ent->NPC &&
				( ent->client->NPC_class == CLASS_STORMTROOPER ||
				  ent->client->NPC_class == CLASS_SWAMPTROOPER ||
				  ent->client->NPC_class == CLASS_SHADOWTROOPER ) )
			{
				angs[PITCH] += ( Q_flrand(-1.0f, 1.0f) * (REPEATER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f) );
				angs[YAW]	+= ( Q_flrand(-1.0f, 1.0f) * (REPEATER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f) );
			}
			else
			{
				// add some slop to the alt-fire direction
				angs[PITCH] += Q_flrand(-1.0f, 1.0f) * REPEATER_SPREAD;
				angs[YAW]	+= Q_flrand(-1.0f, 1.0f) * REPEATER_SPREAD;
			}
		}

		AngleVectors( angs, dir, NULL, NULL );

		// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
		WP_RepeaterMainFire( ent, dir );
	}
}

//---------------------------------------------------------
static void WP_Z6MainFire( gentity_t *ent, vec3_t dir )
//---------------------------------------------------------
{
	vec3_t	start;
	int		damage	= weaponData[WP_Z6_ROTARY].damage;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t *missile = CreateMissile( start, dir, REPEATER_VELOCITY, 10000, ent );

	missile->classname = "z6_proj";
	missile->s.weapon = WP_Z6_ROTARY;

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = REPEATER_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = REPEATER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = REPEATER_NPC_DAMAGE_HARD;
		}
	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_REPEATER;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
// Z-6 identity: a real heat/spread mechanic (instead of the old FP_LEVITATION drain hack), its own
// projectile visuals, and Force Push interruption -- meant to suppress rather than just be a bigger
// repeater. AI-controlled burst-fire pacing already exists (NPC_combat.cpp's WP_Z6_ROTARY case).
//---------------------------------------------------------
void WP_FireZ6Rotary( gentity_t *ent )
//---------------------------------------------------------
{
	if ( ent->forcePushTime > level.time )
	{//staggered -- can't keep the barrel spinning up straight
		ent->client->ps.z6Heat = 0;
		return;
	}

	vec3_t	dir, angs;

	vectoangles( forwardVec, angs );

	// The hotter the barrel, the worse the spread -- cubic curve so low/moderate heat barely matters
	// (e.g. half-heated is still fairly tight) but the last stretch to a fully red-hot barrel falls
	// off a cliff (up to 6x baseline spread), so sustained fire trades accuracy for suppression
	// instead of just being a bigger repeater.
	float heatFrac = ent->client->ps.z6Heat * 0.01f;
	float spreadScale = 1.0f + ( heatFrac * heatFrac * heatFrac ) * 5.0f;

	if ( !(ent->client->ps.forcePowersActive&(1<<FP_SEE))
		|| ent->client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2 )
	{
		if ( ent->client && ent->NPC &&
			( ent->client->NPC_class == CLASS_STORMTROOPER ||
			  ent->client->NPC_class == CLASS_SWAMPTROOPER ||
			  ent->client->NPC_class == CLASS_SHADOWTROOPER ) )
		{
			float npcSpread = (REPEATER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f) * spreadScale;
			angs[PITCH] += ( Q_flrand(-1.0f, 1.0f) * npcSpread );
			angs[YAW]	+= ( Q_flrand(-1.0f, 1.0f) * npcSpread );
		}
		else
		{
			angs[PITCH] += Q_flrand(-1.0f, 1.0f) * REPEATER_SPREAD * spreadScale;
			angs[YAW]	+= Q_flrand(-1.0f, 1.0f) * REPEATER_SPREAD * spreadScale;
		}
	}

	AngleVectors( angs, dir, NULL, NULL );

	WP_Z6MainFire( ent, dir );

	ent->client->ps.z6Heat += 3;
	if ( ent->client->ps.z6Heat > 100 )
	{
		ent->client->ps.z6Heat = 100;
	}
	ent->client->ps.z6HeatDecayTime = level.time + 600;//brief hold before cooldown starts once fire stops

	if ( ent->client->ps.z6Heat > 60 )
	{//barrel glowing -- refresh the effect each shot rather than a persistent loop, simplest way to
	//guarantee it turns off promptly once heat drops back down
		G_PlayEffect( G_EffectIndex( "z6/barrel_glow" ), muzzle, forwardVec );
	}
}