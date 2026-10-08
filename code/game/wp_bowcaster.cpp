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
//	Wookiee Bowcaster
//-------------------

//---------------------------------------------------------
static void WP_BowcasterMainFire( gentity_t *ent )
//---------------------------------------------------------
{
	int			damage	= weaponData[WP_BOWCASTER].damage;
	vec3_t		start;
	gentity_t	*missile;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	// same damage as the player, scaled by difficulty for NPCs
	damage = WP_ShooterDamage( ent, damage );

	WP_ApplyWeaponSpread( ent, WP_BOWCASTER, qfalse, 1.0f, forwardVec );
	WP_MissileTargetHint(ent, start, forwardVec);

	missile = CreateMissile( start, forwardVec, WP_ShooterVelocity( ent, weaponData[WP_BOWCASTER].velocity ), 10000, ent );

	missile->classname = "bowcaster_proj";
	missile->s.weapon = WP_BOWCASTER;

	VectorSet( missile->maxs, BOWCASTER_SIZE, BOWCASTER_SIZE, BOWCASTER_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BOWCASTER;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = weaponData[WP_BOWCASTER].splashDamage;
	missile->splashRadius = weaponData[WP_BOWCASTER].splashRadius;

	missile->bounceCount = 0;
}

//---------------------------------------------------------
static void WP_BowcasterAltFire( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t	start;
	int		count;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	// read charge level, same logic as the old main fire
	count = ( level.time - ent->client->ps.weaponChargeTime ) / BOWCASTER_CHARGE_UNIT;

	if ( count < 1 )
	{
		count = 1;
	}
	else if ( count > 5 )
	{
		count = 5;
	}

	if ( !(count & 1) )
	{
		count--;
	}

	// stronger base damage (weapons.dat altdamage, default 70) that scales with charge, +10 per step;
	// NPCs get the same, scaled by difficulty
	int damage = WP_ShooterDamage( ent, weaponData[WP_BOWCASTER].altDamage + (count - 1) * 10 );

	WP_ApplyWeaponSpread( ent, WP_BOWCASTER, qtrue, 1.0f, forwardVec );
	WP_MissileTargetHint(ent, start, forwardVec);

	gentity_t *missile = CreateMissile( start, forwardVec, WP_ShooterVelocity( ent, weaponData[WP_BOWCASTER].altVelocity ), 10000, ent, qtrue );

	missile->classname = "bowcaster_alt_proj";
	missile->s.weapon = WP_BOWCASTER;

	VectorSet( missile->maxs, BOWCASTER_SIZE, BOWCASTER_SIZE, BOWCASTER_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BOWCASTER_ALT;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = weaponData[WP_BOWCASTER].altSplashDamage;
	missile->splashRadius = weaponData[WP_BOWCASTER].altSplashRadius;
	missile->splashMethodOfDeath = MOD_BOWCASTER_ALT;

	missile->bounceCount = 0;

	ent->client->sess.missionStats.shotsFired++;
}

//---------------------------------------------------------
void WP_FireBowcaster( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	if ( alt_fire )
	{
		WP_BowcasterAltFire( ent );
	}
	else
	{
		WP_BowcasterMainFire( ent );
	}
}