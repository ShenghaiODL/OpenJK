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


//---------------
//	Blaster
//---------------

//---------------------------------------------------------
void WP_FireBlasterMissile( gentity_t *ent, vec3_t start, vec3_t dir, qboolean altFire )
//---------------------------------------------------------
{
	// E-11, E-5 and DC-15S share this function; each uses its own weapons.dat profile
	int		weapon	= ( ent->s.weapon == WP_E5_CARBINE || ent->s.weapon == WP_DC15S_CARBINE ) ? ent->s.weapon : WP_BLASTER;
	float	velocity = altFire ? weaponData[weapon].altVelocity : weaponData[weapon].velocity;
	int		damage	= altFire ? weaponData[weapon].altDamage : weaponData[weapon].damage;

	if ( ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE )
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// NPC bolts are slowed (g_npcBoltSpeed) so you have a chance to evade/deflect
		velocity = WP_ShooterVelocity( ent, velocity );
		damage = WP_ShooterDamage( ent, damage );
	}

	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t *missile = CreateMissile( start, dir, velocity, 10000, ent, altFire );

	missile->classname = "blaster_proj";
	missile->s.weapon = weapon;

//	if ( ent->client )
//	{
//		if ( ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//		{
//			// in overcharge mode, so doing double damage
//			missile->flags |= FL_OVERCHARGED;
//			damage *= 2;
//		}
//	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	if ( altFire )
	{
		missile->methodOfDeath = MOD_BLASTER_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BLASTER;
	}
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
void WP_FireBlaster( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	vec3_t	dir;
	int		weapon = ( ent->s.weapon == WP_E5_CARBINE || ent->s.weapon == WP_DC15S_CARBINE ) ? ent->s.weapon : WP_BLASTER;

	// weapon spread, plus skill-based spread for NPCs (vehicles and Force Sight 2+ are exempt)
	VectorCopy( forwardVec, dir );
	WP_ApplyWeaponSpread( ent, weapon, alt_fire, 1.0f, dir );

	// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
	WP_FireBlasterMissile( ent, muzzle, dir, alt_fire );
}