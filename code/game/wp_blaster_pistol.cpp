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
#include "wp_saber.h"
#include "w_local.h"


//---------------
//	Bryar Pistol
//---------------

//---------------------------------------------------------
void WP_FireBryarPistol( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	vec3_t	start;
	// Bryar, DL-44 (WP_BLASTER_PISTOL), Jawa and DC-15A share this function; each now uses its
	// own weapons.dat profile (the DL-44 used to read the Bryar's damage).
	int		weapon = ( ent->s.weapon == WP_DC15A_RIFLE || ent->s.weapon == WP_BLASTER_PISTOL || ent->s.weapon == WP_JAWA )
					? ent->s.weapon : WP_BRYAR_PISTOL;
	int		damage = !alt_fire ? weaponData[weapon].damage : weaponData[weapon].altDamage;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	WP_ApplyWeaponSpread( ent, weapon, alt_fire, 1.0f, forwardVec );

	WP_MissileTargetHint(ent, start, forwardVec);

	float	velocity = WP_ShooterVelocity( ent, alt_fire ? weaponData[weapon].altVelocity : weaponData[weapon].velocity );
	gentity_t	*missile = CreateMissile( start, forwardVec, velocity, 10000, ent, alt_fire );

	missile->classname = "bryar_proj";
	missile->s.weapon = weapon;


	if ( alt_fire )
	{
		int count = ( level.time - ent->client->ps.weaponChargeTime ) / BRYAR_CHARGE_UNIT;

		if ( count < 1 )
		{
			count = 1;
		}
		else if ( count > 5 )
		{
			count = 5;
		}

		damage *= count;
		missile->count = count; // this will get used in the projectile rendering code to make a beefier effect
	}

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		missile->flags |= FL_OVERCHARGED;
//		damage *= 2;
//	}

	missile->damage = WP_ShooterDamage( ent, damage );
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;

	if ( alt_fire )
	{
		missile->methodOfDeath = MOD_BRYAR_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BRYAR;
	}

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	if ( ent->weaponModel[1] > 0 )
	{//dual pistols, toggle the muzzle point back and forth between the two pistols each time he fires
		ent->count = (ent->count)?0:1;
	}
}