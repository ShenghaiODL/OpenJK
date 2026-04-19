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

////////////////////////////////////////////////////////////////////////////////////////
// RAVEN SOFTWARE - STAR WARS: JK II
//  (c) 2002 Activision
//
// Mando made from Boba
// ---------
// Ah yes, this file is pretty messy.  I've tried to move everything in here, but in fact
// a lot of his AI occurs in the seeker and jedi AI files.  Some of these functions
//
//
//
////////////////////////////////////////////////////////////////////////////////////////
#include "b_local.h"
#include "../Ravl/CVec.h"
#include "../cgame/cg_local.h"

extern void     Boba_Printf( const char *format, ... );
extern void     Boba_Precache( void );
extern void     Boba_ChangeWeapon( int wp );
extern void     Boba_DustFallNear( const vec3_t origin, int dustcount );
extern qboolean Boba_StopKnockdown( gentity_t*, gentity_t*, const vec3_t, qboolean );
extern void     Boba_FireFlameThrower( gentity_t *self );
extern void     Boba_StopFlameThrower( gentity_t *self );
extern void     Boba_StartFlameThrower( gentity_t *self );
extern void     Boba_DoFlameThrower( gentity_t *self );
extern void     Boba_TacticsSelect( void );
extern bool     Boba_Tactics( void );

////////////////////////////////////////////////////////////////////////////////////////
// External Functions
////////////////////////////////////////////////////////////////////////////////////////
extern void		G_SoundAtSpot( vec3_t org, int soundIndex, qboolean broadcast );
extern void		G_CreateG2AttachedWeaponModel( gentity_t *ent, const char *weaponModel, int boltNum, int weaponNum );
extern void		ChangeWeapon( gentity_t *ent, int newWeapon );
extern void		WP_ResistForcePush( gentity_t *self, gentity_t *pusher, qboolean noPenalty );
extern void		ForceJump( gentity_t *self, usercmd_t *ucmd );
extern void		G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );

extern void CG_DrawEdge( vec3_t start, vec3_t end, int type );

////////////////////////////////////////////////////////////////////////////////////////
// External Data
////////////////////////////////////////////////////////////////////////////////////////
extern cvar_t*		g_bobaDebug;

////////////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////////////
#define		BOBA_FLAMEDURATION			3000
#define		BOBA_FLAMETHROWRANGE		128
#define		BOBA_FLAMETHROWSIZE			40
#define		BOBA_FLAMETHROWDAMAGEMIN	1//10
#define		BOBA_FLAMETHROWDAMAGEMAX	5//40
#define		BOBA_ROCKETRANGEMIN			300
#define		BOBA_ROCKETRANGEMAX			2000


////////////////////////////////////////////////////////////////////////////////////////
// Enums
////////////////////////////////////////////////////////////////////////////////////////
enum	EBobaTacticsState
{
	BTS_NONE,

	// Attack
	//--------
	BTS_RIFLE,			// Uses Jedi / Seeker Movement
	BTS_MISSILE,		// Uses Jedi / Seeker Movement
	BTS_SNIPER,			// Uses Special Movement Internal To This File
	BTS_FLAMETHROW,		// Locked In Place

	// Waiting
	//---------

	BTS_MAX
};

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool	Mando_CanSeeEnemy( gentity_t *self )
{
	assert(self && self->NPC && self->client && self->client->NPC_class==CLASS_MANDO);
 	return ((level.time - self->NPC->enemyLastSeenTime)<1000);
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void	NPC_Mando_Pain( gentity_t *self, gentity_t *inflictor, gentity_t *other, const vec3_t point, int damage, int mod,int hitLoc )
{
	NPC_Pain( self, inflictor, other, point, damage, mod, hitLoc );

	if ( !damage && self->health > 0 )
	{
		self->client->ps.torsoAnimTimer  =    0;
        G_StopEffect( G_EffectIndex("boba/fthrw"), self->playerModel, self->genericBolt3, self->s.number);
        return;
    }
    if ((NPCInfo->aiFlags&NPCAI_FLAMETHROW))
    {
        self->NPC->aiFlags                &= ~NPCAI_FLAMETHROW;
        self->client->ps.torsoAnimTimer  =    0;

        TIMER_Set( self, "flameTime",            0);
        TIMER_Set( self, "nextAttackDelay",        0);
        TIMER_Set( self, "Boba_TacticsSelect",    0);

    //    G_SoundOnEnt( self, CHAN_WEAPON, "sound/effects/flameoff.mp3" );
        G_StopEffect( G_EffectIndex("boba/fthrw"), self->playerModel, self->genericBolt3, self->s.number);

        Boba_Printf("FlameThrower OFF");
	}
}