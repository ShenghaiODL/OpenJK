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
#include "../qcommon/tri_coll_test.h"
#include "../cgame/cg_local.h"
#include "AI_BobaFett.h"
#include "g_difficulty.h"
#include "g_force.h"

#define JK2_RAGDOLL_GRIPNOHEALTH

#define MAX_SABER_VICTIMS 16
static int		victimEntityNum[MAX_SABER_VICTIMS];
static float	totalDmg[MAX_SABER_VICTIMS];
static vec3_t	dmgDir[MAX_SABER_VICTIMS];
static vec3_t	dmgNormal[MAX_SABER_VICTIMS];
static vec3_t	dmgBladeVec[MAX_SABER_VICTIMS];
static vec3_t	dmgSpot[MAX_SABER_VICTIMS];
static float	dmgFraction[MAX_SABER_VICTIMS];
static int		hitLoc[MAX_SABER_VICTIMS];
static qboolean	hitDismember[MAX_SABER_VICTIMS];
static int		hitDismemberLoc[MAX_SABER_VICTIMS];
static vec3_t	saberHitLocation, saberHitNormal={0,0,1.0};
static float	saberHitFraction;
static float	sabersCrossed;
static int		saberHitEntity;
static int		numVictims = 0;

extern cvar_t	*g_sex;
extern cvar_t	*g_timescale;
extern cvar_t	*g_dismemberment;
extern cvar_t	*g_mindTrickPacify;
extern cvar_t	*g_debugSaberLock;
extern cvar_t	*g_saberLockRandomNess;
extern cvar_t	*d_slowmodeath;
extern cvar_t	*g_cheats;
extern cvar_t	*g_debugMelee;
extern cvar_t	*g_saberRestrictForce;
extern cvar_t	*g_saberPickuppableDroppedSabers;
extern cvar_t	*debug_subdivision;
extern cvar_t	*g_forceRegenTime;


extern qboolean WP_SaberBladeUseSecondBladeStyle( saberInfo_t *saber, int bladeNum );
extern qboolean WP_SaberBladeDoTransitionDamage( saberInfo_t *saber, int bladeNum );
extern qboolean Q3_TaskIDPending( gentity_t *ent, taskID_t taskType );
extern qboolean		G_ClearViewEntity( gentity_t *ent );
extern void			G_SetViewEntity( gentity_t *self, gentity_t *viewEntity );
extern qboolean G_ControlledByPlayer( gentity_t *self );
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void CG_ChangeWeapon( int num );
extern void CG_SaberDoWeaponHitMarks( gclient_t *client, gentity_t *saberEnt, gentity_t *hitEnt, int saberNum, int bladeNum, vec3_t hitPos, vec3_t hitDir, vec3_t uaxis, vec3_t splashBackDir, float sizeTimeScale );
extern void G_AngerAlert( gentity_t *self );
extern void G_ReflectMissile( gentity_t *ent, gentity_t *missile, vec3_t forward, qboolean perfect = qfalse );
extern int G_CheckLedgeDive( gentity_t *self, float checkDist, const vec3_t checkVel, qboolean tryOpposite, qboolean tryPerp );
extern void G_BounceMissile( gentity_t *ent, trace_t *trace );
extern qboolean G_PointInBounds( const vec3_t point, const vec3_t mins, const vec3_t maxs );
extern void NPC_UseResponse( gentity_t *self, gentity_t *user, qboolean useWhenDone );
extern void WP_FireDreadnoughtBeam( gentity_t *ent );
extern void G_MissileImpacted( gentity_t *ent, gentity_t *other, vec3_t impactPos, vec3_t normal, int hitLoc=HL_NONE );
extern evasionType_t Jedi_SaberBlockGo( gentity_t *self, usercmd_t *cmd, vec3_t pHitloc, vec3_t phitDir, gentity_t *incoming, float dist = 0.0f );
extern void Jedi_RageStop( gentity_t *self );
extern int PM_PickAnim( gentity_t *self, int minAnim, int maxAnim );
extern void NPC_SetPainEvent( gentity_t *self );
extern qboolean PM_SwimmingAnim( int anim );
extern qboolean PM_InAnimForSaberMove( int anim, int saberMove );
extern qboolean PM_SpinningSaberAnim( int anim );
extern qboolean PM_SaberInSpecialAttack( int anim );
extern qboolean PM_SaberInAttack( int move );
extern qboolean PM_SaberInAttackPure( int move );
extern qboolean PM_SaberInTransition( int move );
extern qboolean PM_SaberInStart( int move );
extern qboolean PM_SaberInTransitionAny( int move );
extern qboolean PM_SaberInReturn( int move );
extern qboolean PM_SaberInBounce( int move );
extern qboolean PM_SaberInParry( int move );
extern qboolean PM_SaberInKnockaway( int move );
extern qboolean PM_SaberInBrokenParry( int move );
extern qboolean PM_SpinningSaberAnim( int anim );
extern saberMoveName_t PM_SaberBounceForAttack( int move );
extern saberMoveName_t PM_BrokenParryForAttack( int move );
extern saberMoveName_t PM_KnockawayForParry( int move );
extern qboolean PM_FlippingAnim( int anim );
extern qboolean PM_RollingAnim( int anim );
extern qboolean PM_CrouchAnim( int anim );
extern qboolean PM_SaberInIdle( int move );
extern qboolean PM_SaberInReflect( int move );
extern qboolean PM_InSpecialJump( int anim );
extern qboolean PM_InKnockDown( playerState_t *ps );
extern qboolean PM_ForceUsingSaberAnim( int anim );
extern qboolean PM_SuperBreakLoseAnim( int anim );
extern qboolean PM_SuperBreakWinAnim( int anim );
extern qboolean PM_SaberLockBreakAnim( int anim );
extern qboolean PM_InOnGroundAnim ( playerState_t *ps );
extern qboolean PM_KnockDownAnim( int anim );
extern qboolean PM_SaberInKata( saberMoveName_t saberMove );
extern qboolean PM_StabDownAnim( int anim );
extern int PM_PowerLevelForSaberAnim( playerState_t *ps, int saberNum = 0 );
extern void PM_VelocityForSaberMove( playerState_t *ps, vec3_t throwDir );
extern qboolean PM_VelocityForBlockedMove( playerState_t *ps, vec3_t throwDir );
extern qboolean PM_SaberCanInterruptMove( int move, int anim );
extern int Jedi_ReCalcParryTime( gentity_t *self, evasionType_t evasionType );
extern qboolean Jedi_DodgeEvasion( gentity_t *self, gentity_t *shooter, trace_t *tr, int hitLoc );
extern void Jedi_PlayDeflectSound( gentity_t *self );
extern void Jedi_PlayBlockedPushSound( gentity_t *self );
extern qboolean Jedi_WaitingAmbush( gentity_t *self );
extern void Jedi_Ambush( gentity_t *self );
extern qboolean Jedi_SaberBusy( gentity_t *self );
extern qboolean Jedi_CultistDestroyer( gentity_t *self );
extern qboolean Boba_Flying( gentity_t *self );
extern void JET_FlyStart( gentity_t *self );
extern void Boba_DoFlameThrower( gentity_t *self );
extern void Boba_StopFlameThrower( gentity_t *self );

extern Vehicle_t *G_IsRidingVehicle( gentity_t *ent );
extern int SaberDroid_PowerLevelForSaberAnim( gentity_t *self );
extern qboolean G_ValidEnemy( gentity_t *self, gentity_t *enemy );
extern void G_StartMatrixEffect( gentity_t *ent, int meFlags = 0, int length = 1000, float timeScale = 0.0f, int spinTime = 0 );
extern int PM_AnimLength( int index, animNumber_t anim );
extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
extern void G_KnockOffVehicle( gentity_t *pRider, gentity_t *self, qboolean bPull );
extern qboolean PM_LockedAnim( int anim );
extern qboolean Rosh_BeingHealed( gentity_t *self );
extern qboolean G_OkayToLean( playerState_t *ps, usercmd_t *cmd, qboolean interruptOkay );

int WP_AbsorbConversion(gentity_t *attacked, int atdAbsLevel, gentity_t *attacker, int atPower, int atPowerLevel, int atForceSpent);
static const char *WP_ForceDebugWho( const gentity_t *ent );	//JKEnhanced g_debugForce (with WP_ForcePowerDrain)
const char *WP_ForcePowerName( int power );
extern cvar_t *g_debugForce;
void WP_ForcePowerStart( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
void WP_ForcePowerStop( gentity_t *self, forcePowers_t forcePower );
qboolean WP_ForcePowerUsable( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
void WP_SaberInFlightReflectCheck( gentity_t *self, usercmd_t *ucmd  );

void WP_SaberDrop( gentity_t *self, gentity_t *saber );
qboolean WP_SaberLose( gentity_t *self, vec3_t throwDir );
void WP_SaberReturn( gentity_t *self, gentity_t *saber );
void WP_SaberBlockNonRandom( gentity_t *self, vec3_t hitloc, qboolean missileBlock );
qboolean WP_ForcePowerAvailable( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
void WP_ForcePowerDrain( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
qboolean WP_NPCForceFree( const gentity_t *self, int forcePower );	//JKEnhanced 4B: NPC pays nothing for this power
static const char *WP_NPCForceFreeReason( const gentity_t *self, int forcePower );
void WP_DeactivateSaber( gentity_t *self, qboolean clearLength = qfalse );
qboolean FP_ForceDrainGrippableEnt( gentity_t *victim );

extern cvar_t	*g_saberAutoBlocking;
extern cvar_t	*g_saberRealisticCombat;
extern cvar_t	*g_saberDamageCapping;
extern cvar_t	*g_saberNewControlScheme;
extern cvar_t	*g_flippedHolsters;
extern cvar_t	*g_forceStaggerLockoutScale;
extern int g_crosshairEntNum;

qboolean g_saberNoEffects = qfalse;
qboolean g_noClashFlare = qfalse;
int		g_saberFlashTime = 0;
vec3_t	g_saberFlashPos = {0,0,0};

int forcePowerDarkLight[NUM_FORCE_POWERS] = //0 == neutral
{ //nothing should be usable at rank 0..
	FORCE_LIGHTSIDE,//FP_HEAL,//instant
	0,//FP_LEVITATION,//hold/duration
	0,//FP_SPEED,//duration
	0,//FP_PUSH,//hold/duration
	0,//FP_PULL,//hold/duration
	FORCE_LIGHTSIDE,//FP_TELEPATHY,//instant
	FORCE_DARKSIDE,//FP_GRIP,//hold/duration
	FORCE_DARKSIDE,//FP_LIGHTNING,//hold/duration
	0,//FP_SABERATTACK,
	0,//FP_SABERDEFEND,
	0,//FP_SABERTHROW,
	//new Jedi Academy powers
	FORCE_DARKSIDE,//FP_RAGE,//duration
	FORCE_LIGHTSIDE,//FP_PROTECT,//duration
	FORCE_LIGHTSIDE,//FP_ABSORB,//duration
	FORCE_DARKSIDE,//FP_DRAIN,//hold/duration
	0,//FP_SEE,//duration
	FORCE_DARKSIDE,//FP_INSANITY
	FORCE_LIGHTSIDE,//FP_STASIS
	0,//FP_REPULSE (JKEnhanced, restored)
	//NUM_FORCE_POWERS
};

int forcePowerNeeded[NUM_FORCE_POWERS] =
{
	0,//FP_HEAL,//instant
	10,//FP_LEVITATION,//hold/duration
	50,//FP_SPEED,//duration
	15,//FP_PUSH,//hold/duration
	15,//FP_PULL,//hold/duration
	20,//FP_TELEPATHY,//instant
	1,//FP_GRIP,//hold/duration - FIXME: 30?
	1,//FP_LIGHTNING,//hold/duration
	20,//FP_SABERTHROW,
	1,//FP_SABER_DEFENSE,
	0,//FP_SABER_OFFENSE,
	//new Jedi Academy powers
	50,//FP_RAGE,//duration - speed, invincibility and extra damage for short period, drains your health and leaves you weak and slow afterwards.
	30,//FP_PROTECT,//duration - protect against physical/energy (level 1 stops blaster/energy bolts, level 2 stops projectiles, level 3 protects against explosions)
	30,//FP_ABSORB,//duration - protect against dark force powers (grip, lightning, drain)
	1,//FP_DRAIN,//hold/duration - drain force power for health
	20,//FP_SEE,//duration - detect/see hidden enemies
	50,//FP_INSANITY
	35,//FP_STASIS
	20,//FP_REPULSE (JKEnhanced, restored)
	//NUM_FORCE_POWERS
};

float forceJumpStrength[NUM_FORCE_POWER_LEVELS] =
{
	JUMP_VELOCITY,//normal jump
	420,
	590,
	840
};

float forceJumpHeight[NUM_FORCE_POWER_LEVELS] =
{
	32,//normal jump (+stepheight+crouchdiff = 66)
	96,//(+stepheight+crouchdiff = 130)
	192,//(+stepheight+crouchdiff = 226)
	384//(+stepheight+crouchdiff = 418)
};

float forceJumpHeightMax[NUM_FORCE_POWER_LEVELS] =
{
	66,//normal jump (32+stepheight(18)+crouchdiff(24) = 74)
	130,//(96+stepheight(18)+crouchdiff(24) = 138)
	226,//(192+stepheight(18)+crouchdiff(24) = 234)
	418//(384+stepheight(18)+crouchdiff(24) = 426)
};

float forcePushPullRadius[NUM_FORCE_POWER_LEVELS] =
{
	0,//none
	384,//256,
	448,//384,
	512
};

float forcePushCone[NUM_FORCE_POWER_LEVELS] =
{
	1.0f,//none
	1.0f,
	0.8f,
	0.6f
};

float forcePullCone[NUM_FORCE_POWER_LEVELS] =
{
	1.0f,//none
	1.0f,
	1.0f,
	0.8f
};

float forceSpeedValue[NUM_FORCE_POWER_LEVELS] =
{
	1.0f,//none
	0.75f,
	0.5f,
	0.25f
};

float forceSpeedRangeMod[NUM_FORCE_POWER_LEVELS] =
{
	0.0f,//none
	30.0f,
	45.0f,
	60.0f
};

float forceSpeedFOVMod[NUM_FORCE_POWER_LEVELS] =
{
	0.0f,//none
	20.0f,
	30.0f,
	40.0f
};

int forceGripDamage[NUM_FORCE_POWER_LEVELS] =
{
	0,//none
	0,
	6,
	9
};

int mindTrickTime[NUM_FORCE_POWER_LEVELS] =
{
	0,//none
	10000,//5000,
	15000,//10000,
	30000//15000
};

int insanityTime[NUM_FORCE_POWER_LEVELS] =
{
	0,//none
	5000,//5000,
	10000,//10000,
	15000//15000
};

int stasisTime[NUM_FORCE_POWER_LEVELS] =
{
	0,//none
	5000,//5000,
	10000,//10000,
	15000//15000
};

//NOTE: keep in synch with table below!!!
int saberThrowDist[NUM_FORCE_POWER_LEVELS] =
{
	0,//none
	256,
	400,
	400
};

//NOTE: keep in synch with table above!!!
int saberThrowDistSquared[NUM_FORCE_POWER_LEVELS] =
{
	0,//none
	65536,
	160000,
	160000
};

int parryDebounce[NUM_FORCE_POWER_LEVELS] =
{
	100,//if don't even have defense, can't use defense!
	100,
	50,
	50
};

// Separate, much shorter recovery window for blocking blaster bolts vs. saber-on-saber
// parries - repeater firetime is 50ms, so this must stay under that even at low Defense.
int missileParryDebounce[NUM_FORCE_POWER_LEVELS] =
{
	100,
	35,
	25,
	15
};

// Per-style multiplier applied on top of the FP_SABER_DEFENSE-based recovery times above.
// <1.0 recovers faster, >1.0 recovers slower. Gives each saber style a distinct parry feel.
float saberStyleParryRecoveryScale[SS_NUM_SABER_STYLES] =
{
	1.0f,//SS_NONE
	0.75f,//SS_FAST - quick to recover, rewards aggressive fencing
	1.0f,//SS_MEDIUM
	1.3f,//SS_STRONG - heavy swings leave you open longer
	1.3f,//SS_DESANN
	1.0f,//SS_TAVION
	1.0f,//SS_KATARN
	0.9f,//SS_DUAL - sustained pressure, rapid direction changes
	0.95f,//SS_STAFF
};

// Additional per-style multiplier applied only to missile (blaster bolt) parries, on top
// of saberStyleParryRecoveryScale above. Staff is meant to have superior missile defense.
float saberStyleMissileParryScale[SS_NUM_SABER_STYLES] =
{
	1.0f,//SS_NONE
	1.0f,//SS_FAST
	1.0f,//SS_MEDIUM
	1.0f,//SS_STRONG
	1.0f,//SS_DESANN
	1.0f,//SS_TAVION
	1.0f,//SS_KATARN
	1.0f,//SS_DUAL
	0.7f,//SS_STAFF - superior missile defense
};

float saberAnimSpeedMod[NUM_FORCE_POWER_LEVELS] =
{
	0.0f,//if don't even have offense, can't use offense!
	0.75f,
	1.0f,
	2.0f
};

stringID_table_t SaberStyleTable[] =
{
	{ "NULL",SS_NONE },
	ENUM2STRING(SS_FAST),
	{ "fast",SS_FAST },
	ENUM2STRING(SS_MEDIUM),
	{ "medium",SS_MEDIUM },
	ENUM2STRING(SS_STRONG),
	{ "strong",SS_STRONG },
	ENUM2STRING(SS_DESANN),
	{ "desann",SS_DESANN },
	ENUM2STRING(SS_TAVION),
	{ "tavion",SS_TAVION },
	ENUM2STRING(SS_KATARN),
	{ "katarn",SS_KATARN },
	ENUM2STRING(SS_DUAL),
	{ "dual",SS_DUAL },
	ENUM2STRING(SS_STAFF),
	{ "staff",SS_STAFF },
	{ "", 0 },
};

//SABER INITIALIZATION======================================================================

void G_CreateG2HolsteredWeaponModel( gentity_t *ent, const char *psWeaponModel, int boltNum, int weaponNum, vec3_t angles, vec3_t offset )
{
	if (!psWeaponModel)
	{
		assert (psWeaponModel);
		return;
	}
	if ( ent->playerModel == -1 )
	{
		return;
	}
	if ( boltNum == -1 )
	{
		return;
	}
	
	if ( weaponNum < 0 || weaponNum >= MAX_INHAND_WEAPONS )
	{
		return;
	}
	char weaponModel[64];
	
	strcpy (weaponModel, psWeaponModel);
	if (char *spot = strstr(weaponModel, ".md3") ) {
		*spot = 0;
		spot = strstr(weaponModel, "_w");//i'm using the in view weapon array instead of scanning the item list, so put the _w back on
		if (!spot&&!strstr(weaponModel, "noweap"))
		{
			strcat (weaponModel, "_w");
		}
		strcat (weaponModel, ".glm");	//and change to ghoul2
	}
		
	// give us a saber model
	int wModelIndex = G_ModelIndex( weaponModel );
	if ( wModelIndex )
	{
		ent->holsterModel[weaponNum] = gi.G2API_InitGhoul2Model(ent->ghoul2, weaponModel, wModelIndex, NULL_HANDLE, NULL_HANDLE, 0, 0 );
		if ( ent->holsterModel[weaponNum] != -1 )
		{
			// attach it to the hip. need some correction of rotation first though!
			int holsterorigin = gi.G2API_AddBolt(&ent->ghoul2[ent->holsterModel[weaponNum]], "*holsterorigin", qfalse);
			mdxaBone_t boltMatrix2;
			if (holsterorigin != -1)
			{
				vec3_t origin = {0, 0, 0};
				gi.G2API_GetBoltMatrix(ent->ghoul2, ent->holsterModel[weaponNum], holsterorigin, &boltMatrix2,
									   origin, origin, 0,
									   NULL, ent->s.modelScale);
			}
			gi.G2API_AttachG2Model(&ent->ghoul2[ent->holsterModel[weaponNum]], &ent->ghoul2[ent->playerModel],
								   boltNum, ent->playerModel);
			if (holsterorigin == -1)
			{
				gi.G2API_SetBoneAnglesOffset(&ent->ghoul2[ent->holsterModel[weaponNum]], "ModView internal default", angles, BONE_ANGLES_PREMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0, offset);
			}
			else
			{
				boltMatrix2.matrix[1][3] -= 1.0f;//TODO: this is no good for back holstered weapons
				gi.G2API_SetBoneAnglesMatrix(&ent->ghoul2[ent->holsterModel[weaponNum]], "ModView internal default", boltMatrix2, BONE_ANGLES_PREMULT,
											 NULL, 0, 0);
				
			}
			// set up a bolt on the end so we can get where the sabre muzzle is - we can assume this is always bolt 0
			gi.G2API_AddBolt(&ent->ghoul2[ent->holsterModel[weaponNum]], "*flash", qfalse);
	  		//gi.G2API_SetLodBias( &ent->ghoul2[ent->weaponModel[weaponNum]], 0 );
		}
	}
	
}

void G_CreateG2AttachedWeaponModel( gentity_t *ent, const char *psWeaponModel, int boltNum, int weaponNum )
{
	if (!psWeaponModel)
	{
		assert (psWeaponModel);
		return;
	}
	if ( ent->playerModel == -1 )
	{
		return;
	}
	if ( boltNum == -1 )
	{
		return;
	}

	if ( ent && ent->client && ent->client->NPC_class == CLASS_GALAKMECH )
	{//hack for galakmech, no weaponmodel
		ent->weaponModel[0] = ent->weaponModel[1] = -1;
		return;
	}
	if ( weaponNum < 0 || weaponNum >= MAX_INHAND_WEAPONS )
	{
		return;
	}
	char weaponModel[64];

	strcpy (weaponModel, psWeaponModel);
	if (char *spot = strstr(weaponModel, ".md3") ) {
		*spot = 0;
		spot = strstr(weaponModel, "_w");//i'm using the in view weapon array instead of scanning the item list, so put the _w back on
		if (!spot&&!strstr(weaponModel, "noweap"))
		{
			strcat (weaponModel, "_w");
		}
		strcat (weaponModel, ".glm");	//and change to ghoul2
	}

	// give us a saber model
	int wModelIndex = G_ModelIndex( weaponModel );
	if ( wModelIndex )
	{
		ent->weaponModel[weaponNum] = gi.G2API_InitGhoul2Model(ent->ghoul2, weaponModel, wModelIndex, NULL_HANDLE, NULL_HANDLE, 0, 0 );
		if ( ent->weaponModel[weaponNum] != -1 )
		{
			// attach it to the hand
			gi.G2API_AttachG2Model(&ent->ghoul2[ent->weaponModel[weaponNum]], &ent->ghoul2[ent->playerModel],
						boltNum, ent->playerModel);
			// set up a bolt on the end so we can get where the sabre muzzle is - we can assume this is always bolt 0
			if (gi.G2API_AddBolt(&ent->ghoul2[ent->weaponModel[weaponNum]], "*cannonflash", qfalse) != -1)
			{
				vec3_t gunAngles = { 0.0f, 0.0f, 0.0f };
				vec3_t offset = { 0.0f, 0.0f, -10.0f };
				gi.G2API_SetSurfaceOnOff(&ent->ghoul2[ent->weaponModel[weaponNum]], "eweb_cannon", 0x00000002);
				gi.G2API_SetBoneAnglesOffset(&ent->ghoul2[ent->weaponModel[weaponNum]], "base", gunAngles, BONE_ANGLES_PREMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, 0, offset);
			}
			else
			{
				gi.G2API_AddBolt(&ent->ghoul2[ent->weaponModel[weaponNum]], "*flash", qfalse);
			}
	  		//gi.G2API_SetLodBias( &ent->ghoul2[ent->weaponModel[weaponNum]], 0 );
		}
	}

}

void WP_SaberAddG2SaberModels( gentity_t *ent, int specificSaberNum )
{
	int saberNum = 0, maxSaber = 1;
	if ( specificSaberNum != -1 && specificSaberNum <= maxSaber )
	{
		saberNum = maxSaber = specificSaberNum;
	}
	for ( ; saberNum <= maxSaber; saberNum++ )
	{
		if ( ent->weaponModel[saberNum] > 0 )
		{//we already have a weapon model in this slot
			//remove it
			gi.G2API_SetSkin( &ent->ghoul2[ent->weaponModel[saberNum]], -1, 0 );
			gi.G2API_RemoveGhoul2Model( ent->ghoul2, ent->weaponModel[saberNum] );
			ent->weaponModel[saberNum] = -1;
		}
		if ( saberNum > 0 )
		{//second saber
			if ( !ent->client->ps.dualSabers
				|| G_IsRidingVehicle( ent ) )
			{//only have one saber or riding a vehicle and can only use one saber
				return;
			}
		}
		else if ( saberNum == 0 )
		{//first saber
			if ( ent->client->ps.saberInFlight )
			{//it's still out there somewhere, don't add it
				//FIXME: call it back?
				continue;
			}
		}
		int handBolt = ((saberNum == 0) ? ent->handRBolt : ent->handLBolt);
		if ( (ent->client->ps.saber[saberNum].saberFlags&SFL_BOLT_TO_WRIST) )
		{//special case, bolt to forearm
			if ( saberNum == 0 )
			{
				handBolt = gi.G2API_AddBolt( &ent->ghoul2[ent->playerModel], "*r_hand_cap_r_arm", qfalse );
			}
			else
			{
				handBolt = gi.G2API_AddBolt( &ent->ghoul2[ent->playerModel], "*l_hand_cap_l_arm", qfalse );
			}
		}
		G_CreateG2AttachedWeaponModel( ent, ent->client->ps.saber[saberNum].model, handBolt, saberNum );

        gi.G2API_SetTintType( &ent->ghoul2[ent->weaponModel[saberNum]], saberNum ? G2_TINT_SABER2 : G2_TINT_SABER );

		if ( ent->client->ps.saber[saberNum].skin != NULL )
		{//if this saber has a customSkin, use it
			// lets see if it's out there
			int saberSkin = gi.RE_RegisterSkin( ent->client->ps.saber[saberNum].skin );
			if ( saberSkin )
			{
				// put it in the config strings
				// and set the ghoul2 model to use it
				gi.G2API_SetSkin( &ent->ghoul2[ent->weaponModel[saberNum]], G_SkinIndex( ent->client->ps.saber[saberNum].skin ), saberSkin );
			}
		}
	}
}

void WP_SaberAddHolsteredG2SaberModels( gentity_t *ent, int specificSaberNum )
{
	int saberNum = 0, maxSaber = 1;
	if (!(ent && ent->client && (ent->client->ps.weapons[WP_SABER])))
	{
		return;
	}
	if ( specificSaberNum != -1 && specificSaberNum <= maxSaber )
	{
		saberNum = maxSaber = specificSaberNum;
	}
	for ( ; saberNum <= maxSaber; saberNum++ )
	{
		if ( ent->holsterModel[saberNum] > 0 )
		{//we already have a weapon model in this slot
			//remove it
			gi.G2API_SetSkin( &ent->ghoul2[ent->holsterModel[saberNum]], -1, 0 );
			gi.G2API_RemoveGhoul2Model( ent->ghoul2, ent->holsterModel[saberNum] );
			ent->holsterModel[saberNum] = -1;
		}
		if ( saberNum > 0 )
		{//second saber
			if ( !ent->client->ps.dualSabers )
			{//only have one saber or riding a vehicle and can only use one saber
				return;
			}
		}
		else if ( saberNum == 0 )
		{//first saber
			if ( ent->client->ps.saberInFlight )
			{//it's still out there somewhere, don't add it
				//FIXME: call it back?
				continue;
			}
		}
		else if ( ent->client->ps.saber[saberNum].holsterPlace == HOLSTER_NONE )
		{
			continue;
		}
		int handBolt = -1;
		holster_locations_t holsterPlace = ent->client->ps.saber[saberNum].holsterPlace;
		vec3_t offset = { 0.0f, 0.0f, 0.0f };
		vec3_t angles = { 0.0f, 0.0f, 0.0f };
		if ( (holsterPlace == HOLSTER_HIPS) || (holsterPlace == HOLSTER_LHIP) )
		{
			angles[PITCH] = 180.0f;
			angles[YAW] = 0.0f;
			angles[ROLL] = 180.0f;
			if (g_flippedHolsters && g_flippedHolsters->integer > 0)
			{
				angles[YAW] = 180.0f;
			}
			VectorSet(offset, 0.0f, -1.0f, -5.0f);
		}
		else if ( holsterPlace == HOLSTER_BACK )
		{
			angles[YAW] = 180.0f;
			angles[PITCH] = 22.5f;
			VectorSet(offset, 0.0f, -2.0f, 4.0f);
		}
		if ( saberNum == 0 )
		{
			if ( holsterPlace == HOLSTER_LHIP )
			{
				handBolt = gi.G2API_AddBolt( &ent->ghoul2[ent->playerModel], "*hip_l", qfalse );
			}
			else if ( holsterPlace == HOLSTER_HIPS )
			{
				handBolt = gi.G2API_AddBolt( &ent->ghoul2[ent->playerModel], "*hip_r", qfalse );
			}
			else if ( holsterPlace == HOLSTER_BACK )
			{
				handBolt = gi.G2API_AddBolt( &ent->ghoul2[ent->playerModel], "*back", qfalse );
			}
		}
		else
		{
			if ( holsterPlace == HOLSTER_HIPS || holsterPlace == HOLSTER_LHIP )
			{
				if ( ent->client->ps.saber[0].holsterPlace == HOLSTER_LHIP )
				{
					handBolt = gi.G2API_AddBolt( &ent->ghoul2[ent->playerModel], "*hip_r", qfalse );
				}
				else
				{
					handBolt = gi.G2API_AddBolt( &ent->ghoul2[ent->playerModel], "*hip_l", qfalse );
				}
			}
			else if ( holsterPlace == HOLSTER_BACK )
			{
				if ( ent->client->ps.saber[0].holsterPlace == HOLSTER_BACK )
				{
					continue;
				}
				handBolt = gi.G2API_AddBolt( &ent->ghoul2[ent->playerModel], "*back", qfalse );
			}
		}
		G_CreateG2HolsteredWeaponModel( ent, ent->client->ps.saber[saberNum].model, handBolt, saberNum, angles, offset );
		
        gi.G2API_SetTintType( &ent->ghoul2[ent->holsterModel[saberNum]], saberNum ? G2_TINT_SABER2 : G2_TINT_SABER );
        
		if ( ent->client->ps.saber[saberNum].skin != NULL )
		{//if this saber has a customSkin, use it
			// lets see if it's out there
			int saberSkin = gi.RE_RegisterSkin( ent->client->ps.saber[saberNum].skin );
			if ( saberSkin )
			{
				// put it in the config strings
				// and set the ghoul2 model to use it
				gi.G2API_SetSkin( &ent->ghoul2[ent->holsterModel[saberNum]], G_SkinIndex( ent->client->ps.saber[saberNum].skin ), saberSkin );
			}
		}
	}
}

//----------------------------------------------------------
void G_Throw( gentity_t *targ, const vec3_t newDir, float push )
//----------------------------------------------------------
{
	vec3_t	kvel;
	float	mass;

	if ( targ
		&& targ->client
		&& ( targ->client->NPC_class == CLASS_ATST
			|| targ->client->NPC_class == CLASS_RANCOR
			|| targ->client->NPC_class == CLASS_SAND_CREATURE ) )
	{//much to large to *ever* throw
		return;
	}

	if ( targ->physicsBounce > 0 )	//overide the mass
	{
		mass = targ->physicsBounce;
	}
	else
	{
		mass = 200;
	}

	if ( g_gravity->value > 0 )
	{
		VectorScale( newDir, g_knockback->value * (float)push / mass * 0.8, kvel );
		if ( !targ->client || targ->client->ps.groundEntityNum != ENTITYNUM_NONE )
		{//give them some z lift to get them off the ground
			kvel[2] = newDir[2] * g_knockback->value * (float)push / mass * 1.5;
		}
	}
	else
	{
		VectorScale( newDir, g_knockback->value * (float)push / mass, kvel );
	}

	if ( targ->client )
	{
		VectorAdd( targ->client->ps.velocity, kvel, targ->client->ps.velocity );
	}
	else if ( targ->s.pos.trType != TR_STATIONARY && targ->s.pos.trType != TR_LINEAR_STOP && targ->s.pos.trType != TR_NONLINEAR_STOP )
	{
		VectorAdd( targ->s.pos.trDelta, kvel, targ->s.pos.trDelta );
		VectorCopy( targ->currentOrigin, targ->s.pos.trBase );
		targ->s.pos.trTime = level.time;
	}

	// set the timer so that the other client can't cancel
	// out the movement immediately
	if ( targ->client && !targ->client->ps.pm_time )
	{
		int		t;

		t = push * 2;

		if ( t < 50 )
		{
			t = 50;
		}
		if ( t > 200 )
		{
			t = 200;
		}
		targ->client->ps.pm_time = t;
		targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	}
}

int WP_SetSaberModel( gclient_t *client, class_t npcClass )
{//FIXME: read from NPCs.cfg
	if ( client )
	{
		switch ( npcClass )
		{
		case CLASS_DESANN://Desann
			client->ps.saber[0].model = "models/weapons2/saber_desann/saber_w.glm";
			break;
		case CLASS_LUKE://Luke
			client->ps.saber[0].model = "models/weapons2/saber_luke/saber_w.glm";
			break;
		case CLASS_PLAYER://Kyle NPC and player
		case CLASS_KYLE://Kyle NPC and player
			client->ps.saber[0].model = "models/weapons2/saber/saber_w.glm";
			break;
		default://reborn and tavion and everyone else
			client->ps.saber[0].model = "models/weapons2/saber_reborn/saber_w.glm";
			break;
		}
		return ( G_ModelIndex( client->ps.saber[0].model ) );
	}
	else
	{
		switch ( npcClass )
		{
		case CLASS_DESANN://Desann
			return ( G_ModelIndex( "models/weapons2/saber_desann/saber_w.glm" ) );
			break;
		case CLASS_LUKE://Luke
			return ( G_ModelIndex( "models/weapons2/saber_luke/saber_w.glm" ) );
			break;
		case CLASS_PLAYER://Kyle NPC and player
		case CLASS_KYLE://Kyle NPC and player
			return ( G_ModelIndex( "models/weapons2/saber/saber_w.glm" ) );
			break;
		default://reborn and tavion and everyone else
			return ( G_ModelIndex( "models/weapons2/saber_reborn/saber_w.glm" ) );
			break;
		}
	}
}

void WP_SetSaberEntModelSkin( gentity_t *ent, gentity_t *saberent )
{
	int	saberModel = 0;
	qboolean	newModel = qfalse;
	//FIXME: get saberModel from NPCs.cfg
	if ( !ent->client->ps.saber[0].model )
	{
		saberModel = WP_SetSaberModel( ent->client, ent->client->NPC_class );
	}
	else
	{
		//got saberModel from NPCs.cfg
		saberModel = G_ModelIndex( ent->client->ps.saber[0].model );
	}
	if ( saberModel && saberent->s.modelindex != saberModel )
	{
		if ( saberent->playerModel >= 0 )
		{//remove the old one, if there is one
			gi.G2API_RemoveGhoul2Model( saberent->ghoul2, saberent->playerModel );
		}
		//add the new one
		saberent->playerModel = gi.G2API_InitGhoul2Model( saberent->ghoul2, ent->client->ps.saber[0].model, saberModel, NULL_HANDLE, NULL_HANDLE, 0, 0);
		saberent->s.modelindex = saberModel;
		newModel = qtrue;
	}
	//set skin, too
	if ( ent->client->ps.saber[0].skin == NULL )
	{
		gi.G2API_SetSkin( &saberent->ghoul2[0], -1, 0 );
	}
	else
	{//if this saber has a customSkin, use it
		// lets see if it's out there
		int saberSkin = gi.RE_RegisterSkin( ent->client->ps.saber[0].skin );
		if ( saberSkin && (newModel || saberent->s.modelindex2 != saberSkin) )
		{
			// put it in the config strings
			// and set the ghoul2 model to use it
			gi.G2API_SetSkin( &saberent->ghoul2[0], G_SkinIndex( ent->client->ps.saber[0].skin ), saberSkin );
			saberent->s.modelindex2 = saberSkin;
		}
	}
}

void WP_SaberFallSound( gentity_t *owner, gentity_t *saber )
{
	if ( !saber )
	{
		return;
	}
	if ( owner && owner->client )
	{//have an owner, use their data (assume saberNum is 0 because only the 0 saber can be thrown)
		if ( owner->client->ps.saber[0].fallSound[0] )
		{//have an override
			G_Sound( saber, owner->client->ps.saber[0].fallSound[Q_irand( 0, 2 )] );
		}
		else if ( owner->client->ps.saber[0].type == SABER_SITH_SWORD )
		{//is a sith sword
			G_Sound( saber, G_SoundIndex( va( "sound/weapons/sword/fall%d.wav", Q_irand( 1, 7 ) ) ) );
		}
		else
		{//normal saber
			G_Sound( saber, G_SoundIndex( va( "sound/weapons/saber/bounce%d.wav", Q_irand( 1, 3 ) ) ) );
		}
	}
	else if ( saber->NPC_type && saber->NPC_type[0] )
	{//have a saber name to look up
		saberInfo_t saberInfo;
		if ( WP_SaberParseParms( saber->NPC_type, &saberInfo ) )
		{//found it
			if ( saberInfo.fallSound[0] )
			{//have an override sound
				G_Sound( saber, saberInfo.fallSound[Q_irand( 0, 2 )] );
			}
			else if ( saberInfo.type == SABER_SITH_SWORD )
			{//is a sith sword
				G_Sound( saber, G_SoundIndex( va( "sound/weapons/sword/fall%d.wav", Q_irand( 1, 7 ) ) ) );
			}
			else
			{//normal saber
				G_Sound( saber, G_SoundIndex( va( "sound/weapons/saber/bounce%d.wav", Q_irand( 1, 3 ) ) ) );
			}
		}
		else
		{//can't find it
			G_Sound( saber, G_SoundIndex( va( "sound/weapons/saber/bounce%d.wav", Q_irand( 1, 3 ) ) ) );
		}
	}
	else
	{//no saber name specified
		G_Sound( saber, G_SoundIndex( va( "sound/weapons/saber/bounce%d.wav", Q_irand( 1, 3 ) ) ) );
	}
}

void WP_SaberSwingSound( gentity_t *ent, int saberNum, swingType_t swingType )
{
	int index = 1;
	if ( !ent || !ent->client )
	{
		return;
	}
	if ( swingType == SWING_FAST )
	{
		index = Q_irand( 1, 3 );
	}
	else if ( swingType == SWING_MEDIUM )
	{
		index = Q_irand( 4, 6 );
	}
	else if ( swingType == SWING_STRONG )
	{
		index = Q_irand( 7, 9 );
	}

	if ( ent->client->ps.saber[saberNum].swingSound[0] )
	{
		G_SoundIndexOnEnt( ent, CHAN_WEAPON, ent->client->ps.saber[saberNum].swingSound[Q_irand( 0, 2 )] );
	}
	else if ( ent->client->ps.saber[saberNum].type == SABER_SITH_SWORD )
	{
		G_SoundOnEnt( ent, CHAN_WEAPON, va( "sound/weapons/sword/swing%d.wav", Q_irand( 1, 4 ) ) );
	}
	else
	{
		G_SoundOnEnt( ent, CHAN_WEAPON, va( "sound/weapons/saber/saberhup%d.wav", index ) );
	}
}

void WP_SaberHitSound( gentity_t *ent, int saberNum, int bladeNum )
{
	int index = 1;
	if ( !ent || !ent->client )
	{
		return;
	}
	index = Q_irand( 1, 3 );

	if ( !WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
		&& ent->client->ps.saber[saberNum].hitSound[0] )
	{
		G_Sound( ent, ent->client->ps.saber[saberNum].hitSound[Q_irand( 0, 2 )] );
	}
	else if ( WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
		&& ent->client->ps.saber[saberNum].hit2Sound[0] )
	{
		G_Sound( ent, ent->client->ps.saber[saberNum].hit2Sound[Q_irand( 0, 2 )] );
	}
	else if ( ent->client->ps.saber[saberNum].type == SABER_SITH_SWORD )
	{
		G_Sound( ent, G_SoundIndex( va( "sound/weapons/sword/stab%d.wav", Q_irand( 1, 4 ) ) ) );
	}
	else
	{
		G_Sound( ent, G_SoundIndex( va( "sound/weapons/saber/saberhit%d.wav", index ) ) );
	}
}

void WP_SaberBlockSound( gentity_t *ent, gentity_t *hitEnt, int saberNum, int bladeNum )
{
	int index = 1;
	if ( !ent || !ent->client )
	{
		return;
	}
	index = Q_irand( 1, 9 );

	if ( !WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
		&& ent->client->ps.saber[saberNum].blockSound[0] )
	{
		G_Sound( ent, ent->client->ps.saber[saberNum].blockSound[Q_irand( 0, 2 )] );
	}
	else if ( WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
		&& ent->client->ps.saber[saberNum].block2Sound[0] )
	{
		G_Sound( ent, ent->client->ps.saber[saberNum].block2Sound[Q_irand( 0, 2 )] );
	}
	else
	{
		G_Sound( ent, G_SoundIndex( va( "sound/weapons/saber/saberblock%d.wav", index ) ) );
	}
}


void WP_SaberBounceOnWallSound( gentity_t *ent, int saberNum, int bladeNum )
{
	int index = 1;
	if ( !ent || !ent->client )
	{
		return;
	}
	index = Q_irand( 1, 9 );

	if ( !WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
		&& ent->client->ps.saber[saberNum].bounceSound[0] )
	{
		G_Sound( ent, ent->client->ps.saber[saberNum].bounceSound[Q_irand( 0, 2 )] );
	}
	else if ( WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
		&& ent->client->ps.saber[saberNum].bounce2Sound[0] )
	{
		G_Sound( ent, ent->client->ps.saber[saberNum].bounce2Sound[Q_irand( 0, 2 )] );
	}
	else if ( !WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
		&& ent->client->ps.saber[saberNum].blockSound[0] )
	{
		G_Sound( ent, ent->client->ps.saber[saberNum].blockSound[Q_irand( 0, 2 )] );
	}
	else if ( WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
		&& ent->client->ps.saber[saberNum].block2Sound[0] )
	{
		G_Sound( ent, ent->client->ps.saber[saberNum].block2Sound[Q_irand( 0, 2 )] );
	}
	else
	{
		G_Sound( ent, G_SoundIndex( va( "sound/weapons/saber/saberblock%d.wav", index ) ) );
	}
}

void WP_SaberBounceSound( gentity_t *ent, gentity_t *hitEnt, gentity_t *playOnEnt, int saberNum, int bladeNum, qboolean doForce )
{
	int index = 1;
	if ( !ent || !ent->client )
	{
		return;
	}
	index = Q_irand( 1, 3 );

	if ( !playOnEnt )
	{
		playOnEnt = ent;
	}
	//NOTE: we don't allow overriding of the saberbounce sound, but since it's just a variant on the saberblock sound, we use that as the override
	if ( !WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
		&& ent->client->ps.saber[saberNum].blockSound[0] )
	{
		G_Sound( playOnEnt, ent->client->ps.saber[saberNum].blockSound[Q_irand( 0, 2 )] );
	}
	else if ( WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
		&& ent->client->ps.saber[saberNum].block2Sound[0] )
	{
		G_Sound( playOnEnt, ent->client->ps.saber[saberNum].block2Sound[Q_irand( 0, 2 )] );
	}
	else
	{
		G_Sound( playOnEnt, G_SoundIndex( va( "sound/weapons/saber/saberbounce%d.wav", index ) ) );
	}
}

int WP_SaberInitBladeData( gentity_t *ent )
{
	if ( !ent->client )
	{
		return 0;
	}
	if ( 1 )
	{
		VectorClear( ent->client->renderInfo.muzzlePoint );
		VectorClear( ent->client->renderInfo.muzzlePointOld );
		//VectorClear( ent->client->renderInfo.muzzlePointNext );
		VectorClear( ent->client->renderInfo.muzzleDir );
		VectorClear( ent->client->renderInfo.muzzleDirOld );
		//VectorClear( ent->client->renderInfo.muzzleDirNext );
		for ( int saberNum = 0; saberNum < MAX_SABERS; saberNum++ )
		{
			for ( int bladeNum = 0; bladeNum < MAX_BLADES; bladeNum++ )
			{
				VectorClear( ent->client->ps.saber[saberNum].blade[bladeNum].muzzlePoint );
				VectorClear( ent->client->ps.saber[saberNum].blade[bladeNum].muzzlePointOld );
				VectorClear( ent->client->ps.saber[saberNum].blade[bladeNum].muzzleDir );
				VectorClear( ent->client->ps.saber[saberNum].blade[bladeNum].muzzleDirOld );
				ent->client->ps.saber[saberNum].blade[bladeNum].lengthOld = ent->client->ps.saber[saberNum].blade[bladeNum].length = 0;
				if ( !ent->client->ps.saber[saberNum].blade[bladeNum].lengthMax )
				{
					if ( ent->client->NPC_class == CLASS_DESANN )
					{//longer saber
						ent->client->ps.saber[saberNum].blade[bladeNum].lengthMax = 48;
					}
					else if ( ent->client->NPC_class == CLASS_REBORN )
					{//shorter saber
						ent->client->ps.saber[saberNum].blade[bladeNum].lengthMax = 32;
					}
					else
					{//standard saber length
						ent->client->ps.saber[saberNum].blade[bladeNum].lengthMax = 40;
					}
				}
			}
		}
		ent->client->ps.saberLockEnemy = ENTITYNUM_NONE;
		ent->client->ps.saberLockTime = 0;
		if ( ent->s.number )
		{
			if ( !ent->client->ps.saberAnimLevel )
			{
				if ( ent->client->NPC_class == CLASS_DESANN )
				{
					ent->client->ps.saberAnimLevel = SS_DESANN;
				}
				else if ( ent->client->NPC_class == CLASS_TAVION )
				{
					ent->client->ps.saberAnimLevel = SS_TAVION;
				}
				else if ( ent->client->NPC_class == CLASS_ALORA )
				{
					ent->client->ps.saberAnimLevel = SS_DUAL;
				}
				//FIXME: CLASS_CULTIST instead of this Q_stricmpn?
				else if ( !Q_stricmpn( "cultist", ent->NPC_type, 7 ) )
				{//should already be set in the .npc file
					ent->client->ps.saberAnimLevel = Q_irand( SS_FAST, SS_STRONG );
				}
				else if ( ent->NPC && ent->client->playerTeam == TEAM_ENEMY && (ent->NPC->rank == RANK_CIVILIAN || ent->NPC->rank == RANK_LT_JG) )
				{//grunt and fencer always uses quick attacks
					ent->client->ps.saberAnimLevel = SS_FAST;
				}
				else if ( ent->NPC && ent->client->playerTeam == TEAM_ENEMY && (ent->NPC->rank == RANK_CREWMAN || ent->NPC->rank == RANK_ENSIGN) )
				{//acrobat & force-users always use medium attacks
					ent->client->ps.saberAnimLevel = SS_MEDIUM;
				}
				else if ( ent->client->playerTeam == TEAM_ENEMY && ent->client->NPC_class == CLASS_SHADOWTROOPER )
				{//shadowtroopers
					ent->client->ps.saberAnimLevel = Q_irand( SS_FAST, SS_STRONG );
				}
				else if ( ent->NPC && ent->client->playerTeam == TEAM_ENEMY && ent->NPC->rank == RANK_LT )
				{//boss always starts with strong attacks
					ent->client->ps.saberAnimLevel = SS_STRONG;
				}
				else if ( ent->client->NPC_class == CLASS_PLAYER )
				{
					ent->client->ps.saberAnimLevel = g_entities[0].client->ps.saberAnimLevel;
				}
				else
				{//?
					ent->client->ps.saberAnimLevel = Q_irand( SS_FAST, SS_STRONG );
				}
			}
		}
		else
		{
			if ( !ent->client->ps.saberAnimLevel )
			{//initialize, but don't reset
				if (ent->s.number < MAX_CLIENTS)
				{
					if (!ent->client->ps.saberStylesKnown)
					{
						ent->client->ps.saberStylesKnown = (1<<SS_MEDIUM);
					}


					if (ent->client->ps.saberStylesKnown & (1<<SS_FAST))
					{
						ent->client->ps.saberAnimLevel = SS_FAST;
					}
					else if (ent->client->ps.saberStylesKnown & (1<<SS_STRONG))
					{
						ent->client->ps.saberAnimLevel = SS_STRONG;
					}
					else
					{
						ent->client->ps.saberAnimLevel = SS_MEDIUM;
					}

				}
				else
				{
					ent->client->ps.saberAnimLevel = SS_MEDIUM;
				}
			}

			cg.saberAnimLevelPending = ent->client->ps.saberAnimLevel;
			if ( ent->client->sess.missionStats.weaponUsed[WP_SABER] <= 0 )
			{//let missionStats know that we actually do have the saber, even if we never use it
				ent->client->sess.missionStats.weaponUsed[WP_SABER] = 1;
			}
		}
		ent->client->ps.saberAttackChainCount = 0;

		if ( ent->client->ps.saberEntityNum <= 0 || ent->client->ps.saberEntityNum >= ENTITYNUM_WORLD )
		{//FIXME: if you do have a saber already, be sure to re-set the model if it's changed (say, via a script).
			gentity_t *saberent = G_Spawn();
			ent->client->ps.saberEntityNum = saberent->s.number;
			saberent->classname = "lightsaber";

			saberent->s.eType = ET_GENERAL;
			saberent->svFlags = SVF_USE_CURRENT_ORIGIN;
			saberent->s.weapon = WP_SABER;
			saberent->owner = ent;
			saberent->s.otherEntityNum = ent->s.number;
			//clear the enemy
			saberent->enemy = NULL;

			saberent->clipmask = MASK_SOLID | CONTENTS_LIGHTSABER;
			saberent->contents = CONTENTS_LIGHTSABER;//|CONTENTS_SHOTCLIP;

			VectorSet( saberent->mins, -3.0f, -3.0f, -3.0f );
			VectorSet( saberent->maxs, 3.0f, 3.0f, 3.0f );
			saberent->mass = 10;//necc?

			saberent->s.eFlags |= EF_NODRAW;
			saberent->svFlags |= SVF_NOCLIENT;
/*
Ghoul2 Insert Start
*/
			saberent->playerModel = -1;
			WP_SetSaberEntModelSkin( ent, saberent );

			// set up a bolt on the end so we can get where the sabre muzzle is - we can assume this is always bolt 0
			gi.G2API_AddBolt( &saberent->ghoul2[0], "*flash", qfalse );
			//gi.G2API_SetLodBias( &saberent->ghoul2[0], 0 );
			if ( ent->client->ps.dualSabers )
			{
				//int saber2 =
				G_ModelIndex( ent->client->ps.saber[1].model );
				//gi.G2API_InitGhoul2Model( saberent->ghoul2, ent->client->ps.saber[1].model, saber2 );
				// set up a bolt on the end so we can get where the sabre muzzle is - we can assume this is always bolt 0
				//gi.G2API_AddBolt( &saberent->ghoul2[0], "*flash", qfalse );
				//gi.G2API_SetLodBias( &saberent->ghoul2[0], 0 );
			}

/*
Ghoul2 Insert End
*/

			ent->client->ps.saberInFlight = qfalse;
			ent->client->ps.saberEntityDist = 0;
			ent->client->ps.saberEntityState = SES_LEAVING;

			ent->client->ps.saberMove = ent->client->ps.saberMoveNext = LS_NONE;

			//FIXME: need a think function to create alerts when turned on or is on, etc.
		}
		else
		{//already have one, might just be changing sabers, register the model and skin and use them if different from what we're using now.
			WP_SetSaberEntModelSkin( ent, &g_entities[ent->client->ps.saberEntityNum] );
		}
	}
	else
	{
		ent->client->ps.saberEntityNum = ENTITYNUM_NONE;
		ent->client->ps.saberInFlight = qfalse;
		ent->client->ps.saberEntityDist = 0;
		ent->client->ps.saberEntityState = SES_LEAVING;
	}

	if ( ent->client->ps.dualSabers )
	{
		return 2;
	}

	return 1;
}

void WP_SaberUpdateOldBladeData( gentity_t *ent )
{
	if ( ent->client )
	{
		qboolean didEvent = qfalse;
		for ( int saberNum = 0; saberNum < 2; saberNum++ )
		{
			for ( int bladeNum = 0; bladeNum < ent->client->ps.saber[saberNum].numBlades; bladeNum++ )
			{
				VectorCopy( ent->client->ps.saber[saberNum].blade[bladeNum].muzzlePoint, ent->client->ps.saber[saberNum].blade[bladeNum].muzzlePointOld );
				VectorCopy( ent->client->ps.saber[saberNum].blade[bladeNum].muzzleDir, ent->client->ps.saber[saberNum].blade[bladeNum].muzzleDirOld );
				if ( !didEvent )
				{
					if ( ent->client->ps.saber[saberNum].blade[bladeNum].lengthOld <= 0 && ent->client->ps.saber[saberNum].blade[bladeNum].length > 0 )
					{//just turned on
						//do sound event
						vec3_t	saberOrg;
						VectorCopy( g_entities[ent->client->ps.saberEntityNum].currentOrigin, saberOrg );
						if ( (!ent->client->ps.saberInFlight && ent->client->ps.groundEntityNum == ENTITYNUM_WORLD)//holding saber and on ground
							|| g_entities[ent->client->ps.saberEntityNum].s.pos.trType == TR_STATIONARY )//saber out there somewhere and on ground
						{//a ground alert
							AddSoundEvent( ent, saberOrg, 256, AEL_SUSPICIOUS, qfalse, qtrue );
						}
						else
						{//an in-air alert
							AddSoundEvent( ent, saberOrg, 256, AEL_SUSPICIOUS );
						}
						didEvent = qtrue;
					}
				}
				ent->client->ps.saber[saberNum].blade[bladeNum].lengthOld = ent->client->ps.saber[saberNum].blade[bladeNum].length;
			}
		}
		VectorCopy( ent->client->renderInfo.muzzlePoint, ent->client->renderInfo.muzzlePointOld );
		VectorCopy( ent->client->renderInfo.muzzleDir, ent->client->renderInfo.muzzleDirOld );
	}
}



//SABER DAMAGE==============================================================================
//SABER DAMAGE==============================================================================
//SABER DAMAGE==============================================================================
//SABER DAMAGE==============================================================================
//SABER DAMAGE==============================================================================
//SABER DAMAGE==============================================================================
int WPDEBUG_SaberColor( saber_colors_t saberColor )
{
	switch( (int)(saberColor) )
	{
		case SABER_RED:
			return 0x000000ff;
			break;
		case SABER_ORANGE:
			return 0x000088ff;
			break;
		case SABER_YELLOW:
			return 0x0000ffff;
			break;
		case SABER_GREEN:
			return 0x0000ff00;
			break;
		case SABER_BLUE:
			return 0x00ff0000;
			break;
		case SABER_PURPLE:
			return 0x00ff00ff;
			break;
		default:
			return 0x00ffffff;//white
			break;
	}
}

qboolean WP_GetSaberDeflectionAngle( gentity_t *attacker, gentity_t *defender )
{
	vec3_t	temp, att_SaberBase, att_StartPos, saberMidNext, att_HitDir, att_HitPos, def_BladeDir;
	float	att_SaberHitLength, hitDot;

	if ( !attacker || !attacker->client || attacker->client->ps.saberInFlight || attacker->client->ps.SaberLength() <= 0 )
	{
		return qfalse;
	}
	if ( !defender || !defender->client || defender->client->ps.saberInFlight || defender->client->ps.SaberLength() <= 0 )
	{
		return qfalse;
	}
	if ( PM_SuperBreakLoseAnim( attacker->client->ps.torsoAnim )
		|| PM_SuperBreakWinAnim( attacker->client->ps.torsoAnim ) )
	{
		return qfalse;
	}
	attacker->client->ps.saberBounceMove = LS_NONE;

	//get the attacker's saber base pos at time of impact
	VectorSubtract( attacker->client->renderInfo.muzzlePoint, attacker->client->renderInfo.muzzlePointOld, temp );
	VectorMA( attacker->client->renderInfo.muzzlePointOld, saberHitFraction, temp, att_SaberBase );

	//get the position along the length of the blade where the hit occured
	att_SaberHitLength = Distance( saberHitLocation, att_SaberBase )/attacker->client->ps.SaberLength();

	//now get the start of that midpoint in the swing and the actual impact point in the swing (shouldn't the latter just be saberHitLocation?)
	VectorMA( attacker->client->renderInfo.muzzlePointOld, att_SaberHitLength, attacker->client->renderInfo.muzzleDirOld, att_StartPos );
	VectorMA( attacker->client->renderInfo.muzzlePoint, att_SaberHitLength, attacker->client->renderInfo.muzzleDir, saberMidNext );
	VectorSubtract( saberMidNext, att_StartPos, att_HitDir );
	VectorMA( att_StartPos, saberHitFraction, att_HitDir, att_HitPos );
	VectorNormalize( att_HitDir );

	//get the defender's saber dir at time of impact
	VectorSubtract( defender->client->renderInfo.muzzleDirOld, defender->client->renderInfo.muzzleDir, temp );
	VectorMA( defender->client->renderInfo.muzzleDirOld, saberHitFraction, temp, def_BladeDir );

	//now compare
	hitDot = DotProduct( att_HitDir, def_BladeDir );
	if ( hitDot < 0.25f && hitDot > -0.25f )
	{//hit pretty much perpendicular, pop straight back
		attacker->client->ps.saberBounceMove = PM_SaberBounceForAttack( attacker->client->ps.saberMove );
		return qfalse;
	}
	else
	{//a deflection
		vec3_t	att_Right, att_Up, att_DeflectionDir;
		float	swingRDot, swingUDot;

		//get the direction of the deflection
		VectorScale( def_BladeDir, hitDot, att_DeflectionDir );
		//get our bounce straight back direction
		VectorScale( att_HitDir, -1.0f, temp );
		//add the bounce back and deflection
		VectorAdd( att_DeflectionDir, temp, att_DeflectionDir );
		//normalize the result to determine what direction our saber should bounce back toward
		VectorNormalize( att_DeflectionDir );

		//need to know the direction of the deflectoin relative to the attacker's facing
		VectorSet( temp, 0, attacker->client->ps.viewangles[YAW], 0 );//presumes no pitch!
		AngleVectors( temp, NULL, att_Right, att_Up );
		swingRDot = DotProduct( att_Right, att_DeflectionDir );
		swingUDot = DotProduct( att_Up, att_DeflectionDir );

		if ( swingRDot > 0.25f )
		{//deflect to right
			if ( swingUDot > 0.25f )
			{//deflect to top
				attacker->client->ps.saberBounceMove = LS_D1_TR;
			}
			else if ( swingUDot < -0.25f )
			{//deflect to bottom
				attacker->client->ps.saberBounceMove = LS_D1_BR;
			}
			else
			{//deflect horizontally
				attacker->client->ps.saberBounceMove = LS_D1__R;
			}
		}
		else if ( swingRDot < -0.25f )
		{//deflect to left
			if ( swingUDot > 0.25f )
			{//deflect to top
				attacker->client->ps.saberBounceMove = LS_D1_TL;
			}
			else if ( swingUDot < -0.25f )
			{//deflect to bottom
				attacker->client->ps.saberBounceMove = LS_D1_BL;
			}
			else
			{//deflect horizontally
				attacker->client->ps.saberBounceMove = LS_D1__L;
			}
		}
		else
		{//deflect in middle
			if ( swingUDot > 0.25f )
			{//deflect to top
				attacker->client->ps.saberBounceMove = LS_D1_T_;
			}
			else if ( swingUDot < -0.25f )
			{//deflect to bottom
				attacker->client->ps.saberBounceMove = LS_D1_B_;
			}
			else
			{//deflect horizontally?  Well, no such thing as straight back in my face, so use top
				if ( swingRDot > 0 )
				{
					attacker->client->ps.saberBounceMove = LS_D1_TR;
				}
				else if ( swingRDot < 0 )
				{
					attacker->client->ps.saberBounceMove = LS_D1_TL;
				}
				else
				{
					attacker->client->ps.saberBounceMove = LS_D1_T_;
				}
			}
		}
#ifndef FINAL_BUILD
		if ( d_saberCombat->integer )
		{
			gi.Printf( S_COLOR_BLUE"%s deflected from %s to %s\n", attacker->targetname, saberMoveData[attacker->client->ps.saberMove].name, saberMoveData[attacker->client->ps.saberBounceMove].name );
		}
#endif
		return qtrue;
	}
}


void WP_SaberClearDamageForEntNum( gentity_t *attacker, int entityNum, int saberNum, int bladeNum )
{
#ifndef FINAL_BUILD
	if ( d_saberCombat->integer )
	{
		if ( entityNum )
		{
			Com_Printf( "clearing damage for entnum %d\n", entityNum );
		}
	}
#endif// FINAL_BUILD
	if ( g_saberRealisticCombat->integer > 1 )
	{
		return;
	}

	//FIXME: if hit their saber in WP_SaberDamageForTrace, need to still do knockback on them...
	float knockBackScale = 0.0f;
	if ( attacker && attacker->client )
	{
		if ( !WP_SaberBladeUseSecondBladeStyle( &attacker->client->ps.saber[saberNum], bladeNum )
			&& attacker->client->ps.saber[saberNum].knockbackScale > 0.0f )
		{
			knockBackScale = attacker->client->ps.saber[saberNum].knockbackScale;
		}
		else if ( WP_SaberBladeUseSecondBladeStyle( &attacker->client->ps.saber[saberNum], bladeNum )
			&& attacker->client->ps.saber[saberNum].knockbackScale2 > 0.0f )
		{
			knockBackScale = attacker->client->ps.saber[saberNum].knockbackScale2;
		}
	}

	for ( int i = 0; i < numVictims; i++ )
	{
		if ( victimEntityNum[i] == entityNum )
		{
			//hold on a sec, let's still do any accumulated knockback
			if ( knockBackScale )
			{
				gentity_t *victim = &g_entities[victimEntityNum[i]];
				if ( victim && victim->client )
				{
					vec3_t center, dirToCenter;
					float	knockDownThreshHold, knockback = knockBackScale * totalDmg[i] * 0.5f;

					VectorAdd( victim->absmin, victim->absmax, center );
					VectorScale( center, 0.5, center );
					VectorSubtract( victim->currentOrigin, saberHitLocation, dirToCenter );
					VectorNormalize( dirToCenter );
					G_Throw( victim, dirToCenter, knockback );
					if ( victim->client->ps.groundEntityNum != ENTITYNUM_NONE
						&& dirToCenter[2] <= 0  )
					{//hit downward on someone who is standing on firm ground, so more likely to knock them down
						knockDownThreshHold = Q_irand( 25, 50 );
					}
					else
					{
						knockDownThreshHold = Q_irand( 75, 125 );
					}

					if ( knockback > knockDownThreshHold )
					{
						G_Knockdown( victim, attacker, dirToCenter, 350, qtrue );
					}
				}
			}
			//now clear everything
			totalDmg[i] = 0;//no damage
			hitLoc[i] = HL_NONE;
			hitDismemberLoc[i] = HL_NONE;
			hitDismember[i] = qfalse;
			victimEntityNum[i] = ENTITYNUM_NONE;//like we never hit him
		}
	}
}

extern float damageModifier[];
extern float hitLocHealthPercentage[];
extern float saberLocationMod[];
extern qboolean G_DoDismemberment( gentity_t *self, vec3_t point, int mod, int damage, int hitLoc, qboolean force );
extern void G_FlagNPCAsKilled( gentity_t *self, gentity_t *attacker );
void WP_DropWeapon( gentity_t *dropper, vec3_t velocity );
extern cvar_t *g_saberMaxPlayerHitPct;
extern cvar_t *g_debugDamage;
extern cvar_t *g_debugBlock;
extern qboolean G_InCinematicSaberAnim( gentity_t *self );
qboolean WP_IsHeavyAttack( const gentity_t *ent );

/*
g_debugBlock (JKEnhanced)

Why a saber hit got through the player's block. Branches that refuse or break a block record a
reason with WP_NoteBlockFail; when a saber hit then damages the player while block is held or
active, WP_ReportBlockFail prints one line: the recorded reason (if recent), plus the state that
decides blocking at that moment - block button / active flag, Force, which block box was up (the
wide box needs block active, Force > 0 and no weaponTime; otherwise only the blade itself
blocks), the player's saber move, staggers, and where the attacker was.
*/
static const char	*s_blockFailNote = NULL;
static int			s_blockFailNoteTime = -1000;

void WP_NoteBlockFail( const gentity_t *defender, const char *reason )
{
	if ( defender && defender->s.number == 0 && g_debugBlock && g_debugBlock->integer )
	{
		s_blockFailNote = reason;
		s_blockFailNoteTime = level.time;
	}
}

/*
==========================================================================
Saber rules: "a blade in the way" (JKEnhanced, g_saberRules 1)

Plans and Handoff\Saber_System_Concept.md. Stage 1: Guard is a state.

A saber fighter's Guard is up or it isn't, and you can see which. A raised Guard stops every
in-hand swing from the front, wherever the two blades happen to be; each stop costs the
blocker Force, by the attacker's style (ext_data/saberanims.cfg). Guard fails in three ways only:
the blocker is Open (off balance), has no Force, or the swing comes from behind the guard arc.
The player's Guard is the block button. NPCs raise theirs a reaction time after the enemy's
wind-up starts (Jedi_GuardReact, AI_Jedi.cpp). With no Guard up, a blade that reaches the body
cuts: a saber held at ready no longer intercepts swings.

Open is the one off-balance state (the "parryStaggered" timer): no guard, no attack, one
animation per style. A guard out of Force, a perfect-parried attacker and everything that
drained the old NPC guard meter (G_DamageGuard) all end there.
==========================================================================
*/
extern cvar_t	*g_saberRules;
extern cvar_t	*g_saberOpenShort;
extern cvar_t	*g_saberOpenMedium;
extern cvar_t	*g_saberOpenLong;
extern cvar_t	*g_saberGuardRegenPause;
extern cvar_t	*g_saberHeavyStopCost;
extern cvar_t	*g_npcForcePool;
extern cvar_t	*g_guardBreakSound;
extern qboolean G_IsSaberBoss( const gentity_t *ent, qboolean includeElites );
static void WP_SaberCamKick( float intensity, int duration );	//(defined with the hit-stop, further down)

qboolean WP_SaberRules( void )
{
	return (qboolean)( g_saberRules && g_saberRules->integer );
}

// one row per style; ext_data/saberanims.cfg overrides any of it
// (guard pose -1 = the style's own saber stance; Neutral is hands at the sides)
#define PH( a, b, c, d )	{ LS_A_##a, LS_A_##b, LS_A_##c, LS_A_##d }
static const saberStyleSheet_t s_defaultStyleSheet[SS_NUM_SABER_STYLES] =
{
	//	stop	arc		guard	Open pose		Neutral			windup	recover	weight	phrases
	{	20,		200,	-1,		-1,				BOTH_STAND1,	300,	500,	2,		0 },	// SS_NONE
	{	12,		200,	-1,		BOTH_H1_S1_TR,	BOTH_STAND1,	200,	350,	1,		4, { 4, 3, 4, 3 },	// SS_FAST: long, light strings
		{ PH( L2R, R2L, L2R, T2B ), PH( TR2BL, BL2TR, R2L, T2B ), PH( BR2TL, TL2BR, R2L, L2R ), PH( T2B, R2L, L2R, T2B ) } },
	{	20,		200,	-1,		BOTH_H1_S1_T_,	BOTH_STAND1,	300,	500,	2,		4, { 3, 2, 3, 2 },	// SS_MEDIUM
		{ PH( TL2BR, R2L, T2B, T2B ), PH( L2R, R2L, T2B, T2B ), PH( TR2BL, BL2TR, T2B, T2B ), PH( BR2TL, TL2BR, T2B, T2B ) } },
	{	32,		200,	-1,		BOTH_H1_S1_BL,	BOTH_STAND1,	450,	750,	3,		4, { 1, 2, 1, 2 },	// SS_STRONG: one or two big swings
		{ PH( T2B, T2B, T2B, T2B ), PH( TL2BR, TR2BL, T2B, T2B ), PH( R2L, T2B, T2B, T2B ), PH( BL2TR, T2B, T2B, T2B ) } },
	{	40,		200,	-1,		BOTH_H1_S1_BR,	BOTH_STAND1,	450,	800,	3,		3, { 2, 1, 2 },		// SS_DESANN
		{ PH( T2B, R2L, T2B, T2B ), PH( TR2BL, T2B, T2B, T2B ), PH( L2R, T2B, T2B, T2B ) } },
	{	24,		200,	-1,		BOTH_H1_S1_TL,	BOTH_STAND1,	200,	450,	2,		3, { 3, 2, 4 },		// SS_TAVION
		{ PH( TL2BR, R2L, TL2BR, T2B ), PH( R2L, T2B, T2B, T2B ), PH( TR2BL, BL2TR, R2L, T2B ) } },
	{	20,		200,	-1,		BOTH_H1_S1_T_,	BOTH_STAND1,	300,	500,	2,		2, { 3, 2 },		// SS_KATARN
		{ PH( TL2BR, R2L, T2B, T2B ), PH( L2R, R2L, T2B, T2B ) } },
	{	16,		260,	-1,		BOTH_H6_S6_T_,	BOTH_STAND1,	250,	450,	2,		3, { 4, 3, 3 },		// SS_DUAL: many light hits
		{ PH( L2R, R2L, L2R, R2L ), PH( TL2BR, TR2BL, T2B, T2B ), PH( BR2TL, BL2TR, R2L, T2B ) } },
	{	18,		280,	-1,		BOTH_H7_S7_T_,	BOTH_STAND1,	300,	500,	2,		3, { 3, 4, 2 },		// SS_STAFF
		{ PH( R2L, L2R, T2B, T2B ), PH( TL2BR, BR2TL, TR2BL, BL2TR ), PH( T2B, R2L, T2B, T2B ) } },
};
#undef PH
static saberStyleSheet_t	s_styleSheet[SS_NUM_SABER_STYLES];
static qboolean				s_styleSheetLoaded = qfalse;

const saberStyleSheet_t *WP_SaberStyleSheet( int saberAnimLevel )
{
	if ( saberAnimLevel < SS_NONE || saberAnimLevel >= SS_NUM_SABER_STYLES )
	{
		saberAnimLevel = SS_MEDIUM;
	}
	return s_styleSheetLoaded ? &s_styleSheet[saberAnimLevel] : &s_defaultStyleSheet[saberAnimLevel];
}

static void WP_ParseSaberStyleBlock( const char **p, saberStyleSheet_t *sheet )
{
	qboolean ownPhrases = qfalse;
	const char *token = COM_ParseExt( p, qtrue );
	if ( Q_stricmp( token, "{" ) )
	{
		Com_Printf( S_COLOR_YELLOW"saberanims.cfg: expected { after style NAME\n" );
		return;
	}
	while ( 1 )
	{
		token = COM_ParseExt( p, qtrue );
		if ( !token[0] || !Q_stricmp( token, "}" ) )
		{
			return;
		}
		int n = 0;
		if ( !Q_stricmp( token, "stopCost" ) )		{ if ( !COM_ParseInt( p, &n ) ) sheet->stopCost = n; }
		else if ( !Q_stricmp( token, "guardArc" ) )	{ if ( !COM_ParseInt( p, &n ) ) sheet->guardArc = n; }
		else if ( !Q_stricmp( token, "windup" ) )	{ if ( !COM_ParseInt( p, &n ) ) sheet->windup = n; }
		else if ( !Q_stricmp( token, "recover" ) )	{ if ( !COM_ParseInt( p, &n ) ) sheet->recover = n; }
		else if ( !Q_stricmp( token, "weight" ) )	{ if ( !COM_ParseInt( p, &n ) ) sheet->weight = n; }
		else if ( !Q_stricmp( token, "phrase" ) )
		{//phrase TL2BR R2L T2B: up to 4 swings on one line; a style's first phrase line replaces the built-in set
			static const char *swingNames[7] = { "TL2BR", "L2R", "BL2TR", "BR2TL", "R2L", "TR2BL", "T2B" };
			if ( !ownPhrases )
			{
				ownPhrases = qtrue;
				sheet->numPhrases = 0;
			}
			int len = 0;
			int swings[SABER_PHRASE_SWINGS];
			while ( 1 )
			{
				token = COM_ParseExt( p, qfalse );
				if ( !token[0] )
				{
					break;
				}
				int s;
				for ( s = 0; s < 7; s++ )
				{
					if ( !Q_stricmp( token, swingNames[s] ) )
					{
						break;
					}
				}
				if ( s == 7 )
				{
					Com_Printf( S_COLOR_YELLOW"saberanims.cfg: unknown swing '%s' (TL2BR L2R BL2TR BR2TL R2L TR2BL T2B)\n", token );
				}
				else if ( len < SABER_PHRASE_SWINGS )
				{
					swings[len++] = LS_A_TL2BR + s;
				}
			}
			if ( len > 0 && sheet->numPhrases < SABER_STYLE_PHRASES )
			{
				sheet->phraseLen[sheet->numPhrases] = len;
				memcpy( sheet->phrase[sheet->numPhrases], swings, sizeof( int ) * len );
				sheet->numPhrases++;
			}
		}
		else if ( !Q_stricmp( token, "guardAnim" ) || !Q_stricmp( token, "openAnim" ) || !Q_stricmp( token, "neutralAnim" ) )
		{//an animation name from anims.h, or "none" for the stock pose
			int *dest = !Q_stricmp( token, "guardAnim" ) ? &sheet->guardAnim
				: ( !Q_stricmp( token, "openAnim" ) ? &sheet->openAnim : &sheet->neutralAnim );
			token = COM_ParseExt( p, qfalse );
			if ( !Q_stricmp( token, "none" ) )
			{
				*dest = -1;
			}
			else
			{
				const int anim = GetIDForString( animTable, token );
				if ( anim < 0 )
				{
					Com_Printf( S_COLOR_YELLOW"saberanims.cfg: unknown animation '%s'\n", token );
				}
				else
				{
					*dest = anim;
				}
			}
		}
		else
		{
			Com_Printf( S_COLOR_YELLOW"saberanims.cfg: unknown key '%s'\n", token );
			SkipRestOfLine( p );
		}
	}
}

void WP_LoadSaberStyleSheet( void )
{
	static const char *styleNames[SS_NUM_SABER_STYLES] = { "none", "fast", "medium", "strong", "desann", "tavion", "katarn", "dual", "staff" };
	memcpy( s_styleSheet, s_defaultStyleSheet, sizeof( s_styleSheet ) );
	s_styleSheetLoaded = qtrue;

	char *buffer = NULL;
	const int len = gi.FS_ReadFile( "ext_data/saberanims.cfg", (void **)&buffer );
	if ( len <= 0 || !buffer )
	{
		Com_Printf( S_COLOR_YELLOW"WP_LoadSaberStyleSheet: no ext_data/saberanims.cfg, using built-in values\n" );
		return;
	}
	COM_BeginParseSession( "saberanims.cfg" );
	const char *p = buffer;
	while ( 1 )
	{
		const char *token = COM_ParseExt( &p, qtrue );
		if ( !token[0] )
		{
			break;
		}
		if ( Q_stricmp( token, "style" ) )
		{
			Com_Printf( S_COLOR_YELLOW"saberanims.cfg: expected 'style NAME', got '%s'\n", token );
			SkipRestOfLine( &p );
			continue;
		}
		token = COM_ParseExt( &p, qfalse );
		int style = -1;
		for ( int i = SS_FAST; i < SS_NUM_SABER_STYLES; i++ )
		{
			if ( !Q_stricmp( token, styleNames[i] ) )
			{
				style = i;
				break;
			}
		}
		if ( style == -1 )
		{
			Com_Printf( S_COLOR_YELLOW"saberanims.cfg: unknown style '%s'\n", token );
			break;
		}
		WP_ParseSaberStyleBlock( &p, &s_styleSheet[style] );
	}
	COM_EndParseSession();
	gi.FS_FreeFile( buffer );
}

//ms of Open left (0 = not Open)
int WP_SaberOpenTime( gentity_t *ent )
{
	if ( !ent || !ent->client )
	{
		return 0;
	}
	int left = TIMER_Done( ent, "parryStaggered" ) ? 0 : TIMER_Get( ent, "parryStaggered" ) - level.time;
	if ( ent->NPC && ent->NPC->guardBreakTime - level.time > left )
	{
		left = ent->NPC->guardBreakTime - level.time;
	}
	return Q_max( 0, left );
}

/*
WP_SaberSetOpen

Knocks a fighter Open for ms: guard down, no parry (WP_SaberParry), no attack or block
(ClientThink_real for the player, "attackDelay" for NPCs). Pmove plays the style's Open pose for
as long as it lasts (PM_WeaponLightsaber, BLOCKED_PARRY_BROKEN). Never shortens one already running.
*/
void WP_SaberSetOpen( gentity_t *ent, int ms )
{
	if ( !ent || !ent->client || ent->health <= 0 || ms <= 0 )
	{
		return;
	}
	if ( TIMER_Done( ent, "parryStaggered" ) || TIMER_Get( ent, "parryStaggered" ) < level.time + ms )
	{
		TIMER_Set( ent, "parryStaggered", ms );
	}
	if ( ent->NPC && ( TIMER_Done( ent, "attackDelay" ) || TIMER_Get( ent, "attackDelay" ) < level.time + ms ) )
	{
		TIMER_Set( ent, "attackDelay", ms );
	}
	playerState_t *ps = &ent->client->ps;
	ps->saberBlockingTime = 0;
	if ( ps->weapon == WP_SABER && !ps->saberInFlight && !PM_InKnockDown( ps ) && ps->saberLockTime < level.time )
	{
		ps->saberBlocked = BLOCKED_PARRY_BROKEN;
		ps->saberBounceMove = LS_NONE;
	}
}

typedef enum
{
	GUARD_UP = 0,
	GUARD_DOWN,		//not guarding: Neutral, or committed to a swing
	GUARD_OPEN,		//off balance
	GUARD_NO_FORCE,
	GUARD_BEHIND,	//the attacker is outside the guard arc
} guardState_t;

static qboolean WP_SaberSwinging( const playerState_t *ps )
{
	//(a transition between two strikes is part of the swing: the blade is on its way to the next one)
	return (qboolean)( PM_SaberInStart( ps->saberMove ) || PM_SaberInAttack( ps->saberMove )
		|| PM_SaberInTransition( ps->saberMove ) || PM_SaberInSpecialAttack( ps->torsoAnim ) );
}

//attacker NULL: is the Guard up at all (no arc test)
static guardState_t WP_GuardState( gentity_t *ent, const gentity_t *attacker )
{
	if ( !ent || !ent->client || ent->health <= 0 )
	{
		return GUARD_DOWN;
	}
	playerState_t *ps = &ent->client->ps;
	if ( ps->weapon != WP_SABER || !ps->SaberActive()
		|| ( ps->saberInFlight && !( ps->dualSabers && ps->saber[1].Active() ) ) )
	{
		return GUARD_DOWN;
	}
	if ( WP_SaberOpenTime( ent ) > 0 || PM_InKnockDown( ps ) || PM_SaberInBrokenParry( ps->saberMove ) )
	{
		return GUARD_OPEN;
	}
	if ( ps->saberBlockingTime <= level.time || WP_SaberSwinging( ps ) )
	{
		return GUARD_DOWN;
	}
	if ( ps->forcePowerMax > 0 && ps->forcePower <= 0 )
	{
		return GUARD_NO_FORCE;
	}
	if ( attacker )
	{
		vec3_t fwd, toAtt;
		const vec3_t yawOnly = { 0, ps->viewangles[YAW], 0 };
		AngleVectors( yawOnly, fwd, NULL, NULL );
		VectorSubtract( attacker->currentOrigin, ent->currentOrigin, toAtt );
		toAtt[2] = 0;
		if ( VectorNormalize( toAtt ) > 0.0f )
		{
			const float halfArc = WP_SaberStyleSheet( ps->saberAnimLevel )->guardArc * 0.5f;
			if ( RAD2DEG( acos( Com_Clamp( -1.0f, 1.0f, DotProduct( fwd, toAtt ) ) ) ) > halfArc )
			{
				return GUARD_BEHIND;
			}
		}
	}
	return GUARD_UP;
}

qboolean WP_SaberGuardUp( gentity_t *ent )
{
	return (qboolean)( WP_GuardState( ent, NULL ) == GUARD_UP );
}

//when each fighter's current run of swings began (wind-up, strike or the transition between two
//strikes); an NPC's guard reaction time counts from here. Not saved: it restarts with the next swing.
static int s_swingRunStart[MAX_GENTITIES];
//which swing each fighter is on: +1 every time a strike starts (a new attack move, or the same one
//restarting), so attack -> transition -> the same attack is two swings. Transitions and returns keep
//the strike's id, so contact that runs past the end of it is still that swing. Not saved.
static int s_swingId[MAX_GENTITIES];
static int s_swingLastMove[MAX_GENTITIES];
static int s_swingLastTimer[MAX_GENTITIES];

static void WP_SaberTrackSwingRun( gentity_t *self )
{
	const playerState_t *ps = &self->client->ps;
	const int n = self->s.number;
	const qboolean inAttack = (qboolean)( PM_SaberInAttack( ps->saberMove ) || PM_SaberInSpecialAttack( ps->torsoAnim ) );
	if ( inAttack && ( ps->saberMove != s_swingLastMove[n] || ps->torsoAnimTimer > s_swingLastTimer[n] ) )
	{//a new strike
		s_swingId[n]++;
	}
	s_swingLastMove[n] = inAttack ? ps->saberMove : LS_NONE;
	s_swingLastTimer[n] = ps->torsoAnimTimer;
	if ( ps->weapon == WP_SABER && !ps->saberInFlight && self->health > 0
		&& ( WP_SaberSwinging( ps ) || PM_SaberInTransition( ps->saberMove ) ) )
	{
		if ( !s_swingRunStart[n] || s_swingRunStart[n] > level.time )
		{
			s_swingRunStart[n] = level.time;
		}
	}
	else
	{
		s_swingRunStart[n] = 0;
	}
}

//ms since ent's run of swings began, -1 if it isn't swinging
int WP_SaberSwingRunTime( const gentity_t *ent )
{
	if ( !ent || !ent->client || ent->s.number < 0 || ent->s.number >= MAX_GENTITIES )
	{
		return -1;
	}
	const int start = s_swingRunStart[ent->s.number];
	return ( start && start <= level.time ) ? level.time - start : -1;
}

//swings in a row a fighter's guard has stopped (lapses after 3 s without one). An NPC that has
//taken enough answers with an attack of its own; that replaces the old "they read you" knockaway.
static int s_stopsTaken[MAX_GENTITIES];
static int s_stopsTime[MAX_GENTITIES];

int WP_SaberStopsTaken( gentity_t *ent )
{
	if ( !ent || ent->s.number < 0 || ent->s.number >= MAX_GENTITIES )
	{
		return 0;
	}
	const int n = ent->s.number;
	return ( level.time >= s_stopsTime[n] && level.time - s_stopsTime[n] <= 3000 ) ? s_stopsTaken[n] : 0;
}

void WP_SaberStopsClear( gentity_t *ent )
{
	if ( ent && ent->s.number >= 0 && ent->s.number < MAX_GENTITIES )
	{
		s_stopsTaken[ent->s.number] = 0;
	}
}

/*
WP_SaberFollowThrough (saber rules)

The opening after an NPC's attack phrase, made visible: the last swing's end pose is held for
holdMs (the anim stops on its last frame, blade out wide) before the blade comes back. For that
time the blade is spent (WP_SaberDamageTrace does nothing) and, still being in the swing, the
fighter has no guard. left = ms of the swing still to play.
*/
static int s_followFrom[MAX_GENTITIES];
static int s_followUntil[MAX_GENTITIES];

void WP_SaberFollowThrough( gentity_t *ent, int left, int holdMs )
{
	if ( !ent || !ent->client || ent->s.number < 0 || ent->s.number >= MAX_GENTITIES || holdMs <= 0 )
	{
		return;
	}
	playerState_t *ps = &ent->client->ps;
	const int n = ent->s.number;
	s_followFrom[n] = level.time + left;
	s_followUntil[n] = level.time + left + holdMs;
	if ( ps->legsAnim == ps->torsoAnim && ps->legsAnimTimer > 0 )
	{
		ps->legsAnimTimer += holdMs;
	}
	if ( ps->torsoAnimTimer > 0 )
	{
		ps->torsoAnimTimer += holdMs;
	}
	if ( ps->weaponTime > 0 )
	{
		ps->weaponTime += holdMs;
	}
}
void WP_SaberFollowThroughClear( gentity_t *ent )
{//the swing was dropped: no hold, no opening
	if ( ent && ent->s.number >= 0 && ent->s.number < MAX_GENTITIES )
	{
		s_followFrom[ent->s.number] = s_followUntil[ent->s.number] = 0;
	}
}


qboolean WP_SaberInFollowThrough( const gentity_t *ent )
{
	if ( !ent || ent->s.number < 0 || ent->s.number >= MAX_GENTITIES )
	{
		return qfalse;
	}
	const int n = ent->s.number;
	return (qboolean)( level.time >= s_followFrom[n] && level.time < s_followUntil[n]
		&& s_followUntil[n] - level.time < 5000 );
}

//g_debugBlock: why a guard didn't stop a swing. The player's own failures go into the BLOCK FAILED
//line (WP_ReportBlockFail); an NPC's are printed here, once per swing, when the player is the one swinging.
extern const char *Jedi_GuardDownReason( gentity_t *npc, gentity_t *attacker );
static void WP_NoteGuardFail( gentity_t *defender, gentity_t *attacker, guardState_t gs )
{
	if ( defender && defender->NPC && attacker && attacker->s.number == 0 && attacker->client
		&& g_debugBlock && g_debugBlock->integer )
	{
		static int s_move[MAX_GENTITIES];
		static int s_time[MAX_GENTITIES];
		const int d = defender->s.number;
		if ( s_move[d] != attacker->client->ps.saberMove || level.time < s_time[d] || level.time - s_time[d] > 600 )
		{
			s_move[d] = attacker->client->ps.saberMove;
			s_time[d] = level.time;
			gi.Printf( S_COLOR_CYAN "GUARD DOWN" S_COLOR_WHITE " %s (Force %d): %s\n",
				defender->NPC_type ? defender->NPC_type : "?", defender->client->ps.forcePower,
				gs == GUARD_UP ? "his guard was UP and facing you - the cut reached the body anyway (a bug)"
				: gs == GUARD_OPEN ? "OPEN (off balance)"
				: gs == GUARD_NO_FORCE ? "NO FORCE left"
				: gs == GUARD_BEHIND ? "your swing came from BEHIND his guard"
				: Jedi_GuardDownReason( defender, attacker ) );
		}
		return;
	}
	switch ( gs )
	{
	case GUARD_OPEN:
		WP_NoteBlockFail( defender, "guard failed: you were OPEN (off balance)" );
		break;
	case GUARD_NO_FORCE:
		WP_NoteBlockFail( defender, "guard failed: NO FORCE left" );
		break;
	case GUARD_BEHIND:
		WP_NoteBlockFail( defender, "guard failed: the swing came from BEHIND your guard" );
		break;
	case GUARD_DOWN:
		WP_NoteBlockFail( defender, "guard not up (block not held, or you were mid-swing)" );
		break;
	default:
		break;
	}
}

/*
WP_SaberGuardPay

Takes Force off a guard. At 0 the guard collapses: Open (long), with the bark and the clang.
The stop that emptied it still held. NPCs with no Force pool (or g_npcForcePool 0) pay nothing.
*/
static void WP_SaberGuardPay( gentity_t *ent, gentity_t *attacker, int cost )
{
	if ( !ent || !ent->client || ent->health <= 0 || cost <= 0 )
	{
		return;
	}
	playerState_t *ps = &ent->client->ps;
	if ( ps->forcePowerMax <= 0 || ( ent->NPC && ( !g_npcForcePool || !g_npcForcePool->integer ) ) )
	{
		return;
	}
	if ( WP_SaberOpenTime( ent ) > 0 )
	{//already Open: nothing left to wear down
		return;
	}
	ps->forcePower = Q_max( 0, ps->forcePower - cost );
	const int pause = level.time + ( g_saberGuardRegenPause ? g_saberGuardRegenPause->integer : 1000 );
	if ( ps->forcePowerRegenDebounceTime < pause )
	{
		ps->forcePowerRegenDebounceTime = pause;
	}
	if ( ( ent->s.number == 0 && g_debugBlock && g_debugBlock->integer ) || d_saberCombat->integer )
	{
		gi.Printf( "guard: %s pays %d Force (%d left)\n", ent->NPC_type ? ent->NPC_type : "player", cost, ps->forcePower );
	}
	if ( ps->forcePower > 0 )
	{
		return;
	}
	const int openMs = g_saberOpenLong ? g_saberOpenLong->integer : 1500;
	WP_SaberSetOpen( ent, openMs );
	if ( ent->NPC )
	{//boss stages and the target bar read this as "guard broken"; the Force comes back when it ends (NPC_BSJedi_Default)
		ent->NPC->guardBreakTime = level.time + openMs;
	}
	G_AddVoiceEvent( ent, Q_irand( EV_PUSHED1, EV_PUSHED3 ), 500 );
	if ( g_guardBreakSound && g_guardBreakSound->string[0] )
	{
		G_SoundOnEnt( ent, CHAN_AUTO, g_guardBreakSound->string );
	}
	if ( ent->s.number == 0 || ( attacker && attacker->s.number == 0 ) )
	{
		WP_SaberCamKick( 0.6f, 200 );
	}
	if ( ( g_debugBlock && g_debugBlock->integer ) || d_saberCombat->integer )
	{
		gi.Printf( S_COLOR_RED "GUARD COLLAPSED:" S_COLOR_WHITE " %s is out of Force, Open for %d ms\n",
			ent->NPC_type ? ent->NPC_type : "player", openMs );
	}
}

//for callers outside this file (kicks): what a raised guard pays to take a hit that isn't a swing
void WP_SaberGuardCost( gentity_t *ent, gentity_t *attacker, int cost )
{
	WP_SaberGuardPay( ent, attacker, cost );
}

/*
WP_SaberLockPush (saber rules)

One tap in a saber lock. Costs g_saberLockTapCost Force and pushes the lock by the style's
weight (ext_data/saberanims.cfg); with too little Force left the tap does nothing, so a
fighter who runs dry gets pushed out. Returns how far this tap pushes.
*/
extern cvar_t *g_saberLockTapCost;
int WP_SaberLockPush( gentity_t *ent )
{
	playerState_t *ps = &ent->client->ps;
	const int cost = g_saberLockTapCost ? g_saberLockTapCost->integer : 0;
	if ( cost > 0 && ps->forcePowerMax > 0 && !( ent->NPC && ( !g_npcForcePool || !g_npcForcePool->integer ) ) )
	{
		if ( ps->forcePower < cost )
		{
			return 0;
		}
		ps->forcePower -= cost;
		if ( ps->forcePowerRegenDebounceTime < level.time + 500 )
		{
			ps->forcePowerRegenDebounceTime = level.time + 500;
		}
	}
	return Q_max( 1, WP_SaberStyleSheet( ps->saberAnimLevel )->weight );
}

/*
WP_SaberGuardStop

A raised Guard stops an in-hand swing. Returns qtrue if the swing glanced off (deflection sound).
- A perfect parry (the player's timed tap) is free and
  leaves the attacker Open: short, or medium if the swing was a heavy attack or kata.
- Anything else costs the blocker Force once per swing: the attacker's style stopCost
  (g_saberHeavyStopCost for a heavy), x0.85 / x0.7 at Saber Defense 2 / 3.
*/
qboolean WP_SaberParry( gentity_t *victim, gentity_t *attacker, int saberNum, int bladeNum );
qboolean WP_InPerfectParryWindowVs( const gentity_t *self, const gentity_t *attacker, qboolean missile );
extern qboolean WP_NPCPerfectParry( gentity_t *self );
void WP_PerfectParryReward( gentity_t *defender, gentity_t *attacker, qboolean heavy, qboolean ranged );
void WP_PerfectParrySuccess( gentity_t *self, vec3_t impactPoint, vec3_t normal );
static qboolean WP_SaberGuardStop( gentity_t *attacker, gentity_t *defender, int saberNum, int bladeNum, qboolean heavy, int attackPower )
{
	static int	s_stopBy[MAX_GENTITIES];
	static int	s_stopMove[MAX_GENTITIES];
	static int	s_stopTime[MAX_GENTITIES];
	playerState_t	*aps = &attacker->client->ps;
	playerState_t	*dps = &defender->client->ps;
	const int		d = defender->s.number;
	const qboolean	perfect = (qboolean)( WP_InPerfectParryWindowVs( defender, attacker, qfalse ) );
	qboolean		deflected = qfalse;

	if ( !perfect && attackPower < FORCE_LEVEL_3 )
	{//lighter swings can glance off at an angle
		deflected = WP_GetSaberDeflectionAngle( attacker, defender );
	}
	aps->saberEventFlags |= SEF_BLOCKED;
	WP_SaberParry( defender, attacker, saberNum, bladeNum );

	if ( perfect )
	{
		dps->saberBounceMove = PM_KnockawayForParry( dps->saberBlocked );
		WP_PerfectParryReward( defender, attacker, heavy, qfalse );
		WP_PerfectParrySuccess( defender, saberHitLocation, saberHitNormal );
		if ( defender->s.number == 0 || attacker->s.number == 0 )
		{
			WP_SaberCamKick( 0.3f, 100 );
		}
		return qfalse;
	}

	//once per swing: the blade stays in contact for several frames
	if ( s_stopBy[d] == attacker->s.number + 1 && s_stopMove[d] == aps->saberMove
		&& level.time >= s_stopTime[d] && level.time - s_stopTime[d] < 1000 )
	{
		return deflected;
	}
	s_stopBy[d] = attacker->s.number + 1;
	s_stopMove[d] = aps->saberMove;
	s_stopTime[d] = level.time;
	//how many swings in a row this guard has stopped: an NPC answers after enough (Jedi_PhraseThink)
	s_stopsTaken[d] = ( level.time < s_stopsTime[d] || level.time - s_stopsTime[d] > 3000 ) ? 1 : s_stopsTaken[d] + 1;
	s_stopsTime[d] = level.time;

	int cost = heavy ? ( g_saberHeavyStopCost ? g_saberHeavyStopCost->integer : 50 )
		: WP_SaberStyleSheet( aps->saberAnimLevel )->stopCost;
	const int defLevel = dps->forcePowerLevel[FP_SABER_DEFENSE];
	if ( defLevel >= FORCE_LEVEL_3 )
	{
		cost = cost * 7 / 10;
	}
	else if ( defLevel >= FORCE_LEVEL_2 )
	{
		cost = cost * 85 / 100;
	}
	WP_SaberGuardPay( defender, attacker, Q_max( 1, cost ) );
	if ( defender->s.number == 0 )
	{//attack right off a block and the parry recovery is skipped (G_CheckClampUcmd)
		TIMER_Set( defender, "riposteWindow", 300 );
	}
	return deflected;
}

/*
Cuts (saber rules, Stage 2)

A real swing that reaches a saber fighter's body is a cut, and what it does comes from how much
blade went through and where - not from a health number.
- depth: the distance the blade's points travelled inside the model, added up over the swing's
  frames (contact damage / the swing's base damage). Under g_saberCutDepth (half that on a limb)
  it's a graze.
- graze, anywhere: a wound.
- solid cut on a limb: a wound plus what that limb costs them (WP_SaberCutEffects). Never a kill
  on its own: the engine's "went through the limb" test says yes to every limb hit with
  g_saberMoreRealistic 2+, so it can't be trusted to mean a severed arm. The wound that finishes
  them (the third) takes the limb off.
- solid cut on the torso or head: a kill.
A wound is a third of max health, so three finish anyone. A staged boss loses a third of a stage
to a wound and the whole stage to a kill-grade cut (G_BossStageClamp holds them at the floor).
The player is never maimed: graze 12%, limb 30%, torso/head by difficulty (coreCutToPlayer).
One swing is one cut: later frames of the same swing only count if they make it worse. A swing is
the attacker's swing id (WP_SaberTrackSwingRun), so a quick repeat in the same direction is a new cut.
Thrown sabers, finishers and ordinary troopers keep the older damage rules.
*/
typedef enum
{
	CUT_NONE = 0,
	CUT_GRAZE,
	CUT_LIMB,
	CUT_KILL,
} saberCut_t;

extern cvar_t	*g_saberCutDepth;
extern cvar_t	*g_saberPlayerDamageScale;
extern int G_BossStageSize( const gentity_t *ent );

static qboolean WP_HitLocIsLimb( int hl )
{
	switch ( hl )
	{
	case HL_FOOT_RT:
	case HL_FOOT_LT:
	case HL_LEG_RT:
	case HL_LEG_LT:
	case HL_ARM_RT:
	case HL_ARM_LT:
	case HL_HAND_RT:
	case HL_HAND_LT:
		return qtrue;
	}
	return qfalse;
}

//returns the cut this frame made it (CUT_NONE = nothing worse than the swing has already done)
//and the health to take for it
static saberCut_t WP_SaberCutResolve( gentity_t *attacker, gentity_t *victim, float depth, int hitLoc, qboolean through, int *damage )
{
	static int		s_by[MAX_GENTITIES];
	static int		s_id[MAX_GENTITIES];
	static int		s_time[MAX_GENTITIES];
	static int		s_level[MAX_GENTITIES];
	static int		s_dealt[MAX_GENTITIES];
	static float	s_depth[MAX_GENTITIES];
	const int v = victim->s.number;
	const int id = s_swingId[attacker->s.number];
	if ( s_by[v] != attacker->s.number + 1 || s_id[v] != id
		|| level.time < s_time[v] || level.time - s_time[v] > 5000 )//(stale: a load or map change)
	{//a new swing
		s_by[v] = attacker->s.number + 1;
		s_id[v] = id;
		s_level[v] = CUT_NONE;
		s_dealt[v] = 0;
		s_depth[v] = 0.0f;
	}
	s_time[v] = level.time;
	s_depth[v] += depth;

	const qboolean	isPlayer = (qboolean)( victim->s.number < MAX_CLIENTS );
	const qboolean	limb = WP_HitLocIsLimb( hitLoc );
	const float		need = ( g_saberCutDepth ? g_saberCutDepth->value : 10.0f ) * ( limb ? 0.5f : 1.0f );
	saberCut_t		cut;
	if ( s_depth[v] < need )
	{
		cut = CUT_GRAZE;
	}
	else if ( limb )
	{
		cut = CUT_LIMB;
	}
	else
	{
		cut = CUT_KILL;
	}
	if ( g_debugDamage->integer )
	{
		gi.Printf( "cut: %s on %d, depth %.1f (solid at %.1f), hitloc %d%s%s -> %s%s\n",
			attacker->NPC_type ? attacker->NPC_type : "player", v, s_depth[v], need, hitLoc,
			limb ? " limb" : " core", through ? " through" : "",
			cut == CUT_GRAZE ? "GRAZE" : ( cut == CUT_LIMB ? "LIMB" : "KILL" ),
			cut <= s_level[v] ? " (no worse than this swing has done)" : "" );
	}
	if ( cut <= s_level[v] )
	{
		return CUT_NONE;
	}

	int take;
	if ( isPlayer )
	{
		const float frac = cut == CUT_GRAZE ? 0.12f : ( cut == CUT_LIMB ? 0.3f : G_Difficulty()->coreCutToPlayer );
		const float scale = g_saberPlayerDamageScale ? g_saberPlayerDamageScale->value : 1.0f;
		//(the real max, not the 999 the undying cheat sets)
		int maxHealth = victim->client->ps.stats[STAT_MAX_HEALTH] > 0 ? victim->client->ps.stats[STAT_MAX_HEALTH] : victim->max_health;
		if ( (victim->flags&FL_UNDYING) && maxHealth > 100 )
		{//the undying cheat sets max health to 999, which made every cut read ten times too big
			maxHealth = 100;
		}
		take = (int)ceil( maxHealth * frac * scale ) - s_dealt[v];
	}
	else if ( cut == CUT_KILL )
	{//(a staged boss is held at its stage floor by G_BossStageClamp)
		take = victim->health;
	}
	else
	{
		const int stage = G_BossStageSize( victim );
		take = (int)ceil( ( stage ? stage : victim->max_health ) / 3.0f ) - s_dealt[v];
	}
	take = Q_max( 1, take );
	s_level[v] = cut;
	s_dealt[v] += take;
	*damage = take;
	return cut;
}

#define SABER_WOUND_TIME	0x3FFFFFFF	//"woundArm" / "woundLeg" timers: for good

/*
WP_SaberCutEffects

After a cut on a saber fighter who lived through it.
- One opening, one cut: if they were Open, the cut ends it (they can guard the next swing).
  If they weren't, an NPC flinches: Open (short). The player doesn't (it cost them the guard for the
  rest of the enemy's phrase).
- A solid limb cut on an NPC costs them that limb's use: an arm slows their guard and the gap
  after their attacks ("woundArm", read by the Jedi AI); a hand drops the saber; a leg slows
  them and stops flips and rolls ("woundLeg").
*/
static void WP_SaberCutEffects( gentity_t *attacker, gentity_t *victim, saberCut_t cut, int hitLoc, qboolean wasOpen )
{
	playerState_t *ps = &victim->client->ps;
	if ( wasOpen )
	{
		TIMER_Remove( victim, "parryStaggered" );
		if ( victim->NPC )
		{
			TIMER_Remove( victim, "attackDelay" );
			if ( victim->NPC->guardBreakTime > level.time )
			{//(NPC_BSJedi_Default gives the Force back when this lapses)
				victim->NPC->guardBreakTime = level.time;
			}
		}
		if ( ps->weapon == WP_SABER && !PM_InKnockDown( ps ) )
		{
			ps->saberMove = ps->saberMoveNext = LS_READY;
			ps->saberBounceMove = LS_NONE;
			ps->saberBlocked = BLOCKED_NONE;
			ps->weaponTime = 0;
			ps->torsoAnimTimer = 0;
		}
	}
	//(not the player: a flinch took their guard away for the rest of the phrase - every other hit of a
	//string re-staggered them - so a cut costs them health and nothing else)
	else if ( victim->s.number != 0 && TIMER_Done( victim, "bossStageRecover" ) )
	{
		WP_SaberSetOpen( victim, g_saberOpenShort ? g_saberOpenShort->integer : 400 );
	}
	if ( cut != CUT_LIMB || !victim->NPC )
	{
		return;
	}
	const char *what = "arm";
	switch ( hitLoc )
	{
	case HL_HAND_RT:
	case HL_HAND_LT:
		if ( ps->weapon == WP_SABER && !ps->saberInFlight
			&& !(ps->saber[0].saberFlags&SFL_NOT_DISARMABLE) && !G_IsSaberBoss( victim, qfalse ) )
		{
			vec3_t throwDir;
			PM_VelocityForSaberMove( &attacker->client->ps, throwDir );
			WP_SaberLose( victim, throwDir );
			what = "hand: disarmed";
			break;
		}
		//fall through: a boss keeps hold of it, the arm is still hurt
	case HL_ARM_RT:
	case HL_ARM_LT:
		TIMER_Set( victim, "woundArm", SABER_WOUND_TIME );
		break;
	default:
		if ( TIMER_Done( victim, "woundLeg" ) )
		{
			victim->NPC->stats.runSpeed = (int)( victim->NPC->stats.runSpeed * 0.7f );
			victim->NPC->stats.walkSpeed = (int)( victim->NPC->stats.walkSpeed * 0.85f );
		}
		TIMER_Set( victim, "woundLeg", SABER_WOUND_TIME );
		what = "leg";
		break;
	}
	if ( g_debugDamage->integer || d_saberCombat->integer )
	{
		gi.Printf( "cut: %s wounded in the %s\n", victim->NPC_type ? victim->NPC_type : "?", what );
	}
}

/*
WP_BlockReleaseGraceParry (JKEnhanced)

The player let go of block to attack (the keybind can't block and attack at once) and a hit lands
within g_blockReleaseGrace ms (block is still "active": bg_pmove keeps saberBlockingTime up that
long after release). If they're only starting the attack - ready, wind-up, transition, return -
not already in the live strike, and the hit comes from in front, the attack is cancelled into a
normal parry (Force/guard rules as usual; not a perfect parry). Broken parries, bounces,
knockdowns and staggers get no grace: being knocked away is the punish for a parried swing.
Returns qtrue if the hit was parried (the caller skips the damage).
*/
qboolean WP_SaberParry( gentity_t *victim, gentity_t *attacker, int saberNum, int bladeNum );
extern cvar_t *g_blockReleaseGrace;
static qboolean WP_BlockReleaseGraceParry( gentity_t *player, gentity_t *attacker, vec3_t hitSpot, int saberNum, int bladeNum )
{
	if ( !g_blockReleaseGrace || g_blockReleaseGrace->integer <= 0 || g_saberAutoBlocking->integer
		|| !player || player->s.number != 0 || !player->client || !attacker || !attacker->client )
	{
		return qfalse;
	}
	if ( WP_SaberRules() )
	{//saber rules: a swing you've started is a swing, not a guard
		return qfalse;
	}
	playerState_t *ps = &player->client->ps;
	if ( ps->weapon != WP_SABER || !ps->SaberActive() || ps->saberInFlight
		|| ( player->client->usercmd.buttons & BUTTON_SABERBLOCK )	//holding block: the normal rules already ran
		|| ps->saberBlockingTime <= level.time )						//grace over
	{
		return qfalse;
	}
	if ( PM_SaberInBrokenParry( ps->saberMove ) || PM_SaberInBounce( ps->saberMove ) || PM_InKnockDown( ps )
		|| !TIMER_Done( player, "parryStaggered" ) )
	{//no grace when you've been knocked off balance
		return qfalse;
	}
	if ( PM_SaberInAttack( ps->saberMove ) )
	{//already in the live part of the strike: committed
		return qfalse;
	}
	vec3_t fwd, toAtt;
	const vec3_t yawOnly = { 0, ps->viewangles[YAW], 0 };
	AngleVectors( yawOnly, fwd, NULL, NULL );
	VectorSubtract( attacker->currentOrigin, player->currentOrigin, toAtt );
	toAtt[2] = 0;
	VectorNormalize( toAtt );
	if ( DotProduct( fwd, toAtt ) < 0.0f )
	{//from behind
		return qfalse;
	}
	//cancel the wind-up and parry
	ps->weaponTime = 0;
	ps->saberMove = ps->saberMoveNext = LS_READY;
	ps->saberBounceMove = LS_NONE;
	if ( !WP_SaberParry( player, attacker, saberNum, bladeNum ) )
	{
		return qfalse;
	}
	WP_SaberBlockNonRandom( player, hitSpot, qfalse );
	attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
	if ( g_debugBlock && g_debugBlock->integer )
	{
		gi.Printf( S_COLOR_GREEN "BLOCKED (release grace)" S_COLOR_WHITE " vs %s: block released %d ms ago, attack cancelled into a parry\n",
			attacker->NPC_type ? attacker->NPC_type : "?", level.time - ( ps->saberBlockingTime - g_blockReleaseGrace->integer ) );
	}
	return qtrue;
}

/*
WP_ClashBlock (JKEnhanced, g_saberClashBlock)

The player is mid-swing and an enemy's swing lands on them: instead of trading hits, the player's
swing becomes a block (from any direction in front) and the enemy's swing bounces. Not against a
telegraphed heavy attack or kata (perfect parry only), not from behind, not out of a broken parry,
bounce, knockdown or stagger. Normal block rules otherwise (Force cost). Returns qtrue if blocked.
*/
/*
WP_NPCSaberHitOnPlayer (JKEnhanced, Step 3C)

What one NPC saber hit that gets through can do to the player, by who's swinging: peon saberists
take ~4-5 hits to kill, trained Jedi/Reborn ~3, masters 2-3, bosses 2 (blockSkill 1-5, bosses by
G_IsSaberBoss). A telegraphed heavy attack x1.5; Easy x0.75, Hard x1.25; g_saberPlayerDamageScale on
top. Never more than 85% of max health in one hit, so only finishers (lock loss, stab-down) one-shot.
The same in realistic mode and not: realistic is lethal to NPCs, not to the player.
*/
extern cvar_t	*g_saberPlayerDamageScale;
extern int		AI_JediBlockSkill( const gentity_t *npc );
qboolean		G_IsSaberBoss( const gentity_t *ent, qboolean includeElites );
static float WP_NPCSaberHitOnPlayer( const gentity_t *attacker, const gentity_t *player )
{
	static const float tierDamage[6] = { 25.0f, 20.0f, 25.0f, 35.0f, 40.0f, 45.0f };	//by blockSkill 1-5
	const int skill = Q_max( 1, Q_min( 5, AI_JediBlockSkill( attacker ) ) );
	float dmg = G_IsSaberBoss( attacker, qfalse ) ? 55.0f : tierDamage[skill];
	if ( WP_IsHeavyAttack( attacker ) )
	{
		dmg *= 1.5f;
	}
	dmg *= G_Difficulty()->saberDamageToPlayer;	//ext_data/difficulty.cfg
	if ( g_saberPlayerDamageScale )
	{
		dmg *= g_saberPlayerDamageScale->value;
	}
	return Q_min( dmg, player->max_health * 0.85f );
}

/*
WP_PlayerFeint (JKEnhanced)

The player pulled back their own wind-up or the start of a strike with block (bg_pmove block
cancel). Every saber enemy within reach in front of them that was reacting to it - in a parry or
block, holding its defensive stance, or standing ready in reach - bites: ~400 ms where it can't
parry or attack ("parryStaggered" + "attackDelay"), and the player's next swing comes out at once
("riposteWindow"). Enemies adapt: one that bit is wary of feints for 4 s ("feintWary"), and
blockSkill 5 masters see through about half. Returns qtrue if anyone bit.
*/
qboolean WP_PlayerFeint( gentity_t *player )
{
	if ( !player || player->s.number != 0 || !player->client )
	{
		return qfalse;
	}
	if ( WP_SaberRules() )
	{//saber rules: no feint bite - it knocked an NPC Open with nothing to show for it
		return qfalse;
	}
	extern cvar_t *g_debugBlock;
	const float	reach = 160.0f;
	const int	openTime = 400;
	vec3_t		fwd, mins, maxs;
	const vec3_t yawOnly = { 0, player->client->ps.viewangles[YAW], 0 };
	AngleVectors( yawOnly, fwd, NULL, NULL );
	for ( int k = 0; k < 3; k++ )
	{
		mins[k] = player->currentOrigin[k] - reach;
		maxs[k] = player->currentOrigin[k] + reach;
	}
	gentity_t	*list[MAX_GENTITIES];
	const int	n = gi.EntitiesInBox( mins, maxs, list, MAX_GENTITIES );
	qboolean	bit = qfalse;
	for ( int i = 0; i < n; i++ )
	{
		gentity_t *npc = list[i];
		if ( !npc || !npc->NPC || !npc->client || npc->health <= 0 || npc->enemy != player
			|| npc->client->ps.weapon != WP_SABER || !npc->client->ps.SaberActive()
			|| !TIMER_Done( npc, "parryStaggered" ) || PM_InKnockDown( &npc->client->ps ) )
		{
			continue;
		}
		vec3_t toNpc;
		VectorSubtract( npc->currentOrigin, player->currentOrigin, toNpc );
		const float dist = VectorNormalize( toNpc );
		if ( dist > reach || DotProduct( fwd, toNpc ) < 0.5f )
		{//out of reach or not in front of the swing
			continue;
		}
		const playerState_t *nps = &npc->client->ps;
		const qboolean reacting = (qboolean)( PM_SaberInParry( nps->saberMove ) || nps->saberBlocked != BLOCKED_NONE
			|| npc->NPC->duelRole == JT_DEFEND || ( nps->saberMove == LS_READY && dist < 128.0f ) );
		if ( !reacting )
		{
			continue;
		}
		const char *name = npc->NPC_type ? npc->NPC_type : "?";
		if ( !TIMER_Done( npc, "feintWary" ) )
		{
			if ( g_debugBlock && g_debugBlock->integer )
			{
				gi.Printf( S_COLOR_YELLOW "FEINT: %s is wary (fooled them recently)\n", name );
			}
			continue;
		}
		TIMER_Set( npc, "feintWary", 4000 );
		if ( AI_JediBlockSkill( npc ) >= 5 && Q_irand( 0, 1 ) )
		{
			if ( g_debugBlock && g_debugBlock->integer )
			{
				gi.Printf( S_COLOR_YELLOW "FEINT: %s saw through it\n", name );
			}
			continue;
		}
		TIMER_Set( npc, "parryStaggered", openTime );
		TIMER_Set( npc, "attackDelay", openTime );
		bit = qtrue;
		if ( g_debugBlock && g_debugBlock->integer )
		{
			gi.Printf( S_COLOR_GREEN "FEINT: %s bit - open for %d ms\n", name, openTime );
		}
	}
	if ( bit )
	{
		TIMER_Set( player, "riposteWindow", openTime );
	}
	return bit;
}

extern cvar_t *g_saberClashBlock;
extern cvar_t *g_saberClashWeight;
void G_DamageGuard( gentity_t *victim, gentity_t *attacker, int amount );
static int s_riposteTarget = ENTITYNUM_NONE;	//WP_PerfectParryReward: who the player's riposte is open on
qboolean WP_RiposteOpenOn( const gentity_t *attacker, const gentity_t *victim );
//JKEnhanced: how much a style's swing weighs in a clash (WP_ClashBlock): Strong beats Medium-class beats Fast
static int WP_ClashWeight( const playerState_t *ps )
{
	switch ( ps->saberAnimLevel )
	{
	case SS_FAST:	return 1;
	case SS_STRONG:
	case SS_DESANN:	return 3;
	default:		return 2;	//Medium, Tavion, Katarn, Dual, Staff
	}
}
static qboolean WP_ClashBlock( gentity_t *player, gentity_t *attacker, vec3_t hitSpot, int saberNum, int bladeNum )
{
	if ( WP_SaberRules() )
	{//saber rules: a swing is not a guard. Two fighters swinging at each other both risk the cut;
	 //whichever blade gets there first lands, and the flinch stops the other swing.
		return qfalse;
	}
	if ( !g_saberClashBlock || !g_saberClashBlock->integer
		|| !player || player->s.number != 0 || !player->client || !attacker || !attacker->client )
	{
		return qfalse;
	}
	playerState_t *ps = &player->client->ps;
	if ( ps->weapon != WP_SABER || !ps->SaberActive() || ps->saberInFlight )
	{
		return qfalse;
	}
	const qboolean swinging = (qboolean)( PM_SaberInStart( ps->saberMove ) || PM_SaberInAttack( ps->saberMove )
		|| PM_SaberInTransitionAny( ps->saberMove ) || PM_SaberInSpecialAttack( ps->torsoAnim ) );
	if ( !swinging || PM_SaberInBrokenParry( ps->saberMove ) || PM_SaberInBounce( ps->saberMove )
		|| PM_InKnockDown( ps ) || !TIMER_Done( player, "parryStaggered" ) )
	{
		return qfalse;
	}
	const playerState_t *aps = &attacker->client->ps;
	if ( aps->weapon != WP_SABER
		|| !( PM_SaberInAttack( aps->saberMove ) || PM_SaberInSpecialAttack( aps->torsoAnim ) )
		|| WP_IsHeavyAttack( attacker ) )
	{//they have to be swinging too, and not a heavy attack
		return qfalse;
	}
	vec3_t fwd, toAtt;
	const vec3_t yawOnly = { 0, ps->viewangles[YAW], 0 };
	AngleVectors( yawOnly, fwd, NULL, NULL );
	VectorSubtract( attacker->currentOrigin, player->currentOrigin, toAtt );
	toAtt[2] = 0;
	VectorNormalize( toAtt );
	if ( DotProduct( fwd, toAtt ) < 0.0f )
	{
		return qfalse;
	}
	if ( g_saberClashWeight && g_saberClashWeight->integer
		&& WP_ClashWeight( ps ) > WP_ClashWeight( aps ) )
	{//the heavier swing wins: theirs bounces off, yours carries on (windup included - Strong's commitment)
		playerState_t *atps = &attacker->client->ps;
		if ( atps->saberBlocked == BLOCKED_ATK_BOUNCE )
		{//already won this one on an earlier frame of contact (their bounce starts in their next pmove):
		 //don't hit their guard and play the clang again every frame
			return qtrue;
		}
		atps->saberBounceMove = PM_SaberBounceForAttack( atps->saberMove );
		atps->saberBlocked = BLOCKED_ATK_BOUNCE;
		atps->saberEventFlags |= SEF_BLOCKED;
		if ( attacker->NPC && attacker->NPC->guardMax > 0 )
		{
			G_DamageGuard( attacker, player, 8 * ( WP_ClashWeight( ps ) - WP_ClashWeight( aps ) ) );
		}
		WP_SaberBlockSound( player, attacker, saberNum, bladeNum );
		if ( g_debugBlock && g_debugBlock->integer )
		{
			gi.Printf( S_COLOR_GREEN "CLASH WON" S_COLOR_WHITE " vs %s: your style %d outweighs their %d - they bounced, your swing carries on\n",
				attacker->NPC_type ? attacker->NPC_type : "?", ps->saberAnimLevel, aps->saberAnimLevel );
		}
		return qtrue;
	}
	//the player's swing becomes the block
	ps->weaponTime = 0;
	ps->saberMove = ps->saberMoveNext = LS_READY;
	ps->saberBounceMove = LS_NONE;
	if ( ps->saberBlockingTime < level.time + 100 )
	{
		ps->saberBlockingTime = level.time + 100;
	}
	if ( !WP_SaberParry( player, attacker, saberNum, bladeNum ) )
	{
		return qfalse;
	}
	WP_SaberBlockNonRandom( player, hitSpot, qfalse );
	attacker->client->ps.saberEventFlags |= SEF_BLOCKED;
	if ( g_debugBlock && g_debugBlock->integer )
	{
		gi.Printf( S_COLOR_GREEN "BLOCKED (clash)" S_COLOR_WHITE " vs %s: you were both swinging - your swing blocked theirs\n",
			attacker->NPC_type ? attacker->NPC_type : "?" );
	}
	return qtrue;
}

/*
Hit-stop and camera kick (JKEnhanced saber rules, Stage 5)

WP_SaberHitStop: a cut catches for a moment. The fighter is frozen on the contact frame for
g_saberHitStop ms: each bone's anim is held where it was and resumed from the same frame, so
the anim timers just get the freeze's length added. If something else changes their animation
meanwhile (a pain flinch, a death) the freeze lets go of that part. While frozen they don't
move, the saber code is paused (PM_WeaponLightsaber) and the blade deals no damage; a fresh
attack or block press from the player ends it early.
WP_SaberCamKick: a short camera shake for the moments that matter to the player - their guard
collapsing or collapsing someone's, a perfect parry, a cut given or taken (g_saberCamKick scales
it, 0 = off).
*/
extern cvar_t *g_saberHitStop;
extern cvar_t *g_saberCamKick;
extern void CGCam_Shake( float intensity, int duration );
typedef struct
{
	int		model, bone;	//bone < 0 = not frozen
	int		start, end, flags;
	float	current, speed;
} hitStopBone_t;
#define HITSTOP_BONES 6
static struct
{
	int				until;					//0 = not frozen
	int				torsoAnim, legsAnim;	//-1 = that part wasn't frozen
	hitStopBone_t	bones[HITSTOP_BONES];
} s_hitStop[MAX_GENTITIES];

qboolean WP_SaberHitStopped( const gentity_t *ent )
{
	if ( !ent || ent->s.number < 0 || ent->s.number >= MAX_GENTITIES )
	{
		return qfalse;
	}
	const int until = s_hitStop[ent->s.number].until;
	return (qboolean)( until > level.time && until <= level.time + 1000 );
}

static void WP_HitStopFreezeBone( gentity_t *ent, int model, int bone, hitStopBone_t *b )
{
	b->bone = -1;
	if ( model < 0 || bone < 0 )
	{
		return;
	}
	const int t = cg.time ? cg.time : level.time;
	if ( !gi.G2API_GetBoneAnimIndex( &ent->ghoul2[model], bone, t, &b->current, &b->start, &b->end, &b->flags, &b->speed, NULL ) )
	{
		return;
	}
	b->model = model;
	b->bone = bone;
	const int frame = (int)b->current;
	gi.G2API_SetBoneAnimIndex( &ent->ghoul2[model], bone, frame, frame+1, BONE_ANIM_OVERRIDE_FREEZE, 1, t, b->current, 0 );
}

static void WP_HitStopResumeBone( gentity_t *ent, hitStopBone_t *b )
{
	if ( b->bone < 0 )
	{
		return;
	}
	const int t = cg.time ? cg.time : level.time;
	gi.G2API_SetBoneAnimIndex( &ent->ghoul2[b->model], b->bone, b->start, b->end, b->flags&~BONE_ANIM_BLEND, b->speed, t, b->current, 0 );
	b->bone = -1;
}

static void WP_SaberHitStopRelease( gentity_t *ent )
{
	const int n = ent->s.number;
	if ( !s_hitStop[n].until )
	{
		return;
	}
	s_hitStop[n].until = 0;
	if ( !ent->client || !gi.G2API_HaveWeGhoul2Models( ent->ghoul2 ) )
	{
		return;
	}
	//only resume a part that's still on the anim we froze (a pain or knockdown anim already replaced it)
	const qboolean torso = (qboolean)( s_hitStop[n].torsoAnim != -1 && ent->client->ps.torsoAnim == s_hitStop[n].torsoAnim );
	const qboolean legs = (qboolean)( s_hitStop[n].legsAnim != -1 && ent->client->ps.legsAnim == s_hitStop[n].legsAnim );
	for ( int i = 0; i < HITSTOP_BONES; i++ )
	{
		if ( ( i < 4 && torso ) || ( i >= 4 && legs ) )
		{
			WP_HitStopResumeBone( ent, &s_hitStop[n].bones[i] );
		}
		s_hitStop[n].bones[i].bone = -1;
	}
}

static void WP_SaberHitStop( gentity_t *ent )
{
	if ( !g_saberHitStop || g_saberHitStop->integer <= 0 || in_camera
		|| !ent || !ent->client || ent->health <= 0 || ent->s.number >= MAX_GENTITIES
		|| ent->client->NPC_class == CLASS_SABER_DROID || ent->lowerLumbarBone == -1
		|| !gi.G2API_HaveWeGhoul2Models( ent->ghoul2 )
		|| ent->client->ps.saberLockTime > level.time || PM_InKnockDown( &ent->client->ps ) )
	{
		return;
	}
	const int		n = ent->s.number;
	const int		ms = Q_min( 200, g_saberHitStop->integer );
	playerState_t	*ps = &ent->client->ps;
	if ( s_hitStop[n].until > level.time + 1000 )
	{//left over from another map / an older save
		s_hitStop[n].until = 0;
	}
	if ( WP_SaberHitStopped( ent ) )
	{
		return;
	}
	const qboolean	legs = (qboolean)( ps->legsAnim == ps->torsoAnim );	//a full-body swing: hold the legs too
	WP_HitStopFreezeBone( ent, ent->playerModel, ent->lowerLumbarBone, &s_hitStop[n].bones[0] );
	WP_HitStopFreezeBone( ent, ent->playerModel, ent->motionBone, &s_hitStop[n].bones[1] );
	WP_HitStopFreezeBone( ent, ent->headModel > 0 ? ent->headModel : -1, ent->headLowerLumbarBone, &s_hitStop[n].bones[2] );
	WP_HitStopFreezeBone( ent, ent->headModel > 0 ? ent->headModel : -1, ent->headMotionBone, &s_hitStop[n].bones[3] );
	s_hitStop[n].bones[4].bone = s_hitStop[n].bones[5].bone = -1;
	if ( legs )
	{
		WP_HitStopFreezeBone( ent, ent->playerModel, ent->rootBone, &s_hitStop[n].bones[4] );
		WP_HitStopFreezeBone( ent, ent->headModel > 0 ? ent->headModel : -1, ent->headRootBone, &s_hitStop[n].bones[5] );
	}
	s_hitStop[n].torsoAnim = ps->torsoAnim;
	s_hitStop[n].legsAnim = legs ? ps->legsAnim : -1;
	if ( ps->torsoAnimTimer > 0 )
	{
		ps->torsoAnimTimer += ms;
	}
	if ( legs && ps->legsAnimTimer > 0 )
	{
		ps->legsAnimTimer += ms;
	}
	if ( ps->weaponTime > 0 )
	{
		ps->weaponTime += ms;
	}
	VectorClear( ps->velocity );
	s_hitStop[n].until = level.time + ms;
}

//every client think (g_active, before Pmove): hold a frozen fighter still, let go when it's over
void WP_SaberHitStopThink( gentity_t *ent, usercmd_t *ucmd )
{
	if ( !ent || !ent->client || ent->s.number >= MAX_GENTITIES || !s_hitStop[ent->s.number].until )
	{
		return;
	}
	const int n = ent->s.number;
	if ( !WP_SaberHitStopped( ent ) || ent->health <= 0
		|| ent->client->ps.torsoAnim != s_hitStop[n].torsoAnim || PM_InKnockDown( &ent->client->ps )
		|| ( n == 0 && ( ucmd->buttons & ~ent->client->buttons & ( BUTTON_ATTACK|BUTTON_SABERBLOCK ) ) ) )
	{
		WP_SaberHitStopRelease( ent );
		return;
	}
	ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
	VectorClear( ent->client->ps.velocity );
}

static void WP_SaberCamKick( float intensity, int duration )
{
	if ( g_saberCamKick && g_saberCamKick->value > 0.0f && !in_camera )
	{
		CGCam_Shake( intensity * g_saberCamKick->value, duration );
	}
}

static void WP_ReportBlockFail( gentity_t *player, gentity_t *attacker, int damage, int hitLoc )
{
	if ( !player || !player->client || !g_debugBlock || !g_debugBlock->integer )
	{
		return;
	}
	playerState_t	*ps = &player->client->ps;
	const qboolean	held = (qboolean)( ( player->client->usercmd.buttons & BUTTON_SABERBLOCK ) != 0 );
	const qboolean	active = (qboolean)( ps->saberBlockingTime > level.time );
	static int lastPrint = -1;
	if ( lastPrint == level.time )
	{
		return;
	}
	lastPrint = level.time;

	//which block box was up (WP_SaberUpdate: the wide box covers the body in front of you)
	const char *box;
	if ( ps->saberBlocking != BLK_WIDE && ps->saberBlocking != BLK_TIGHT )
	{
		box = "NONE (not blocking)";
	}
	else if ( ps->saberBlocking == BLK_WIDE && active && ps->forcePower > 0 && ps->weaponTime <= 0 && !G_InCinematicSaberAnim( player ) )
	{
		box = "wide";
	}
	else if ( !active )
	{
		box = "blade only (block not active)";
	}
	else if ( ps->forcePower <= 0 )
	{
		box = "blade only (no Force)";
	}
	else if ( ps->weaponTime > 0 )
	{
		box = va( "blade only (mid-anim, weaponTime %d)", ps->weaponTime );
	}
	else
	{
		box = "blade only (tight)";
	}

	//the player's own saber move
	const char *moveState =
		PM_SaberInBrokenParry( ps->saberMove ) ? " BROKEN-PARRY" :
		PM_SaberInKnockaway( ps->saberMove ) ? " knockaway" :
		PM_SaberInParry( ps->saberMove ) ? " parry" :
		PM_SaberInBounce( ps->saberMove ) ? " BOUNCE" :
		PM_SaberInAttack( ps->saberMove ) ? " ATTACKING" :
		PM_SaberInTransitionAny( ps->saberMove ) ? " TRANSITION" : "";

	//where the attacker is, relative to where the player faces
	vec3_t fwd, toAtt;
	const vec3_t yawOnly = { 0, ps->viewangles[YAW], 0 };
	AngleVectors( yawOnly, fwd, NULL, NULL );
	float angle = 0.0f;
	if ( attacker )
	{
		VectorSubtract( attacker->currentOrigin, player->currentOrigin, toAtt );
		toAtt[2] = 0;
		VectorNormalize( toAtt );
		angle = RAD2DEG( acos( Com_Clamp( -1.0f, 1.0f, DotProduct( fwd, toAtt ) ) ) );
	}

	const qboolean recentNote = (qboolean)( s_blockFailNote && level.time - s_blockFailNoteTime <= 300 );
	//(saber rules: block can read as "active" for a moment after it's released, but a swing has no guard)
	const qboolean midSwing = (qboolean)( WP_SaberRules() && WP_SaberSwinging( ps ) );
	gi.Printf( "%s" S_COLOR_WHITE " vs %s (style %d%s): %d dmg, hitloc %d | %s | block %s%s, Force %d, box %s | move %s%s%s | attacker at %.0f deg%s\n",
		( ( held || active ) && !midSwing ) ? S_COLOR_RED "BLOCK FAILED" : S_COLOR_YELLOW "HIT (not blocking)",
		attacker && attacker->NPC_type ? attacker->NPC_type : "?",
		attacker && attacker->client ? attacker->client->ps.saberAnimLevel : 0,
		attacker && WP_IsHeavyAttack( attacker ) ? ", HEAVY" : "",
		damage, hitLoc,
		recentNote ? s_blockFailNote : ( midSwing ? "you were mid-swing (wind-up, strike or transition): a swing is not a guard"
			: ( ( held || active ) ? "blade reached you without hitting your saber (angle/reach/box)" : "block button not held" ) ),
		held ? "held" : "released", active ? "+active" : " but NOT active",
		ps->forcePower, box,
		ps->saberMove >= 0 && ps->saberMove < LS_MOVE_MAX ? saberMoveData[ps->saberMove].name : "?", moveState,
		!TIMER_Done( player, "parryStaggered" ) ? " STAGGERED" : "",
		angle, angle > 100.0f ? " (BEHIND)" : ( angle > 60.0f ? " (side)" : "" ) );
}
qboolean WP_SaberApplyDamage( gentity_t *ent, float baseDamage, int baseDFlags,
							 qboolean brokenParry, int saberNum, int bladeNum, qboolean thrownSaber )
{
	qboolean	didDamage = qfalse;
	gentity_t	*victim;
	int			dFlags = baseDFlags;
	saberType_t saberType = ent->client->ps.saber[saberNum].type;

	if ( !numVictims )
	{
		return qfalse;
	}
	for ( int i = 0; i < numVictims; i++ )
	{
		dFlags = baseDFlags|DAMAGE_DEATH_KNOCKBACK|DAMAGE_NO_HIT_LOC;
		if ( victimEntityNum[i] != ENTITYNUM_NONE && &g_entities[victimEntityNum[i]] != NULL )
		{	// Don't bother with this damage if the fraction is higher than the saber's fraction
			if ( dmgFraction[i] < saberHitFraction || brokenParry )
			{
				victim = &g_entities[victimEntityNum[i]];
				if ( !victim )
				{
					continue;
				}
				// who gets "real" saber damage (location-scaled, 5-100) rather than the lethal-anywhere
				// rule for normal NPCs: the player, saber users, Jedi/Reborn/Shadowtroopers, bosses, wampas
				const qboolean saberFighter = (qboolean)( victim->client
					&& ( victim->s.number < MAX_CLIENTS
						|| victim->s.weapon == WP_SABER
						|| victim->client->NPC_class == CLASS_REBORN
						|| victim->client->NPC_class == CLASS_JEDI
						|| victim->client->NPC_class == CLASS_SHADOWTROOPER
						|| victim->client->NPC_class == CLASS_WAMPA
						|| ( victim->NPC && ( victim->NPC->aiFlags & (NPCAI_BOSS_CHARACTER|NPCAI_SUBBOSS_CHARACTER) ) ) ) );
				const float rawContactDmg = totalDmg[i];

				if ( victim->e_DieFunc == dieF_maglock_die )
				{//*sigh*, special check for maglocks
					vec3_t testFrom;
					if ( ent->client->ps.saberInFlight )
					{
						VectorCopy( g_entities[ent->client->ps.saberEntityNum].currentOrigin, testFrom );
					}
					else
					{
						VectorCopy( ent->currentOrigin, testFrom );
					}
					testFrom[2] = victim->currentOrigin[2];
					trace_t testTrace;
					gi.trace( &testTrace, testFrom, vec3_origin, vec3_origin, victim->currentOrigin, ent->s.number, MASK_SHOT, (EG2_Collision)0, 0 );
					if ( testTrace.entityNum != victim->s.number )
					{//can only damage maglocks if have a clear trace to the thing's origin
						continue;
					}
				}
				//JKEnhanced: a saber-lock win or a stab-down is only an uncapped finisher on someone actually
				//in the lock-loss or knocked down; the same anim brushing anyone else is a normal hit
				//Never against the player: a stab-down on a knocked-down player went uncapped AND skipped the
				//one-hit-per-swing rule, so Tavion did 3635, 1155, ... 266185 every 50 ms. The player takes
				//the normal tiered (capped) hit, once per swing.
				const qboolean finisher = (qboolean)( victim->client && victim->s.number >= MAX_CLIENTS
					&& ( ( PM_SuperBreakWinAnim( ent->client->ps.torsoAnim ) && PM_SuperBreakLoseAnim( victim->client->ps.torsoAnim ) )
						|| ( PM_StabDownAnim( ent->client->ps.torsoAnim ) && PM_InKnockDown( &victim->client->ps ) ) ) );
				if ( totalDmg[i] > 0 )
				{//actually want to do *some* damage here
					if ( victim->client
						&& victim->client->NPC_class==CLASS_WAMPA
						&& victim->activator == ent )
					{
					}
					else if ( finisher )
					{//never cap the superbreak wins (JKEnhanced: and they finish - uncapped also skipped the 5
					 //floor, so a stab-down's brief contact landed 0-2; boss stage floors still apply in G_Damage)
						if ( totalDmg[i] < victim->health )
						{
							totalDmg[i] = victim->health;
						}
					}
					else
					{// Saber damage = continuous contact damage (blade depth/time inside the model) x
					//saberLocationMod, for every victim (this used to apply only vs saber users, and
					//G_Damage then applied the gun table on top). Normal NPCs: lethal anywhere on a
					//real swing. Saber fighters/bosses/player: real damage, no per-difficulty caps --
					//the guard/parry system carries saber difficulty.
						if ( !g_saberRealisticCombat->integer )
						{
							totalDmg[i] *= saberLocationMod[hitLoc[i]];
						}
						if ( !victim->client )
						{//breakables etc: as before, 25-100 so a swing actually breaks things
							if ( totalDmg[i] < 25 )
							{
								totalDmg[i] = 25;
							}
							if ( totalDmg[i] > 100 )
							{
								totalDmg[i] = 100;
							}
						}
						else if ( !saberFighter )
						{//troopers etc: a real attack swing does at least 80% of their max health
						//wherever it lands; transition/defence brushes (baseDamage < 2.5) keep their small
						//real contact damage (used to get a 25 floor, so any touch nearly killed)
							if ( baseDamage >= 2.5f && totalDmg[i] < victim->max_health * 0.8f )
							{
								totalDmg[i] = victim->max_health * 0.8f;
							}
							if ( totalDmg[i] > 150 )
							{
								totalDmg[i] = 150;
							}
						}
						else if ( victim->s.number == 0 && ent->NPC )
						{//JKEnhanced 3C: an NPC's hit on the player is worth what that NPC's tier says
							const float hitCap = WP_NPCSaberHitOnPlayer( ent, victim );
							if ( totalDmg[i] > hitCap )
							{
								totalDmg[i] = hitCap;
							}
							else if ( totalDmg[i] < 5 )
							{
								totalDmg[i] = 5;
							}
						}
						else
						{//saber fighters, bosses and the player: clamp 5-100
							if ( totalDmg[i] > 100 )
							{
								totalDmg[i] = 100;
							}
							else if ( totalDmg[i] < 5 )
							{
								totalDmg[i] = 5;
							}
						}
					}

					if ( totalDmg[i] > 0 )
					{
						gentity_t *inflictor = ent;
						didDamage = qtrue;
						qboolean vicWasDismembered = qtrue;
						qboolean vicWasAlive = (qboolean)(victim->health>0);

						if ( baseDamage <= 0.1f && !finisher )
						{//just get their attention? (not a finisher: the stab-down / lock win's anim is the attack)
							dFlags |= DAMAGE_NO_DAMAGE;
						}

						if( victim->client )
						{
							if ( victim->client->ps.pm_time > 0 && victim->client->ps.pm_flags & PMF_TIME_KNOCKBACK && victim->client->ps.velocity[2] > 0 )
							{//already being knocked around
								dFlags |= DAMAGE_NO_KNOCKBACK;
							}
							if ( !WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
								&& (ent->client->ps.saber[saberNum].saberFlags2&SFL2_NO_DISMEMBERMENT) )
							{//no dismemberment! (blunt/stabbing weapon?)
							}
							else if ( WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
								&& (ent->client->ps.saber[saberNum].saberFlags2&SFL2_NO_DISMEMBERMENT2) )
							{//no dismemberment! (blunt/stabbing weapon?)
							}
							else
							{
								if ( debug_subdivision->integer || g_saberRealisticCombat->integer )
								{
									dFlags |= DAMAGE_DISMEMBER;
									if ( hitDismember[i] )
									{
										victim->client->dismembered = false;
									}
								}
								else if ( hitDismember[i] )
								{
									dFlags |= DAMAGE_DISMEMBER;
								}
								if ( !victim->client->dismembered )
								{
									vicWasDismembered = qfalse;
								}
							}
							if ( baseDamage <= 1.0f )
							{//very mild damage
								if ( victim->s.number == 0 || victim->client->ps.weapon == WP_SABER || victim->client->NPC_class == CLASS_GALAKMECH )
								{//if it's the player or a saber-user, don't kill them with this blow
									dFlags |= DAMAGE_NO_KILL;
								}
							}
						}
						else
						{
							if ( victim->takedamage )
							{//some other breakable thing
								//create a flash here
								if ( !g_noClashFlare )
								{
									g_saberFlashTime = level.time-50;
									VectorCopy( dmgSpot[i], g_saberFlashPos );
								}
							}
						}
						if ( !finisher
							&& !g_saberRealisticCombat->integer
							&& g_saberDamageCapping->integer )
						{//never cap the superbreak wins
						//One cap only: a single saber hit never takes more than g_saberMaxPlayerHitPct of the
						//player's max health (no one-shots). The old per-difficulty caps (bosses 10-25, saber
						//users 30-75, player 7-50) are gone; the guard/parry system carries saber difficulty.
							if ( victim->s.number < MAX_CLIENTS && victim->client && !ent->NPC )
							{//(NPC hits on the player already use WP_NPCSaberHitOnPlayer)
								const float playerCap = victim->max_health * g_saberMaxPlayerHitPct->value;
								if ( playerCap > 0.0f && totalDmg[i] > playerCap )
								{
									totalDmg[i] = playerCap;
								}
							}
						}
						//victim->hitLoc = hitLoc[i];

						dFlags |= DAMAGE_NO_KNOCKBACK;//okay, let's try no knockback whatsoever...
						dFlags &= ~DAMAGE_DEATH_KNOCKBACK;
						if ( g_saberRealisticCombat->integer )
						{
							dFlags |= DAMAGE_NO_KNOCKBACK;
							dFlags &= ~DAMAGE_DEATH_KNOCKBACK;
							dFlags &= ~DAMAGE_NO_KILL;
						}
						if ( ent->client && !ent->s.number )
						{
							switch( hitLoc[i] )
							{
							case HL_FOOT_RT:
							case HL_FOOT_LT:
							case HL_LEG_RT:
							case HL_LEG_LT:
								ent->client->sess.missionStats.legAttacksCnt++;
								break;
							case HL_WAIST:
							case HL_BACK_RT:
							case HL_BACK_LT:
							case HL_BACK:
							case HL_CHEST_RT:
							case HL_CHEST_LT:
							case HL_CHEST:
								ent->client->sess.missionStats.torsoAttacksCnt++;
								break;
							case HL_ARM_RT:
							case HL_ARM_LT:
							case HL_HAND_RT:
							case HL_HAND_LT:
								ent->client->sess.missionStats.armAttacksCnt++;
								break;
							default:
								ent->client->sess.missionStats.otherAttacksCnt++;
								break;
							}
						}

						if ( saberType == SABER_SITH_SWORD )
						{//do knockback
							dFlags &= ~(DAMAGE_NO_KNOCKBACK|DAMAGE_DEATH_KNOCKBACK);
						}
						if ( !WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
							&& ent->client->ps.saber[saberNum].knockbackScale > 0.0f )
						{
							dFlags &= ~(DAMAGE_NO_KNOCKBACK|DAMAGE_DEATH_KNOCKBACK);
							if ( saberNum < 1 )
							{
								dFlags |= DAMAGE_SABER_KNOCKBACK1;
							}
							else
							{
								dFlags |= DAMAGE_SABER_KNOCKBACK2;
							}
						}
						else if ( WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
							&& ent->client->ps.saber[saberNum].knockbackScale2 > 0.0f )
						{
							dFlags &= ~(DAMAGE_NO_KNOCKBACK|DAMAGE_DEATH_KNOCKBACK);
							if ( saberNum < 1 )
							{
								dFlags |= DAMAGE_SABER_KNOCKBACK1_B2;
							}
							else
							{
								dFlags |= DAMAGE_SABER_KNOCKBACK2_B2;
							}
						}
						if ( thrownSaber )
						{
							inflictor = &g_entities[ent->client->ps.saberEntityNum];
						}
						int damage = 0;
						if ( !WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
							&& ent->client->ps.saber[saberNum].damageScale != 1.0f )
						{
							damage = ceil(totalDmg[i]*ent->client->ps.saber[saberNum].damageScale);
						}
						else if ( WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
							&& ent->client->ps.saber[saberNum].damageScale2 != 1.0f )
						{
							damage = ceil(totalDmg[i]*ent->client->ps.saber[saberNum].damageScale2);
						}
						else
						{
							damage = ceil(totalDmg[i]);
						}
						// Severing a regular NPC's hand is non-lethal: wound them, drop their gun, cut the
						// hand off the living body, and count them as killed for mission purposes (they
						// go neutral and harmless). Dismemberment normally only runs in player_die, so
						// it's done here directly; the hit itself must not kill or the death path would
						// double up.
						qboolean severHand = qfalse;
						if ( victim->client && victim->NPC && victim->s.number >= MAX_CLIENTS
							&& !saberFighter
							&& victim->health > 1
							//a real swing (or a thrown blade), not the idle blade brushing them
							&& ( ent->client->ps.saberInFlight
								|| PM_SaberInAttack( ent->client->ps.saberMove )
								|| PM_SaberInSpecialAttack( ent->client->ps.torsoAnim ) )
							&& (dFlags&DAMAGE_DISMEMBER)
							&& ( hitDismemberLoc[i] == HL_HAND_RT || hitDismemberLoc[i] == HL_HAND_LT )
							&& victim->locationDamage[hitDismemberLoc[i]] < Q3_INFINITE//hand still attached
							&& ( g_dismemberment->integer || g_saberRealisticCombat->integer > 1 ) )
						{
							severHand = qtrue;
							damage = Q_min( victim->health - 1, Q_max( 1, (int)( victim->max_health * 0.3f ) ) );
							dFlags &= ~DAMAGE_DISMEMBER;
						}
						if ( victim->s.number == 0 && damage > 0
							&& WP_ClashBlock( victim, ent, dmgSpot[i], saberNum, bladeNum ) )
						{//JKEnhanced: both swinging - the player's swing blocks theirs instead of trading hits
							continue;
						}
						if ( victim->s.number == 0 && damage > 0
							&& WP_BlockReleaseGraceParry( victim, ent, dmgSpot[i], saberNum, bladeNum ) )
						{//JKEnhanced: let go of block to attack and got hit straight away: parried instead
							continue;
						}
						//JKEnhanced saber rules: a real swing on a saber fighter is a cut - how much blade, and where
						saberCut_t cut = CUT_NONE;
						const qboolean cutWasOpen = (qboolean)( WP_SaberRules() && WP_SaberOpenTime( victim ) > 0 );
						if ( WP_SaberRules() && saberFighter && !finisher && !thrownSaber && baseDamage >= 2.5f
							&& victim->client && victim->health > 0 && damage > 0 )
						{
							cut = WP_SaberCutResolve( ent, victim, rawContactDmg / baseDamage, hitLoc[i],
								(qboolean)( hitDismember[i] && WP_HitLocIsLimb( hitDismemberLoc[i] ) ), &damage );
							if ( cut == CUT_NONE )
							{//this frame added nothing to what the swing has already done
								continue;
							}
							if ( victim->NPC && ent->s.number == 0 )
							{//g_debugBlock: every cut the player lands says why the guard wasn't there (before the flinch changes it)
								WP_NoteGuardFail( victim, ent, WP_GuardState( victim, ent ) );
							}
						}
						if ( saberFighter && !finisher && damage > 0 && cut == CUT_NONE )
						{//JKEnhanced: one damaging hit per swing. The blade stays in contact for several
						 //frames and each frame used to deal a full capped hit (2-5 x 100 from one swing).
						 //The swing is the attacker's swing id (WP_SaberTrackSwingRun). A thrown saber counts
						 //as its own "swing" (key -1, over 600 ms): once out, once on the way back.
						 //Not saved: a stale entry after a load just expires.
							static int s_swingHitBy[MAX_GENTITIES];
							static int s_swingHitKey[MAX_GENTITIES];
							static int s_swingHitTime[MAX_GENTITIES];
							const int v = victim->s.number;
							const int swingKey = thrownSaber ? -1 : s_swingId[ent->s.number];
							if ( s_swingHitBy[v] == ent->s.number + 1
								&& s_swingHitKey[v] == swingKey
								&& level.time >= s_swingHitTime[v]
								&& level.time - s_swingHitTime[v] < ( thrownSaber ? 600 : 5000 ) )
							{
								continue;
							}
							s_swingHitBy[v] = ent->s.number + 1;
							s_swingHitKey[v] = swingKey;
							s_swingHitTime[v] = level.time;
						}
						if ( victim->s.number == 0 && damage > 0 )
						{//g_debugBlock: if the player was blocking, say why this got through
							WP_ReportBlockFail( victim, ent, damage, hitLoc[i] );
							extern cvar_t *g_blockFailSound;
							static int s_blockFailSoundTime = -1000;
							if ( g_blockFailSound && g_blockFailSound->string[0]
								&& ( ( victim->client->usercmd.buttons & BUTTON_SABERBLOCK ) || victim->client->ps.saberBlockingTime > level.time )
								&& ( level.time - s_blockFailSoundTime > 300 || level.time < s_blockFailSoundTime ) )
							{//JKEnhanced 3D: block was up and the hit still got through - say so
								s_blockFailSoundTime = level.time;
								G_SoundOnEnt( victim, CHAN_AUTO, g_blockFailSound->string );
							}
						}
						if ( damage > 0 && WP_RiposteOpenOn( ent, victim ) )
						{//JKEnhanced: the riposte after a perfect parry gets through once (no damage bonus)
							TIMER_Remove( ent, "perfectRiposte" );
						}
						if ( g_debugDamage->integer )
						{
							gi.Printf( "saber: victim %d hitloc %d (x%.2f) contact %.1f -> %d dmg%s, dismemberLoc %d\n",
								victim->s.number, hitLoc[i], g_saberRealisticCombat->integer ? 1.0f : saberLocationMod[hitLoc[i]],
								rawContactDmg, damage, (dFlags&DAMAGE_NO_DAMAGE) ? " (no-damage brush: not in an attack)" : saberFighter ? "" : " (lethal-anywhere)", hitDismemberLoc[i] );
						}
						G_Damage( victim, inflictor, ent, dmgDir[i], dmgSpot[i], damage, dFlags, MOD_SABER, hitDismemberLoc[i] );
						if ( victim->s.number == 0 && victim->health > 0 && victim->client
							&& ( PM_SaberInBrokenParry( victim->client->ps.saberMove ) || !TIMER_Done( victim, "parryStaggered" ) ) )
						{//JKEnhanced 3C: a broken parry/stagger is punished once - the hit that lands ends it, so
						 //you can block the next swing instead of eating a combo off one mistake
							playerState_t *vps = &victim->client->ps;
							vps->saberMove = vps->saberMoveNext = LS_READY;
							vps->saberBounceMove = LS_NONE;
							vps->saberBlocked = BLOCKED_NONE;
							vps->weaponTime = 0;
							vps->torsoAnimTimer = 0;
							TIMER_Remove( victim, "parryStaggered" );
							if ( g_debugBlock && g_debugBlock->integer )
							{
								gi.Printf( "punished: broken parry/stagger ended, block again\n" );
							}
						}
						if ( cut != CUT_NONE && victim->health > 0 && victim->client )
						{//JKEnhanced saber rules: the flinch (or the end of their opening), and what a hurt limb costs
							WP_SaberCutEffects( ent, victim, cut, hitLoc[i], cutWasOpen );
						}
						if ( cut != CUT_NONE )
						{//JKEnhanced saber rules: the blade catches for a moment, and the camera feels it when the player is in it
						 //(the player isn't frozen when they are the one cut: on top of the flinch it read as a hitch)
							WP_SaberHitStop( ent );
							if ( victim->s.number != 0 )
							{
								WP_SaberHitStop( victim );
							}
							if ( victim->s.number == 0 )
							{
								WP_SaberCamKick( cut == CUT_GRAZE ? 0.25f : ( cut == CUT_LIMB ? 0.5f : 0.9f ),
									cut == CUT_GRAZE ? 100 : ( cut == CUT_LIMB ? 150 : 220 ) );
							}
							else if ( ent->s.number == 0 && cut == CUT_KILL )
							{
								WP_SaberCamKick( 0.25f, 100 );
							}
						}
						if ( severHand && victim->health > 0 )
						{
							// take them out of the fight first: G_Neutralize drops the gun as a real pickup
							// (so it isn't copied onto the hand) and does the mission bookkeeping
							G_FlagNPCAsKilled( victim, ent );
							// cut it off the living body (force: skip the damage-threshold check), then
							// allow a later killing blow to dismember again
							victim->client->dismembered = false;
							G_DoDismemberment( victim, dmgSpot[i], MOD_SABER, damage, hitDismemberLoc[i], qtrue );
							victim->client->dismembered = false;
							// cry out (their own pain100 or death scream), then a moment of shock in a held
							// pain anim before NPC_RunBehavior takes them to hands-up and/or running away
							if ( Q_irand( 0, 1 ) )
							{
								G_SoundOnEnt( victim, CHAN_VOICE_ATTEN, "*pain100.wav" );
							}
							else
							{
								G_AddEvent( victim, Q_irand( EV_DEATH1, EV_DEATH3 ), victim->health );
							}
							const int reactTime = Q_irand( 800, 1400 );
							TIMER_Set( victim, "maimedReact", reactTime );
							NPC_SetAnim( victim, SETANIM_BOTH, Q_irand( BOTH_PAIN1, BOTH_PAIN4 ), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
							victim->client->ps.torsoAnimTimer = victim->client->ps.legsAnimTimer = reactTime;
							if ( g_debugDamage->integer )
							{
								gi.Printf( "saber: hand severed on %d, non-lethal, disarmed, flagged as killed\n", victim->s.number );
							}
						}
						if ( damage > 0 && cg.time )
						{
							float sizeTimeScale = 1.0f;
							if ( (vicWasAlive
								  && victim->health <= 0 )
								|| (!vicWasDismembered
									&& victim->client->dismembered
									&& hitDismemberLoc[i] != HL_NONE
									&& hitDismember[i]) )
							{
								sizeTimeScale = 3.0f;
							}
							//FIXME: if not hitting the first model on the enemy, don't do this!
							CG_SaberDoWeaponHitMarks( ent->client,
								(ent->client->ps.saberInFlight?&g_entities[ent->client->ps.saberEntityNum]:NULL),
								victim,
								saberNum,
								bladeNum,
								dmgSpot[i],
								dmgDir[i],
								dmgBladeVec[i],
								dmgNormal[i],
								sizeTimeScale );
						}
#ifndef FINAL_BUILD
						if ( d_saberCombat->integer )
						{
							if ( (dFlags&DAMAGE_NO_DAMAGE) )
							{
								gi.Printf( S_COLOR_RED"damage: fake, hitLoc %d\n", hitLoc[i] );
							}
							else
							{
								gi.Printf( S_COLOR_RED"damage: %4.2f, hitLoc %d\n", totalDmg[i], hitLoc[i] );
							}
						}
#endif
						//do the effect
						//vec3_t splashBackDir;
						//VectorScale( dmgNormal[i], -1, splashBackDir );
						//G_PlayEffect( G_EffectIndex( "blood_sparks" ), dmgSpot[i], splashBackDir );
						if ( ent->s.number == 0 )
						{
							AddSoundEvent( victim->owner, dmgSpot[i], 256, AEL_DISCOVERED );
							AddSightEvent( victim->owner, dmgSpot[i], 512, AEL_DISCOVERED, 50 );
						}
						if ( ent->client )
						{
							if ( ent->enemy && ent->enemy == victim )
							{//just so Jedi knows that he hit his enemy
								ent->client->ps.saberEventFlags |= SEF_HITENEMY;
							}
							else
							{
								ent->client->ps.saberEventFlags |= SEF_HITOBJECT;
							}
						}
					}
				}
			}
		}
	}
	return didDamage;
}

void WP_SaberDamageAdd( float trDmg, int trVictimEntityNum, vec3_t trDmgDir, vec3_t trDmgBladeVec, vec3_t trDmgNormal, vec3_t trDmgSpot, float dmg, float fraction, int trHitLoc, qboolean trDismember, int trDismemberLoc )
{
	int curVictim = 0;
	int i;

	if ( trVictimEntityNum < 0 || trVictimEntityNum >= ENTITYNUM_WORLD )
	{
		return;
	}
	if ( trDmg * dmg < 10.0f )
	{//too piddly an amount of damage to really count?
		//FIXME: but already did the effect, didn't we... sigh...
		//return;
	}
	if ( trDmg )
	{//did some damage to something
		for ( i = 0; i < numVictims; i++ )
		{
			if ( victimEntityNum[i] == trVictimEntityNum )
			{//already hit this guy before
				curVictim = i;
				break;
			}
		}
		if ( i == numVictims )
		{//haven't hit his guy before
			if ( numVictims + 1 >= MAX_SABER_VICTIMS )
			{//can't add another victim at this time
				return;
			}
			//add a new victim to the list
			curVictim = numVictims;
			victimEntityNum[numVictims++] = trVictimEntityNum;
		}

		float addDmg = trDmg*dmg;
		if ( trHitLoc!=HL_NONE && (hitLoc[curVictim]==HL_NONE||saberLocationMod[trHitLoc]>saberLocationMod[hitLoc[curVictim]]) )
		{//this hitLoc is more damaging than the previous one this frame (a swing through neck and
		//shoulder counts as the head; used to prefer the "most health" location, which ranked the head low)
			hitLoc[curVictim] = trHitLoc;
		}

		totalDmg[curVictim] += addDmg;
		if ( !VectorLengthSquared( dmgDir[curVictim] ) )
		{
			VectorCopy( trDmgDir, dmgDir[curVictim] );
		}
		if ( !VectorLengthSquared( dmgBladeVec[curVictim] ) )
		{
			VectorCopy( trDmgBladeVec, dmgBladeVec[curVictim] );
		}
		if ( !VectorLengthSquared( dmgNormal[curVictim] ) )
		{
			VectorCopy( trDmgNormal, dmgNormal[curVictim] );
		}
		if ( !VectorLengthSquared( dmgSpot[curVictim] ) )
		{
			VectorCopy( trDmgSpot, dmgSpot[curVictim] );
		}

		// Make sure we keep track of the fraction.  Why?
		// Well, if the saber hits something that stops it, the damage isn't done past that point.
		dmgFraction[curVictim] = fraction;
		if ( (trDismemberLoc != HL_NONE && hitDismemberLoc[curVictim] == HL_NONE)
			|| (!hitDismember[curVictim] && trDismember) )
		{//either this is the first dismember loc we got or we got a loc before, but it wasn't a dismember loc, so take the new one
			hitDismemberLoc[curVictim] = trDismemberLoc;
		}
		if ( trDismember )
		{//we scored a dismemberment hit...
			hitDismember[curVictim] = trDismember;
		}
	}
}

/*
WP_SabersIntersect

Breaks the two saber paths into 2 tris each and tests each tri for the first saber path against each of the other saber path's tris

FIXME: subdivide the arc into a consistant increment
FIXME: test the intersection to see if the sabers really did intersect (weren't going in the same direction and/or passed through same point at different times)?
*/
extern qboolean tri_tri_intersect(vec3_t V0,vec3_t V1,vec3_t V2,vec3_t U0,vec3_t U1,vec3_t U2);
qboolean WP_SabersIntersect( gentity_t *ent1, int ent1SaberNum, int ent1BladeNum, gentity_t *ent2, qboolean checkDir )
{
	vec3_t	saberBase1, saberTip1, saberBaseNext1, saberTipNext1;
	vec3_t	saberBase2, saberTip2, saberBaseNext2, saberTipNext2;
	int		ent2SaberNum = 0, ent2BladeNum = 0;
	vec3_t	dir;

	/*
#ifndef FINAL_BUILD
	if ( d_saberCombat->integer )
	{
		gi.Printf( S_COLOR_GREEN"Doing precise saber intersection check\n" );
	}
#endif
	*/

	if ( !ent1 || !ent2 )
	{
		return qfalse;
	}
	if ( !ent1->client || !ent2->client )
	{
		return qfalse;
	}
	if ( ent1->client->ps.SaberLength() <= 0 || ent2->client->ps.SaberLength() <= 0 )
	{
		return qfalse;
	}

	for ( ent2SaberNum = 0; ent2SaberNum < MAX_SABERS; ent2SaberNum++ )
	{
		for ( ent2BladeNum = 0; ent2BladeNum < ent2->client->ps.saber[ent2SaberNum].numBlades; ent2BladeNum++ )
		{
			if ( ent2->client->ps.saber[ent2SaberNum].type != SABER_NONE
				&& ent2->client->ps.saber[ent2SaberNum].blade[ent2BladeNum].length > 0 )
			{//valid saber and this blade is on
				//if ( ent1->client->ps.saberInFlight )
				{
					VectorCopy( ent1->client->ps.saber[ent1SaberNum].blade[ent1BladeNum].muzzlePointOld, saberBase1 );
					VectorCopy( ent1->client->ps.saber[ent1SaberNum].blade[ent1BladeNum].muzzlePoint, saberBaseNext1 );

					VectorSubtract( ent1->client->ps.saber[ent1SaberNum].blade[ent1BladeNum].muzzlePoint, ent1->client->ps.saber[ent1SaberNum].blade[ent1BladeNum].muzzlePointOld, dir );
					VectorNormalize( dir );
					VectorMA( saberBaseNext1, SABER_EXTRAPOLATE_DIST, dir, saberBaseNext1 );

					VectorMA( saberBase1, ent1->client->ps.saber[ent1SaberNum].blade[ent1BladeNum].length, ent1->client->ps.saber[ent1SaberNum].blade[ent1BladeNum].muzzleDirOld, saberTip1 );
					VectorMA( saberBaseNext1, ent1->client->ps.saber[ent1SaberNum].blade[ent1BladeNum].length, ent1->client->ps.saber[ent1SaberNum].blade[ent1BladeNum].muzzleDir, saberTipNext1 );

					VectorSubtract( saberTipNext1, saberTip1, dir );
					VectorNormalize( dir );
					VectorMA( saberTipNext1, SABER_EXTRAPOLATE_DIST, dir, saberTipNext1 );
				}
				/*
				else
				{
					VectorCopy( ent1->client->ps.saber[ent1SaberNum].blade[ent1BladeNum].muzzlePoint, saberBase1 );
					VectorCopy( ent1->client->ps.saber[ent1SaberNum].blade[ent1BladeNum].muzzlePointNext, saberBaseNext1 );
					VectorMA( saberBase1, ent1->client->ps.saber[ent1SaberNum].blade[ent1BladeNum].length, ent1->client->ps.saber[ent1SaberNum].blade[ent1BladeNum].muzzleDir, saberTip1 );
					VectorMA( saberBaseNext1, ent1->client->ps.saber[ent1SaberNum].blade[ent1BladeNum].length, ent1->client->ps.saber[ent1SaberNum].blade[ent1BladeNum].muzzleDirNext, saberTipNext1 );
				}
				*/

				//if ( ent2->client->ps.saberInFlight )
				{
					VectorCopy( ent2->client->ps.saber[ent2SaberNum].blade[ent2BladeNum].muzzlePointOld, saberBase2 );
					VectorCopy( ent2->client->ps.saber[ent2SaberNum].blade[ent2BladeNum].muzzlePoint, saberBaseNext2 );

					VectorSubtract( ent2->client->ps.saber[ent2SaberNum].blade[ent2BladeNum].muzzlePoint, ent2->client->ps.saber[ent2SaberNum].blade[ent2BladeNum].muzzlePointOld, dir );
					VectorNormalize( dir );
					VectorMA( saberBaseNext2, SABER_EXTRAPOLATE_DIST, dir, saberBaseNext2 );

					VectorMA( saberBase2, ent2->client->ps.saber[ent2SaberNum].blade[ent2BladeNum].length, ent2->client->ps.saber[ent2SaberNum].blade[ent2BladeNum].muzzleDirOld, saberTip2 );
					VectorMA( saberBaseNext2, ent2->client->ps.saber[ent2SaberNum].blade[ent2BladeNum].length, ent2->client->ps.saber[ent2SaberNum].blade[ent2BladeNum].muzzleDir, saberTipNext2 );

					VectorSubtract( saberTipNext2, saberTip2, dir );
					VectorNormalize( dir );
					VectorMA( saberTipNext2, SABER_EXTRAPOLATE_DIST, dir, saberTipNext2 );
				}
				/*
				else
				{
					VectorCopy( ent2->client->ps.saber[ent2SaberNum].blade[ent2BladeNum].muzzlePoint, saberBase2 );
					VectorCopy( ent2->client->ps.saber[ent2SaberNum].blade[ent2BladeNum].muzzlePointNext, saberBaseNext2 );
					VectorMA( saberBase2, ent2->client->ps.saber[ent2SaberNum].blade[ent2BladeNum].length, ent2->client->ps.saber[ent2SaberNum].blade[ent2BladeNum].muzzleDir, saberTip2 );
					VectorMA( saberBaseNext2, ent2->client->ps.saber[ent2SaberNum].blade[ent2BladeNum].length, ent2->client->ps.saber[ent2SaberNum].blade[ent2BladeNum].muzzleDirNext, saberTipNext2 );
				}
				*/
				if ( checkDir )
				{//check the direction of the two swings to make sure the sabers are swinging towards each other
					vec3_t saberDir1, saberDir2;

					VectorSubtract( saberTipNext1, saberTip1, saberDir1 );
					VectorSubtract( saberTipNext2, saberTip2, saberDir2 );
					VectorNormalize( saberDir1 );
					VectorNormalize( saberDir2 );
					if ( DotProduct( saberDir1, saberDir2 ) > 0.6f )
					{//sabers moving in same dir, probably didn't actually hit
						continue;
					}
					//now check orientation of sabers, make sure they're not parallel or close to it
					float dot = DotProduct( ent1->client->ps.saber[ent1SaberNum].blade[ent1BladeNum].muzzleDir, ent2->client->ps.saber[ent2SaberNum].blade[ent2BladeNum].muzzleDir );
					if ( dot > 0.9f || dot < -0.9f )
					{//too parallel to really block effectively?
						continue;
					}
				}

				if ( tri_tri_intersect( saberBase1, saberTip1, saberBaseNext1, saberBase2, saberTip2, saberBaseNext2 ) )
				{
					return qtrue;
				}
				if ( tri_tri_intersect( saberBase1, saberTip1, saberBaseNext1, saberBase2, saberTip2, saberTipNext2 ) )
				{
					return qtrue;
				}
				if ( tri_tri_intersect( saberBase1, saberTip1, saberTipNext1, saberBase2, saberTip2, saberBaseNext2 ) )
				{
					return qtrue;
				}
				if ( tri_tri_intersect( saberBase1, saberTip1, saberTipNext1, saberBase2, saberTip2, saberTipNext2 ) )
				{
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

float WP_SabersDistance( gentity_t *ent1, gentity_t *ent2 )
{
	vec3_t	saberBaseNext1, saberTipNext1, saberPoint1;
	vec3_t	saberBaseNext2, saberTipNext2, saberPoint2;

	/*
#ifndef FINAL_BUILD
	if ( d_saberCombat->integer )
	{
		gi.Printf( S_COLOR_GREEN"Doing precise saber intersection check\n" );
	}
#endif
	*/

	if ( !ent1 || !ent2 )
	{
		return qfalse;
	}
	if ( !ent1->client || !ent2->client )
	{
		return qfalse;
	}
	if ( ent1->client->ps.SaberLength() <= 0 || ent2->client->ps.SaberLength() <= 0 )
	{
		return qfalse;
	}

	// Single-blade distance pre-check only; full multi-blade intersection is handled by WP_SabersIntersection()

	//if ( ent1->client->ps.saberInFlight )
	{
		VectorCopy( ent1->client->ps.saber[0].blade[0].muzzlePoint, saberBaseNext1 );
		VectorMA( saberBaseNext1, ent1->client->ps.saber[0].blade[0].length, ent1->client->ps.saber[0].blade[0].muzzleDir, saberTipNext1 );
	}
	/*
	else
	{
		VectorCopy( ent1->client->ps.saber[0].blade[0].muzzlePointNext, saberBaseNext1 );
		VectorMA( saberBaseNext1, ent1->client->ps.saber[0].blade[0].length, ent1->client->ps.saber[0].blade[0].muzzleDirNext, saberTipNext1 );
	}
	*/

	//if ( ent2->client->ps.saberInFlight )
	{
		VectorCopy( ent2->client->ps.saber[0].blade[0].muzzlePoint, saberBaseNext2 );
		VectorMA( saberBaseNext2, ent2->client->ps.saber[0].blade[0].length, ent2->client->ps.saber[0].blade[0].muzzleDir, saberTipNext2 );
	}
	/*
	else
	{
		VectorCopy( ent2->client->ps.saber[0].blade[0].muzzlePointNext, saberBaseNext2 );
		VectorMA( saberBaseNext2, ent2->client->ps.saber[0].blade[0].length, ent2->client->ps.saber[0].blade[0].muzzleDirNext, saberTipNext2 );
	}
	*/

	float sabersDist = ShortestLineSegBewteen2LineSegs( saberBaseNext1, saberTipNext1, saberBaseNext2, saberTipNext2, saberPoint1, saberPoint2 );

	//okay, this is a super hack, but makes saber collisions look better from the player point of view
	/*
	if ( sabersDist < 16.0f )
	{
		vec3_t	saberDistDir, saberMidPoint, camLookDir;

		VectorSubtract( saberPoint2, saberPoint1, saberDistDir );
		VectorMA( saberPoint1, 0.5f, saberDistDir, saberMidPoint );
		VectorSubtract( saberMidPoint, cg.refdef.vieworg, camLookDir );
		VectorNormalize( saberDistDir );
		VectorNormalize( camLookDir );
		float dot = fabs(DotProduct( camLookDir, saberDistDir ));
		sabersDist -= 8.0f*dot;
		if ( sabersDist < 0.0f )
		{
			sabersDist = 0.0f;
		}
	}
	*/

#ifndef FINAL_BUILD
	if ( d_saberCombat->integer > 2 )
	{
		G_DebugLine( saberPoint1, saberPoint2, FRAMETIME, 0x00ffffff, qtrue );
	}
#endif
	return sabersDist;
}

qboolean WP_SabersIntersection( gentity_t *ent1, gentity_t *ent2, vec3_t intersect )
{
	vec3_t	saberBaseNext1, saberTipNext1, saberPoint1;
	vec3_t	saberBaseNext2, saberTipNext2, saberPoint2;
	int		saberNum1, saberNum2, bladeNum1, bladeNum2;
	float	lineSegLength, bestLineSegLength = Q3_INFINITE;

	if ( !ent1 || !ent2 )
	{
		return qfalse;
	}
	if ( !ent1->client || !ent2->client )
	{
		return qfalse;
	}
	if ( ent1->client->ps.SaberLength() <= 0 || ent2->client->ps.SaberLength() <= 0 )
	{
		return qfalse;
	}

	//UGH, had to make this work for multiply-bladed sabers
	for ( saberNum1 = 0; saberNum1 < MAX_SABERS; saberNum1++ )
	{
		for ( bladeNum1 = 0; bladeNum1 < ent1->client->ps.saber[saberNum1].numBlades; bladeNum1++ )
		{
			if ( ent1->client->ps.saber[saberNum1].type != SABER_NONE
				&& ent1->client->ps.saber[saberNum1].blade[bladeNum1].length > 0 )
			{//valid saber and this blade is on
				for ( saberNum2 = 0; saberNum2 < MAX_SABERS; saberNum2++ )
				{
					for ( bladeNum2 = 0; bladeNum2 < ent2->client->ps.saber[saberNum2].numBlades; bladeNum2++ )
					{
						if ( ent2->client->ps.saber[saberNum2].type != SABER_NONE
							&& ent2->client->ps.saber[saberNum2].blade[bladeNum2].length > 0 )
						{//valid saber and this blade is on
							VectorCopy( ent1->client->ps.saber[saberNum1].blade[bladeNum1].muzzlePoint, saberBaseNext1 );
							VectorMA( saberBaseNext1, ent1->client->ps.saber[saberNum1].blade[bladeNum1].length, ent1->client->ps.saber[saberNum1].blade[bladeNum1].muzzleDir, saberTipNext1 );

							VectorCopy( ent2->client->ps.saber[saberNum2].blade[bladeNum2].muzzlePoint, saberBaseNext2 );
							VectorMA( saberBaseNext2, ent2->client->ps.saber[saberNum2].blade[bladeNum2].length, ent2->client->ps.saber[saberNum2].blade[bladeNum2].muzzleDir, saberTipNext2 );

							lineSegLength = ShortestLineSegBewteen2LineSegs( saberBaseNext1, saberTipNext1, saberBaseNext2, saberTipNext2, saberPoint1, saberPoint2 );
							if ( lineSegLength < bestLineSegLength )
							{
								bestLineSegLength = lineSegLength;
								VectorAdd( saberPoint1, saberPoint2, intersect );
								VectorScale( intersect, 0.5, intersect );
							}
						}
					}
				}
			}
		}
	}
	return qtrue;
}

const char *hit_blood_sparks = "sparks/blood_sparks2"; // could have changed this effect directly, but this is just safer in case anyone anywhere else is using the old one for something?
const char *hit_sparks = "saber/saber_cut";

qboolean WP_SaberDamageEffects( trace_t *tr, const vec3_t start, float length, float dmg, vec3_t dmgDir, vec3_t bladeVec, int enemyTeam, saberType_t saberType, saberInfo_t *saber, int bladeNum )
{

	int			hitEntNum[MAX_G2_COLLISIONS];
	for ( int hen = 0; hen < MAX_G2_COLLISIONS; hen++ )
	{
		hitEntNum[hen] = ENTITYNUM_NONE;
	}
	//NOTE: = {0} does NOT work on anything but bytes?
	float		hitEntDmgAdd[MAX_G2_COLLISIONS] = {0};
	float		hitEntDmgSub[MAX_G2_COLLISIONS] = {0};
	vec3_t		hitEntPoint[MAX_G2_COLLISIONS];
	vec3_t		hitEntNormal[MAX_G2_COLLISIONS];
	vec3_t		bladeDir;
	float		hitEntStartFrac[MAX_G2_COLLISIONS] = {0};
	int			trHitLoc[MAX_G2_COLLISIONS] = {HL_NONE};//same as 0
	int			trDismemberLoc[MAX_G2_COLLISIONS] = {HL_NONE};//same as 0
	qboolean	trDismember[MAX_G2_COLLISIONS] = {qfalse};//same as 0
	int			i,z;
	int			numHitEnts = 0;
	float		distFromStart,doDmg;
	int			hitEffect = 0;
	const char	*trSurfName;
	gentity_t	*hitEnt;

	VectorNormalize2( bladeVec, bladeDir );

	for (z=0; z < MAX_G2_COLLISIONS; z++)
	{
		if ( tr->G2CollisionMap[z].mEntityNum == -1 )
		{//actually, completely break out of this for loop since nothing after this in the aray should ever be valid either
			continue;//break;//
		}

		CCollisionRecord &coll	= tr->G2CollisionMap[z];
		//distFromStart			= Distance( start, coll.mCollisionPosition );
		distFromStart			= coll.mDistance;

		/*
		//FIXME: (distFromStart/length) is not guaranteed to be from 0 to 1... *sigh*...
		if ( length && saberHitFraction < 1.0f && (distFromStart/length) < 1.0f && (distFromStart/length) > saberHitFraction )
		{//a saber was hit before this point, don't count it
#ifndef FINAL_BUILD
			if ( d_saberCombat->integer )
			{
				gi.Printf( S_COLOR_MAGENTA"rejecting G2 collision- %4.2f farther than saberHitFraction %4.2f\n", (distFromStart/length), saberHitFraction  );
			}
#endif
			continue;
		}
		*/

		for ( i = 0; i < numHitEnts; i++ )
		{
			if ( hitEntNum[i] == coll.mEntityNum )
			{//we hit this ent before
				//we'll want to add this dist
				hitEntDmgAdd[i] = distFromStart;
				break;
			}
		}
		if ( i == numHitEnts )
		{//first time we hit this ent
			if ( numHitEnts == MAX_G2_COLLISIONS )
			{//hit too many damn ents!
				continue;
			}
			hitEntNum[numHitEnts] = coll.mEntityNum;
			if ( !coll.mFlags )
			{//hmm, we came out first, so we must have started inside
				//we'll want to subtract this dist
				hitEntDmgAdd[numHitEnts] = distFromStart;
			}
			else
			{//we're entering the model
				//we'll want to subtract this dist
				hitEntDmgSub[numHitEnts] = distFromStart;
			}
			//keep track of how far in the damage was done
			hitEntStartFrac[numHitEnts] = hitEntDmgSub[numHitEnts]/length;
			//remember the entrance point
			VectorCopy( coll.mCollisionPosition, hitEntPoint[numHitEnts] );
			//remember the normal of the face we hit
			VectorCopy( coll.mCollisionNormal, hitEntNormal[numHitEnts] );
			VectorNormalize( hitEntNormal[numHitEnts] );

			//do the effect

			//FIXME: check material rather than team?
			hitEnt = &g_entities[hitEntNum[numHitEnts]];
			if ( dmg > 1.0f && hitEnt && !hitEnt->client && coll.mModelIndex == 0
				&& hitEnt->count == BOTH_DISMEMBER_TORSO1 && !Q_stricmp( "limb", hitEnt->classname ) )
			{//JKEnhanced: a real swing through the top half of someone cut in two takes off the head / arm it hit
				extern qboolean G_TorsoLimbCut( gentity_t *limb, const char *surfName, const vec3_t point, const vec3_t dir );
				G_TorsoLimbCut( hitEnt, gi.G2API_GetSurfaceName( &hitEnt->ghoul2[0], coll.mSurfaceIndex ), coll.mCollisionPosition, dmgDir );
			}
			if ( hitEnt
				&& hitEnt->client
				&& coll.mModelIndex > 0 )
			{//hit a submodel on the enemy, not their actual body!
				if ( !WP_SaberBladeUseSecondBladeStyle( saber, bladeNum )
					&& saber->hitOtherEffect )
				{
					hitEffect = saber->hitOtherEffect;
				}
				else if ( WP_SaberBladeUseSecondBladeStyle( saber, bladeNum )
					&& saber->hitOtherEffect2 )
				{
					hitEffect = saber->hitOtherEffect2;
				}
				else
				{
					hitEffect = G_EffectIndex( hit_sparks );
				}
			}
			else
			{
				if ( !WP_SaberBladeUseSecondBladeStyle( saber, bladeNum )
					&& saber->hitPersonEffect )
				{
					hitEffect = saber->hitPersonEffect;
				}
				else if ( WP_SaberBladeUseSecondBladeStyle( saber, bladeNum )
					&& saber->hitPersonEffect2 )
				{
					hitEffect = saber->hitPersonEffect2;
				}
				else
				{
					hitEffect = G_EffectIndex( hit_blood_sparks );
				}
			}
			if ( hitEnt != NULL )
			{
				if ( hitEnt->client )
				{
					class_t npc_class = hitEnt->client->NPC_class;
					if ( npc_class == CLASS_SEEKER || npc_class == CLASS_PROBE || npc_class == CLASS_MOUSE || npc_class == CLASS_REMOTE ||
					     npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class == CLASS_R5D2 ||
					     npc_class == CLASS_PROTOCOL || npc_class == CLASS_MARK1 || npc_class == CLASS_MARK2 ||
					     npc_class == CLASS_INTERROGATOR || npc_class == CLASS_ATST || npc_class == CLASS_SENTRY )
					{
						if ( !WP_SaberBladeUseSecondBladeStyle( saber, bladeNum )
							&& saber->hitOtherEffect )
						{
							hitEffect = saber->hitOtherEffect;
						}
						else if ( WP_SaberBladeUseSecondBladeStyle( saber, bladeNum )
							&& saber->hitOtherEffect2 )
						{
							hitEffect = saber->hitOtherEffect2;
						}
						else
						{
							hitEffect = G_EffectIndex( hit_sparks );
						}
					}
				}
				else
				{
					// So sue me, this is the easiest way to check to see if this is the turbo laser from t2_wedge,
					// in which case I don't want the saber effects goin off on it.
					if ( (hitEnt->flags&FL_DMG_BY_HEAVY_WEAP_ONLY)
						&& hitEnt->takedamage == qfalse
						&& Q_stricmp( hitEnt->classname, "misc_turret" ) == 0 )
					{
						continue;
					}
					else
					{
						if ( dmg )
						{//only do these effects if actually trying to damage the thing...
							if ( (hitEnt->svFlags&SVF_BBRUSH)//a breakable brush
								&& ( (hitEnt->spawnflags&1)//INVINCIBLE
									||(hitEnt->flags&FL_DMG_BY_HEAVY_WEAP_ONLY) )//HEAVY weapon damage only
								)
							{//no hit effect (besides regular client-side one)
								hitEffect = 0;
							}
							else
							{
								if ( !WP_SaberBladeUseSecondBladeStyle( saber, bladeNum )
									&& saber->hitOtherEffect )
								{
									hitEffect = saber->hitOtherEffect;
								}
								else if ( WP_SaberBladeUseSecondBladeStyle( saber, bladeNum )
									&& saber->hitOtherEffect2 )
								{
									hitEffect = saber->hitOtherEffect2;
								}
								else
								{
									hitEffect = G_EffectIndex( hit_sparks );
								}
							}
						}
					}
				}
			}

			//FIXME: play less if damage is less?
			if ( !g_saberNoEffects )
			{
				if ( hitEffect != 0 )
				{
					//FIXME: when you have multiple blades hitting someone for many sequential server frames, this can get a bit chuggy!
					G_PlayEffect( hitEffect, coll.mCollisionPosition, coll.mCollisionNormal );
				}
			}

			//Get the hit location based on surface name
			if ( (hitLoc[numHitEnts] == HL_NONE && trHitLoc[numHitEnts] == HL_NONE)
				|| (hitDismemberLoc[numHitEnts] == HL_NONE && trDismemberLoc[numHitEnts] == HL_NONE)
				|| (!hitDismember[numHitEnts] && !trDismember[numHitEnts]) )
			{//no hit loc set for this ent this damage cycle yet
				//FIXME: find closest impact surf *first* (per ent), then call G_GetHitLocFromSurfName?
				//FIXED: if hit multiple ents in this collision record, these trSurfName, trDismember and trDismemberLoc will get stomped/confused over the multiple ents I hit
				trSurfName = gi.G2API_GetSurfaceName( &g_entities[coll.mEntityNum].ghoul2[coll.mModelIndex], coll.mSurfaceIndex );
				trDismember[numHitEnts] = G_GetHitLocFromSurfName( &g_entities[coll.mEntityNum], trSurfName, &trHitLoc[numHitEnts], coll.mCollisionPosition, dmgDir, bladeDir, MOD_SABER, saberType );
				if ( trDismember[numHitEnts] )
				{
					trDismemberLoc[numHitEnts] = trHitLoc[numHitEnts];
				}
				/*
				if ( trDismember[numHitEnts] )
				{
					Com_Printf( S_COLOR_RED"Okay to dismember %s on ent %d\n", hitLocName[trDismemberLoc[numHitEnts]], hitEntNum[numHitEnts] );
				}
				else
				{
					Com_Printf( "Hit (no dismember) %s on ent %d\n", hitLocName[trHitLoc[numHitEnts]], hitEntNum[numHitEnts] );
				}
				*/
			}
			numHitEnts++;
		}
	}

	//now go through all the ents we hit and do the damage
	for ( i = 0; i < numHitEnts; i++ )
	{
		doDmg = dmg;
		if ( hitEntNum[i] != ENTITYNUM_NONE )
		{
			if ( doDmg < 10 )
			{//base damage is less than 10
				if ( hitEntNum[i] != 0 )
				{//not the player
					hitEnt = &g_entities[hitEntNum[i]];
					if ( !hitEnt->client || (hitEnt->client->ps.weapon!=WP_SABER&&hitEnt->client->NPC_class!=CLASS_GALAKMECH&&hitEnt->client->playerTeam==enemyTeam) )
					{//did *not* hit a jedi and did *not* hit the player
						//make sure the base damage is high against non-jedi, feels better
						doDmg = 10;
					}
				}
			}
			if ( !hitEntDmgAdd[i] && !hitEntDmgSub[i] )
			{//spent entire time in model
				//NOTE: will we even get a collision then?
				doDmg *= length;
			}
			else if ( hitEntDmgAdd[i] && hitEntDmgSub[i] )
			{//we did enter and exit
				doDmg *= hitEntDmgAdd[i] - hitEntDmgSub[i];
			}
			else if ( !hitEntDmgAdd[i] )
			{//we didn't exit, just entered
				doDmg *= length - hitEntDmgSub[i];
			}
			else if ( !hitEntDmgSub[i] )
			{//we didn't enter, only exited
				doDmg *= hitEntDmgAdd[i];
			}
			if ( doDmg > 0 )
			{
				WP_SaberDamageAdd( 1.0, hitEntNum[i], dmgDir, bladeVec, hitEntNormal[i], hitEntPoint[i], ceil(doDmg), hitEntStartFrac[i], trHitLoc[i], trDismember[i], trDismemberLoc[i] );
			}
		}
	}
	return (qboolean)(numHitEnts>0);
}

void WP_SaberBlockEffect( gentity_t *attacker, int saberNum, int bladeNum, vec3_t position, vec3_t normal, qboolean cutNotBlock )
{
	saberInfo_t *saber = NULL;

	if ( attacker && attacker->client )
	{
		saber = &attacker->client->ps.saber[saberNum];
	}

	if ( saber
		&& !WP_SaberBladeUseSecondBladeStyle( saber, bladeNum )
		&& saber->blockEffect )
	{
		if ( normal )
		{
			G_PlayEffect( saber->blockEffect, position, normal );
		}
		else
		{
			G_PlayEffect( saber->blockEffect, position );
		}
	}
	else if ( saber
		&& WP_SaberBladeUseSecondBladeStyle( saber, bladeNum )
		&& saber->blockEffect2 )
	{
		if ( normal )
		{
			G_PlayEffect( saber->blockEffect2, position, normal );
		}
		else
		{
			G_PlayEffect( saber->blockEffect2, position );
		}
	}
	else if ( cutNotBlock )
	{
		if ( normal )
		{
			G_PlayEffect( "saber/saber_cut", position, normal );
		}
		else
		{
			G_PlayEffect( "saber/saber_cut", position );
		}
	}
	else
	{

		if ( normal )
		{
			G_PlayEffect( "saber/saber_block", position, normal );
		}
		else
		{
			G_PlayEffect( "saber/saber_block", position );
		}
	}
}

void WP_SaberKnockaway( gentity_t *attacker, trace_t *tr )
{
	WP_SaberDrop( attacker, &g_entities[attacker->client->ps.saberEntityNum] );
	WP_SaberBlockSound( attacker, NULL, 0, 0 );
	//G_Sound( &g_entities[attacker->client->ps.saberEntityNum], G_SoundIndex( va( "sound/weapons/saber/saberblock%d.wav", Q_irand(1, 9) ) ) );
	WP_SaberBlockEffect( attacker, 0, 0, tr->endpos, NULL, qfalse );
	saberHitFraction = tr->fraction;
#ifndef FINAL_BUILD
	if ( d_saberCombat->integer )
	{
		gi.Printf( S_COLOR_MAGENTA"WP_SaberKnockaway: saberHitFraction %4.2f\n", saberHitFraction );
	}
#endif
	VectorCopy( tr->endpos, saberHitLocation );
	saberHitEntity = tr->entityNum;
	if ( !g_noClashFlare )
	{
		g_saberFlashTime = level.time-50;
		VectorCopy( saberHitLocation, g_saberFlashPos );
	}

	//FIXME: make hitEnt play an attack anim or some other special anim when this happens
	//gentity_t *hitEnt = &g_entities[tr->entityNum];
	//NPC_SetAnim( hitEnt, SETANIM_BOTH, BOTH_KNOCKSABER, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
}

qboolean G_InCinematicSaberAnim( gentity_t *self )
{
	if ( self->NPC
		&& self->NPC->behaviorState == BS_CINEMATIC
		&& (self->client->ps.torsoAnim == BOTH_CIN_16 ||self->client->ps.torsoAnim == BOTH_CIN_17) )
	{
		return qtrue;
	}
	return qfalse;
}

#define SABER_COLLISION_DIST 6//was 2//was 4//was 8//was 16
extern qboolean InFront( vec3_t spot, vec3_t from, vec3_t fromAngles, float threshHold = 0.0f );
qboolean WP_SaberDamageForTrace( int ignore, vec3_t start, vec3_t end, float dmg,
								vec3_t bladeDir, qboolean noGhoul, int attackStrength,
								saberType_t saberType, qboolean extrapolate,
								int saberNum, int bladeNum )
{
	trace_t		tr;
	vec3_t		dir;
	int			mask = (MASK_SHOT|CONTENTS_LIGHTSABER);
	gentity_t	*attacker = &g_entities[ignore];

	vec3_t		end2;
	VectorCopy( end, end2 );
	if ( extrapolate )
	{
		//NOTE: since we can no longer use the predicted point, extrapolate the trace some.
		//		this may allow saber hits that aren't actually hits, but it doesn't look too bad
		vec3_t diff;
		VectorSubtract( end, start, diff );
		VectorNormalize( diff );
		VectorMA( end2, SABER_EXTRAPOLATE_DIST, diff, end2 );
	}

	if ( !noGhoul )
	{
		float useRadiusForDamage = 0;

		if ( attacker
			&& attacker->client )
		{//see if we're not drawing the blade, if so, do a trace based on radius of blade (because the radius is being used to simulate a larger/smaller piece of a solid weapon)...
			if ( !WP_SaberBladeUseSecondBladeStyle( &attacker->client->ps.saber[saberNum], bladeNum )
				&& (attacker->client->ps.saber[saberNum].saberFlags2&SFL2_NO_BLADE) )
			{//not drawing blade
				useRadiusForDamage = attacker->client->ps.saber[saberNum].blade[bladeNum].radius;
			}
			else if ( WP_SaberBladeUseSecondBladeStyle( &attacker->client->ps.saber[saberNum], bladeNum )
				&& (attacker->client->ps.saber[saberNum].saberFlags2&SFL2_NO_BLADE2) )
			{//not drawing blade
				useRadiusForDamage = attacker->client->ps.saber[saberNum].blade[bladeNum].radius;
			}
		}
		if ( !useRadiusForDamage )
		{//do normal check for larger-size saber traces
			if ( !attacker->s.number
				|| (attacker->client
					&& (attacker->client->playerTeam==TEAM_PLAYER
						|| attacker->client->NPC_class==CLASS_SHADOWTROOPER
						|| attacker->client->NPC_class==CLASS_ALORA
						|| (attacker->NPC && (attacker->NPC->aiFlags&NPCAI_BOSS_CHARACTER))
						)
					)
				)//&&attackStrength==FORCE_LEVEL_3)
			{
				useRadiusForDamage = 2;
			}
		}

		if ( useRadiusForDamage > 0 )//&&attackStrength==FORCE_LEVEL_3)
		{//player,. player allies, shadowtroopers, tavion and desann use larger traces
			vec3_t	traceMins = {-useRadiusForDamage,-useRadiusForDamage,-useRadiusForDamage}, traceMaxs = {useRadiusForDamage,useRadiusForDamage,useRadiusForDamage};
			gi.trace( &tr, start, traceMins, traceMaxs, end2, ignore, mask, G2_COLLIDE, 10 );//G2_SUPERSIZEDBBOX
		}
		/*
		else if ( !attacker->s.number )
		{
			vec3_t	traceMins = {-1,-1,-1}, traceMaxs = {1,1,1};
			gi.trace( &tr, start, traceMins, traceMaxs, end2, ignore, mask, G2_COLLIDE, 10 );//G2_SUPERSIZEDBBOX
		}
		*/
		else
		{//reborn use smaller traces
			gi.trace( &tr, start, NULL, NULL, end2, ignore, mask, G2_COLLIDE, 10 );//G2_SUPERSIZEDBBOX
		}
	}
	else
	{
		gi.trace( &tr, start, NULL, NULL, end2, ignore, mask, G2_NOCOLLIDE, 10 );
	}


#ifndef FINAL_BUILD
	if ( d_saberCombat->integer > 1 )
	{
		if ( attacker != NULL && attacker->client != NULL )
		{
			G_DebugLine(start, end2, FRAMETIME, WPDEBUG_SaberColor( attacker->client->ps.saber[0].blade[0].color ), qtrue);
		}
	}
#endif

	if ( tr.entityNum == ENTITYNUM_NONE )
	{
		return qfalse;
	}

	if ( tr.entityNum == ENTITYNUM_WORLD )
	{
		if ( attacker && attacker->client && (attacker->client->ps.saber[saberNum].saberFlags&SFL_BOUNCE_ON_WALLS) )
		{
			VectorCopy( tr.endpos, saberHitLocation );
			VectorCopy( tr.plane.normal, saberHitNormal );
		}
		return qtrue;
	}

	if ( &g_entities[tr.entityNum] )
	{
		gentity_t *hitEnt = &g_entities[tr.entityNum];
		gentity_t *owner = g_entities[tr.entityNum].owner;
		if ( hitEnt->contents & CONTENTS_LIGHTSABER )
		{
			if ( attacker && attacker->client && attacker->client->ps.saberInFlight )
			{//thrown saber hit something
				 if ( owner
					 && owner->s.number
					 && owner->client
					 && owner->NPC
					 && owner->health > 0 )
				 {
					 if ( owner->client->NPC_class == CLASS_ALORA )
					 {//alora takes less damage
						dmg *= 0.25f;
					 }
					 else if ( owner->client->NPC_class == CLASS_TAVION
							/*|| (owner->client->NPC_class == CLASS_SHADOWTROOPER && !Q_irand( 0, g_spskill->integer*3 ))
							|| (Q_irand( -5, owner->NPC->rank ) > RANK_CIVILIAN && !Q_irand( 0, g_spskill->integer*3 ))*/ )
					{//Tavion can toss a blocked thrown saber aside
						WP_SaberKnockaway( attacker, &tr );
						Jedi_PlayDeflectSound( owner );
						return qfalse;
					}
				}
			}
			//FIXME: take target FP_SABER_DEFENSE and attacker FP_SABER_OFFENSE into account here somehow?
			qboolean sabersIntersect = WP_SabersIntersect( attacker, saberNum, bladeNum, owner, qfalse );//qtrue );
			float sabersDist;
			if ( attacker && attacker->client && attacker->client->ps.saberInFlight
				&& owner && owner->s.number == 0 && (g_saberAutoBlocking->integer||attacker->client->ps.saberBlockingTime>level.time) )//NPC flying saber hit player's saber bounding box
			{//players have g_saberAutoBlocking, do the more generous check against flying sabers
				//FIXME: instead of hitting the player's saber bounding box
				//and picking an anim afterwards, have him use AI similar
				//to the AI the jedi use for picking a saber melee block...?
				sabersDist = 0;
			}
			else
			{//sabers must actually collide with the attacking saber
				sabersDist = WP_SabersDistance( attacker, owner );
				if ( attacker && attacker->client && attacker->client->ps.saberInFlight )
				{
					sabersDist /= 2.0f;
					if ( sabersDist <= 16.0f )
					{
						sabersIntersect = qtrue;
					}
				}
#ifndef FINAL_BUILD
				if ( d_saberCombat->integer > 1 )
				{
					gi.Printf( "sabersDist: %4.2f\n", sabersDist );
				}
#endif//FINAL_BUILD
			}
			qboolean guardMiss = qfalse;	//no guard up: their saber only counts if the blades really meet
			if ( WP_SaberRules() && owner && owner->client && attacker && attacker->client && owner != attacker
				&& !attacker->client->ps.saberInFlight && !owner->client->ps.saberInFlight
				&& !G_InCinematicSaberAnim( owner ) )
			{//JKEnhanced "a blade in the way"
				const guardState_t gs = WP_GuardState( owner, attacker );
				if ( gs == GUARD_UP )
				{//a raised guard stops it, wherever the two blades happen to be
					sabersIntersect = qtrue;
					sabersDist = 0;
				}
				else if ( owner->client->ps.saberLockTime < level.time )
				{//no guard (standing, swinging, staggered, busy with a Force power): the saber still stops a
				 //swing that actually runs into the blade - a blade is a blade - but nothing short of that
					guardMiss = qtrue;
					if ( owner->s.number == 0 && WP_SaberSwinging( &attacker->client->ps ) )
					{
						WP_NoteGuardFail( owner, attacker, gs );
					}
				}
			}
			if ( !guardMiss && ( sabersCrossed == -1 || sabersCrossed > sabersDist ) )
			{
				sabersCrossed = sabersDist;
			}
			float	collisionDist;
			if ( g_saberRealisticCombat->integer )
			{
				collisionDist = SABER_COLLISION_DIST;
			}
			else
			{
				collisionDist = SABER_COLLISION_DIST+6+g_spskill->integer*4;
			}
			{
			if ( G_InCinematicSaberAnim( owner )
				&& G_InCinematicSaberAnim( attacker ) )
			{
				sabersIntersect = qtrue;
			}
			}
			if ( owner && owner->client && (attacker != NULL)
				&& ( ( (sabersDist > ( guardMiss ? (float)SABER_COLLISION_DIST : collisionDist ) )//|| !InFront( attacker->currentOrigin, owner->currentOrigin, owner->client->ps.viewangles, 0.35f ))
						&& !sabersIntersect )//was qtrue, but missed too much?
					|| ( owner != attacker && PM_InKnockDown( &owner->client->ps ) ) ) )//JKEnhanced: a blade lying on the floor with its owner stops nothing
			{//swing came from behind and/or was not stopped by a lightsaber
				//re-try the trace without checking for lightsabers
				//(JKEnhanced saber rules: no guard in the way - against the model, so the cut lands where the blade is)
				gi.trace ( &tr, start, NULL, NULL, end2, ignore, mask&~CONTENTS_LIGHTSABER, ( guardMiss && !noGhoul ) ? G2_COLLIDE : G2_NOCOLLIDE, 10 );
				if ( tr.entityNum == ENTITYNUM_WORLD )
				{
 					return qtrue;
				}
				if ( tr.entityNum == ENTITYNUM_NONE || &g_entities[tr.entityNum] == NULL )
				{//didn't hit the owner
					/*
					if ( attacker
						&& attacker->client
						&& (PM_SaberInAttack( attacker->client->ps.saberMove ) || PM_SaberInStart( attacker->client->ps.saberMove ))
						&& DistanceSquared( tr.endpos, owner->currentOrigin ) < 10000 )
					{
						if ( owner->NPC
							&& !owner->client->ps.saberInFlight
							&& owner->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN
							&& !Jedi_SaberBusy( owner ) )
						{//owner parried, just make sure they're saber is in the right spot - only does this if they're not already doing something with saber
							if ( g_spskill->integer && (g_spskill->integer > 1 || Q_irand( 0, 1 )))
							{//if on easy, they don't cheat like this, if on medium, they cheat 50% of the time, if on hard, they always cheat
								//FIXME: also take into account the owner's FP_DEFENSE?
								if ( Q_irand( 0, owner->NPC->rank ) >= RANK_CIVILIAN )
								{//lower-rank Jedi aren't as good blockers
									vec3_t attDir;
									VectorSubtract( end2, start, attDir );
									VectorNormalize( attDir );
									Jedi_SaberBlockGo( owner, owner->NPC->last_ucmd, start, attDir, NULL );
								}
							}
						}
					}
					*/
  					return qfalse;	// Exit, but we didn't hit the wall.
				}
#ifndef FINAL_BUILD
				if ( d_saberCombat->integer > 1 )
				{
					if ( !attacker->s.number )
					{
						gi.Printf( S_COLOR_MAGENTA"%d saber hit owner through saber %4.2f, dist = %4.2f\n", level.time, saberHitFraction, sabersDist );
					}
				}
#endif//FINAL_BUILD
				hitEnt = &g_entities[tr.entityNum];
				owner = g_entities[tr.entityNum].owner;
			}
			else
			{//hit a lightsaber
				if ( (tr.fraction < saberHitFraction || tr.startsolid)
					&& sabersDist < (8.0f+g_spskill->value)*4.0f// 50.0f//16.0f
					&& (sabersIntersect || sabersDist < (4.0f+g_spskill->value)*2.0f) )//32.0f) )
				{	// This saber hit closer than the last one.
					if ( (tr.allsolid || tr.startsolid) && owner && owner->client )
					{//tr.fraction will be 0, unreliable... so calculate actual
						float dist = Distance( start, end2 );
						if ( dist )
						{
							float hitFrac = WP_SabersDistance( attacker, owner )/dist;
							if ( hitFrac > 1.0f )
							{//umm... minimum distance between sabers was longer than trace...?
								hitFrac = 1.0f;
							}
							if ( hitFrac < saberHitFraction )
							{
								saberHitFraction = hitFrac;
							}
						}
						else
						{
							saberHitFraction = 0.0f;
						}
#ifndef FINAL_BUILD
						if ( d_saberCombat->integer > 1 )
						{
							if ( !attacker->s.number )
							{
								gi.Printf( S_COLOR_GREEN"%d saber hit saber dist %4.2f allsolid %4.2f\n", level.time, sabersDist, saberHitFraction );
							}
						}
#endif//FINAL_BUILD
					}
					else
					{
#ifndef FINAL_BUILD
						if ( d_saberCombat->integer > 1 )
						{
							if ( !attacker->s.number )
							{
								gi.Printf( S_COLOR_BLUE"%d saber hit saber dist %4.2f, frac %4.2f\n", level.time, sabersDist, saberHitFraction );
							}
							saberHitFraction = tr.fraction;
						}
#endif//FINAL_BUILD
					}
#ifndef FINAL_BUILD
					if ( d_saberCombat->integer )
					{
						gi.Printf( S_COLOR_MAGENTA"hit saber: saberHitFraction %4.2f, allsolid %d, startsolid %d\n", saberHitFraction, tr.allsolid, tr.startsolid );
					}
#endif//FINAL_BUILD
					VectorCopy(tr.endpos, saberHitLocation);
					saberHitEntity = tr.entityNum;
				}
				/*
				if ( owner
					&& owner->client
					&& attacker
					&& attacker->client
					&& (PM_SaberInAttack( attacker->client->ps.saberMove ) || PM_SaberInStart( attacker->client->ps.saberMove ))
					&& DistanceSquared( tr.endpos, owner->currentOrigin ) < 10000 )
				{
					if ( owner->NPC
						&& !owner->client->ps.saberInFlight
						&& owner->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN
						&& !Jedi_SaberBusy( owner ) )
					{//owner parried, just make sure they're saber is in the right spot - only does this if they're not already doing something with saber
						if ( g_spskill->integer && (g_spskill->integer > 1 || Q_irand( 0, 1 )))
						{//if on easy, they don't cheat like this, if on medium, they cheat 50% of the time, if on hard, they always cheat
							//FIXME: also take into account the owner's FP_DEFENSE?
							if ( Q_irand( 0, owner->NPC->rank ) >= RANK_CIVILIAN )
							{//lower-rank Jedi aren't as good blockers
								vec3_t attDir;
								VectorSubtract( end2, start, attDir );
								VectorNormalize( attDir );
								Jedi_SaberBlockGo( owner, owner->NPC->last_ucmd, start, attDir, NULL );
							}
						}
					}
				}
				*/
				//FIXME: check to see if we broke the saber
				//		go through the impacted surfaces and call WP_BreakSaber
				//		PROBLEM: saberEnt doesn't actually have a saber g2 model
				//		and/or isn't in same location as saber model attached
				//		to the client.  We'd have to fake it somehow...
  				return qfalse;	// Exit, but we didn't hit the wall.
			}
		}
		else
		{
#ifndef FINAL_BUILD
			if ( d_saberCombat->integer > 1 )
			{
				if ( !attacker->s.number )
				{
					gi.Printf( S_COLOR_RED"%d saber hit owner directly %4.2f\n", level.time, saberHitFraction );
				}
			}
#endif//FINAL_BUILD
		}

		if ( attacker && attacker->client && attacker->client->ps.saberInFlight )
		{//thrown saber hit something
			if ( ( hitEnt && hitEnt->client && hitEnt->health > 0 && ( hitEnt->client->NPC_class == CLASS_DESANN || !Q_stricmp("Yoda",hitEnt->NPC_type) || hitEnt->client->NPC_class == CLASS_LUKE || hitEnt->client->NPC_class == CLASS_BOBAFETT || (hitEnt->client->ps.powerups[PW_GALAK_SHIELD] > 0) ) ) ||
				 ( owner && owner->client && owner->health > 0 && ( owner->client->NPC_class == CLASS_DESANN || !Q_stricmp("Yoda",owner->NPC_type) || owner->client->NPC_class == CLASS_LUKE || (owner->client->ps.powerups[PW_GALAK_SHIELD] > 0) ) ) )
			{//Luke and Desann slap thrown sabers aside
				//FIXME: control the direction of the thrown saber... if hit Galak's shield, bounce directly away from his origin?
				WP_SaberKnockaway( attacker, &tr );
				if ( hitEnt->client )
				{
					Jedi_PlayDeflectSound( hitEnt );
				}
				else
				{
					Jedi_PlayDeflectSound( owner );
				}
  				return qfalse;	// Exit, but we didn't hit the wall.
			}
		}

		const qboolean rulesSwing = (qboolean)( WP_SaberRules() && attacker && attacker->client
			&& !attacker->client->ps.saberInFlight );
		if ( rulesSwing && hitEnt && hitEnt->client && hitEnt != attacker
			&& ( PM_SaberInAttack( attacker->client->ps.saberMove ) || PM_SaberInSpecialAttack( attacker->client->ps.torsoAnim ) )//a live strike, not a wind-up brushing them
			&& WP_GuardState( hitEnt, attacker ) == GUARD_UP )
		{//JKEnhanced saber rules: the blade got round the guard's box, but the guard is up and facing it -
		 //stopped all the same (same cost and bounce as a hit on the box)
			VectorCopy( tr.endpos, saberHitLocation );
			WP_SaberGuardStop( attacker, hitEnt, saberNum, bladeNum, WP_IsHeavyAttack( attacker ),
				PM_PowerLevelForSaberAnim( &attacker->client->ps, saberNum ) );
			if ( attacker->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
			{
				attacker->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
			}
			return qfalse;	// Exit, but we didn't hit the wall.
		}
		if ( !rulesSwing//(saber rules: no guard up means no block, whatever pose they're in)
			&& hitEnt && hitEnt->client && hitEnt != attacker && hitEnt->health > 0
			&& hitEnt->client->ps.weapon == WP_SABER
			&& !hitEnt->client->ps.saberInFlight
			&& hitEnt->client->ps.saberLockTime < level.time
			&& ( hitEnt->s.number != 0 || g_saberAutoBlocking->integer || hitEnt->client->ps.saberBlockingTime > level.time )//actually trying to block, not just standing in ready stance
			&& ( PM_SaberInParry( hitEnt->client->ps.saberMove ) || hitEnt->client->ps.saberMove == LS_READY )
			&& !PM_InKnockDown( &hitEnt->client->ps ) && !G_IsDownedBody( hitEnt ) )//JKEnhanced: lying on the floor in LS_READY isn't a block pose (every hit on a downed NPC was "blocked")
		{//the blade geometrically slipped past their saber (missed the CONTENTS_LIGHTSABER trace/re-trace
		//above) even though they were actively holding a block/parry pose -- resolve it as a proper
		//block (anim + force cost) via the same self-contained function used for missile/saber blocks
		//elsewhere, instead of letting raw, unmitigated damage through. Don't fake saberHitFraction/
		//saberHitEntity here: those feed WP_SabersDamageTrace's hitOwner lookup via hitEnt->owner,
		//which expects a saber-prop entity, not the client entity itself -- stamping the client's own
		//number there previously caused a null hitOwner crash a few frames later.
			WP_SaberBlockNonRandom( hitEnt, tr.endpos, qfalse );
			if ( hitEnt->client->ps.saberBlocked != BLOCKED_NONE )
			{//the block actually took (they had force power / were eligible to auto-block) -- no damage through
				return qfalse;	// Exit, but we didn't hit the wall.
			}
			//else: couldn't actually block (e.g. out of force power) -- fall through to normal damage below
		}

		if ( hitEnt->takedamage
			|| ( !hitEnt->client && hitEnt->count == BOTH_DISMEMBER_TORSO1 && hitEnt->ghoul2.size() && !Q_stricmp( "limb", hitEnt->classname ) ) )//JKEnhanced: the top half of someone cut in two (G_TorsoLimbCut via WP_SaberDamageEffects; G_Damage ignores it)
		{
			//no team damage: if ( !hitEnt->client || attacker == NULL || !attacker->client || (hitEnt->client->playerTeam != attacker->client->playerTeam) )
			{
				vec3_t bladeVec={0};
				if ( attacker && attacker->client )
				{
					VectorScale( bladeDir, attacker->client->ps.saber[saberNum].blade[bladeNum].length, bladeVec );
				}
				//multiply the damage by the total distance of the swipe
				VectorSubtract( end2, start, dir );
				float len = VectorNormalize( dir );//VectorLength( dir );
				if ( noGhoul || !hitEnt->ghoul2.size() )
				{//we weren't doing a ghoul trace
					int hitEffect = 0;
					if ( dmg >= 1.0 && hitEnt->bmodel )
					{
						dmg = 1.0;
					}
					if ( len > 1 )
					{
						dmg *= len;
					}
#ifndef FINAL_BUILD
					if ( d_saberCombat->integer > 1 )
					{
						if ( !(hitEnt->contents & CONTENTS_LIGHTSABER) )
						{
							gi.Printf( S_COLOR_GREEN"Hit ent, but no ghoul collisions\n" );
						}
					}
#endif
					float	trFrac, dmgFrac;
					if ( tr.allsolid )
					{//totally inside them
						trFrac = 1.0;
						dmgFrac = 0.0;
					}
					else if ( tr.startsolid )
					{//started inside them
						//we don't know how much was inside, we know it's less than all, so use half?
						trFrac = 0.5;
						dmgFrac = 0.0;
					}
					else
					{//started outside them and hit them
						//yeah. this doesn't account for coming out the other wide, but we can worry about that later (use ghoul2)
						trFrac = (1.0f - tr.fraction);
						dmgFrac = tr.fraction;
					}
					vec3_t backdir;
					VectorScale( dir, -1, backdir );
					WP_SaberDamageAdd( trFrac, tr.entityNum, dir, bladeVec, backdir, tr.endpos, dmg, dmgFrac, HL_NONE, qfalse, HL_NONE );
					if ( !tr.allsolid && !tr.startsolid )
					{
						VectorScale( dir, -1, dir );
					}
					if ( hitEnt != NULL )
					{
						if ( hitEnt->client )
						{
							//don't do blood sparks on non-living things
							class_t npc_class = hitEnt->client->NPC_class;
							if ( npc_class == CLASS_SEEKER || npc_class == CLASS_PROBE || npc_class == CLASS_MOUSE ||
								 npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class == CLASS_R5D2 || npc_class == CLASS_REMOTE ||
								 npc_class == CLASS_PROTOCOL || npc_class == CLASS_MARK1 || npc_class == CLASS_MARK2 ||
							     npc_class == CLASS_INTERROGATOR || npc_class == CLASS_ATST || npc_class == CLASS_SENTRY )
							{
								if ( !WP_SaberBladeUseSecondBladeStyle( &attacker->client->ps.saber[saberNum], bladeNum )
									&& attacker->client->ps.saber[saberNum].hitOtherEffect )
								{
									hitEffect = attacker->client->ps.saber[saberNum].hitOtherEffect;
								}
								else if ( WP_SaberBladeUseSecondBladeStyle( &attacker->client->ps.saber[saberNum], bladeNum )
									&& attacker->client->ps.saber[saberNum].hitOtherEffect2 )
								{
									hitEffect = attacker->client->ps.saber[saberNum].hitOtherEffect2;
								}
								else
								{
									hitEffect = G_EffectIndex( hit_sparks );
								}
							}
						}
						else
						{
							if ( dmg )
							{//only do these effects if actually trying to damage the thing...
								if ( (hitEnt->svFlags&SVF_BBRUSH)//a breakable brush
									&& ( (hitEnt->spawnflags&1)//INVINCIBLE
										||(hitEnt->flags&FL_DMG_BY_HEAVY_WEAP_ONLY)//HEAVY weapon damage only
										||(hitEnt->NPC_targetname&&attacker&&attacker->targetname&&Q_stricmp(attacker->targetname,hitEnt->NPC_targetname)) ) )//only breakable by an entity who is not the attacker
								{//no hit effect (besides regular client-side one)
								}
								else
								{
									if ( !WP_SaberBladeUseSecondBladeStyle( &attacker->client->ps.saber[saberNum], bladeNum )
										&& attacker->client->ps.saber[saberNum].hitOtherEffect )
									{
										hitEffect = attacker->client->ps.saber[saberNum].hitOtherEffect;
									}
									else if ( WP_SaberBladeUseSecondBladeStyle( &attacker->client->ps.saber[saberNum], bladeNum )
										&& attacker->client->ps.saber[saberNum].hitOtherEffect2 )
									{
										hitEffect = attacker->client->ps.saber[saberNum].hitOtherEffect2;
									}
									else
									{
										hitEffect = G_EffectIndex( hit_sparks );
									}
								}
							}
						}
					}

					if ( !g_saberNoEffects && hitEffect != 0 )
					{
						G_PlayEffect( hitEffect, tr.endpos, dir );//"saber_cut"
					}
				}
				else
				{//we were doing a ghoul trace
					if ( !attacker
						|| !attacker->client
						|| attacker->client->ps.saberLockTime < level.time )
					{
						if ( !WP_SaberDamageEffects( &tr, start, len, dmg, dir, bladeVec, attacker->client->enemyTeam, saberType, &attacker->client->ps.saber[saberNum], bladeNum ) )
						{//didn't hit a ghoul ent
							/*
							if ( && hitEnt->ghoul2.size() )
							{//it was a ghoul2 model so we should have hit it
								return qfalse;
							}
							*/
						}
					}
				}
			}
		}
	}

	return qfalse;
}

#define LOCK_IDEAL_DIST_TOP 32.0f
#define LOCK_IDEAL_DIST_CIRCLE 48.0f
#define LOCK_IDEAL_DIST_JKA 46.0f//all of the new saberlocks are 46.08 from each other because Richard Lico is da MAN
extern void PM_SetAnimFrame( gentity_t *gent, int frame, qboolean torso, qboolean legs );
extern qboolean ValidAnimFileIndex ( int index );
int G_SaberLockAnim( int attackerSaberStyle, int defenderSaberStyle, int topOrSide, int lockOrBreakOrSuperBreak, int winOrLose )
{
	int baseAnim = -1;
	if ( lockOrBreakOrSuperBreak == SABERLOCK_LOCK )
	{//special case: if we're using the same style and locking
		if ( attackerSaberStyle == defenderSaberStyle 
			|| (attackerSaberStyle>=SS_FAST&&attackerSaberStyle<=SS_KATARN&&defenderSaberStyle>=SS_FAST&&defenderSaberStyle<=SS_KATARN) )
		{//using same style
			if ( winOrLose == SABERLOCK_LOSE )
			{//you want the defender's stance...
				switch ( defenderSaberStyle )
				{
				case SS_DUAL:
					if ( topOrSide == SABERLOCK_TOP )
					{
						baseAnim = BOTH_LK_DL_DL_T_L_2;
					}
					else
					{
						baseAnim = BOTH_LK_DL_DL_S_L_2;
					}
					break;
				case SS_STAFF:
					if ( topOrSide == SABERLOCK_TOP )
					{
						baseAnim = BOTH_LK_ST_ST_T_L_2;
					}
					else
					{
						baseAnim = BOTH_LK_ST_ST_S_L_2;
					}
					break;
				default:
					if ( topOrSide == SABERLOCK_TOP )
					{
						baseAnim = BOTH_LK_S_S_T_L_2;
					}
					else
					{
						baseAnim = BOTH_LK_S_S_S_L_2;
					}
					break;
				}
			}
		}
	}
	if ( baseAnim == -1 )
	{
		switch ( attackerSaberStyle )
		{
		case SS_DUAL:
			switch ( defenderSaberStyle )
			{
				case SS_DUAL:
					baseAnim = BOTH_LK_DL_DL_S_B_1_L;
					break;
				case SS_STAFF:
					baseAnim = BOTH_LK_DL_ST_S_B_1_L;
					break;
				default://single
					baseAnim = BOTH_LK_DL_S_S_B_1_L;
					break;
			}
			break;
		case SS_STAFF:
			switch ( defenderSaberStyle )
			{
				case SS_DUAL:
					baseAnim = BOTH_LK_ST_DL_S_B_1_L;
					break;
				case SS_STAFF:
					baseAnim = BOTH_LK_ST_ST_S_B_1_L;
					break;
				default://single
					baseAnim = BOTH_LK_ST_S_S_B_1_L;
					break;
			}
			break;
		default://single
			switch ( defenderSaberStyle )
			{
				case SS_DUAL:
					baseAnim = BOTH_LK_S_DL_S_B_1_L;
					break;
				case SS_STAFF:
					baseAnim = BOTH_LK_S_ST_S_B_1_L;
					break;
				default://single
					baseAnim = BOTH_LK_S_S_S_B_1_L;
					break;
			}
			break;
		}
		//side lock or top lock?
		if ( topOrSide == SABERLOCK_TOP )
		{
			baseAnim += 5;
		}
		//lock, break or superbreak?
		if ( lockOrBreakOrSuperBreak == SABERLOCK_LOCK )
		{
			baseAnim += 2;
		}
		else
		{//a break or superbreak
			if ( lockOrBreakOrSuperBreak == SABERLOCK_SUPERBREAK )
			{
				baseAnim += 3;
			}
			//winner or loser?
			if ( winOrLose == SABERLOCK_WIN )
			{
				baseAnim += 1;
			}
		}
	}
	return baseAnim;
}

qboolean G_CheckIncrementLockAnim( int anim, int winOrLose )
{
	qboolean increment = qfalse;//???
	//RULE: if you are the first style in the lock anim, you advance from LOSING position to WINNING position
	//		if you are the second style in the lock anim, you advance from WINNING position to LOSING position
	switch ( anim )
	{
	//increment to win:
	case BOTH_LK_DL_DL_S_L_1:	//lock if I'm using dual vs. dual and I initiated
	case BOTH_LK_DL_DL_S_L_2:	//lock if I'm using dual vs. dual and other initiated
	case BOTH_LK_DL_DL_T_L_1:	//lock if I'm using dual vs. dual and I initiated
	case BOTH_LK_DL_DL_T_L_2:	//lock if I'm using dual vs. dual and other initiated
	case BOTH_LK_DL_S_S_L_1:		//lock if I'm using dual vs. a single
	case BOTH_LK_DL_S_T_L_1:		//lock if I'm using dual vs. a single
	case BOTH_LK_DL_ST_S_L_1:	//lock if I'm using dual vs. a staff
	case BOTH_LK_DL_ST_T_L_1:	//lock if I'm using dual vs. a staff
	case BOTH_LK_S_S_S_L_1:		//lock if I'm using single vs. a single and I initiated
	case BOTH_LK_S_S_T_L_2:		//lock if I'm using single vs. a single and other initiated
	case BOTH_LK_ST_S_S_L_1:		//lock if I'm using staff vs. a single
	case BOTH_LK_ST_S_T_L_1:		//lock if I'm using staff vs. a single
	case BOTH_LK_ST_ST_T_L_1:	//lock if I'm using staff vs. a staff and I initiated
	case BOTH_LK_ST_ST_T_L_2:	//lock if I'm using staff vs. a staff and other initiated
		if ( winOrLose == SABERLOCK_WIN )
		{
			increment = qtrue;
		}
		else
		{
			increment = qfalse;
		}
		break;

	//decrement to win:
	case BOTH_LK_S_DL_S_L_1:		//lock if I'm using single vs. a dual
	case BOTH_LK_S_DL_T_L_1:		//lock if I'm using single vs. a dual
	case BOTH_LK_S_S_S_L_2:		//lock if I'm using single vs. a single and other intitiated
	case BOTH_LK_S_S_T_L_1:		//lock if I'm using single vs. a single and I initiated
	case BOTH_LK_S_ST_S_L_1:		//lock if I'm using single vs. a staff
	case BOTH_LK_S_ST_T_L_1:		//lock if I'm using single vs. a staff
	case BOTH_LK_ST_DL_S_L_1:	//lock if I'm using staff vs. dual
	case BOTH_LK_ST_DL_T_L_1:	//lock if I'm using staff vs. dual
	case BOTH_LK_ST_ST_S_L_1:	//lock if I'm using staff vs. a staff and I initiated
	case BOTH_LK_ST_ST_S_L_2:	//lock if I'm using staff vs. a staff and other initiated
		if ( winOrLose == SABERLOCK_WIN )
		{
			increment = qfalse;
		}
		else
		{
			increment = qtrue;
		}
		break;
	default:
#ifndef FINAL_BUILD
		Com_Printf( S_COLOR_RED"ERROR: unknown Saber Lock Anim: %s!!!\n", animTable[anim].name );
#endif
		break;
	}
	return increment;
}

qboolean WP_SabersCheckLock2( gentity_t *attacker, gentity_t *defender, sabersLockMode_t lockMode )
{
	animation_t *anim;
	int		attAnim, defAnim, advance = 0;
	float	attStart = 0.5f, defStart = 0.5f;
	float	idealDist = 48.0f;
	//FIXME: this distances need to be modified by the lengths of the sabers involved...
	//MATCH ANIMS
	if ( lockMode == LOCK_KYLE_GRAB1
		|| lockMode == LOCK_KYLE_GRAB2
		|| lockMode == LOCK_KYLE_GRAB3 )
	{
		float	numSpins = 1.0f;
		idealDist = 46.0f;//42.0f;
		attStart = defStart = 0.0f;

		switch ( lockMode )
		{
		default:
		case LOCK_KYLE_GRAB1:
			attAnim = BOTH_KYLE_PA_1;
			defAnim = BOTH_PLAYER_PA_1;
			numSpins = 2.0f;
			break;
		case LOCK_KYLE_GRAB2:
			attAnim = BOTH_KYLE_PA_3;
			defAnim = BOTH_PLAYER_PA_3;
			numSpins = 1.0f;
			break;
		case LOCK_KYLE_GRAB3:
			attAnim = BOTH_KYLE_PA_2;
			defAnim = BOTH_PLAYER_PA_2;
			defender->forcePushTime = level.time + PM_AnimLength( defender->client->clientInfo.animFileIndex, BOTH_PLAYER_PA_2 );
			numSpins = 3.0f;
			break;
		}
		attacker->client->ps.SaberDeactivate();
		defender->client->ps.SaberDeactivate();
		if ( d_slowmodeath->integer > 3
			&& ( defender->s.number < MAX_CLIENTS
				|| attacker->s.number < MAX_CLIENTS ) )
		{
			if( ValidAnimFileIndex( attacker->client->clientInfo.animFileIndex ) )
			{
				int effectTime = PM_AnimLength( attacker->client->clientInfo.animFileIndex, (animNumber_t)attAnim );
				int spinTime = floor((float)effectTime/numSpins);
				int meFlags = (MEF_MULTI_SPIN);//MEF_NO_TIMESCALE|MEF_NO_VERTBOB|
				if ( Q_irand( 0, 1 ) )
				{
					meFlags |= MEF_REVERSE_SPIN;
				}
				G_StartMatrixEffect( attacker, meFlags, effectTime, 0.75f, spinTime );
			}
		}
	}
	else if ( lockMode == LOCK_FORCE_DRAIN )
	{
		idealDist = 46.0f;//42.0f;
		attStart = defStart = 0.0f;

		attAnim = BOTH_FORCE_DRAIN_GRAB_START;
		defAnim = BOTH_FORCE_DRAIN_GRABBED;
		attacker->client->ps.SaberDeactivate();
		defender->client->ps.SaberDeactivate();
	}
	else
	{
		if ( lockMode == LOCK_RANDOM )
		{
			lockMode = (sabersLockMode_t)Q_irand( (int)LOCK_FIRST, (int)(LOCK_RANDOM)-1 );
		}
		//FIXME: attStart% and idealDist will change per saber lock anim pairing... do we need a big table like in bg_panimate.cpp?
		if ( attacker->client->ps.saberAnimLevel >= SS_FAST
			&& attacker->client->ps.saberAnimLevel <= SS_KATARN
			&& defender->client->ps.saberAnimLevel >= SS_FAST
			&& defender->client->ps.saberAnimLevel <= SS_KATARN )
		{//2 single sabers?  Just do it the old way...
			switch ( lockMode )
			{
			case LOCK_TOP:
				attAnim = BOTH_BF2LOCK;// - starts in middle
				defAnim = BOTH_BF1LOCK;// - starts in middle
				attStart = defStart = 0.5f;
				idealDist = LOCK_IDEAL_DIST_TOP;
				break;
			case LOCK_DIAG_TR:
				attAnim = BOTH_CCWCIRCLELOCK; //- starts in middle
				defAnim = BOTH_CWCIRCLELOCK;// - starts in middle
				attStart = defStart = 0.5f;
				idealDist = LOCK_IDEAL_DIST_CIRCLE;
				break;
			case LOCK_DIAG_TL:
				attAnim = BOTH_CWCIRCLELOCK;// - starts in middle
				defAnim = BOTH_CCWCIRCLELOCK;// - starts in middle
				attStart = defStart = 0.5f;
				idealDist = LOCK_IDEAL_DIST_CIRCLE;
				break;
			case LOCK_DIAG_BR:
				attAnim = BOTH_CWCIRCLELOCK;// - starts on left, to right
				defAnim = BOTH_CCWCIRCLELOCK;// - starts on right, to left
				attStart = defStart = 0.85f;//move to end of anim
				idealDist = LOCK_IDEAL_DIST_CIRCLE;
				break;
			case LOCK_DIAG_BL:
				attAnim = BOTH_CCWCIRCLELOCK;// - starts on right, to left
				defAnim = BOTH_CWCIRCLELOCK;// - starts on left, to right
				attStart = defStart = 0.85f;//move to end of anim
				idealDist = LOCK_IDEAL_DIST_CIRCLE;
				break;
			case LOCK_R:
				attAnim = BOTH_CWCIRCLELOCK;// - starts on left, to right
				defAnim = BOTH_CCWCIRCLELOCK;// - starts on right, to left
				attStart = defStart = 0.75f;//move to end of anim
				idealDist = LOCK_IDEAL_DIST_CIRCLE;
				break;
			case LOCK_L:
				attAnim = BOTH_CCWCIRCLELOCK;// - starts on right, to left
				defAnim = BOTH_CWCIRCLELOCK;// - starts on left, to right
				attStart = defStart = 0.75f;//move to end of anim
				idealDist = LOCK_IDEAL_DIST_CIRCLE;
				break;
			default:
				return qfalse;
				break;
			}
		}
		else
		{//use the new system
			idealDist = LOCK_IDEAL_DIST_JKA;//all of the new saberlocks are 46.08 from each other because Richard Lico is da MAN
			if ( lockMode == LOCK_TOP )
			{//top lock
				attAnim = G_SaberLockAnim( attacker->client->ps.saberAnimLevel, defender->client->ps.saberAnimLevel, SABERLOCK_TOP, SABERLOCK_LOCK, SABERLOCK_WIN );
				defAnim = G_SaberLockAnim( defender->client->ps.saberAnimLevel, attacker->client->ps.saberAnimLevel, SABERLOCK_TOP, SABERLOCK_LOCK, SABERLOCK_LOSE );
				attStart = defStart = 0.5f;
			}
			else
			{//side lock
				switch ( lockMode )
				{
				case LOCK_DIAG_TR:
					attAnim = G_SaberLockAnim( attacker->client->ps.saberAnimLevel, defender->client->ps.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_WIN );
					defAnim = G_SaberLockAnim( defender->client->ps.saberAnimLevel, attacker->client->ps.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_LOSE );
					attStart = defStart = 0.5f;
					break;
				case LOCK_DIAG_TL:
					attAnim = G_SaberLockAnim( attacker->client->ps.saberAnimLevel, defender->client->ps.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_LOSE );
					defAnim = G_SaberLockAnim( defender->client->ps.saberAnimLevel, attacker->client->ps.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_WIN );
					attStart = defStart = 0.5f;
					break;
				case LOCK_DIAG_BR:
					attAnim = G_SaberLockAnim( attacker->client->ps.saberAnimLevel, defender->client->ps.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_WIN );
					defAnim = G_SaberLockAnim( defender->client->ps.saberAnimLevel, attacker->client->ps.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_LOSE );
					if ( G_CheckIncrementLockAnim( attAnim, SABERLOCK_WIN ) )
					{
						attStart = 0.85f;//move to end of anim
					}
					else
					{
						attStart = 0.15f;//start at beginning of anim
					}
					if ( G_CheckIncrementLockAnim( defAnim, SABERLOCK_LOSE ) )
					{
						defStart = 0.85f;//start at end of anim
					}
					else
					{
						defStart = 0.15f;//start at beginning of anim
					}
					break;
				case LOCK_DIAG_BL:
					attAnim = G_SaberLockAnim( attacker->client->ps.saberAnimLevel, defender->client->ps.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_LOSE );
					defAnim = G_SaberLockAnim( defender->client->ps.saberAnimLevel, attacker->client->ps.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_WIN );
					if ( G_CheckIncrementLockAnim( attAnim, SABERLOCK_WIN ) )
					{
						attStart = 0.85f;//move to end of anim
					}
					else
					{
						attStart = 0.15f;//start at beginning of anim
					}
					if ( G_CheckIncrementLockAnim( defAnim, SABERLOCK_LOSE ) )
					{
						defStart = 0.85f;//start at end of anim
					}
					else
					{
						defStart = 0.15f;//start at beginning of anim
					}
					break;
				case LOCK_R:
					attAnim = G_SaberLockAnim( attacker->client->ps.saberAnimLevel, defender->client->ps.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_LOSE );
					defAnim = G_SaberLockAnim( defender->client->ps.saberAnimLevel, attacker->client->ps.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_WIN );
					if ( G_CheckIncrementLockAnim( attAnim, SABERLOCK_WIN ) )
					{
						attStart = 0.75f;//move to end of anim
					}
					else
					{
						attStart = 0.25f;//start at beginning of anim
					}
					if ( G_CheckIncrementLockAnim( defAnim, SABERLOCK_LOSE ) )
					{
						defStart = 0.75f;//start at end of anim
					}
					else
					{
						defStart = 0.25f;//start at beginning of anim
					}
					break;
				case LOCK_L:
					attAnim = G_SaberLockAnim( attacker->client->ps.saberAnimLevel, defender->client->ps.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_WIN );
					defAnim = G_SaberLockAnim( defender->client->ps.saberAnimLevel, attacker->client->ps.saberAnimLevel, SABERLOCK_SIDE, SABERLOCK_LOCK, SABERLOCK_LOSE );
					//attacker starts with advantage
					if ( G_CheckIncrementLockAnim( attAnim, SABERLOCK_WIN ) )
					{
						attStart = 0.75f;//move to end of anim
					}
					else
					{
						attStart = 0.25f;//start at beginning of anim
					}
					if ( G_CheckIncrementLockAnim( defAnim, SABERLOCK_LOSE ) )
					{
						defStart = 0.75f;//start at end of anim
					}
					else
					{
						defStart = 0.25f;//start at beginning of anim
					}
					break;
				default:
					return qfalse;
					break;
				}
			}
		}
	}
	//set the proper anims
	NPC_SetAnim( attacker, SETANIM_BOTH, attAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	NPC_SetAnim( defender, SETANIM_BOTH, defAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	//don't let them store a kick for the whole saberlock....
	attacker->client->ps.saberMoveNext = defender->client->ps.saberMoveNext = LS_NONE;
	//
	if ( attStart > 0.0f )
	{
		if( ValidAnimFileIndex( attacker->client->clientInfo.animFileIndex ) )
		{
			anim = &level.knownAnimFileSets[attacker->client->clientInfo.animFileIndex].animations[attAnim];
			advance = floor( anim->numFrames*attStart );
			PM_SetAnimFrame( attacker, anim->firstFrame + advance, qtrue, qtrue );
#ifndef FINAL_BUILD
			if ( d_saberCombat->integer )
			{
				Com_Printf( "%s starting saber lock, anim = %s, %d frames to go!\n", attacker->NPC_type, animTable[attAnim].name, anim->numFrames-advance );
			}
#endif
		}
	}
	if ( defStart > 0.0f )
	{
		if( ValidAnimFileIndex( defender->client->clientInfo.animFileIndex ) )
		{
			anim = &level.knownAnimFileSets[defender->client->clientInfo.animFileIndex].animations[defAnim];
			advance = ceil( anim->numFrames*defStart );
			PM_SetAnimFrame( defender, anim->firstFrame + advance, qtrue, qtrue );//was anim->firstFrame + anim->numFrames - advance, but that's wrong since they are matched anims
#ifndef FINAL_BUILD
			if ( d_saberCombat->integer )
			{
				Com_Printf( "%s starting saber lock, anim = %s, %d frames to go!\n", defender->NPC_type, animTable[defAnim].name, advance );
			}
#endif
		}
	}
	VectorClear( attacker->client->ps.velocity );
	VectorClear( attacker->client->ps.moveDir );
	VectorClear( defender->client->ps.velocity );
	VectorClear( defender->client->ps.moveDir );
	if ( lockMode == LOCK_KYLE_GRAB1
		|| lockMode == LOCK_KYLE_GRAB2
		|| lockMode == LOCK_KYLE_GRAB3
		|| lockMode == LOCK_FORCE_DRAIN )
	{//not a real lock, just freeze them both in place
		//can't move or attack
		attacker->client->ps.pm_time = attacker->client->ps.weaponTime = attacker->client->ps.legsAnimTimer;
		attacker->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		attacker->painDebounceTime = level.time + attacker->client->ps.pm_time;
		if ( lockMode != LOCK_FORCE_DRAIN )
		{
			defender->client->ps.torsoAnimTimer += 200;
			defender->client->ps.legsAnimTimer += 200;
		}
		defender->client->ps.pm_time = defender->client->ps.weaponTime = defender->client->ps.legsAnimTimer;
		defender->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		if ( lockMode != LOCK_FORCE_DRAIN )
		{
			attacker->aimDebounceTime = level.time + attacker->client->ps.pm_time;
		}
	}
	else
	{
		attacker->client->ps.saberLockTime = defender->client->ps.saberLockTime = level.time + SABER_LOCK_TIME;
		attacker->client->ps.legsAnimTimer = attacker->client->ps.torsoAnimTimer = defender->client->ps.legsAnimTimer = defender->client->ps.torsoAnimTimer = SABER_LOCK_TIME;
		//attacker->client->ps.weaponTime = defender->client->ps.weaponTime = SABER_LOCK_TIME;
		attacker->client->ps.saberLockEnemy = defender->s.number;
		defender->client->ps.saberLockEnemy = attacker->s.number;
	}

	//MATCH ANGLES
	if ( lockMode == LOCK_KYLE_GRAB1
		|| lockMode == LOCK_KYLE_GRAB2
		|| lockMode == LOCK_KYLE_GRAB3 )
	{//not a real lock, just set pitch to 0
		attacker->client->ps.viewangles[PITCH] = defender->client->ps.viewangles[PITCH] = 0;
	}
	else
	{
		//FIXME: if zDiff in elevation, make lower look up and upper look down and move them closer?
		float defPitchAdd = 0, zDiff = ((attacker->currentOrigin[2]+attacker->client->standheight)-(defender->currentOrigin[2]+defender->client->standheight));
		if ( zDiff > 24 )
		{
			defPitchAdd = -30;
		}
		else if ( zDiff < -24 )
		{
			defPitchAdd = 30;
		}
		else
		{
			defPitchAdd = zDiff/24.0f*-30.0f;
		}
		if ( attacker->NPC && defender->NPC )
		{//if 2 NPCs, just set pitch to 0
			attacker->client->ps.viewangles[PITCH] = -defPitchAdd;
			defender->client->ps.viewangles[PITCH] = defPitchAdd;
		}
		else
		{//if a player is involved, clamp player's pitch and match NPC's to player
			if ( !attacker->s.number )
			{
				//clamp to defPitch
				if ( attacker->client->ps.viewangles[PITCH] > -defPitchAdd + 10 )
				{
					attacker->client->ps.viewangles[PITCH] = -defPitchAdd + 10;
				}
				else if ( attacker->client->ps.viewangles[PITCH] < -defPitchAdd-10 )
				{
					attacker->client->ps.viewangles[PITCH] = -defPitchAdd-10;
				}
				//clamp to sane numbers
				if ( attacker->client->ps.viewangles[PITCH] > 50 )
				{
					attacker->client->ps.viewangles[PITCH] = 50;
				}
				else if ( attacker->client->ps.viewangles[PITCH] < -50 )
				{
					attacker->client->ps.viewangles[PITCH] = -50;
				}
				defender->client->ps.viewangles[PITCH] = attacker->client->ps.viewangles[PITCH]*-1;
				defPitchAdd = defender->client->ps.viewangles[PITCH];
			}
			else if ( !defender->s.number )
			{
				//clamp to defPitch
				if ( defender->client->ps.viewangles[PITCH] > defPitchAdd + 10 )
				{
					defender->client->ps.viewangles[PITCH] = defPitchAdd + 10;
				}
				else if ( defender->client->ps.viewangles[PITCH] < defPitchAdd-10 )
				{
					defender->client->ps.viewangles[PITCH] = defPitchAdd-10;
				}
				//clamp to sane numbers
				if ( defender->client->ps.viewangles[PITCH] > 50 )
				{
					defender->client->ps.viewangles[PITCH] = 50;
				}
				else if ( defender->client->ps.viewangles[PITCH] < -50 )
				{
					defender->client->ps.viewangles[PITCH] = -50;
				}
				defPitchAdd = defender->client->ps.viewangles[PITCH];
				attacker->client->ps.viewangles[PITCH] = defender->client->ps.viewangles[PITCH]*-1;
			}
		}
	}
	vec3_t	attAngles, defAngles, defDir;
	VectorSubtract( defender->currentOrigin, attacker->currentOrigin, defDir );
	VectorCopy( attacker->client->ps.viewangles, attAngles );
	attAngles[YAW] = vectoyaw( defDir );
	SetClientViewAngle( attacker, attAngles );
	defAngles[PITCH] = attAngles[PITCH]*-1;
	defAngles[YAW] = AngleNormalize180( attAngles[YAW] + 180);
	defAngles[ROLL] = 0;
	SetClientViewAngle( defender, defAngles );

	//MATCH POSITIONS
	vec3_t	newOrg;
	/*
	idealDist -= fabs(defPitchAdd)/8.0f;
	*/
	float scale = (attacker->s.modelScale[0]+attacker->s.modelScale[1])*0.5f;
	if ( scale && scale != 1.0f )
	{
		idealDist += 8*(scale-1.0f);
	}
	scale = (defender->s.modelScale[0]+defender->s.modelScale[1])*0.5f;
	if ( scale && scale != 1.0f )
	{
		idealDist += 8*(scale-1.0f);
	}

	float diff = VectorNormalize( defDir ) - idealDist;//diff will be the total error in dist
	//try to move attacker half the diff towards the defender
	VectorMA( attacker->currentOrigin, diff*0.5f, defDir, newOrg );
	trace_t	trace;
	gi.trace( &trace, attacker->currentOrigin, attacker->mins, attacker->maxs, newOrg, attacker->s.number, attacker->clipmask, (EG2_Collision)0, 0 );
	if ( !trace.startsolid && !trace.allsolid )
	{
		G_SetOrigin( attacker, trace.endpos );
		gi.linkentity( attacker );
	}
	//now get the defender's dist and do it for him too
	vec3_t	attDir;
	VectorSubtract( attacker->currentOrigin, defender->currentOrigin, attDir );
	diff = VectorNormalize( attDir ) - idealDist;//diff will be the total error in dist
	//try to move defender all of the remaining diff towards the attacker
	VectorMA( defender->currentOrigin, diff, attDir, newOrg );
	gi.trace( &trace, defender->currentOrigin, defender->mins, defender->maxs, newOrg, defender->s.number, defender->clipmask, (EG2_Collision)0, 0 );
	if ( !trace.startsolid && !trace.allsolid )
	{
		G_SetOrigin( defender, trace.endpos );
		gi.linkentity( defender );
	}

	//DONE!
	{//JKEnhanced g_saberDeterministic: no new lock between these two for a while (WP_DetSaberLockGate)
		extern cvar_t *g_saberDeterministic;
		if ( g_saberDeterministic && g_saberDeterministic->integer )
		{
			TIMER_Set( attacker, "saberLockCD", 8000 );
			TIMER_Set( defender, "saberLockCD", 8000 );
		}
	}

	return qtrue;
}

qboolean WP_SabersCheckLock( gentity_t *ent1, gentity_t *ent2 )
{
	if ( ent1->client->playerTeam == ent2->client->playerTeam )
	{
		return qfalse;
	}
	if ( ent1->client->NPC_class == CLASS_SABER_DROID
		|| ent2->client->NPC_class == CLASS_SABER_DROID )
	{//they don't have saberlock anims
		return qfalse;
	}
	if ( ent1->client->ps.groundEntityNum == ENTITYNUM_NONE ||
		ent2->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{
		return qfalse;
	}
	if ( (ent1->client->ps.saber[0].saberFlags&SFL_NOT_LOCKABLE)
		|| (ent2->client->ps.saber[0].saberFlags&SFL_NOT_LOCKABLE) )
	{//one of these sabers cannot lock (like a lance)
		return qfalse;
	}
	if ( ent1->client->ps.dualSabers
		&& ent1->client->ps.saber[1].Active()
		&& (ent1->client->ps.saber[1].saberFlags&SFL_NOT_LOCKABLE) )
	{//one of these sabers cannot lock (like a lance)
		return qfalse;
	}
	if ( ent2->client->ps.dualSabers
		&& ent2->client->ps.saber[1].Active()
		&& (ent2->client->ps.saber[1].saberFlags&SFL_NOT_LOCKABLE) )
	{//one of these sabers cannot lock (like a lance)
		return qfalse;
	}
	if ( ent1->painDebounceTime > level.time-1000 || ent2->painDebounceTime > level.time-1000 )
	{//can't saberlock if you're not ready
		return qfalse;
	}
	if ( ent1->client->ps.repulseChargeStart || ent2->client->ps.repulseChargeStart )
	{//JKEnhanced: charging Repulse (a lock would cancel the charge)
		return qfalse;
	}
	if ( fabs( ent1->currentOrigin[2]-ent2->currentOrigin[2]) > 18 )
	{
		return qfalse;
	}
	float dist = DistanceSquared(ent1->currentOrigin,ent2->currentOrigin);
	if ( dist < 64 || dist > 6400 )//( dist < 128 || dist > 2304 )
	{//between 8 and 80 from each other//was 16 and 48
		return qfalse;
	}
	if ( !InFOV( ent1, ent2, 40, 180 ) || !InFOV( ent2, ent1, 40, 180 ) )
	{
		return qfalse;
	}
	//Check for certain anims that *cannot* lock
	//FIXME: there should probably be a whole *list* of these, but I'll put them in as they come up
	if ( ent1->client->ps.torsoAnim == BOTH_A2_STABBACK1 && ent1->client->ps.torsoAnimTimer > 300 )
	{//can't lock when saber is behind you
		return qfalse;
	}
	if ( ent2->client->ps.torsoAnim == BOTH_A2_STABBACK1 && ent2->client->ps.torsoAnimTimer > 300 )
	{//can't lock when saber is behind you
		return qfalse;
	}
	if ( PM_LockedAnim( ent1->client->ps.torsoAnim )
		|| PM_LockedAnim( ent2->client->ps.torsoAnim ) )
	{//stuck doing something else
		return qfalse;
	}
	if ( PM_SaberLockBreakAnim( ent1->client->ps.torsoAnim )
		|| PM_SaberLockBreakAnim( ent2->client->ps.torsoAnim ) )
	{//still finishing the last lock break!
		return qfalse;
	}
	if ( ent1->client->ps.torsoAnim == BOTH_A1_BR_TL ||
		ent1->client->ps.torsoAnim == BOTH_A2_BR_TL ||
		ent1->client->ps.torsoAnim == BOTH_A3_BR_TL ||
		ent1->client->ps.torsoAnim == BOTH_A4_BR_TL ||
		ent1->client->ps.torsoAnim == BOTH_A5_BR_TL ||
		ent1->client->ps.torsoAnim == BOTH_A6_BR_TL ||
		ent1->client->ps.torsoAnim == BOTH_A7_BR_TL )
	{//ent1 is attacking in the opposite diagonal
		return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_BR );
	}
	if ( ent2->client->ps.torsoAnim == BOTH_A1_BR_TL ||
		ent2->client->ps.torsoAnim == BOTH_A2_BR_TL ||
		ent2->client->ps.torsoAnim == BOTH_A3_BR_TL ||
		ent2->client->ps.torsoAnim == BOTH_A4_BR_TL ||
		ent2->client->ps.torsoAnim == BOTH_A5_BR_TL ||
		ent2->client->ps.torsoAnim == BOTH_A6_BR_TL ||
		ent2->client->ps.torsoAnim == BOTH_A7_BR_TL )
	{//ent2 is attacking in the opposite diagonal
		return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_BR );
	}
	//BL to TR lock
	if ( ent1->client->ps.torsoAnim == BOTH_A1_BL_TR ||
		ent1->client->ps.torsoAnim == BOTH_A2_BL_TR ||
		ent1->client->ps.torsoAnim == BOTH_A3_BL_TR ||
		ent1->client->ps.torsoAnim == BOTH_A4_BL_TR ||
		ent1->client->ps.torsoAnim == BOTH_A5_BL_TR ||
		ent1->client->ps.torsoAnim == BOTH_A6_BL_TR ||
		ent1->client->ps.torsoAnim == BOTH_A7_BL_TR )
	{//ent1 is attacking in the opposite diagonal
		return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_BL );
	}
	if ( ent2->client->ps.torsoAnim == BOTH_A1_BL_TR ||
		ent2->client->ps.torsoAnim == BOTH_A2_BL_TR ||
		ent2->client->ps.torsoAnim == BOTH_A3_BL_TR ||
		ent2->client->ps.torsoAnim == BOTH_A4_BL_TR ||
		ent2->client->ps.torsoAnim == BOTH_A5_BL_TR ||
		ent2->client->ps.torsoAnim == BOTH_A6_BL_TR ||
		ent2->client->ps.torsoAnim == BOTH_A7_BL_TR )
	{//ent2 is attacking in the opposite diagonal
		return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_BL );
	}
	//L to R lock
	if ( ent1->client->ps.torsoAnim == BOTH_A1__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A2__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A3__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A4__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A5__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A6__L__R ||
		ent1->client->ps.torsoAnim == BOTH_A7__L__R )
	{//ent1 is attacking l to r
		return WP_SabersCheckLock2( ent1, ent2, LOCK_L );
		/*
		if ( ent2BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent1, ent2, LOCK_L );
		}
		if ( ent2->client->ps.torsoAnim == BOTH_A1_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A2_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A3_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A4_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A5_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A6_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A7_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_TR ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_BL )
		{//ent2 is attacking or blocking on the r
			return WP_SabersCheckLock2( ent1, ent2, LOCK_L );
		}
		if ( ent2Boss && !Q_irand( 0, 3 ) )
		{
			return WP_SabersCheckLock2( ent1, ent2, LOCK_L );
		}
		*/
	}
	if ( ent2->client->ps.torsoAnim == BOTH_A1__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A2__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A3__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A4__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A5__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A6__L__R ||
		ent2->client->ps.torsoAnim == BOTH_A7__L__R )
	{//ent2 is attacking l to r
		return WP_SabersCheckLock2( ent2, ent1, LOCK_L );
		/*
		if ( ent1BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent2, ent1, LOCK_L );
		}
		if ( ent1->client->ps.torsoAnim == BOTH_A1_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A2_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A3_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A4_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A5_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A6_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A7_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_TR ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_BL )
		{//ent1 is attacking or blocking on the r
			return WP_SabersCheckLock2( ent2, ent1, LOCK_L );
		}
		if ( ent1Boss && !Q_irand( 0, 3 ) )
		{
			return WP_SabersCheckLock2( ent2, ent1, LOCK_L );
		}
		*/
	}
	//R to L lock
	if ( ent1->client->ps.torsoAnim == BOTH_A1__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A2__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A3__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A4__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A5__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A6__R__L ||
		ent1->client->ps.torsoAnim == BOTH_A7__R__L )
	{//ent1 is attacking r to l
		return WP_SabersCheckLock2( ent1, ent2, LOCK_R );
		/*
		if ( ent2BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent1, ent2, LOCK_R );
		}
		if ( ent2->client->ps.torsoAnim == BOTH_A1_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A2_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A3_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A4_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A5_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A6_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A7_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_TL ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_BR )
		{//ent2 is attacking or blocking on the l
			return WP_SabersCheckLock2( ent1, ent2, LOCK_R );
		}
		if ( ent2Boss && !Q_irand( 0, 3 ) )
		{
			return WP_SabersCheckLock2( ent1, ent2, LOCK_R );
		}
		*/
	}
	if ( ent2->client->ps.torsoAnim == BOTH_A1__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A2__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A3__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A4__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A5__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A6__R__L ||
		ent2->client->ps.torsoAnim == BOTH_A7__R__L )
	{//ent2 is attacking r to l
		return WP_SabersCheckLock2( ent2, ent1, LOCK_R );
		/*
		if ( ent1BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent2, ent1, LOCK_R );
		}
		if ( ent1->client->ps.torsoAnim == BOTH_A1_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A2_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A3_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A4_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A5_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A6_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A7_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_TL ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_BR )
		{//ent1 is attacking or blocking on the l
			return WP_SabersCheckLock2( ent2, ent1, LOCK_R );
		}
		if ( ent1Boss && !Q_irand( 0, 3 ) )
		{
			return WP_SabersCheckLock2( ent2, ent1, LOCK_R );
		}
		*/
	}
	//TR to BL lock
	if ( ent1->client->ps.torsoAnim == BOTH_A1_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A2_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A3_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A4_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A5_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A6_TR_BL ||
		ent1->client->ps.torsoAnim == BOTH_A7_TR_BL )
	{//ent1 is attacking diagonally
		return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TR );
		/*
		if ( ent2BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TR );
		}
		if ( ent2->client->ps.torsoAnim == BOTH_A1_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A2_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A3_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A4_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A5_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A6_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_A7_TR_BL ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_TL )
		{//ent2 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TR );
		}
		if ( ent2->client->ps.torsoAnim == BOTH_A1_BR_TL ||
			ent2->client->ps.torsoAnim == BOTH_A2_BR_TL ||
			ent2->client->ps.torsoAnim == BOTH_A3_BR_TL ||
			ent2->client->ps.torsoAnim == BOTH_A4_BR_TL ||
			ent2->client->ps.torsoAnim == BOTH_A5_BR_TL ||
			ent2->client->ps.torsoAnim == BOTH_A6_BR_TL ||
			ent2->client->ps.torsoAnim == BOTH_A7_BR_TL ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_BL )
		{//ent2 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_BL );
		}
		if ( ent2Boss && !Q_irand( 0, 3 ) )
		{
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TR );
		}
		*/
	}

	if ( ent2->client->ps.torsoAnim == BOTH_A1_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A2_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A3_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A4_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A5_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A6_TR_BL ||
		ent2->client->ps.torsoAnim == BOTH_A7_TR_BL )
	{//ent2 is attacking diagonally
		return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_TR );
		/*
		if ( ent1BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_TR );
		}
		if ( ent1->client->ps.torsoAnim == BOTH_A1_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A2_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A3_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A4_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A5_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A6_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_A7_TR_BL ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_TL )
		{//ent1 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_TR );
		}
		if ( ent1->client->ps.torsoAnim == BOTH_A1_BR_TL ||
			ent1->client->ps.torsoAnim == BOTH_A2_BR_TL ||
			ent1->client->ps.torsoAnim == BOTH_A3_BR_TL ||
			ent1->client->ps.torsoAnim == BOTH_A4_BR_TL ||
			ent1->client->ps.torsoAnim == BOTH_A5_BR_TL ||
			ent1->client->ps.torsoAnim == BOTH_A6_BR_TL ||
			ent1->client->ps.torsoAnim == BOTH_A7_BR_TL ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_BL )
		{//ent1 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_BL );
		}
		if ( ent1Boss && !Q_irand( 0, 3 ) )
		{
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_TR );
		}
		*/
	}

	//TL to BR lock
	if ( ent1->client->ps.torsoAnim == BOTH_A1_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A2_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A3_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A4_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A5_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A6_TL_BR ||
		ent1->client->ps.torsoAnim == BOTH_A7_TL_BR )
	{//ent1 is attacking diagonally
		return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TL );
		/*
		if ( ent2BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TL );
		}
		if ( ent2->client->ps.torsoAnim == BOTH_A1_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A2_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A3_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A4_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A5_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A6_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_A7_TL_BR ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_TR )
		{//ent2 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TL );
		}
		if ( ent2->client->ps.torsoAnim == BOTH_A1_BL_TR ||
			ent2->client->ps.torsoAnim == BOTH_A2_BL_TR ||
			ent2->client->ps.torsoAnim == BOTH_A3_BL_TR ||
			ent2->client->ps.torsoAnim == BOTH_A4_BL_TR ||
			ent2->client->ps.torsoAnim == BOTH_A5_BL_TR ||
			ent2->client->ps.torsoAnim == BOTH_A6_BL_TR ||
			ent2->client->ps.torsoAnim == BOTH_A7_BL_TR ||
			ent2->client->ps.torsoAnim == BOTH_P1_S1_BR )
		{//ent2 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_BR );
		}
		if ( ent2Boss && !Q_irand( 0, 3 ) )
		{
			return WP_SabersCheckLock2( ent1, ent2, LOCK_DIAG_TL );
		}
		*/
	}

	if ( ent2->client->ps.torsoAnim == BOTH_A1_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A2_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A3_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A4_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A5_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A6_TL_BR ||
		ent2->client->ps.torsoAnim == BOTH_A7_TL_BR )
	{//ent2 is attacking diagonally
		return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_TL );
		/*
		if ( ent1BlockingPlayer )
		{//player will block this anyway
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_TL );
		}
		if ( ent1->client->ps.torsoAnim == BOTH_A1_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A2_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A3_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A4_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A5_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A6_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_A7_TL_BR ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_TR )
		{//ent1 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_TL );
		}
		if ( ent1->client->ps.torsoAnim == BOTH_A1_BL_TR ||
			ent1->client->ps.torsoAnim == BOTH_A2_BL_TR ||
			ent1->client->ps.torsoAnim == BOTH_A3_BL_TR ||
			ent1->client->ps.torsoAnim == BOTH_A4_BL_TR ||
			ent1->client->ps.torsoAnim == BOTH_A5_BL_TR ||
			ent1->client->ps.torsoAnim == BOTH_A6_BL_TR ||
			ent1->client->ps.torsoAnim == BOTH_A7_BL_TR ||
			ent1->client->ps.torsoAnim == BOTH_P1_S1_BR )
		{//ent1 is attacking in the opposite diagonal
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_BR );
		}
		if ( ent1Boss && !Q_irand( 0, 3 ) )
		{
			return WP_SabersCheckLock2( ent2, ent1, LOCK_DIAG_TL );
		}
		*/
	}
	//T to B lock
	if ( ent1->client->ps.torsoAnim == BOTH_A1_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A2_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A3_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A4_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A5_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A6_T__B_ ||
		ent1->client->ps.torsoAnim == BOTH_A7_T__B_ )
	{//ent1 is attacking top-down
		/*
		if ( ent2->client->ps.torsoAnim == BOTH_P1_S1_T_ ||
			ent2->client->ps.torsoAnim == BOTH_K1_S1_T_ )
		*/
		{//ent2 is blocking at top
			return WP_SabersCheckLock2( ent1, ent2, LOCK_TOP );
		}
	}

	if ( ent2->client->ps.torsoAnim == BOTH_A1_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A2_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A3_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A4_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A5_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A6_T__B_ ||
		ent2->client->ps.torsoAnim == BOTH_A7_T__B_ )
	{//ent2 is attacking top-down
		/*
		if ( ent1->client->ps.torsoAnim == BOTH_P1_S1_T_ ||
			ent1->client->ps.torsoAnim == BOTH_K1_S1_T_ )
		*/
		{//ent1 is blocking at top
			return WP_SabersCheckLock2( ent2, ent1, LOCK_TOP );
		}
	}
	/*
	if ( !Q_irand( 0, 10 ) )
	{
		return WP_SabersCheckLock2( ent1, ent2, LOCK_RANDOM );
	}
	*/
	return qfalse;
}

extern cvar_t	*g_perfectParryWindow;
extern cvar_t	*g_perfectParryHeavyScale;
extern cvar_t	*g_perfectParryMissileScale;
extern cvar_t	*g_debugParry;

/*
WP_IsHeavyAttack (JKEnhanced)

The attacker is in a telegraphed heavy attack (windup or strike) or a kata special.
*/
qboolean WP_IsHeavyAttack( const gentity_t *ent )
{
	if ( !ent || !ent->client )
	{
		return qfalse;
	}
	const playerState_t *ps = &ent->client->ps;
	//only while actually in the telegraphed windup or its strike, not any later swing
	return (qboolean)( ( ps->heavyAttackPhase == 1 && ps->heavyAttackMove != LS_NONE
			&& ps->saberMove == LS_S_TL2BR + (ps->heavyAttackMove - LS_A_TL2BR) )
		|| ( ps->heavyAttackMove != LS_NONE && ps->saberMove == ps->heavyAttackMove )
		|| ps->torsoAnim == BOTH_A1_SPECIAL
		|| ps->torsoAnim == BOTH_A2_SPECIAL
		|| ps->torsoAnim == BOTH_A3_SPECIAL );
}

/*
WP_PerfectParryWindowMs (JKEnhanced)

How long after the parry tap a hit still counts as perfectly parried. Base g_perfectParryWindow,
scaled by the attacker: slower, heavier styles give a wider window and fast flurries a narrower
one; a faster-playing swing (animSpeedScale) narrows it; a telegraphed heavy attack or kata
widens it (g_perfectParryHeavyScale). Blaster bolts use g_perfectParryMissileScale.
Difficulty scales it too (difficulty.cfg parryWindowScale).
*/
static const float s_parryStyleScale[SS_NUM_SABER_STYLES] =
{
	1.0f,	//SS_NONE
	0.75f,	//SS_FAST
	1.0f,	//SS_MEDIUM
	1.25f,	//SS_STRONG
	1.25f,	//SS_DESANN
	1.1f,	//SS_TAVION
	1.0f,	//SS_KATARN
	0.85f,	//SS_DUAL
	0.85f,	//SS_STAFF
};

int WP_PerfectParryWindowMs( const gentity_t *defender, const gentity_t *attacker, qboolean missile )
{
	float window = g_perfectParryWindow->value * G_Difficulty()->parryWindowScale;	//ext_data/difficulty.cfg
	if ( G_SpeedLevel( defender ) >= FORCE_LEVEL_3 )
	{//JKEnhanced 4B.6: Speed 3 (Focus) - force.cfg speed.parryPct
		window *= G_ForceTuning()->speedParryPct / 100.0f;
	}
	if ( missile )
	{
		return (int)( window * g_perfectParryMissileScale->value );
	}
	if ( attacker && attacker->client && attacker->client->ps.weapon == WP_SABER )
	{
		const int style = attacker->client->ps.saberAnimLevel;
		if ( style >= 0 && style < SS_NUM_SABER_STYLES )
		{
			window *= s_parryStyleScale[style];
		}
		const float speed = attacker->client->ps.saber[0].animSpeedScale;
		if ( speed > 0.01f )
		{
			float speedScale = 1.0f / speed;
			if ( speedScale < 0.6f ) speedScale = 0.6f;
			if ( speedScale > 1.5f ) speedScale = 1.5f;
			window *= speedScale;
		}
		if ( WP_IsHeavyAttack( attacker ) )
		{
			window *= g_perfectParryHeavyScale->value;
		}
	}
	return (int)window;
}

/*
WP_InPerfectParryWindowVs

qtrue while the player (manual block mode) is inside the perfect-parry timing window against
this attacker: the window (WP_PerfectParryWindowMs) counts from the parry tap (tapping attack
while holding block stamps saberBlockStartTime, gated by the perfectParryDebounce cooldown).
g_debugParry 1 prints the timing (once per frame).
*/
qboolean WP_InPerfectParryWindowVs( const gentity_t *self, const gentity_t *attacker, qboolean missile )
{
	if ( !self || self->s.number != 0 || !self->client
		|| g_saberAutoBlocking->integer
		|| self->client->ps.saberBlockingTime <= level.time
		|| !self->client->ps.saberBlockStartTime )
	{
		return qfalse;
	}
	const int since = level.time - self->client->ps.saberBlockStartTime;
	const int window = WP_PerfectParryWindowMs( self, attacker, missile );
	const qboolean in = (qboolean)( since < window );
	if ( g_debugParry->integer && since <= window + 500 )
	{//(only around the window: block can be held long after the tap)
		static int lastPrint = -1;
		if ( lastPrint != level.time )
		{
			lastPrint = level.time;
			gi.Printf( "parry: tap %d ms before the hit, window %d ms (%s%s) -> %s\n", since, window,
				missile ? "missile" : ( attacker && attacker->NPC_type ? attacker->NPC_type : "?" ),
				( !missile && WP_IsHeavyAttack( attacker ) ) ? ", heavy" : "",
				in ? S_COLOR_GREEN "PERFECT" : S_COLOR_YELLOW "too late" );
		}
	}
	return in;
}

// (callers that don't know the attacker: their enemy is the usual source of the hit)
qboolean WP_InPerfectParryWindow( const gentity_t *self )
{
	return WP_InPerfectParryWindowVs( self, self ? self->enemy : NULL, qfalse );
}

int g_perfectParryFlashTime = 0;	// read by CG_DrawCrosshair (same module) for the crosshair swirl flash

/*
WP_PerfectParrySuccess

Feedback + chaining for a landed perfect parry (saber or projectile): resets the
cooldown so streams (repeater/Z-6) and multi-duelist pressure can be ridden
tap-by-tap, consumes the current window (each parry needs its own timed tap),
and fires the distinct audio/visual cue.
*/
void WP_PerfectParrySuccess( gentity_t *self, vec3_t impactPoint, vec3_t normal )
{
	self->client->ps.perfectParryDebounce = 0;
	self->client->ps.saberBlockStartTime = 0;
	if ( self->s.number == 0 )
	{//the crosshair flash is the player's (NPCs land perfect parries too, AI_JediBrain.cpp)
		g_perfectParryFlashTime = level.time;
	}
	G_SoundOnEnt( self, CHAN_ITEM, "sound/items/respawn1.wav" );
	G_PlayEffect( "saber/saber_block", impactPoint, normal );
}

/*
G_DamageGuard

Saber guard/composure (NPCs only). Drains the victim's guard; at zero, forces a
guard break: an extended broken-parry stagger during which they cannot parry or
evade (WP_SaberParry / Jedi_EvasionSaber check guardBreakTime) and take bonus
saber damage (G_Damage). Guard refills when the break window lapses, so bosses
naturally fight in guard phases.
*/
/*
G_IsSaberBoss (JKEnhanced)

Boss-tier saber fighters: anything flagged NPCAI_BOSS_CHARACTER, or Desann, Tavion, Alora, Kyle.
includeElites also counts Shadowtroopers (bigger guard, shorter break windows) - they come in
groups, so they don't get boss guard stages (g_combat.cpp G_BossStageClamp).
*/
qboolean G_IsSaberBoss( const gentity_t *ent, qboolean includeElites )
{
	if ( !ent || !ent->NPC || !ent->client )
	{
		return qfalse;
	}
	return (qboolean)( (ent->NPC->aiFlags&NPCAI_BOSS_CHARACTER)
		//boss by name only (CLASS_REBORN in their .npc): NPC_SetMiscDefaultData flags them AFTER the
		//.npc parse, so NPC_stats' guard + Repulse setup missed them without this
		|| ( ent->NPC_type && ( !Q_stricmp( ent->NPC_type, "tavion_scepter" ) || !Q_stricmp( ent->NPC_type, "alora_dual" ) ) )
		|| ent->client->NPC_class == CLASS_DESANN
		|| ent->client->NPC_class == CLASS_TAVION
		|| ent->client->NPC_class == CLASS_ALORA
		|| ent->client->NPC_class == CLASS_KYLE
		|| ( includeElites && ent->client->NPC_class == CLASS_SHADOWTROOPER ) );
}

void G_DamageGuard( gentity_t *victim, gentity_t *attacker, int amount )
{
	if ( WP_SaberRules() )
	{//JKEnhanced "a blade in the way": Force is the guard - kicks, resisted pushes, lightning and the
	 //rest wear down a saber NPC's Force instead of a separate meter
		if ( victim && victim->NPC && victim->client && victim->client->ps.weapon == WP_SABER )
		{
			WP_SaberGuardPay( victim, attacker, amount );
		}
		return;
	}
	if ( !victim || !victim->NPC || !victim->client || victim->NPC->guardMax <= 0 )
	{
		return;
	}
	if ( victim->health <= 0 || victim->NPC->guardBreakTime > level.time )
	{//dead or already broken
		return;
	}

	victim->NPC->guard -= amount;
	victim->NPC->guardRegenDebounce = level.time + 2000;

	if ( victim->NPC->guard <= 0 )
	{//guard break — stagger open
		victim->NPC->guard = 0;
		const qboolean guardBoss = G_IsSaberBoss( victim, qtrue );
		victim->NPC->guardBreakTime = level.time + ( guardBoss ? Q_irand( 900, 1300 ) : Q_irand( 1600, 2400 ) );
		//same stagger the engine uses for an overwhelmed parry (pmove picks the anim)
		victim->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;
		victim->client->ps.saberBounceMove = LS_NONE;
		//audible tell — stumble bark
		G_AddVoiceEvent( victim, Q_irand( EV_PUSHED1, EV_PUSHED3 ), 500 );
		extern cvar_t *g_guardBreakSound;
		if ( g_guardBreakSound && g_guardBreakSound->string[0] )
		{//JKEnhanced 3D: a distinct clang so a guard break reads even without the bar
			G_SoundOnEnt( victim, CHAN_AUTO, g_guardBreakSound->string );
		}
#ifndef FINAL_BUILD
		if ( d_saberCombat->integer )
		{
			gi.Printf( S_COLOR_RED"%s GUARD BROKEN (%dms window)!\n", victim->NPC_type, victim->NPC->guardBreakTime - level.time );
		}
#endif
	}
}

extern cvar_t	*g_perfectRiposteTime;
extern cvar_t	*g_perfectParryGuardDamage;
extern cvar_t	*g_perfectParryStagger;
extern cvar_t	*g_perfectParryForceGain;
extern cvar_t	*g_heavyBlockStagger;
extern cvar_t	*g_heavyBlockDamage;
extern cvar_t	*g_heavyBlockForceCost;

/*
WP_PerfectParryReward (JKEnhanced)

What a landed perfect parry earns the defender (player or NPC; WP_PerfectParrySuccess is the
cue). The attacker loses guard (a parried heavy attack or kata breaks it outright) and is
staggered: "parryStaggered" stops their parries (WP_SaberParry) and, for a player, their attack
and block buttons (ClientThink_real); NPCs also get "attackDelay". The defender gains Force
(double vs a heavy attack). A player defender gets a riposte: the next swing comes out at once
("riposteWindow") and the first hit gets through the target's parry/evade ("perfectRiposte"; no
damage bonus). A kata perfectly parried is cancelled on the spot into the stagger.
ranged: a thrown saber - no stagger (out of reach), Force and riposte only.
*/
static qboolean WP_InKata( const playerState_t *ps )
{
	return (qboolean)( ps->saberMove == LS_A1_SPECIAL || ps->saberMove == LS_A2_SPECIAL || ps->saberMove == LS_A3_SPECIAL
		|| ps->saberMove == LS_DUAL_SPIN_PROTECT || ps->saberMove == LS_STAFF_SOULCAL
		|| ( ps->saber[0].kataMove > LS_NONE && ps->saberMove == ps->saber[0].kataMove )
		|| ( ps->dualSabers && ps->saber[1].kataMove > LS_NONE && ps->saberMove == ps->saber[1].kataMove ) );
}

void WP_PerfectParryReward( gentity_t *defender, gentity_t *attacker, qboolean heavy, qboolean ranged )
{
	if ( !defender || !defender->client )
	{
		return;
	}
	if ( attacker && attacker->client && !ranged )
	{
		playerState_t *aps = &attacker->client->ps;
		if ( WP_InKata( aps ) )
		{//JKEnhanced: the kata stops here - drop the full-body anim and go straight into the stagger
			aps->saberMove = aps->saberMoveNext = LS_READY;
			aps->saberBounceMove = LS_NONE;
			aps->weaponTime = 0;
			aps->torsoAnimTimer = 0;
			aps->legsAnimTimer = 0;
			aps->saberAttackChainCount = 0;
			aps->saberBlocked = BLOCKED_PARRY_BROKEN;
			if ( g_debugParry && g_debugParry->integer )
			{
				gi.Printf( "perfect parry: %s's kata CANCELLED\n", attacker->NPC_type ? attacker->NPC_type : "player" );
			}
		}
		if ( WP_SaberRules() )
		{//the attacker is Open: short, or medium off a heavy attack or kata
			WP_SaberSetOpen( attacker, heavy ? g_saberOpenMedium->integer : g_saberOpenShort->integer );
		}
		else
		{
			if ( attacker->NPC && attacker->NPC->guardMax > 0 )
			{
				G_DamageGuard( attacker, defender, heavy ? attacker->NPC->guard + 1 : g_perfectParryGuardDamage->integer );
			}
			TIMER_Set( attacker, "parryStaggered", g_perfectParryStagger->integer );
			if ( attacker->NPC )
			{
				TIMER_Set( attacker, "attackDelay", g_perfectParryStagger->integer + 300 );
			}
		}
	}
	if ( defender->client->ps.forcePowerMax > 0 )
	{
		defender->client->ps.forcePower += g_perfectParryForceGain->integer * ( heavy ? 2 : 1 );
		if ( defender->client->ps.forcePower > defender->client->ps.forcePowerMax )
		{
			defender->client->ps.forcePower = defender->client->ps.forcePowerMax;
		}
	}
	if ( defender->s.number == 0 )
	{
		TIMER_Set( defender, "riposteWindow", g_perfectRiposteTime->integer );
		TIMER_Set( defender, "perfectRiposte", g_perfectRiposteTime->integer );
		s_riposteTarget = attacker ? attacker->s.number : ENTITYNUM_NONE;
	}
}

//the player's riposte after a perfect parry only gets through to the one they parried
//(not saved: after a load the timer may still run, but no target matches, so nothing opens)
qboolean WP_RiposteOpenOn( const gentity_t *attacker, const gentity_t *victim )
{
	return (qboolean)( !WP_SaberRules()//(saber rules: a perfect parry leaves the attacker Open instead - visible, and the same for everyone)
		&& attacker && victim && attacker->s.number == 0 && victim->NPC
		&& victim->s.number == s_riposteTarget && !TIMER_Done( (gentity_t *)attacker, "perfectRiposte" ) );
}

/*
g_saberDeterministic (JKEnhanced, default 1)

Saber clashes resolve from things the player did or can see, not hidden dice:
- a held, active block against a normal swing holds (no "parry broken" power roll); what wears
  a blocker down is Force (block cost scales with the attacker's swing power) - at 0 Force the
  block breaks;
- heavy attacks / katas: only a perfect parry stops them cleanly (the heavy-attack branch);
- knockaways come from a perfect parry (player timing, or an NPC reading a predictable attack:
  WP_NPCPerfectParry), or Saber Defense 3 vs a Fast swing (blocking in a Strong stance no longer does);
- a swing pushes through a clash only at power 3 (no 50/50 at power 2);
- saber locks: only when both swings are heavy (power 3), at most once per 8 s per pair.
0 = the stock rolls.
*/
extern cvar_t *g_saberDeterministic;
extern cvar_t *g_debugSaberLock;
static qboolean WP_DetSaberLockGate( gentity_t *a, gentity_t *b, int aPow, int bPow, qboolean forceLock )
{
	if ( g_debugSaberLock->integer || forceLock )
	{
		return qtrue;
	}
	return (qboolean)( aPow >= FORCE_LEVEL_3 && bPow >= FORCE_LEVEL_3
		&& TIMER_Done( a, "saberLockCD" ) && TIMER_Done( b, "saberLockCD" ) );
}

int WP_SaberBlockForceCost( gentity_t *self, qboolean missileBlock );
extern cvar_t *g_npcBlockCost;
qboolean WP_SaberParry( gentity_t *victim, gentity_t *attacker, int saberNum, int bladeNum )
{
	if ( !victim || !victim->client || !attacker )
	{
		return qfalse;
	}
	if ( WP_SaberRules() && attacker->client && !attacker->client->ps.saberInFlight )
	{//JKEnhanced "a blade in the way": an in-hand swing is stopped by a raised guard and by nothing else
	 //(the Force cost is WP_SaberGuardStop's; thrown sabers keep the rules below)
		if ( Rosh_BeingHealed( victim ) || G_InCinematicSaberAnim( victim )
			|| PM_SuperBreakLoseAnim( victim->client->ps.torsoAnim ) || PM_SuperBreakWinAnim( victim->client->ps.torsoAnim ) )
		{
			return qfalse;
		}
		const guardState_t gs = WP_GuardState( victim, attacker );
		if ( gs != GUARD_UP )
		{
			WP_NoteGuardFail( victim, attacker, gs );
			return qfalse;
		}
		if ( victim->s.number == 0 && g_debugBlock && g_debugBlock->integer )
		{
			static int lastStopPrint = -1;
			if ( lastStopPrint != level.time )
			{
				lastStopPrint = level.time;
				gi.Printf( S_COLOR_GREEN "BLOCKED" S_COLOR_WHITE " vs %s (style %d%s), Force %d\n",
					attacker->NPC_type ? attacker->NPC_type : "?", attacker->client->ps.saberAnimLevel,
					WP_IsHeavyAttack( attacker ) ? ", HEAVY" : "", victim->client->ps.forcePower );
			}
		}
		if ( !PM_SaberInTransitionAny( victim->client->ps.saberMove )
			&& !PM_SaberInBounce( victim->client->ps.saberMove )
			&& !PM_SaberInKnockaway( victim->client->ps.saberMove ) )
		{//bring the blade to where the hit landed
			WP_SaberBlockNonRandom( victim, saberHitLocation, qfalse );
		}
		victim->client->ps.saberEventFlags |= SEF_PARRIED;
		//since it was stopped, take away any damage done
		WP_SaberClearDamageForEntNum( attacker, victim->s.number, saberNum, bladeNum );
		if ( victim->enemy != attacker && victim->client->playerTeam != attacker->client->playerTeam )
		{//they're not mad at me and they're not on my team
			G_ClearEnemy( victim );
			G_SetEnemy( victim, attacker );
		}
		return qtrue;
	}
	if ( victim->NPC && victim->NPC->guardBreakTime > level.time )
	{//guard-broken — defenseless until the window lapses
		return qfalse;
	}
	if ( !TIMER_Done( victim, "parryStaggered" ) )
	{//JKEnhanced: knocked off balance by a perfect parry / a broken block against a heavy attack
		WP_NoteBlockFail( victim, "you were staggered (enemy perfect parry, or a broken block vs a heavy attack)" );
		return qfalse;
	}
	if ( WP_RiposteOpenOn( attacker, victim ) )
	{//JKEnhanced: the player's riposte after a perfect parry goes through
		return qfalse;
	}
	// Saber NPCs could play by the player's rules: blocking a swing costs Force, and with none left
	// the block doesn't hold. JKEnhanced 4B: off while blocks are free for NPCs (WP_NPCForceFree -
	// the guard meter covers blocking), so a bar spent on powers never drops their guard.
	const qboolean npcSaber = (qboolean)( victim->NPC && victim->s.number >= MAX_CLIENTS
		&& victim->client->ps.weapon == WP_SABER && victim->client->ps.forcePowerMax > 0
		&& !WP_NPCForceFree( victim, FP_SABER_DEFENSE ) );
	if ( npcSaber && victim->client->ps.forcePower <= 0 )
	{
		return qfalse;
	}
	if ( Rosh_BeingHealed( victim ) )
	{
		return qfalse;
	}
	if ( PM_InKnockDown( &victim->client->ps ) )
	{//JKEnhanced: on the ground or getting up - no parry (a stab-down on a downed Reborn was being
	 //"parried": it ate their guard and did no damage)
		WP_NoteBlockFail( victim, "knocked down / getting up" );
		return qfalse;
	}
	if ( G_InCinematicSaberAnim( victim ) )
	{
		WP_NoteBlockFail( victim, "in a cinematic saber anim" );
		return qfalse;
	}
	if ( PM_SuperBreakLoseAnim( victim->client->ps.torsoAnim )
		|| PM_SuperBreakWinAnim( victim->client->ps.torsoAnim ) )
	{
		WP_NoteBlockFail( victim, "in a saber-lock super-break anim" );
		return qfalse;
	}
	// Manual mode: holding block parries from any direction — timing (the perfect-parry
	// window) is the skill test, and impact position picks the directional block anim.
	qboolean canParry = (qboolean)(victim->s.number != 0 || g_saberAutoBlocking->integer || victim->client->ps.saberBlockingTime > level.time);
	if ( !canParry )
	{
		WP_NoteBlockFail( victim, "block not active at impact (button released, or it lapsed for a frame)" );
	}

	if ( canParry )
	{//either an NPC or a player who is blocking
		if ( victim->s.number == 0 && g_debugBlock && g_debugBlock->integer )
		{//g_debugBlock: successful blocks too, so a run shows the whole picture
			static int lastBlockPrint = -1;
			if ( lastBlockPrint != level.time )
			{
				lastBlockPrint = level.time;
				gi.Printf( S_COLOR_GREEN "BLOCKED" S_COLOR_WHITE " vs %s (style %d%s), Force %d, move %s\n",
					attacker->NPC_type ? attacker->NPC_type : "?", attacker->client ? attacker->client->ps.saberAnimLevel : 0,
					WP_IsHeavyAttack( attacker ) ? ", HEAVY" : "", victim->client->ps.forcePower,
					saberMoveData[victim->client->ps.saberMove].name );
			}
		}
		if ( !PM_SaberInTransitionAny( victim->client->ps.saberMove )
			&& !PM_SaberInBounce( victim->client->ps.saberMove )
			&& !PM_SaberInKnockaway( victim->client->ps.saberMove ) )
		{//I'm not attacking, in transition or in a bounce or knockaway, so play a parry
			WP_SaberBlockNonRandom( victim, saberHitLocation, qfalse );
		}
		victim->client->ps.saberEventFlags |= SEF_PARRIED;

		// Saber guard: blocking costs composure (NPC defenders only)
		if ( victim->NPC && victim->NPC->guardMax > 0 && attacker->client )
		{
			int gdmg;
			switch ( attacker->client->ps.saberAnimLevel )
			{
			case SS_FAST:	gdmg = 6;	break;
			case SS_MEDIUM:	gdmg = 10;	break;
			case SS_STRONG:	gdmg = 16;	break;
			case SS_DESANN:	gdmg = 20;	break;
			case SS_TAVION:	gdmg = 12;	break;
			case SS_DUAL:	gdmg = 8;	break;
			case SS_STAFF:	gdmg = 9;	break;
			default:		gdmg = 10;	break;
			}
			gdmg += attacker->client->ps.saber[saberNum].breakParryBonus * 2;
			// Repeated attacks from the same angle lose guard-damage effectiveness --
			// except Dual, whose rapid direction changes and sustained pressure are its identity
			if ( victim->client->ps.saberBlocked == victim->NPC->lastBlockDir
				&& attacker->client->ps.saberAnimLevel != SS_DUAL )
			{
				victim->NPC->sameBlockDirCount++;
				float scale = 1.0f - 0.25f * victim->NPC->sameBlockDirCount;
				if ( scale < 0.4f )
				{
					scale = 0.4f;
				}
				gdmg = (int)(gdmg * scale);
			}
			else
			{
				victim->NPC->sameBlockDirCount = 0;
				victim->NPC->lastBlockDir = victim->client->ps.saberBlocked;
			}
			G_DamageGuard( victim, attacker, gdmg );
		}

		// pay for it (once per swing: a blade stays in contact over several frames)
		if ( npcSaber && victim->NPC->blockDebounce < level.time && !WP_NPCPerfectParry( victim ) )
		{
			float scale = g_npcBlockCost ? g_npcBlockCost->value : 0.5f;
			if ( (victim->NPC->aiFlags&NPCAI_BOSS_CHARACTER) )
			{
				scale *= 0.5f;
			}
			const int cost = (int)( WP_SaberBlockForceCost( victim, qfalse ) * scale );
			if ( cost > 0 )
			{
				WP_ForcePowerDrain( victim, FP_SABER_DEFENSE, cost );
				victim->client->ps.forcePowerRegenDebounceTime = level.time + 1500;
			}
			victim->NPC->blockDebounce = level.time + 400;
		}
		// the player gets a riposte: attack right off a (non-perfect) block and the parry
		// recovery is skipped (G_CheckClampUcmd)
		if ( victim->s.number == 0 && !WP_InPerfectParryWindowVs( victim, attacker, qfalse ) )
		{
			TIMER_Set( victim, "riposteWindow", 300 );
		}

		//since it was parried, take away any damage done
		//FIXME: what if the damage was done before the parry?
		WP_SaberClearDamageForEntNum( attacker, victim->s.number, saberNum, bladeNum );

		//tell the victim to get mad at me
		if ( victim->enemy != attacker && victim->client->playerTeam != attacker->client->playerTeam )
		{//they're not mad at me and they're not on my team
			G_ClearEnemy( victim );
			G_SetEnemy( victim, attacker );
		}
		return qtrue;
	}
	return qfalse;
}

qboolean WP_BrokenParryKnockDown( gentity_t *victim )
{
	if ( !victim || !victim->client )
	{
		return qfalse;
	}
	if ( PM_SuperBreakLoseAnim( victim->client->ps.torsoAnim )
		|| PM_SuperBreakWinAnim( victim->client->ps.torsoAnim ) )
	{
		return qfalse;
	}
	if ( victim->client->ps.saberMove == LS_PARRY_UP
		|| victim->client->ps.saberMove == LS_PARRY_UR
		|| victim->client->ps.saberMove == LS_PARRY_UL
		|| victim->client->ps.saberMove == LS_H1_BR
		|| victim->client->ps.saberMove == LS_H1_B_
		|| victim->client->ps.saberMove == LS_H1_BL )
	{//knock their asses down!
		int knockAnim = BOTH_KNOCKDOWN1;
		if ( PM_CrouchAnim( victim->client->ps.legsAnim ) )
		{
			knockAnim = BOTH_KNOCKDOWN4;
		}
		NPC_SetAnim( victim, SETANIM_BOTH, knockAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		G_AddEvent( victim, EV_PAIN, victim->health );
		return qtrue;
	}
	return qfalse;
}

qboolean G_TryingKataAttack( gentity_t *self, usercmd_t *cmd )
{
	if ( g_saberNewControlScheme->integer )
	{//use the new control scheme: force focus button
		if ( (cmd->buttons&BUTTON_FORCE_FOCUS) )
		{
			return qtrue;
		}
		else
		{
			return qfalse;
		}
	}
	else //if ( self && self->client )
	{//use the old control scheme: dedicated Kata key (the old alt-attack+attack combo is no
	 //longer reachable now that Block asserts alt-attack for the perfect-parry input scheme)
		if ( (cmd->buttons&BUTTON_KATA) )
		{
			return qtrue;
		}
	}
	return qfalse;
}

qboolean G_TryingPullAttack( gentity_t *self, usercmd_t *cmd, qboolean amPulling )
{
	if ( g_saberNewControlScheme->integer )
	{//use the new control scheme: force focus button
		if ( (cmd->buttons&BUTTON_FORCE_FOCUS) )
		{
			if ( self && self->client )
			{
				if ( self->client->ps.forcePowerLevel[FP_PULL] >= FORCE_LEVEL_3 )
				{//force pull 3
					if ( amPulling
						|| (self->client->ps.forcePowersActive&(1<<FP_PULL))
						|| self->client->ps.forcePowerDebounce[FP_PULL] > level.time ) //force-pulling
					{//pulling
						return qtrue;
					}
				}
			}
		}
		else
		{
			return qfalse;
		}
	}
	else
	{//use the old control scheme
		if ( (cmd->buttons&BUTTON_ATTACK) )
		{//pressing attack
			if ( self && self->client )
			{
				if ( self->client->ps.forcePowerLevel[FP_PULL] >= FORCE_LEVEL_3 )
				{//force pull 3
					if ( amPulling
						|| (self->client->ps.forcePowersActive&(1<<FP_PULL))
						|| self->client->ps.forcePowerDebounce[FP_PULL] > level.time ) //force-pulling
					{//pulling
						return qtrue;
					}
				}
			}
		}
	}
	return qfalse;
}

qboolean G_TryingCartwheel( gentity_t *self, usercmd_t *cmd )
{
	if ( g_saberNewControlScheme->integer )
	{//use the new control scheme: force focus button
		if ( (cmd->buttons&BUTTON_FORCE_FOCUS) )
		{
			return qtrue;
		}
		else
		{
			return qfalse;
		}
	}
	else
	{//use the old control scheme
		if ( (cmd->buttons&BUTTON_ATTACK) )
		{//pressing attack
			if ( cmd->rightmove )
			{
				if ( self && self->client )
				{
					if ( cmd->upmove>0 //)
						&& self->client->ps.groundEntityNum != ENTITYNUM_NONE )
					{//on ground, pressing jump
						return qtrue;
					}
					else
					{//just jumped?
						if ( self->client->ps.groundEntityNum == ENTITYNUM_NONE
							&& level.time - self->client->ps.lastOnGround <= 50//250
							&& (self->client->ps.pm_flags&PMF_JUMPING) )//jumping
						{//just jumped this or last frame
							return qtrue;
						}
					}
				}
			}
		}
	}
	return qfalse;
}

qboolean G_TryingSpecial( gentity_t *self, usercmd_t *cmd )
{
	if ( g_saberNewControlScheme->integer )
	{//use the new control scheme: force focus button
		if ( (cmd->buttons&BUTTON_FORCE_FOCUS) )
		{
			return qtrue;
		}
		else
		{
			return qfalse;
		}
	}
	else
	{//use the old control scheme
		return qfalse;
	}
}

qboolean G_TryingJumpAttack( gentity_t *self, usercmd_t *cmd )
{
	if ( g_saberNewControlScheme->integer )
	{//use the new control scheme: force focus button
		if ( (cmd->buttons&BUTTON_FORCE_FOCUS) )
		{
			return qtrue;
		}
		else
		{
			return qfalse;
		}
	}
	else
	{//use the old control scheme
		if ( (cmd->buttons&BUTTON_ATTACK) )
		{//pressing attack
			if ( cmd->upmove>0 )
			{//pressing jump
				return qtrue;
			}
			else if ( self && self->client )
			{//just jumped?
				if ( self->client->ps.groundEntityNum == ENTITYNUM_NONE
					&& level.time - self->client->ps.lastOnGround <= 250
					&& (self->client->ps.pm_flags&PMF_JUMPING) )//jumping
				{//jumped within the last quarter second
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

qboolean G_TryingJumpForwardAttack( gentity_t *self, usercmd_t *cmd )
{
	if ( g_saberNewControlScheme->integer )
	{//use the new control scheme: force focus button
		if ( (cmd->buttons&BUTTON_FORCE_FOCUS) )
		{
			return qtrue;
		}
		else
		{
			return qfalse;
		}
	}
	else
	{//use the old control scheme
		if ( (cmd->buttons&BUTTON_ATTACK) )
		{//pressing attack
			if ( cmd->forwardmove > 0 )
			{//moving forward
				if ( self && self->client )
				{
					if ( cmd->upmove>0
						&& self->client->ps.groundEntityNum != ENTITYNUM_NONE )
					{//pressing jump
						return qtrue;
					}
					else
					{//no slop on forward jumps - must be precise!
						if ( self->client->ps.groundEntityNum == ENTITYNUM_NONE
							&& level.time - self->client->ps.lastOnGround <= 50
							&& (self->client->ps.pm_flags&PMF_JUMPING) )//jumping
						{//just jumped this or last frame
							return qtrue;
						}
					}
				}
			}
		}
	}
	return qfalse;
}

qboolean G_TryingLungeAttack( gentity_t *self, usercmd_t *cmd )
{
	if ( g_saberNewControlScheme->integer )
	{//use the new control scheme: force focus button
		if ( (cmd->buttons&BUTTON_FORCE_FOCUS) )
		{
			return qtrue;
		}
		else
		{
			return qfalse;
		}
	}
	else
	{//use the old control scheme
		if ( (cmd->buttons&BUTTON_ATTACK) )
		{//pressing attack
			if ( cmd->upmove<0 )
			{//pressing crouch
				return qtrue;
			}
			else if ( self && self->client )
			{//just unducked?
				if ( (self->client->ps.pm_flags&PMF_DUCKED) )
				{//just unducking
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}


//FIXME: for these below funcs, maybe in the old control scheme some moves should still cost power... if so, pass in the saberMove and use a switch statement
qboolean G_EnoughPowerForSpecialMove( int forcePower, int cost, qboolean kataMove )
{
	if ( g_saberNewControlScheme->integer || kataMove )
	{//special moves cost power
		if ( forcePower >= cost )
		{
			return qtrue;
		}
		else
		{
			cg.forceHUDTotalFlashTime = level.time + 1000;
			return qfalse;
		}
	}
	else
	{//old control scheme: uses no power, so just do it
		return qtrue;
	}
}

void G_DrainPowerForSpecialMove( gentity_t *self, forcePowers_t fp, int cost, qboolean kataMove )
{
	if ( !self || !self->client || self->s.number >= MAX_CLIENTS )
	{
		return;
	}
	if ( g_saberNewControlScheme->integer || kataMove )
	{//special moves cost power
		WP_ForcePowerDrain( self, fp, cost );//drain the required force power
	}
	else
	{//old control scheme: uses no power, so just do it
	}
}

int G_CostForSpecialMove( int cost, qboolean kataMove )
{
	if ( g_saberNewControlScheme->integer || kataMove )
	{//special moves cost power
		return cost;
	}
	else
	{//old control scheme: uses no power, so just do it
		return 0;
	}
}

extern qboolean G_EntIsBreakable( int entityNum, gentity_t *breaker );
void WP_SaberRadiusDamage( gentity_t *ent, vec3_t point, float radius, int damage, float knockBack )
{
	if ( !ent || !ent->client )
	{
		return;
	}
	else if ( radius <= 0.0f || (damage <= 0 && knockBack <= 0) )
	{
		return;
	}
	else
	{
		vec3_t		mins, maxs, entDir;
		gentity_t	*radiusEnts[128];
		int			numEnts, i;
		float		dist;

		//Setup the bbox to search in
		for ( i = 0; i < 3; i++ )
		{
			mins[i] = point[i] - radius;
			maxs[i] = point[i] + radius;
		}

		//Get the number of entities in a given space
		numEnts = gi.EntitiesInBox( mins, maxs, radiusEnts, 128 );

		for ( i = 0; i < numEnts; i++ )
		{
			if ( !radiusEnts[i]->inuse )
			{
				continue;
			}

			if ( radiusEnts[i] == ent )
			{//Skip myself
				continue;
			}

			if ( radiusEnts[i]->client == NULL )
			{//must be a client
				if ( G_EntIsBreakable( radiusEnts[i]->s.number, ent ) )
				{//damage breakables within range, but not as much
					G_Damage( radiusEnts[i], ent, ent, vec3_origin, radiusEnts[i]->currentOrigin, 10, 0, MOD_EXPLOSIVE_SPLASH );
				}
				continue;
			}

			if ( (radiusEnts[i]->client->ps.eFlags&EF_HELD_BY_RANCOR)
				|| (radiusEnts[i]->client->ps.eFlags&EF_HELD_BY_WAMPA) )
			{//can't be one being held
				continue;
			}

			VectorSubtract( radiusEnts[i]->currentOrigin, point, entDir );
			dist = VectorNormalize( entDir );
			if ( dist <= radius )
			{//in range
				if ( damage > 0 )
				{//do damage
					int points = ceil((float)damage*dist/radius);
					G_Damage( radiusEnts[i], ent, ent, vec3_origin, radiusEnts[i]->currentOrigin, points, DAMAGE_NO_KNOCKBACK, MOD_EXPLOSIVE_SPLASH );
				}
				if ( knockBack > 0 )
				{//do knockback
					if ( radiusEnts[i]->client
						&& radiusEnts[i]->client->NPC_class != CLASS_RANCOR
						&& radiusEnts[i]->client->NPC_class != CLASS_ATST
						&& !(radiusEnts[i]->flags&FL_NO_KNOCKBACK) )//don't throw them back
					{
						float knockbackStr = knockBack*dist/radius;
						entDir[2] += 0.1f;
						VectorNormalize( entDir );
						G_Throw( radiusEnts[i], entDir, knockbackStr );
						if ( radiusEnts[i]->health > 0 )
						{//still alive
							if ( knockbackStr > 50 )
							{//close enough and knockback high enough to possibly knock down
								if ( dist < (radius*0.5f)
									|| radiusEnts[i]->client->ps.groundEntityNum != ENTITYNUM_NONE )
								{//within range of my fist or within ground-shaking range and not in the air
									G_Knockdown( radiusEnts[i], ent, entDir, 500, qtrue );
								}
							}
						}
					}
				}
			}
		}
	}
}
/*
---------------------------------------------------------
void WP_SaberDamageTrace( gentity_t *ent, int saberNum, int bladeNum )

  Constantly trace from the old blade pos to new, down the saber beam and do damage

  FIXME: if the dot product of the old muzzle dir and the new muzzle dir is < 0.75, subdivide it and do multiple traces so we don't flatten out the arc!
---------------------------------------------------------
*/
#define MAX_SABER_SWING_INC 0.33f
void WP_SaberDamageTrace( gentity_t *ent, int saberNum, int bladeNum )
{
	vec3_t		mp1, mp2, md1, md2, baseOld, baseNew, baseDiff, endOld, endNew, bladePointOld, bladePointNew;
	float		tipDmgMod = 1.0f;
	float		baseDamage;
	int			baseDFlags = 0;
	qboolean	hit_wall = qfalse;
	qboolean	brokenParry = qfalse;

	for ( int ven = 0; ven < MAX_SABER_VICTIMS; ven++ )
	{
		victimEntityNum[ven] = ENTITYNUM_NONE;
	}
	memset( totalDmg, 0, sizeof( totalDmg) );
	memset( dmgDir, 0, sizeof( dmgDir ) );
	memset( dmgNormal, 0, sizeof( dmgNormal ) );
	memset( dmgSpot, 0, sizeof( dmgSpot ) );
	memset( dmgFraction, 0, sizeof( dmgFraction ) );
	memset( hitLoc, HL_NONE, sizeof( hitLoc ) );
	memset( hitDismemberLoc, HL_NONE, sizeof( hitDismemberLoc ) );
	memset( hitDismember, qfalse, sizeof( hitDismember ) );
	numVictims = 0;
	VectorClear(saberHitLocation);
	VectorClear(saberHitNormal);
	saberHitFraction = 1.0;	// Closest saber hit.  The saber can do no damage past this point.
	saberHitEntity = ENTITYNUM_NONE;
	sabersCrossed = -1;

	if ( !ent->client )
	{
		return;
	}

	if ( WP_SaberHitStopped( ent ) )
	{//JKEnhanced: frozen for a hit-stop (a cut just landed) - the blade does nothing more until it ends
		return;
	}
	if ( WP_SaberRules() && WP_SaberInFollowThrough( ent ) )
	{//JKEnhanced saber rules: holding the end of a phrase's last swing (the opening) - the blade is spent
		return;
	}

	if ( !ent->s.number )
	{//player never uses these
		ent->client->ps.saberEventFlags &= ~SEF_EVENTS;
	}

	if ( ent->client->ps.saber[saberNum].blade[bladeNum].length <= 1 )//cen get down to 1 when in a wall
	{//saber is not on
		return;
	}

	if ( VectorCompare( ent->client->renderInfo.muzzlePointOld, vec3_origin ) || VectorCompare( ent->client->renderInfo.muzzleDirOld, vec3_origin ) )
	{
		//just started up the saber?
		return;
	}

	int saberContents = 0;
	if ( !(ent->client->ps.saber[saberNum].saberFlags&SFL_ON_IN_WATER) )
	{//saber can't stay on underwater
		saberContents = gi.pointcontents( ent->client->renderInfo.muzzlePoint, ent->client->ps.saberEntityNum );
	}
	if ( (saberContents&CONTENTS_WATER)||
		(saberContents&CONTENTS_SLIME)||
		(saberContents&CONTENTS_LAVA) )
	{//um... turn off?  Or just set length to 1?
		//FIXME: short-out effect/sound?
		ent->client->ps.saber[saberNum].blade[bladeNum].active = qfalse;
		return;
	}
	else if (!g_saberNoEffects && gi.WE_IsOutside(ent->client->renderInfo.muzzlePoint))
	{
		float chanceOfFizz = gi.WE_GetChanceOfSaberFizz();
		if (chanceOfFizz>0 && Q_flrand(0.0f, 1.0f)<chanceOfFizz)
		{
			vec3_t	end; /*normal = {0,0,1};//FIXME: opposite of rain angles?*/
			VectorMA( ent->client->ps.saber[saberNum].blade[bladeNum].muzzlePoint, ent->client->ps.saber[saberNum].blade[bladeNum].length*Q_flrand(0, 1), ent->client->ps.saber[saberNum].blade[bladeNum].muzzleDir, end );
			G_PlayEffect( "saber/fizz", end );
		}
	}

	//FIXMEFIXMEFIXME: When in force speed (esp. lvl 3), need to interpolate this because
	//		we animate so much faster that the arc is pretty much flat...

	int entPowerLevel = 0;
	if ( ent->client->NPC_class == CLASS_SABER_DROID )
	{
		entPowerLevel = SaberDroid_PowerLevelForSaberAnim( ent );
	}
	else if ( !ent->s.number && (ent->client->ps.forcePowersActive&(1<<FP_SPEED)) )
	{
		entPowerLevel = FORCE_LEVEL_3;
	}
	else
	{
		entPowerLevel = PM_PowerLevelForSaberAnim( &ent->client->ps, saberNum );
	}

	if ( entPowerLevel )
	{
		if ( ent->client->ps.forceRageRecoveryTime > level.time )
		{
			entPowerLevel = FORCE_LEVEL_1;
		}
		else if ( ent->client->ps.forcePowersActive & (1 << FP_RAGE) )
		{
			entPowerLevel += ent->client->ps.forcePowerLevel[FP_RAGE];
		}
	}

	if ( ent->client->ps.saberInFlight )
	{//flying sabers are much more deadly
		//unless you're dead
		if ( ent->health <= 0 && g_saberRealisticCombat->integer < 2 )
		{//so enemies don't keep trying to block it
			//FIXME: still do damage, just not to humanoid clients who should try to avoid it
			//baseDamage = 0.0f;
			return;
		}
		//or unless returning
		else if ( ent->client->ps.saberEntityState == SES_RETURNING
			&& !(ent->client->ps.saber[0].saberFlags&SFL_RETURN_DAMAGE) )//type != SABER_STAR )
		{//special case, since we're returning, chances are if we hit something
			//it's going to be butt-first.  So do less damage.
			baseDamage = 0.1f;
		}
		else
		{
			if ( !ent->s.number )
			{//cheat for player
				baseDamage = 10.0f;
			}
			else
			{
				baseDamage = 2.5f;
			}
		}
		//Use old to current since can't predict it
		VectorCopy( ent->client->ps.saber[saberNum].blade[bladeNum].muzzlePointOld, mp1 );
		VectorCopy( ent->client->ps.saber[saberNum].blade[bladeNum].muzzleDirOld, md1 );
		VectorCopy( ent->client->ps.saber[saberNum].blade[bladeNum].muzzlePoint, mp2 );
		VectorCopy( ent->client->ps.saber[saberNum].blade[bladeNum].muzzleDir, md2 );
	}
	else
	{
		if ( ent->client->ps.torsoAnim == BOTH_A7_HILT )
		{//no effects, no damage
			return;
		}
		else if ( G_InCinematicSaberAnim( ent ) )
		{
			baseDFlags = DAMAGE_NO_KILL;
			baseDamage = 0.1f;
		}
		else if ( ent->client->ps.saberMove == LS_READY
			&& !PM_SaberInSpecialAttack( ent->client->ps.torsoAnim ) )
		{//just do effects
			if ( g_saberRealisticCombat->integer < 2 )
			{//don't kill with this hit
				baseDFlags = DAMAGE_NO_KILL;
			}
			if (  (!WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
						&& (ent->client->ps.saber[saberNum].saberFlags2&SFL2_NO_IDLE_EFFECT) )
				|| ( WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
						&& (ent->client->ps.saber[saberNum].saberFlags2&SFL2_NO_IDLE_EFFECT2) )
				)
			{//do nothing at all when idle
				return;
			}
			baseDamage = 0;
		}
		else if ( ent->client->ps.saberLockTime > level.time )
		{//just do effects
			if (  (!WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
						&& (ent->client->ps.saber[saberNum].saberFlags2&SFL2_NO_IDLE_EFFECT) )
				|| ( WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
						&& (ent->client->ps.saber[saberNum].saberFlags2&SFL2_NO_IDLE_EFFECT2) )
				)
			{//do nothing at all when idle
				return;
			}
			baseDamage = 0;
		}
		else if ( !WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
			&& ent->client->ps.saber[saberNum].damageScale <= 0.0f
			&& ent->client->ps.saber[saberNum].knockbackScale <= 0.0f )
		{//this blade does no damage and no knockback (only for blocking?)
			baseDamage = 0;
		}
		else if ( WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
			&& ent->client->ps.saber[saberNum].damageScale2 <= 0.0f
			&& ent->client->ps.saber[saberNum].knockbackScale2 <= 0.0f )
		{//this blade does no damage and no knockback (only for blocking?)
			baseDamage = 0;
		}
		else if ( ent->client->ps.saberBlocked > BLOCKED_NONE
				 || ( !PM_SaberInAttack( ent->client->ps.saberMove )
					  && !PM_SaberInSpecialAttack( ent->client->ps.torsoAnim )
					  && !PM_SaberInTransitionAny( ent->client->ps.saberMove )
					)
				)
		{//don't do damage if parrying/reflecting/bouncing/deflecting or not actually attacking or in a transition to/from/between attacks
			if (  (!WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
						&& (ent->client->ps.saber[saberNum].saberFlags2&SFL2_NO_IDLE_EFFECT) )
				|| ( WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
						&& (ent->client->ps.saber[saberNum].saberFlags2&SFL2_NO_IDLE_EFFECT2) )
				)
			{//do nothing at all when idle
				return;
			}
			baseDamage = 0;
		}
		else
		{//okay, in a saberMove that does damage
			//make sure we're in the right anim
			if ( !PM_SaberInSpecialAttack( ent->client->ps.torsoAnim )
				&& !PM_InAnimForSaberMove( ent->client->ps.torsoAnim, ent->client->ps.saberMove ) )
			{//forced into some other animation somehow, like a pain or death?
				if (  (!WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
							&& (ent->client->ps.saber[saberNum].saberFlags2&SFL2_NO_IDLE_EFFECT) )
					|| ( WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
							&& (ent->client->ps.saber[saberNum].saberFlags2&SFL2_NO_IDLE_EFFECT2) )
					)
				{//do nothing at all when idle
					return;
				}
				baseDamage = 0;
			}
			else if ( ent->client->ps.weaponstate == WEAPON_FIRING && ent->client->ps.saberBlocked == BLOCKED_NONE &&
				( PM_SaberInAttack(ent->client->ps.saberMove) || PM_SaberInSpecialAttack( ent->client->ps.torsoAnim ) || PM_SpinningSaberAnim(ent->client->ps.torsoAnim) || entPowerLevel > FORCE_LEVEL_2 || (WP_SaberBladeDoTransitionDamage( &ent->client->ps.saber[saberNum], bladeNum )&&PM_SaberInTransitionAny(ent->client->ps.saberMove)) ) )// || ent->client->ps.saberAnimLevel == SS_STAFF ) )
			{//normal attack swing swinging/spinning (or if using strong set), do normal damage //FIXME: or if using staff?
				//FIXME: more damage for higher attack power levels?
				//		More damage based on length/color of saber?
				//FIXME: Desann does double damage?
				if ( g_saberRealisticCombat->integer )
				{
					switch ( entPowerLevel )
					{
					default:
					case FORCE_LEVEL_3:
						baseDamage = 10.0f;
						break;
					case FORCE_LEVEL_2:
						baseDamage = 5.0f;
						break;
					case FORCE_LEVEL_0:
					case FORCE_LEVEL_1:
						baseDamage = 2.5f;
						break;
					}
				}
				else
				{
					if ( g_spskill->integer > 0
						&& ent->s.number < MAX_CLIENTS
						&& ( ent->client->ps.torsoAnim == BOTH_ROLL_STAB
							|| ent->client->ps.torsoAnim == BOTH_SPINATTACK6
							|| ent->client->ps.torsoAnim == BOTH_SPINATTACK7
							|| ent->client->ps.torsoAnim == BOTH_LUNGE2_B__T_ ) )
					{//*sigh*, these anim do less damage since they're so easy to do
						baseDamage = 2.5f;
					}
					else
					{
						baseDamage = 2.5f * (float)entPowerLevel;
					}
				}
			}
			else
			{//saber is transitioning, defending or idle, don't do as much damage
				//FIXME: strong attacks and returns should do damage and be unblockable
				if ( g_timescale->value < 1.0 )
				{//in slow mo or force speed, we need to do damage during the transitions
					if ( g_saberRealisticCombat->integer )
					{
						switch ( entPowerLevel )
						{
						case FORCE_LEVEL_3:
							baseDamage = 10.0f;
							break;
						case FORCE_LEVEL_2:
							baseDamage = 5.0f;
							break;
						default:
						case FORCE_LEVEL_1:
							baseDamage = 2.5f;
							break;
						}
					}
					else
					{
						baseDamage = 2.5f * (float)entPowerLevel;
					}
				}
				else// if ( !ent->s.number )
				{//I have to do *some* damage in transitions or else you feel like a total gimp
					baseDamage = 0.1f;
				}
				/*
				else
				{
					baseDamage = 0;//was 1.0f;//was 0.25
				}
				*/
			}
		}

		//Use current to next since can predict it
		//FIXME: if they're closer than the saber blade start, we don't want the
		//		arm to pass through them without any damage... so check the radius
		//		and push them away (do pain & knockback)
		//FIXME: if going into/coming from a parry/reflection or going into a deflection, don't use old mp & dir?  Otherwise, deflections will cut through?
		//VectorCopy( ent->client->renderInfo.muzzlePoint, mp1 );
		//VectorCopy( ent->client->renderInfo.muzzleDir, md1 );
		//VectorCopy( ent->client->renderInfo.muzzlePointNext, mp2 );
		//VectorCopy( ent->client->renderInfo.muzzleDirNext, md2 );
		//prediction was causing gaps in swing (G2 problem) so *don't* predict
		if ( ent->client->ps.saberDamageDebounceTime > level.time )
		{//really only used when a saber attack start anim starts, not actually for stopping damage
			//we just want to not use the old position to trace the attack from...
			VectorCopy( ent->client->ps.saber[saberNum].blade[bladeNum].muzzlePoint, ent->client->ps.saber[saberNum].blade[bladeNum].muzzlePointOld );
			VectorCopy( ent->client->ps.saber[saberNum].blade[bladeNum].muzzleDir, ent->client->ps.saber[saberNum].blade[bladeNum].muzzleDirOld );
		}
		//do the damage trace from the last position...
		VectorCopy( ent->client->ps.saber[saberNum].blade[bladeNum].muzzlePointOld, mp1 );
		VectorCopy( ent->client->ps.saber[saberNum].blade[bladeNum].muzzleDirOld, md1 );
		//...to the current one.
		VectorCopy( ent->client->ps.saber[saberNum].blade[bladeNum].muzzlePoint, mp2 );
		VectorCopy( ent->client->ps.saber[saberNum].blade[bladeNum].muzzleDir, md2 );

		//NOTE: this is a test, may not be necc, as I can still swing right through someone without hitting them, somehow...
		//see if anyone is so close that they're within the dist from my origin to the start of the saber
		if ( ent->health > 0 && !ent->client->ps.saberLockTime && saberNum == 0 && bladeNum == 0
			&& !G_InCinematicSaberAnim( ent ) )
		{//only do once - for first blade
			trace_t trace;
			gi.trace( &trace, ent->currentOrigin, vec3_origin, vec3_origin, mp1, ent->s.number, (MASK_SHOT&~(CONTENTS_CORPSE|CONTENTS_ITEM)), (EG2_Collision)0, 0 );
			if ( trace.entityNum < ENTITYNUM_WORLD && (trace.entityNum > 0||ent->client->NPC_class == CLASS_DESANN) )//NPCs don't push player away, unless it's Desann
			{//a valid ent
				gentity_t *traceEnt = &g_entities[trace.entityNum];
				if ( traceEnt
					&& traceEnt->client
					&& traceEnt->client->NPC_class != CLASS_RANCOR
					&& traceEnt->client->NPC_class != CLASS_ATST
					&& traceEnt->client->NPC_class != CLASS_WAMPA
					&& traceEnt->client->NPC_class != CLASS_SAND_CREATURE
					&& traceEnt->health > 0
					&& traceEnt->client->playerTeam != ent->client->playerTeam
					&& !PM_SuperBreakLoseAnim( traceEnt->client->ps.legsAnim )
					&& !PM_SuperBreakLoseAnim( traceEnt->client->ps.torsoAnim )
					&& !PM_SuperBreakWinAnim( traceEnt->client->ps.legsAnim )
					&& !PM_SuperBreakWinAnim( traceEnt->client->ps.torsoAnim )
					&& !PM_InKnockDown( &traceEnt->client->ps )
					&& !PM_LockedAnim( traceEnt->client->ps.legsAnim )
					&& !PM_LockedAnim( traceEnt->client->ps.torsoAnim )
					&& !G_InCinematicSaberAnim( traceEnt ))
				{//enemy client, push them away
					if ( !traceEnt->client->ps.saberLockTime
						&& !traceEnt->message
						&& !(traceEnt->flags&FL_NO_KNOCKBACK)
						&& (!traceEnt->NPC||traceEnt->NPC->jumpState!=JS_JUMPING) )
					{//don't push people in saberlock or with security keys or who are in BS_JUMP
						vec3_t hitDir;
						VectorSubtract( trace.endpos, ent->currentOrigin, hitDir );
						float totalDist = Distance( mp1, ent->currentOrigin );
						float knockback = (totalDist-VectorNormalize( hitDir ))/totalDist * 200.0f;
						hitDir[2] = 0;
						//FIXME: do we need to call G_Throw?  Seems unfair to put actual knockback on them, stops the attack
						//G_Throw( traceEnt, hitDir, knockback );
						VectorMA( traceEnt->client->ps.velocity, knockback, hitDir, traceEnt->client->ps.velocity );
						traceEnt->client->ps.pm_time = 200;
						traceEnt->client->ps.pm_flags |= PMF_TIME_NOFRICTION;
#ifndef FINAL_BUILD
						if ( d_saberCombat->integer )
						{
							gi.Printf( "%s pushing away %s at %s\n", ent->NPC_type, traceEnt->NPC_type, vtos( traceEnt->client->ps.velocity ) );
						}
#endif
					}
				}
			}
		}
	}

	//the thicker the blade, the more damage... the thinner, the less damage
	baseDamage *= ent->client->ps.saber[saberNum].blade[bladeNum].radius/SABER_RADIUS_STANDARD;

	if ( g_saberRealisticCombat->integer > 1 )
	{//always do damage, and lots of it
		if ( g_saberRealisticCombat->integer > 2 )
		{//always do damage, and lots of it
			baseDamage = 25.0f;
		}
		else if ( baseDamage > 0.1f )
		{//only do super damage if we would have done damage according to normal rules
			baseDamage = 25.0f;
		}
	}
	else if ( ((!ent->s.number&&ent->client->ps.forcePowersActive&(1<<FP_SPEED))||ent->client->ps.forcePowersActive&(1<<FP_RAGE))
		&& g_timescale->value < 1.0f )
	{
		baseDamage *= (1.0f-g_timescale->value);
	}
	if ( baseDamage > 0.1f )
	{
		if ( (ent->client->ps.forcePowersActive&(1<<FP_RAGE)) )
		{//add some damage if raged
			baseDamage += ent->client->ps.forcePowerLevel[FP_RAGE] * 5.0f;
		}
		else if ( ent->client->ps.forceRageRecoveryTime )
		{//halve it if recovering
			baseDamage *= 0.5f;
		}
	}
	// Get the old state of the blade
	VectorCopy( mp1, baseOld );
	VectorMA( baseOld, ent->client->ps.saber[saberNum].blade[bladeNum].length, md1, endOld );
	// Get the future state of the blade
	VectorCopy( mp2, baseNew );
	VectorMA( baseNew, ent->client->ps.saber[saberNum].blade[bladeNum].length, md2, endNew );

	sabersCrossed = -1;
	if ( VectorCompare2( baseOld, baseNew ) && VectorCompare2( endOld, endNew ) )
	{
		hit_wall = WP_SaberDamageForTrace( ent->s.number, mp2, endNew, baseDamage*4, md2,
			qfalse, entPowerLevel, ent->client->ps.saber[saberNum].type, qfalse,
			saberNum, bladeNum );
	}
	else
	{
		float aveLength, step = 8, stepsize = 8;
		vec3_t	ma1, ma2, md2ang, curBase1, curBase2;
		int	xx;
		//do the trace at the base first
		hit_wall = WP_SaberDamageForTrace( ent->s.number, baseOld, baseNew, baseDamage, md2,
			qfalse, entPowerLevel, ent->client->ps.saber[saberNum].type, qtrue,
			saberNum, bladeNum );

		//if hit a saber, shorten rest of traces to match
		if ( saberHitFraction < 1.0 )
		{
			//adjust muzzleDir...
			vec3_t ma1, ma2;
			vectoangles( md1, ma1 );
			vectoangles( md2, ma2 );
			for ( xx = 0; xx < 3; xx++ )
			{
				md2ang[xx] = LerpAngle( ma1[xx], ma2[xx], saberHitFraction );
			}
			AngleVectors( md2ang, md2, NULL, NULL );
			//shorten the base pos
			VectorSubtract( mp2, mp1, baseDiff );
			VectorMA( mp1, saberHitFraction, baseDiff, baseNew );
			VectorMA( baseNew, ent->client->ps.saber[saberNum].blade[bladeNum].length, md2, endNew );
		}

		//If the angle diff in the blade is high, need to do it in chunks of 33 to avoid flattening of the arc
		float dirInc, curDirFrac;
		if ( PM_SaberInAttack( ent->client->ps.saberMove )
			|| PM_SaberInSpecialAttack( ent->client->ps.torsoAnim )
			|| PM_SpinningSaberAnim( ent->client->ps.torsoAnim )
			|| PM_InSpecialJump( ent->client->ps.torsoAnim )
			|| (g_timescale->value<1.0f&&PM_SaberInTransitionAny( ent->client->ps.saberMove )) )
		{
			curDirFrac = DotProduct( md1, md2 );
		}
		else
		{
			curDirFrac = 1.0f;
		}
		//NOTE: if saber spun at least 180 degrees since last damage trace, this is not reliable...!
		if ( fabs(curDirFrac) < 1.0f - MAX_SABER_SWING_INC )
		{//the saber blade spun more than 33 degrees since the last damage trace
			curDirFrac = dirInc = 1.0f/((1.0f - curDirFrac)/MAX_SABER_SWING_INC);
		}
		else
		{
			curDirFrac = 1.0f;
			dirInc = 0.0f;
		}
		qboolean hit_saber = qfalse;

		vectoangles( md1, ma1 );
		vectoangles( md2, ma2 );

		vec3_t curMD1, curMD2;//, mdDiff, dirDiff;
		//VectorSubtract( md2, md1, mdDiff );
		VectorCopy( md1, curMD2 );
		VectorCopy( baseOld, curBase2 );

		while ( 1 )
		{
			VectorCopy( curMD2, curMD1 );
			VectorCopy( curBase2, curBase1 );
			if ( curDirFrac >= 1.0f )
			{
				VectorCopy( md2, curMD2 );
				VectorCopy( baseNew, curBase2 );
			}
			else
			{
				for ( xx = 0; xx < 3; xx++ )
				{
					md2ang[xx] = LerpAngle( ma1[xx], ma2[xx], curDirFrac );
				}
				AngleVectors( md2ang, curMD2, NULL, NULL );
				//VectorMA( md1, curDirFrac, mdDiff, curMD2 );
				VectorSubtract( baseNew, baseOld, baseDiff );
				VectorMA( baseOld, curDirFrac, baseDiff, curBase2 );
			}
			// Move up the blade in intervals of stepsize
			for ( step = stepsize; step < ent->client->ps.saber[saberNum].blade[bladeNum].length && step < ent->client->ps.saber[saberNum].blade[bladeNum].lengthOld; step+=12 )
			{
				VectorMA( curBase1, step, curMD1, bladePointOld );
				VectorMA( curBase2, step, curMD2, bladePointNew );
				if ( WP_SaberDamageForTrace( ent->s.number, bladePointOld, bladePointNew, baseDamage, curMD2,
					qfalse, entPowerLevel, ent->client->ps.saber[saberNum].type, qtrue,
					saberNum, bladeNum ) )
				{
					hit_wall = qtrue;
				}

				//if hit a saber, shorten rest of traces to match
				if ( saberHitFraction < 1.0 )
				{
					//adjust muzzle endpoint
					VectorSubtract( mp2, mp1, baseDiff );
					VectorMA( mp1, saberHitFraction, baseDiff, baseNew );
					VectorMA( baseNew, ent->client->ps.saber[saberNum].blade[bladeNum].length, curMD2, endNew );
					//adjust muzzleDir...
					vec3_t curMA1, curMA2;
					vectoangles( curMD1, curMA1 );
					vectoangles( curMD2, curMA2 );
					for ( xx = 0; xx < 3; xx++ )
					{
						md2ang[xx] = LerpAngle( curMA1[xx], curMA2[xx], saberHitFraction );
					}
					AngleVectors( md2ang, curMD2, NULL, NULL );
					/*
					VectorSubtract( curMD2, curMD1, dirDiff );
					VectorMA( curMD1, saberHitFraction, dirDiff, curMD2 );
					*/
					hit_saber = qtrue;
				}
				if (hit_wall)
				{
					break;
				}
			}
			if ( hit_wall || hit_saber )
			{
				break;
			}
			if ( curDirFrac >= 1.0f )
			{
				break;
			}
			else
			{
				curDirFrac += dirInc;
				if ( curDirFrac >= 1.0f )
				{
					curDirFrac = 1.0f;
				}
			}
		}

		//do the trace at the end last
		//Special check- adjust for length of blade not being a multiple of 12
		aveLength = (ent->client->ps.saber[saberNum].blade[bladeNum].lengthOld + ent->client->ps.saber[saberNum].blade[bladeNum].length)/2;
		if ( step > aveLength )
		{//less dmg if the last interval was not stepsize
			tipDmgMod = (stepsize-(step-aveLength))/stepsize;
		}
		//NOTE: since this is the tip, we do not extrapolate the extra 16
		if ( WP_SaberDamageForTrace( ent->s.number, endOld, endNew, tipDmgMod*baseDamage, md2,
			qfalse, entPowerLevel, ent->client->ps.saber[saberNum].type, qfalse,
			saberNum, bladeNum ) )
		{
			hit_wall = qtrue;
		}
	}

	if ( (saberHitFraction < 1.0f||(sabersCrossed>=0&&sabersCrossed<=32.0f)) && (ent->client->ps.weaponstate == WEAPON_FIRING || ent->client->ps.saberInFlight || G_InCinematicSaberAnim( ent ) ) )
	{// The saber (in-hand) hit another saber, mano.
		qboolean inFlightSaberBlocked = qfalse;
		qboolean collisionResolved = qfalse;
		qboolean deflected = qfalse;

		gentity_t *hitEnt = &g_entities[saberHitEntity];
		gentity_t *hitOwner = NULL;
		int hitOwnerPowerLevel = FORCE_LEVEL_0;

		if ( hitEnt )
		{
			hitOwner = hitEnt->owner;
		}
		if ( hitOwner && hitOwner->client )
		{
			hitOwnerPowerLevel = PM_PowerLevelForSaberAnim( &hitOwner->client->ps );
			/*
			if ( entPowerLevel >= FORCE_LEVEL_3
				&& PM_SaberInSpecialAttack( ent->client->ps.torsoAnim ) )
			{//a special "unblockable" attack
				if ( hitOwner->client->NPC_class == CLASS_ALORA
					|| hitOwner->client->NPC_class == CLASS_SHADOWTROOPER
					|| (hitOwner->NPC&&(hitOwner->NPC->aiFlags&NPCAI_BOSS_CHARACTER)) )
				{//these masters can even block unblockables (stops cheap kills)
					entPowerLevel = FORCE_LEVEL_2;
				}
			}
			*/
		}

		//FIXME: check for certain anims, facing, etc, to make them lock into a sabers-locked pose
		//SEF_LOCKED

		if ( ent->client->ps.saberInFlight && saberNum == 0 &&
			ent->client->ps.saber[saberNum].blade[bladeNum].active &&
			ent->client->ps.saberEntityNum != ENTITYNUM_NONE &&
			ent->client->ps.saberEntityState != SES_RETURNING )
		{//saber was blocked, return it
			inFlightSaberBlocked = qtrue;
		}

		//FIXME: based on strength, position and angle of attack & defense, decide if:
		//	defender and attacker lock sabers
		//	*defender's parry should hold and attack bounces (or deflects, based on angle of sabers)
		//	*defender's parry is somewhat broken and both bounce (or deflect)
		//	*defender's parry is broken and they bounce while attacker's attack deflects or carries through (especially if they're dead)
		//	defender is knocked down and attack goes through

		//Check deflections and broken parries
		if ( hitOwner && hitOwner->health > 0 && ent->health > 0 //both are alive
			&& !inFlightSaberBlocked && hitOwner->client && !hitOwner->client->ps.saberInFlight && !ent->client->ps.saberInFlight//both have sabers in-hand
			&& ent->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN
			&& ent->client->ps.saberLockTime < level.time
			&& hitOwner->client->ps.saberLockTime < level.time )
		{//2 in-hand sabers hit
			//FIXME: defender should not parry or block at all if not in a saber anim... like, if in a roll or knockdown...
			if ( baseDamage )
			{//there is damage involved, not just effects
				qboolean entAttacking = qfalse;
				qboolean hitOwnerAttacking = qfalse;
				qboolean entDefending = qfalse;
				qboolean hitOwnerDefending = qfalse;
				qboolean forceLock = qfalse;
				// Telegraphed heavy attack — an explicitly forced Strong/Tavion/Desann swing,
				// identified directly via the heavyAttackMove flag it stamps on itself, so this can't
				// misfire on kicks/taunts/saberlock-wins the way PM_SaberInSpecialAttack did -- OR a
				// kata/special move (BOTH_A1/2/3_SPECIAL), which is just as committed/hard to time a
				// perfect parry against but never got its own windup telegraph. Perfectly timed defense
				// still routes into the favorable knockaway branch below (unchanged); a held-but-mistimed
				// block gets a guaranteed, softer punish (and no Force cost) instead of the ordinary roll.
				qboolean heavyAttack = (qboolean)( ( ent->client->ps.heavyAttackMove != LS_NONE
						&& ent->client->ps.saberMove == ent->client->ps.heavyAttackMove )
					|| ent->client->ps.torsoAnim == BOTH_A1_SPECIAL
					|| ent->client->ps.torsoAnim == BOTH_A2_SPECIAL
					|| ent->client->ps.torsoAnim == BOTH_A3_SPECIAL );

				if ( (ent->client->NPC_class == CLASS_KYLE && (ent->spawnflags&1) && hitOwner->s.number < MAX_CLIENTS )
					|| (hitOwner->client->NPC_class == CLASS_KYLE && (hitOwner->spawnflags&1) && ent->s.number < MAX_CLIENTS ) )
				{//Player vs. Kyle Boss == lots of saberlocks
					if ( !Q_irand( 0, 2 ) )
					{
						forceLock = qtrue;
					}
				}

				if ( PM_SaberInAttack( ent->client->ps.saberMove ) || PM_SaberInSpecialAttack( ent->client->ps.torsoAnim ) )
				{
					entAttacking = qtrue;
				}
				else if ( entPowerLevel > FORCE_LEVEL_2 )
				{//stronger styles count as attacking even if in a transition
					if ( PM_SaberInTransitionAny( ent->client->ps.saberMove ) )
					{
						entAttacking = qtrue;
					}
				}
				if ( PM_SaberInParry( ent->client->ps.saberMove )
					|| ent->client->ps.saberMove == LS_READY )
				{
					entDefending = qtrue;
				}

				if ( ent->client->ps.torsoAnim == BOTH_A1_SPECIAL
					|| ent->client->ps.torsoAnim == BOTH_A2_SPECIAL
					|| ent->client->ps.torsoAnim == BOTH_A3_SPECIAL )
				{//parry/block/break-parry bonus for single-style kata moves
					entPowerLevel++;
				}
				if ( entAttacking )
				{//add twoHanded bonus and breakParryBonus to entPowerLevel here
					//This makes staff too powerful
					if ( (ent->client->ps.saber[saberNum].saberFlags&SFL_TWO_HANDED) )
					{
						entPowerLevel++;
					}
					//FIXME: what if dualSabers && both sabers are hitting at same time?
					if ( !WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum ) )
					{
						entPowerLevel += ent->client->ps.saber[saberNum].breakParryBonus;
					}
					else if ( WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum ) )
					{
						entPowerLevel += ent->client->ps.saber[saberNum].breakParryBonus2;
					}
				}
				else if ( entDefending )
				{//add twoHanded bonus and dualSaber bonus and parryBonus to entPowerLevel here
					if ( (ent->client->ps.saber[saberNum].saberFlags&SFL_TWO_HANDED)
						|| (ent->client->ps.dualSabers && ent->client->ps.saber[1].Active()) )
					{
						entPowerLevel++;
					}
					//FIXME: what about second saber if dualSabers?
					entPowerLevel += ent->client->ps.saber[saberNum].parryBonus;
				}

				if ( PM_SaberInAttack( hitOwner->client->ps.saberMove ) || PM_SaberInSpecialAttack( hitOwner->client->ps.torsoAnim ) )
				{
					hitOwnerAttacking = qtrue;
				}
				else if ( hitOwnerPowerLevel > FORCE_LEVEL_2 )
				{//stronger styles count as attacking even if in a transition
					if ( PM_SaberInTransitionAny( hitOwner->client->ps.saberMove ) )
					{
						hitOwnerAttacking = qtrue;
					}
				}
				if ( PM_SaberInParry( hitOwner->client->ps.saberMove )
					 || hitOwner->client->ps.saberMove == LS_READY )
				{
					hitOwnerDefending = qtrue;
				}

				if ( hitOwner->client->ps.torsoAnim == BOTH_A1_SPECIAL
					|| hitOwner->client->ps.torsoAnim == BOTH_A2_SPECIAL
					|| hitOwner->client->ps.torsoAnim == BOTH_A3_SPECIAL )
				{//parry/block/break-parry bonus for single-style kata moves
					hitOwnerPowerLevel++;
				}
				if ( hitOwnerAttacking )
				{//add twoHanded bonus and breakParryBonus to entPowerLevel here
					if ( (hitOwner->client->ps.saber[0].saberFlags&SFL_TWO_HANDED) )
					{
						hitOwnerPowerLevel++;
					}
					hitOwnerPowerLevel += hitOwner->client->ps.saber[0].breakParryBonus;
					if ( hitOwner->client->ps.dualSabers && Q_irand( 0, 1 ) )
					{//FIXME: assumes both sabers are hitting at same time...?
						hitOwnerPowerLevel += 1 + hitOwner->client->ps.saber[1].breakParryBonus;
					}
				}
				else if ( hitOwnerDefending )
				{//add twoHanded bonus and dualSaber bonus and parryBonus to entPowerLevel here
					if ( (hitOwner->client->ps.saber[0].saberFlags&SFL_TWO_HANDED)
						|| (hitOwner->client->ps.dualSabers && hitOwner->client->ps.saber[1].Active()) )
					{
						hitOwnerPowerLevel++;
					}
					hitOwnerPowerLevel += hitOwner->client->ps.saber[0].parryBonus;
					if ( hitOwner->client->ps.dualSabers && Q_irand( 0, 1 ) )
					{//FIXME: assumes both sabers are defending at same time...?
						hitOwnerPowerLevel += 1 + hitOwner->client->ps.saber[1].parryBonus;
					}
				}

				if ( PM_SuperBreakLoseAnim( ent->client->ps.torsoAnim )
					|| PM_SuperBreakWinAnim( ent->client->ps.torsoAnim )
					|| PM_SuperBreakLoseAnim( hitOwner->client->ps.torsoAnim )
					|| PM_SuperBreakWinAnim( hitOwner->client->ps.torsoAnim ) )
				{//don't mess with this
					collisionResolved = qtrue;
				}
				else if ( entAttacking
					&& hitOwnerAttacking
					&& ( g_saberDeterministic->integer ? WP_DetSaberLockGate( ent, hitOwner, entPowerLevel, hitOwnerPowerLevel, forceLock ) : !Q_irand( 0, g_saberLockRandomNess->integer ) )
					&& ( g_saberDeterministic->integer || g_debugSaberLock->integer || forceLock
						|| entPowerLevel == hitOwnerPowerLevel
						|| (entPowerLevel > FORCE_LEVEL_2 && hitOwnerPowerLevel > FORCE_LEVEL_2 )
						|| (entPowerLevel < FORCE_LEVEL_3 && hitOwnerPowerLevel < FORCE_LEVEL_3 && hitOwner->client->ps.forcePowerLevel[FP_SABER_OFFENSE] > FORCE_LEVEL_2 && Q_irand( 0, 3 ))
						|| (entPowerLevel < FORCE_LEVEL_2 && hitOwnerPowerLevel < FORCE_LEVEL_3 && hitOwner->client->ps.forcePowerLevel[FP_SABER_OFFENSE] > FORCE_LEVEL_1 && Q_irand( 0, 2 ))
						|| (hitOwnerPowerLevel < FORCE_LEVEL_3 && entPowerLevel < FORCE_LEVEL_3 && ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] > FORCE_LEVEL_2 && !Q_irand( 0, 1 ))
						|| (hitOwnerPowerLevel < FORCE_LEVEL_2 && entPowerLevel < FORCE_LEVEL_3 && ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] > FORCE_LEVEL_1 && !Q_irand( 0, 1 )))
					&& WP_SabersCheckLock( ent, hitOwner ) )
				{
					collisionResolved = qtrue;
				}
				else if ( hitOwnerAttacking
					&& entDefending
					&& ( g_saberDeterministic->integer ? WP_DetSaberLockGate( ent, hitOwner, entPowerLevel, hitOwnerPowerLevel, forceLock ) : !Q_irand( 0, g_saberLockRandomNess->integer*3 ) )
					&& (g_saberDeterministic->integer || g_debugSaberLock->integer || forceLock ||
						((ent->client->ps.saberMove != LS_READY || (hitOwnerPowerLevel-ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE]) < Q_irand( -6, 0 ) )
							&& ((hitOwnerPowerLevel < FORCE_LEVEL_3 && ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] > FORCE_LEVEL_2 )||
								(hitOwnerPowerLevel < FORCE_LEVEL_2 && ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] > FORCE_LEVEL_1 )||
								(hitOwnerPowerLevel < FORCE_LEVEL_3 && ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] > FORCE_LEVEL_0 && !Q_irand( 0, (hitOwnerPowerLevel-ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE]+1)*2 ))) ))
					&& WP_SabersCheckLock( hitOwner, ent ) )
				{
					collisionResolved = qtrue;
				}
				else if ( WP_SaberRules() && entAttacking && !hitOwnerAttacking && saberHitFraction < 1.0f
					&& WP_GuardState( hitOwner, ent ) == GUARD_UP )
				{//JKEnhanced "a blade in the way": their guard was up before contact, so the swing is stopped
				 //(heavy attacks too: they cost the blocker more Force instead of going through)
					deflected = WP_SaberGuardStop( ent, hitOwner, saberNum, bladeNum, heavyAttack, entPowerLevel );
					collisionResolved = qtrue;
				}
				else if ( WP_SaberRules() && entAttacking && !hitOwnerAttacking && saberHitFraction < 1.0f )
				{//JKEnhanced saber rules: no guard up, but their blade really was in the way
				 //(WP_SaberDamageForTrace only counts an unguarded saber on true blade contact). The swing comes
				 //off it. They did nothing: no parry, no Force cost, and it doesn't count towards an answer.
					if ( entPowerLevel < FORCE_LEVEL_3 )
					{
						deflected = WP_GetSaberDeflectionAngle( ent, hitOwner );
					}
					ent->client->ps.saberEventFlags |= SEF_BLOCKED;
					WP_SaberClearDamageForEntNum( ent, hitOwner->s.number, saberNum, bladeNum );
					collisionResolved = qtrue;
					if ( g_debugBlock && g_debugBlock->integer && ( ent->s.number == 0 || hitOwner->s.number == 0 ) )
					{
						static int lastContactPrint = -1000;
						if ( level.time - lastContactPrint > 300 || level.time < lastContactPrint )
						{
							lastContactPrint = level.time;
							gi.Printf( S_COLOR_CYAN "BLADE CONTACT" S_COLOR_WHITE " %s swing ran into %s blade (no guard up, the blades met)\n",
								ent->s.number ? va( "%s's", ent->NPC_type ? ent->NPC_type : "?" ) : "your",
								hitOwner->s.number ? va( "%s's", hitOwner->NPC_type ? hitOwner->NPC_type : "?" ) : "your" );
						}
					}
				}
				else if ( entAttacking && hitOwnerDefending )
				{//I'm attacking hit, they're parrying
					qboolean activeDefense =(qboolean)(hitOwner->s.number||g_saberAutoBlocking->integer||hitOwner->client->ps.saberBlockingTime > level.time);
					if ( ( g_saberDeterministic->integer ? WP_DetSaberLockGate( ent, hitOwner, entPowerLevel, hitOwnerPowerLevel, forceLock ) : !Q_irand( 0, g_saberLockRandomNess->integer*3 ) )
						&& activeDefense
						&& (g_saberDeterministic->integer || g_debugSaberLock->integer || forceLock ||
							((hitOwner->client->ps.saberMove != LS_READY || (entPowerLevel-hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE]) < Q_irand( -6, 0 ) )
								&& ( ( entPowerLevel < FORCE_LEVEL_3 && hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE] > FORCE_LEVEL_2 )
									|| ( entPowerLevel < FORCE_LEVEL_2 && hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE] > FORCE_LEVEL_1 )
									|| ( entPowerLevel < FORCE_LEVEL_3 && hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE] > FORCE_LEVEL_0 && !Q_irand( 0, (entPowerLevel-hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE]+1)*2 )) )  ))
						&& WP_SabersCheckLock( ent, hitOwner ) )
					{
						collisionResolved = qtrue;
					}
					else if ( saberHitFraction < 1.0f )
					{//an actual collision
						// Maxed offense always powers through a clash instead of deflecting/bouncing;
						// partially-invested offense also gets a real chance to punch through now,
						// instead of every single clash against a defending target bouncing the
						// attacker's swing (which was breaking attack rhythm on nearly every hit).
						qboolean skipDeflect = (qboolean)( entPowerLevel >= FORCE_LEVEL_3 );
						if ( !skipDeflect && entPowerLevel >= FORCE_LEVEL_2 && !g_saberDeterministic->integer && !Q_irand( 0, 1 ) )
						{//50/50 chance to push through at level 2 (stock rolls only)
							skipDeflect = qtrue;
						}
						if ( !skipDeflect && activeDefense )
						{//strong attacks cannot be deflected
							//based on angle of attack & angle of defensive saber, see if I should deflect off in another dir rather than bounce back
							deflected = WP_GetSaberDeflectionAngle( ent, hitOwner );
							//just so Jedi knows that he was blocked
							ent->client->ps.saberEventFlags |= SEF_BLOCKED;
						}
						//base parry breaks on animation (saber attack level), not FP_SABER_OFFENSE
						if ( WP_InPerfectParryWindowVs( hitOwner, ent, qfalse ) || WP_NPCPerfectParry( hitOwner )//timed perfect parry always knocks the attack away (NPCs: AI_JediBrain.cpp)
							|| ( entPowerLevel < FORCE_LEVEL_3
							//&& ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] < FORCE_LEVEL_3//if you have high saber offense, you cannot have your attack knocked away, regardless of what style you're using?
							//&& hitOwner->client->ps.saberAnimLevel != FORCE_LEVEL_5
							&& activeDefense
							&& ( g_saberDeterministic->integer
								//deterministic: no knockaway just for blocking in a Strong stance; Defense 3 knocks away Fast swings
								? ( hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE] > FORCE_LEVEL_2 && entPowerLevel <= FORCE_LEVEL_1 )
								: ( hitOwnerPowerLevel > FORCE_LEVEL_2
									|| ( hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE]>FORCE_LEVEL_2&&Q_irand(0,hitOwner->client->ps.forcePowerLevel[FP_SABER_OFFENSE]) ) ) ) ) )
						{//knockaways can make fast-attacker go into a broken parry anim if the ent is using fast or med (but not Tavion)
							//make me parry
							WP_SaberParry( hitOwner, ent, saberNum, bladeNum );
							//turn the parry into a knockaway
							hitOwner->client->ps.saberBounceMove = PM_KnockawayForParry( hitOwner->client->ps.saberBlocked );
							//make them go into a broken parry
							ent->client->ps.saberBounceMove = PM_BrokenParryForAttack( ent->client->ps.saberMove );
							ent->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;
							if ( ent->s.number == 0 && g_debugBlock->integer )
							{//g_debugBlock: why the player's swing got knocked away
								gi.Printf( S_COLOR_YELLOW "your swing was KNOCKED AWAY by %s: %s\n",
									hitOwner->NPC_type ? hitOwner->NPC_type : "?",
									WP_NPCPerfectParry( hitOwner ) ? va( "they READ you (same direction x%d, flurry of %d)",
										hitOwner->NPC ? hitOwner->NPC->sameBlockDirCount : 0, ent->client->ps.saberAttackChainCount )
									: ( ( !g_saberDeterministic->integer && hitOwnerPowerLevel > FORCE_LEVEL_2 ) ? "blocking in a Strong-power stance"
									: "their Saber Defense 3 vs your Fast swing" ) );
							}
							if ( WP_InPerfectParryWindowVs( hitOwner, ent, qfalse ) || WP_NPCPerfectParry( hitOwner ) )
							{//timed perfect parry — guard hit (a parried heavy attack breaks guard), longer
							//stagger, Force, a riposte window, distinct feedback, and reset the tap cooldown
								WP_PerfectParryReward( hitOwner, ent, WP_IsHeavyAttack( ent ), qfalse );
								WP_PerfectParrySuccess( hitOwner, saberHitLocation, saberHitNormal );
							}
							if ( saberNum == 0 )
							{//FIXME: can only lose right-hand saber for now
								if ( !(ent->client->ps.saber[saberNum].saberFlags&SFL_NOT_DISARMABLE)
									&& ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_2
									//&& (ent->s.number||g_saberRealisticCombat->integer)
									&& Q_irand( 0, hitOwner->client->ps.SaberDisarmBonus( 0 ) ) > 0
									&& (hitOwner->s.number || g_saberAutoBlocking->integer || !Q_irand( 0, 2 )) )//if player defending and autoblocking is on, this is less likely to happen, so don't do the random check
								{//knocked the saber right out of his hand! (never happens to player)
									//Get a good velocity to send the saber in based on my parry move
									vec3_t	throwDir;
									if ( !PM_VelocityForBlockedMove( &hitOwner->client->ps, throwDir ) )
									{
										PM_VelocityForSaberMove( &ent->client->ps, throwDir );
									}
									WP_SaberLose( ent, throwDir );
								}
							}
							//just so Jedi knows that he was blocked
							ent->client->ps.saberEventFlags |= SEF_BLOCKED;
#ifndef FINAL_BUILD
							if ( d_saberCombat->integer )
							{
								gi.Printf( S_COLOR_RED"%s knockaway %s's attack, new move = %s, anim = %s\n", hitOwner->NPC_type, ent->NPC_type, saberMoveData[ent->client->ps.saberBounceMove].name, animTable[saberMoveData[ent->client->ps.saberBounceMove].animToUse].name );
							}
#endif
						}
						else if ( heavyAttack && activeDefense )
						{//telegraphed heavy attack (Strong/Tavion/Desann kata), blocked but not perfectly
						//timed. JKEnhanced: only a perfect parry stops it cleanly — a mistimed block is
						//broken: a long stagger (no blocking or attacking), most of the damage, Force lost
						//(NPC defenders lose half their guard instead of the stagger timer alone)
							hitOwner->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;
							hitOwner->client->ps.saberBounceMove = LS_NONE;
							TIMER_Set( hitOwner, "parryStaggered", g_heavyBlockStagger->integer );
							WP_NoteBlockFail( hitOwner, "heavy attack, block mistimed (only a perfect parry stops it)" );
							if ( hitOwner->NPC )
							{
								TIMER_Set( hitOwner, "attackDelay", g_heavyBlockStagger->integer );
								if ( hitOwner->NPC->guardMax > 0 )
								{
									G_DamageGuard( hitOwner, ent, hitOwner->NPC->guardMax / 2 );
								}
							}
							if ( g_heavyBlockForceCost->integer > 0 && hitOwner->client->ps.forcePowerMax > 0 )
							{
								WP_ForcePowerDrain( hitOwner, FP_SABER_DEFENSE, g_heavyBlockForceCost->integer );
							}
							for ( int vi = 0; vi < numVictims; vi++ )
							{
								if ( victimEntityNum[vi] == hitOwner->s.number )
								{
									totalDmg[vi] *= g_heavyBlockDamage->value;
									break;
								}
							}
							if ( g_saberDeterministic->integer
								&& !(hitOwner->client->ps.saber[0].saberFlags&SFL_NOT_DISARMABLE)
								&& hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_2 )
							{//deterministic: a telegraphed heavy attack disarms a Saber Defense 1 blocker who mistimes it
							 //(the disarm below, in the broken-parry branch, only ever reached someone not blocking)
								vec3_t	throwDir;
								PM_VelocityForSaberMove( &ent->client->ps, throwDir );
								WP_SaberLose( hitOwner, throwDir );
							}
#ifndef FINAL_BUILD
							if ( d_saberCombat->integer )
							{
								gi.Printf( S_COLOR_RED"%s's block broken by %s's heavy attack (mistimed: %d ms stagger, x%.2f damage)\n", hitOwner->NPC_type ? hitOwner->NPC_type : "player", ent->NPC_type ? ent->NPC_type : "player", g_heavyBlockStagger->integer, g_heavyBlockDamage->value );
							}
#endif
						}
						else if ( !activeDefense//they're not defending
							|| ( g_saberDeterministic->integer
								//deterministic: a held block holds; it breaks only when the blocker is out of Force
								? ( hitOwner->client->ps.forcePowerMax > 0 && hitOwner->client->ps.forcePower <= 0 && !WP_NPCForceFree( hitOwner, FP_SABER_DEFENSE ) )//4B: NPC blocks are Force-free
								: (!deflected && Q_irand( 0, Q_max(0, PM_PowerLevelForSaberAnim( &ent->client->ps, saberNum ) - hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE])/*PM_PowerLevelForSaberAnim( &hitOwner->client->ps )*/ ) > 0 ) ) )
						{//broke their parry altogether
							if ( g_saberDeterministic->integer
								//deterministic: not blocking at all, or a strong-power swing, carries on into them
								? ( !activeDefense || entPowerLevel >= FORCE_LEVEL_2 )
								: ( entPowerLevel > FORCE_LEVEL_2 || Q_irand( 0, Q_max(0, ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] - hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE]) ) ) )
							{//chance of continuing with the attack (not bouncing back)
								ent->client->ps.saberEventFlags &= ~SEF_BLOCKED;
								ent->client->ps.saberBounceMove = LS_NONE;
								brokenParry = qtrue;
							}
							if ( hitOwner->s.number == 0 && g_debugBlock->integer )
							{//g_debugBlock: which of the two ways in, with the numbers behind the roll
								static char note[160];
								Com_sprintf( note, sizeof( note ), "their swing broke your parry: %s (swing power %d vs your Saber Defense %d)%s",
									!activeDefense ? "you weren't actively blocking" : ( g_saberDeterministic->integer ? "out of Force, your block broke" : "power roll lost" ),
									PM_PowerLevelForSaberAnim( &ent->client->ps, saberNum ), hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE],
									brokenParry ? ", and the swing carried on into you" : "" );
								WP_NoteBlockFail( hitOwner, note );
							}
							//do some time-consuming saber-knocked-aside broken parry anim
							hitOwner->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;
							hitOwner->client->ps.saberBounceMove = LS_NONE;
							//FIXME: for now, you always disarm the right-hand saber
							const qboolean detHeavy = (qboolean)( g_saberDeterministic->integer && WP_IsHeavyAttack( ent ) );
							if ( !(hitOwner->client->ps.saber[0].saberFlags&SFL_NOT_DISARMABLE)
								&& hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_2
								//&& (ent->s.number||g_saberRealisticCombat->integer)
								&& ( g_saberDeterministic->integer ? detHeavy//deterministic: only a telegraphed heavy attack disarms
									: Q_irand( 0, 2-ent->client->ps.SaberDisarmBonus( bladeNum ) ) <= 0 ) )
							{//knocked the saber right out of his hand!
								//get the right velocity for my attack direction
								vec3_t	throwDir;
								PM_VelocityForSaberMove( &ent->client->ps, throwDir );
								WP_SaberLose( hitOwner, throwDir );
								if ( g_saberDeterministic->integer ? detHeavy
									: ( (ent->client->ps.saberAnimLevel == SS_STRONG && !Q_irand(0,3) )
									|| ( ent->client->ps.saberAnimLevel==SS_DESANN&&!Q_irand(0,1) ) ) )
								{// a strong attack
									if ( WP_BrokenParryKnockDown( hitOwner ) )
									{
										hitOwner->client->ps.saberBlocked = BLOCKED_NONE;
										hitOwner->client->ps.saberBounceMove = LS_NONE;
									}
								}
							}
							else
							{
								if ( g_saberDeterministic->integer ? detHeavy//deterministic: only a telegraphed heavy attack knocks down
									: ( (ent->client->ps.saberAnimLevel == SS_STRONG && !Q_irand(0,5) )
									|| ( ent->client->ps.saberAnimLevel==SS_DESANN&&!Q_irand(0,3) ) ) )
								{// a strong attack
									if ( WP_BrokenParryKnockDown( hitOwner ) )
									{
										hitOwner->client->ps.saberBlocked = BLOCKED_NONE;
										hitOwner->client->ps.saberBounceMove = LS_NONE;
									}
								}
							}
#ifndef FINAL_BUILD
							if ( d_saberCombat->integer )
							{
								if ( ent->client->ps.saberEventFlags&SEF_BLOCKED )
								{
									gi.Printf( S_COLOR_RED"%s parry broken (bounce/deflect)!\n", hitOwner->targetname );
								}
								else
								{
									gi.Printf( S_COLOR_RED"%s parry broken (follow-through)!\n", hitOwner->targetname );
								}
							}
#endif
						}
						else
						{//just a parry, possibly the hitOwner can knockaway the ent
							WP_SaberParry( hitOwner, ent, saberNum, bladeNum );
							if ( PM_SaberInBounce( ent->client->ps.saberMove ) //FIXME: saberMove not set until pmove!
								&& activeDefense
								&& hitOwner->client->ps.saberAnimLevel != SS_FAST //&& hitOwner->client->ps.saberAnimLevel != FORCE_LEVEL_5
								&& hitOwner->client->ps.forcePowerLevel[FP_SABER_DEFENSE] > FORCE_LEVEL_2 )
							{//attacker bounced off, and defender has ability to do knockaways, so do one unless we're using fast attacks
								//turn the parry into a knockaway
								hitOwner->client->ps.saberBounceMove = PM_KnockawayForParry( hitOwner->client->ps.saberBlocked );
							}
							else if ( (ent->client->ps.saberAnimLevel == SS_STRONG && !Q_irand(0,6) )
									|| ( ent->client->ps.saberAnimLevel==SS_DESANN && !Q_irand(0,3) ) )
							{// a strong attack can sometimes do a knockdown
								//HMM... maybe only if they're moving backwards?
								if ( WP_BrokenParryKnockDown( hitOwner ) )
								{
									hitOwner->client->ps.saberBlocked = BLOCKED_NONE;
									hitOwner->client->ps.saberBounceMove = LS_NONE;
								}
							}
						}
						// Guard-gated damage scaling -- explicitly restricted to actual saber-wielders
						// with a guard pool (never to e.g. blaster troopers, whose stale/default
						// saberMove can otherwise look like they're "defending" -- this is exactly
						// what caused the stormtrooper lightsaber-immunity regression previously).
						if ( hitOwner->NPC && hitOwner->NPC->guardMax > 0 && hitOwner->client->ps.weapon == WP_SABER )
						{
							if ( hitOwner->NPC->guardBreakTime > level.time )
							{//guard broken: no resistance (the old x1.5 heavy follow-up bonus is gone; g_guardBreakDamage)
							}
							else if ( hitOwner->NPC->guard > 0 )
							{//guard intact -- the more of it remaining, the harder they are to actually hurt
								float guardFrac = (float)hitOwner->NPC->guard / (float)hitOwner->NPC->guardMax;
								float resist = 1.0f - (guardFrac * 0.6f);//up to 60% reduction at full guard
								for ( int vi = 0; vi < numVictims; vi++ )
								{
									if ( victimEntityNum[vi] == hitOwner->s.number )
									{
										totalDmg[vi] = (int)(totalDmg[vi] * resist);
										break;
									}
								}
							}
						}
						collisionResolved = qtrue;
					}
				}
				/*
				else if ( entDefending && hitOwnerAttacking )
				{//I'm parrying, they're attacking
					if ( hitOwnerPowerLevel < FORCE_LEVEL_3 )
					{//strong attacks cannot be deflected
						//based on angle of attack & angle of defensive saber, see if I should deflect off in another dir rather than bounce back
						deflected = WP_GetSaberDeflectionAngle( hitOwner, ent );
						//just so Jedi knows that he was blocked
						hitOwner->client->ps.saberEventFlags |= SEF_BLOCKED;
					}
					//FIXME: base parry breaks on animation (saber attack level), not FP_SABER_OFFENSE
					if ( hitOwnerPowerLevel > FORCE_LEVEL_2 || (!deflected && Q_irand( 0, PM_PowerLevelForSaberAnim( &hitOwner->client->ps ) - ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] ) > 0 ) )
					{//broke my parry altogether
						if ( hitOwnerPowerLevel > FORCE_LEVEL_2 || Q_irand( 0, hitOwner->client->ps.forcePowerLevel[FP_SABER_OFFENSE] - ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] ) )
						{//chance of continuing with the attack (not bouncing back)
							hitOwner->client->ps.saberEventFlags &= ~SEF_BLOCKED;
							hitOwner->client->ps.saberBounceMove = LS_NONE;
						}
						//do some time-consuming saber-knocked-aside broken parry anim
						ent->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;
#ifndef FINAL_BUILD
						if ( d_saberCombat->integer )
						{
							if ( hitOwner->client->ps.saberEventFlags&SEF_BLOCKED )
							{
								gi.Printf( S_COLOR_RED"%s parry broken (bounce/deflect)!\n", ent->targetname );
							}
							else
							{
								gi.Printf( S_COLOR_RED"%s parry broken (follow-through)!\n", ent->targetname );
							}
						}
#endif
					}
					else
					{
						WP_SaberParry( ent, hitOwner, saberNum, bladeNum );
					}
					collisionResolved = qtrue;
				}
				*/
				else
				{//some other kind of in-hand saber collision
				}
			}
		}
		else
		{//some kind of in-flight collision
		}

		if ( saberHitFraction < 1.0f )
		{
			if ( !collisionResolved && baseDamage )
			{//some other kind of in-hand saber collision
				//handle my reaction
				if ( !ent->client->ps.saberInFlight
					&& ent->client->ps.saberLockTime < level.time )
				{//my saber is in hand
					if ( ent->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
					{
						if ( PM_SaberInAttack( ent->client->ps.saberMove ) || PM_SaberInSpecialAttack( ent->client->ps.torsoAnim ) ||
							(entPowerLevel > FORCE_LEVEL_2&&!PM_SaberInIdle(ent->client->ps.saberMove)&&!PM_SaberInParry(ent->client->ps.saberMove)&&!PM_SaberInReflect(ent->client->ps.saberMove)) )
						{//in the middle of attacking
							if ( entPowerLevel < FORCE_LEVEL_3 && hitOwner->health > 0 )
							{//don't deflect/bounce in strong attack or when enemy is dead
								WP_GetSaberDeflectionAngle( ent, hitOwner );
								ent->client->ps.saberEventFlags |= SEF_BLOCKED;
								//since it was blocked/deflected, take away any damage done
								//FIXME: what if the damage was done before the parry?
								WP_SaberClearDamageForEntNum( ent, hitOwner->s.number, saberNum, bladeNum );
							}
						}
						else
						{//saber collided when not attacking, parry it
							//since it was blocked/deflected, take away any damage done
							//FIXME: what if the damage was done before the parry?
							WP_SaberClearDamageForEntNum( ent, hitOwner->s.number, saberNum, bladeNum );
							/*
							if ( ent->s.number || g_saberAutoBlocking->integer || ent->client->ps.saberBlockingTime > level.time )
							{//either an NPC or a player who has blocking
								if ( !PM_SaberInTransitionAny( ent->client->ps.saberMove ) && !PM_SaberInBounce( ent->client->ps.saberMove ) )
								{//I'm not attacking, in transition or in a bounce, so play a parry
									//just so Jedi knows that he parried something
									WP_SaberBlockNonRandom( ent, saberHitLocation, qfalse );
								}
								ent->client->ps.saberEventFlags |= SEF_PARRIED;
							}
							*/
						}
					}
					else
					{
						//since it was blocked/deflected, take away any damage done
						//FIXME: what if the damage was done before the parry?
						WP_SaberClearDamageForEntNum( ent, hitOwner->s.number, saberNum, bladeNum );
					}
				}
				else
				{//nothing happens to *me* when my inFlight saber hits something
				}
				//handle their reaction
				if ( hitOwner
					&& hitOwner->health > 0
					&& hitOwner->client
					&& !hitOwner->client->ps.saberInFlight
					&& hitOwner->client->ps.saberLockTime < level.time )
				{//their saber is in hand
					if ( PM_SaberInAttack( hitOwner->client->ps.saberMove ) || PM_SaberInSpecialAttack( hitOwner->client->ps.torsoAnim ) ||
						(hitOwner->client->ps.saberAnimLevel > SS_MEDIUM&&!PM_SaberInIdle(hitOwner->client->ps.saberMove)&&!PM_SaberInParry(hitOwner->client->ps.saberMove)&&!PM_SaberInReflect(hitOwner->client->ps.saberMove)) )
					{//in the middle of attacking
						/*
						if ( hitOwner->client->ps.saberAnimLevel < SS_STRONG )
						{//don't deflect/bounce in strong attack
							WP_GetSaberDeflectionAngle( hitOwner, ent );
							hitOwner->client->ps.saberEventFlags |= SEF_BLOCKED;
						}
						*/
					}
					else
					{//saber collided when not attacking, parry it
						if ( !PM_SaberInBrokenParry( hitOwner->client->ps.saberMove ) )
						{//not currently in a broken parry
							if ( ent->client->ps.saberInFlight && ( WP_InPerfectParryWindowVs( hitOwner, ent, qfalse ) || WP_NPCPerfectParry( hitOwner ) ) )
							{//a perfectly-timed block against a thrown saber disarms the thrower
								WP_PerfectParryReward( hitOwner, ent, qfalse, qtrue );	//Force + riposte; no stagger (they're out of reach)
								WP_PerfectParrySuccess( hitOwner, saberHitLocation, saberHitNormal );
								vec3_t throwDir;
								if ( !PM_VelocityForBlockedMove( &hitOwner->client->ps, throwDir ) )
								{
									PM_VelocityForSaberMove( &ent->client->ps, throwDir );
								}
								WP_SaberLose( ent, throwDir );
							}
							else if ( !WP_SaberParry( hitOwner, ent, saberNum, bladeNum ) )
							{//FIXME: hitOwner can't parry, do some time-consuming saber-knocked-aside broken parry anim?
								//hitOwner->client->ps.saberBlocked = BLOCKED_PARRY_BROKEN;
							}
						}
					}
				}
				else
				{//nothing happens to *hitOwner* when their inFlight saber hits something
				}
			}

			//collision must have been handled by now
			//Set the blocked attack bounce value in saberBlocked so we actually play our saberBounceMove anim
			if ( ent->client->ps.saberEventFlags & SEF_BLOCKED )
			{
				if ( ent->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
				{
					ent->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
				}
			}
			/*
			if ( hitOwner && hitOwner->client->ps.saberEventFlags & SEF_BLOCKED )
			{
				hitOwner->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
			}
			*/
		}

		if ( saberHitFraction < 1.0f || collisionResolved )
		{//either actually hit or locked
			if ( ent->client->ps.saberLockTime < level.time )
			{
				if ( inFlightSaberBlocked )
				{//FIXME: never hear this sound
					WP_SaberBounceSound( ent, hitOwner, &g_entities[ent->client->ps.saberEntityNum], 0, 0, qfalse );
				}
				else
				{
					if ( deflected )
					{
						WP_SaberBounceSound( ent, hitOwner, NULL, saberNum, bladeNum, qtrue );
					}
					else
					{
						WP_SaberBlockSound( ent, hitOwner, saberNum, bladeNum );
					}
				}
				if ( !g_saberNoEffects )
				{
					WP_SaberBlockEffect( ent, saberNum, bladeNum, saberHitLocation, saberHitNormal, qfalse );
				}
			}
			// Set the little screen flash - only when an attack is blocked
			if ( !g_noClashFlare )
			{
				g_saberFlashTime = level.time-50;
				VectorCopy( saberHitLocation, g_saberFlashPos );
			}
		}

		if ( saberHitFraction < 1.0f )
		{
			if ( inFlightSaberBlocked )
			{//we threw a saber and it was blocked, do any effects, etc.
				G_DamageGuard( hitOwner, ent, 8 );	//blocking a thrown saber costs composure
				int	knockAway = 5;
				if ( hitEnt
					&& hitOwner
					&& hitOwner->client
					&& (PM_SaberInAttack( hitOwner->client->ps.saberMove ) || PM_SaberInSpecialAttack( hitOwner->client->ps.torsoAnim ) || PM_SpinningSaberAnim( hitOwner->client->ps.torsoAnim )) )
				{//if hit someone who was in an attack or spin anim, more likely to have in-flight saber knocked away
					if ( hitOwnerPowerLevel > FORCE_LEVEL_2 )
					{//strong attacks almost always knock it aside!
						knockAway = 1;
					}
					else
					{//33% chance
						knockAway = 2;
					}
					knockAway -= hitOwner->client->ps.SaberDisarmBonus( 0 );
				}
				if ( Q_irand( 0, knockAway ) <= 0 || //random
						( hitOwner
							&& hitOwner->client
							&& hitOwner->NPC
							&& (hitOwner->NPC->aiFlags&NPCAI_BOSS_CHARACTER)
						) //or if blocked by a Boss character FIXME: or base on defense level?
					)//FIXME: player should not auto-block a flying saber, let him override the parry with an attack to knock the saber from the air, rather than this random chance
				{//knock it aside and turn it off
					if ( !g_saberNoEffects )
					{
						if ( !WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
							&& ent->client->ps.saber[saberNum].hitOtherEffect )
						{
							G_PlayEffect( ent->client->ps.saber[saberNum].hitOtherEffect, saberHitLocation, saberHitNormal );
						}
						else if ( WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
							&& ent->client->ps.saber[saberNum].hitOtherEffect2 )
						{
							G_PlayEffect( ent->client->ps.saber[saberNum].hitOtherEffect2, saberHitLocation, saberHitNormal );
						}
						else
						{
							G_PlayEffect( "saber/saber_cut", saberHitLocation, saberHitNormal );
						}
					}
					if ( hitEnt )
					{
						vec3_t newDir;

						VectorSubtract( g_entities[ent->client->ps.saberEntityNum].currentOrigin, hitEnt->currentOrigin, newDir );
						VectorNormalize( newDir );
						G_ReflectMissile( ent, &g_entities[ent->client->ps.saberEntityNum], newDir );
					}
					Jedi_PlayDeflectSound( hitOwner );
					WP_SaberDrop( ent, &g_entities[ent->client->ps.saberEntityNum] );
				}
				else
				{
					if ( !Q_irand( 0, 2 ) && hitEnt )
					{
						vec3_t newDir;
						VectorSubtract( g_entities[ent->client->ps.saberEntityNum].currentOrigin, hitEnt->currentOrigin, newDir );
						VectorNormalize( newDir );
						G_ReflectMissile( ent, &g_entities[ent->client->ps.saberEntityNum], newDir );
					}
					WP_SaberReturn( ent, &g_entities[ent->client->ps.saberEntityNum] );
				}
			}
		}
	}

	if ( ent->client->ps.saberLockTime > level.time )
	{
		if ( ent->s.number < ent->client->ps.saberLockEnemy
			&& !Q_irand( 0, 3 ) )
		{//need to make some kind of effect
			vec3_t	hitNorm = {0,0,1};
			if ( WP_SabersIntersection( ent, &g_entities[ent->client->ps.saberLockEnemy], g_saberFlashPos ) )
			{
				if ( Q_irand( 0, 10 ) )
				{
					if ( !g_saberNoEffects )
					{
						WP_SaberBlockEffect( ent, saberNum, bladeNum, g_saberFlashPos, hitNorm, qfalse );
					}
				}
				else
				{
					if ( !g_noClashFlare )
					{
						g_saberFlashTime = level.time-50;
					}
					if ( !g_saberNoEffects )
					{
						WP_SaberBlockEffect( ent, saberNum, bladeNum, g_saberFlashPos, hitNorm, qtrue );
					}
				}
				WP_SaberBlockSound( ent, &g_entities[ent->client->ps.saberLockEnemy], 0, 0 );
			}
		}
	}
	else
	{
		if ( hit_wall
			&& (ent->client->ps.saber[saberNum].saberFlags&SFL_BOUNCE_ON_WALLS)
			&& (PM_SaberInAttackPure( ent->client->ps.saberMove ) //only in a normal attack anim
				|| ent->client->ps.saberMove == LS_A_JUMP_T__B_ ) //or in the strong jump-fwd-attack "death from above" move
			)
		{//bounce off walls
			//do anim
			ent->client->ps.saberBlocked = BLOCKED_ATK_BOUNCE;
			ent->client->ps.saberBounceMove = LS_D1_BR+(saberMoveData[ent->client->ps.saberMove].startQuad-Q_BR);
			//do bounce sound & force feedback
			WP_SaberBounceOnWallSound( ent, saberNum, bladeNum );
			//do hit effect
			if ( !g_saberNoEffects )
			{
				if ( !WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
					&& ent->client->ps.saber[saberNum].hitOtherEffect )
				{
					G_PlayEffect( ent->client->ps.saber[saberNum].hitOtherEffect, saberHitLocation, saberHitNormal );
				}
				else if ( WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum )
					&& ent->client->ps.saber[saberNum].hitOtherEffect2 )
				{
					G_PlayEffect( ent->client->ps.saber[saberNum].hitOtherEffect2, saberHitLocation, saberHitNormal );
				}
				else
				{
					G_PlayEffect( "saber/saber_cut", saberHitLocation, saberHitNormal );
				}
			}
			//do radius damage/knockback, if any
			if ( !WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[saberNum], bladeNum ) )
			{
				WP_SaberRadiusDamage( ent, saberHitLocation, ent->client->ps.saber[saberNum].splashRadius, ent->client->ps.saber[saberNum].splashDamage, ent->client->ps.saber[saberNum].splashKnockback );
			}
			else
			{
				WP_SaberRadiusDamage( ent, saberHitLocation, ent->client->ps.saber[saberNum].splashRadius2, ent->client->ps.saber[saberNum].splashDamage2, ent->client->ps.saber[saberNum].splashKnockback2 );
			}
		}
	}

	if ( WP_SaberApplyDamage( ent, baseDamage, baseDFlags, brokenParry, saberNum, bladeNum, (qboolean)(saberNum==0&&ent->client->ps.saberInFlight) ) )
	{//actually did damage to something
#ifndef FINAL_BUILD
		if ( d_saberCombat->integer )
		{
			gi.Printf( "base damage was %4.2f\n", baseDamage );
		}
#endif
		WP_SaberHitSound( ent, saberNum, bladeNum );
	}

	if ( hit_wall )
	{
		//just so Jedi knows that he hit a wall
		ent->client->ps.saberEventFlags |= SEF_HITWALL;
		if ( ent->s.number == 0 )
		{
			AddSoundEvent( ent, ent->currentOrigin, 128, AEL_DISCOVERED, qfalse, qtrue );//FIXME: is this impact on ground or not?
			AddSightEvent( ent, ent->currentOrigin, 256, AEL_DISCOVERED, 50 );
		}
	}
}

void WP_SabersDamageTrace( gentity_t *ent, qboolean noEffects )
{
	if ( !ent->client )
	{
		return;
	}
	if ( PM_SuperBreakLoseAnim( ent->client->ps.torsoAnim ) )
	{
		return;
	}
	// Saber 1.
	g_saberNoEffects = noEffects;
	for ( int i = 0; i < ent->client->ps.saber[0].numBlades; i++ )
	{
		// If the Blade is not active and the length is 0, don't trace it, try the next blade...
		if ( !ent->client->ps.saber[0].blade[i].active && ent->client->ps.saber[0].blade[i].length == 0 )
			continue;

		if ( i != 0 )
		{//not first blade
			if ( ent->client->ps.saber[0].type == SABER_BROAD ||
				ent->client->ps.saber[0].type == SABER_SAI ||
				ent->client->ps.saber[0].type == SABER_CLAW )
			{
				g_saberNoEffects = qtrue;
			}
		}
		g_noClashFlare = qfalse;
		if ( (!WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[0], i ) && (ent->client->ps.saber[0].saberFlags2&SFL2_NO_CLASH_FLARE) )
			|| ( WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[0], i ) && (ent->client->ps.saber[0].saberFlags2&SFL2_NO_CLASH_FLARE2) ) )
		{
			g_noClashFlare = qtrue;
		}
		WP_SaberDamageTrace( ent, 0, i );
	}
	// Saber 2.
	g_saberNoEffects = noEffects;
	if ( ent->client->ps.dualSabers )
	{
		for ( int i = 0; i < ent->client->ps.saber[1].numBlades; i++ )
		{
			// If the Blade is not active and the length is 0, don't trace it, try the next blade...
			if ( !ent->client->ps.saber[1].blade[i].active && ent->client->ps.saber[1].blade[i].length == 0 )
				continue;

			if ( i != 0 )
			{//not first blade
				if ( ent->client->ps.saber[1].type == SABER_BROAD ||
					ent->client->ps.saber[1].type == SABER_SAI ||
					ent->client->ps.saber[1].type == SABER_CLAW )
				{
					g_saberNoEffects = qtrue;
				}
			}
			g_noClashFlare = qfalse;
			if ( (!WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[1], i ) && (ent->client->ps.saber[1].saberFlags2&SFL2_NO_CLASH_FLARE) )
				|| ( WP_SaberBladeUseSecondBladeStyle( &ent->client->ps.saber[1], i ) && (ent->client->ps.saber[1].saberFlags2&SFL2_NO_CLASH_FLARE2) ) )
			{
				g_noClashFlare = qtrue;
			}
			WP_SaberDamageTrace( ent, 1, i );
		}
	}
	g_saberNoEffects = qfalse;
	g_noClashFlare = qfalse;
}

//SABER THROWING============================================================================
//SABER THROWING============================================================================
//SABER THROWING============================================================================
//SABER THROWING============================================================================
//SABER THROWING============================================================================
//SABER THROWING============================================================================

/*
================
WP_SaberImpact

================
*/
void WP_SaberImpact( gentity_t *owner, gentity_t *saber, trace_t *trace )
{
	gentity_t		*other;

	other = &g_entities[trace->entityNum];

	if ( other->takedamage && (other->svFlags&SVF_BBRUSH) )
	{//a breakable brush?  break it!
		if ( (other->spawnflags&1)//INVINCIBLE
			||(other->flags&FL_DMG_BY_HEAVY_WEAP_ONLY) )//HEAVY weapon damage only
		{//can't actually break it
			//no hit effect (besides regular client-side one)
		}
		else if ( other->NPC_targetname &&
			(!owner||!owner->targetname||Q_stricmp(owner->targetname,other->NPC_targetname)) )
		{//only breakable by an entity who is not the attacker
			//no hit effect (besides regular client-side one)
		}
		else
		{
			vec3_t dir;
			VectorCopy( saber->s.pos.trDelta, dir );
			VectorNormalize( dir );

			int dmg = other->health*2;
			if ( other->health > 50 && dmg > 20 && !(other->svFlags&SVF_GLASS_BRUSH) )
			{
				dmg = 20;
			}
			G_Damage( other, saber, owner, dir, trace->endpos, dmg, 0, MOD_SABER );
			if ( owner
				&& owner->client
				&& owner->client->ps.saber[0].hitOtherEffect )
			{
				G_PlayEffect( owner->client->ps.saber[0].hitOtherEffect, trace->endpos, dir );
			}
			else
			{
				G_PlayEffect( "saber/saber_cut", trace->endpos, dir );
			}
			if ( owner->s.number == 0 )
			{
				AddSoundEvent( owner, trace->endpos, 256, AEL_DISCOVERED );
				AddSightEvent( owner, trace->endpos, 512, AEL_DISCOVERED, 50 );
			}
			return;
		}
	}

	if ( saber->s.pos.trType == TR_LINEAR )
	{
		//hit a wall? send it back
		WP_SaberReturn( saber->owner, saber );
	}

	if ( other && !other->client && (other->contents&CONTENTS_LIGHTSABER) )//&& other->s.weapon == WP_SABER )
	{//2 in-flight sabers collided!
		//Big flash
		//FIXME: bigger effect/sound?
		//FIXME: STILL DOESNT WORK!!!
		WP_SaberBlockSound( saber->owner, NULL, 0, 0 );
		//G_Sound( saber, G_SoundIndex( va( "sound/weapons/saber/saberblock%d.wav", Q_irand(1, 9) ) ) );
		WP_SaberBlockEffect( saber->owner, 0, 0, trace->endpos, NULL, qfalse);
		qboolean noFlare = qfalse;
		if ( saber->owner
			&& saber->owner->client
			&& (saber->owner->client->ps.saber[0].saberFlags2&SFL2_NO_CLASH_FLARE) )
		{
			noFlare = qtrue;
		}
		if ( !noFlare )
		{
			g_saberFlashTime = level.time-50;
			VectorCopy( trace->endpos, g_saberFlashPos );
		}
	}

	if ( owner && owner->s.number == 0 && owner->client )
	{
		//Add the event
		if ( owner->client->ps.SaberLength() > 0 )
		{//saber is on, very suspicious
			if ( (!owner->client->ps.saberInFlight && owner->client->ps.groundEntityNum == ENTITYNUM_WORLD)//holding saber and on ground
				|| saber->s.pos.trType == TR_STATIONARY )//saber out there somewhere and on ground
			{//an on-ground alert
				AddSoundEvent( owner, saber->currentOrigin, 128, AEL_DISCOVERED, qfalse, qtrue );
			}
			else
			{//an in-air alert
				AddSoundEvent( owner, saber->currentOrigin, 128, AEL_DISCOVERED );
			}
			AddSightEvent( owner, saber->currentOrigin, 256, AEL_DISCOVERED, 50 );
		}
		else
		{//saber is off, not as suspicious
			AddSoundEvent( owner, saber->currentOrigin, 128, AEL_SUSPICIOUS );
			AddSightEvent( owner, saber->currentOrigin, 256, AEL_SUSPICIOUS );
		}
	}

	// check for bounce
	if ( !other->takedamage && ( saber->s.eFlags & ( EF_BOUNCE | EF_BOUNCE_HALF ) ) )
	{
		// Check to see if there is a bounce count
		if ( saber->bounceCount ) {
			// decrement number of bounces and then see if it should be done bouncing
			if ( --saber->bounceCount <= 0 ) {
				// He (or she) will bounce no more (after this current bounce, that is).
				saber->s.eFlags &= ~( EF_BOUNCE | EF_BOUNCE_HALF );
				if ( saber->s.pos.trType == TR_LINEAR && owner && owner->client && owner->client->ps.saberEntityState == SES_RETURNING )
				{
					WP_SaberDrop( saber->owner, saber );
				}
				return;
			}
			else
			{//bounced and still have bounces left
				if ( saber->s.pos.trType == TR_LINEAR && owner && owner->client && owner->client->ps.saberEntityState == SES_RETURNING )
				{//under telekinetic control
					if ( !gi.inPVS( saber->currentOrigin, owner->client->renderInfo.handRPoint ) )
					{//not in the PVS of my master
						saber->bounceCount -= 25;
					}
				}
			}
		}

		if ( saber->s.pos.trType == TR_LINEAR && owner && owner->client && owner->client->ps.saberEntityState == SES_RETURNING )
		{
			//don't home for a few frames so we can get around this thing
			trace_t	bounceTr;
			vec3_t	end;
			float owner_dist = Distance( owner->client->renderInfo.handRPoint, saber->currentOrigin );

			VectorMA( saber->currentOrigin, 10, trace->plane.normal, end );
			gi.trace( &bounceTr, saber->currentOrigin, saber->mins, saber->maxs, end, owner->s.number, saber->clipmask, (EG2_Collision)0, 0 );
			VectorCopy( bounceTr.endpos, saber->currentOrigin );
			if ( owner_dist > 0 )
			{
				if ( owner_dist > 50 )
				{
					owner->client->ps.saberEntityDist = owner_dist-50;
				}
				else
				{
					owner->client->ps.saberEntityDist = 0;
				}
			}
			return;
		}

		G_BounceMissile( saber, trace );

		if ( saber->s.pos.trType == TR_GRAVITY )
		{//bounced
			//play a bounce sound
			WP_SaberFallSound( owner, saber );
			//change rotation
			VectorCopy( saber->currentAngles, saber->s.apos.trBase );
			saber->s.apos.trType = TR_LINEAR;
			saber->s.apos.trTime = level.time;
			VectorSet( saber->s.apos.trDelta, Q_irand( -300, 300 ), Q_irand( -300, 300 ), Q_irand( -300, 300 ) );
		}
		//see if we stopped
		else if ( saber->s.pos.trType == TR_STATIONARY )
		{//stopped
			//play a bounce sound
			WP_SaberFallSound( owner, saber );
			//stop rotation
			VectorClear( saber->s.apos.trDelta );
			pitch_roll_for_slope( saber, trace->plane.normal, saber->currentAngles );
			saber->currentAngles[0] += SABER_PITCH_HACK;
			VectorCopy( saber->currentAngles, saber->s.apos.trBase );
			//remember when it fell so it can return automagically
			saber->aimDebounceTime = level.time;
		}
	}
	else if ( other->client && other->health > 0
		&& ( (other->NPC && (other->NPC->aiFlags&NPCAI_BOSS_CHARACTER))
			//|| other->client->NPC_class == CLASS_ALORA
			|| other->client->NPC_class == CLASS_BOBAFETT
			|| ( other->client->ps.powerups[PW_GALAK_SHIELD] > 0 ) ) )
	{//Luke, Desann and Tavion slap thrown sabers aside
		WP_SaberDrop( owner, saber );
		WP_SaberBlockSound( owner, other, 0, 0 );
		//G_Sound( saber, G_SoundIndex( va( "sound/weapons/saber/saberblock%d.wav", Q_irand(1, 9) ) ) );
		WP_SaberBlockEffect( owner, 0, 0, trace->endpos, NULL, qfalse );
		qboolean noFlare = qfalse;
		if ( owner
			&& owner->client
			&& (owner->client->ps.saber[0].saberFlags2&SFL2_NO_CLASH_FLARE) )
		{
			noFlare = qtrue;
		}
		if ( !noFlare )
		{
			g_saberFlashTime = level.time-50;
			VectorCopy( trace->endpos, g_saberFlashPos );
		}
		//FIXME: make Luke/Desann/Tavion play an attack anim or some other special anim when this happens
		Jedi_PlayDeflectSound( other );
	}
}

extern float G_PointDistFromLineSegment( const vec3_t start, const vec3_t end, const vec3_t from );
void WP_SaberInFlightReflectCheck( gentity_t *self, usercmd_t *ucmd  )
{
	gentity_t	*ent;
	gentity_t	*entityList[MAX_GENTITIES];
	gentity_t	*missile_list[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	int			i, e, numSabers;
	int			ent_count = 0;
	int			radius = 180;
	vec3_t		center;
	vec3_t		tip;
	vec3_t		up = {0,0,1};
	qboolean	willHit = qfalse;

	if ( self->NPC && (self->NPC->scriptFlags&SCF_IGNORE_ALERTS) )
	{//don't react to things flying at me...
		return;
	}
	//sanity checks: make sure we actually have a saberent
	if ( self->client->ps.weapon != WP_SABER )
	{
		return;
	}
	if ( !self->client->ps.saberInFlight )
	{
		return;
	}
	if ( !self->client->ps.SaberLength() )
	{
		return;
	}
	if ( self->client->ps.saberEntityNum == ENTITYNUM_NONE )
	{
		return;
	}
	gentity_t *saberent = &g_entities[self->client->ps.saberEntityNum];
	if ( !saberent )
	{
		return;
	}
	//okay, enough damn sanity checks

	VectorCopy( saberent->currentOrigin, center );

	for ( i = 0 ; i < 3 ; i++ )
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}

	numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	//FIXME: check visibility?
	for ( e = 0 ; e < numListedEntities ; e++ )
	{
		ent = entityList[ e ];

		if (ent == self)
			continue;
		if (ent->owner == self)
			continue;
		if ( ent->owner && ent->owner->client && OnSameTeam( self, ent->owner ) )
			continue;
		if ( !(ent->inuse) )
			continue;
		if ( ent->s.eType != ET_MISSILE )
		{
			if ( ent->client || ent->s.weapon != WP_SABER )
			{//FIXME: wake up bad guys?
				continue;
			}
			if ( ent->s.eFlags & EF_NODRAW )
			{
				continue;
			}
			if ( Q_stricmp( "lightsaber", ent->classname ) != 0 )
			{//not a lightsaber
				continue;
			}
		}
		else
		{//FIXME: make exploding missiles explode?
			if ( ent->s.pos.trType == TR_STATIONARY )
			{//nothing you can do with a stationary missile
				continue;
			}
			if ( ent->splashDamage || ent->splashRadius )
			{//can't deflect exploding missiles
				if ( DistanceSquared( ent->currentOrigin, center ) < 256 )//16 squared
				{
					G_MissileImpacted( ent, saberent, ent->currentOrigin, up );
				}
				continue;
			}
		}

		//don't deflect it if it's not within 16 units of the blade
		//do this for all blades
		willHit = qfalse;
		numSabers = 1;
		if ( self->client->ps.dualSabers )
		{
			numSabers = 2;
		}
		for ( int saberNum = 0; saberNum < numSabers; saberNum++ )
		{
			for ( int bladeNum = 0; bladeNum < self->client->ps.saber[saberNum].numBlades; bladeNum++ )
			{
				VectorMA( self->client->ps.saber[saberNum].blade[bladeNum].muzzlePoint, self->client->ps.saber[saberNum].blade[bladeNum].length, self->client->ps.saber[saberNum].blade[bladeNum].muzzleDir, tip );

				if( G_PointDistFromLineSegment( self->client->ps.saber[saberNum].blade[bladeNum].muzzlePoint, tip, ent->currentOrigin ) <= 32 )
				{
					willHit = qtrue;
					break;
				}
			}
			if ( willHit )
			{
				break;
			}
		}
		if ( !willHit )
		{
			continue;
		}
		// ok, we are within the radius, add us to the incoming list
		missile_list[ent_count] = ent;
		ent_count++;

	}

	if ( ent_count )
	{
		vec3_t	fx_dir;
		// we are done, do we have any to deflect?
		if ( ent_count )
		{
			for ( int x = 0; x < ent_count; x++ )
			{
				if ( missile_list[x]->s.weapon == WP_SABER )
				{//just send it back
					if ( missile_list[x]->owner && missile_list[x]->owner->client && missile_list[x]->owner->client->ps.saber[0].Active() && missile_list[x]->s.pos.trType == TR_LINEAR && missile_list[x]->owner->client->ps.saberEntityState != SES_RETURNING )
					{//it's on and being controlled
						//FIXME: prevent it from damaging me?
						WP_SaberReturn( missile_list[x]->owner, missile_list[x] );
						VectorNormalize2( missile_list[x]->s.pos.trDelta, fx_dir );
						WP_SaberBlockEffect( self, 0, 0, missile_list[x]->currentOrigin, fx_dir, qfalse );
						if ( missile_list[x]->owner->client->ps.saberInFlight && self->client->ps.saberInFlight )
						{
							WP_SaberBlockSound( self, missile_list[x]->owner, 0, 0 );
							//G_Sound( missile_list[x], G_SoundIndex( va( "sound/weapons/saber/saberblock%d.wav", Q_irand(1, 9) ) ) );
							qboolean noFlare = qfalse;
							if ( (missile_list[x]->owner->client->ps.saber[0].saberFlags2&SFL2_NO_CLASH_FLARE)
								&& (self->client->ps.saber[0].saberFlags2&SFL2_NO_CLASH_FLARE) )
							{
								noFlare = qtrue;
							}
							if ( !noFlare )
							{
								g_saberFlashTime = level.time-50;
								gentity_t *saber = &g_entities[self->client->ps.saberEntityNum];
								vec3_t	org;
								VectorSubtract( missile_list[x]->currentOrigin, saber->currentOrigin, org );
								VectorMA( saber->currentOrigin, 0.5, org, org );
								VectorCopy( org, g_saberFlashPos );
							}
						}
					}
				}
				else
				{//bounce it
					vec3_t	reflectAngle, forward;
					if ( self->client && !self->s.number )
					{
						self->client->sess.missionStats.saberBlocksCnt++;
					}
					VectorCopy( saberent->s.apos.trBase, reflectAngle );
					reflectAngle[PITCH] = Q_flrand( -90, 90 );
					AngleVectors( reflectAngle, forward, NULL, NULL );

					// NPCs only reflect with a 55% chance for non-saber projectiles (e.g. rockets)
					if ( self->s.number == 0 || Q_irand( 0, 99 ) < 55 )
					{
						G_ReflectMissile( self, missile_list[x], forward );
						//do an effect (only when it actually bounced - a failed roll lets the shot through)
						VectorNormalize2( missile_list[x]->s.pos.trDelta, fx_dir );
						G_PlayEffect( "blaster/deflect", missile_list[x]->currentOrigin, fx_dir );
					}
				}
			}
		}
	}
}

qboolean WP_SaberValidateEnemy( gentity_t *self, gentity_t *enemy )
{
	if ( !enemy )
	{
		return qfalse;
	}

	if ( !enemy || enemy == self || !enemy->inuse || !enemy->client )
	{//not valid
		return qfalse;
	}

	if ( enemy->health <= 0 )
	{//corpse
		return qfalse;
	}

	/*
	if ( enemy->client->ps.weapon == WP_SABER
		&& enemy->client->ps.SaberActive() )
	{//not other saber-users?
		return qfalse;
	}
	*/
	if ( enemy->s.number >= MAX_CLIENTS )
	{//NPCs can cheat and use the homing saber throw 3 on the player
		if ( enemy->client->ps.forcePowersKnown )
		{//not other jedi?
			return qfalse;
		}
	}

	if ( DistanceSquared( self->client->renderInfo.handRPoint, enemy->currentOrigin ) > saberThrowDistSquared[self->client->ps.forcePowerLevel[FP_SABERTHROW]] )
	{//too far
		return qfalse;
	}

	if ( (!InFront( enemy->currentOrigin, self->currentOrigin, self->client->ps.viewangles, 0.0f) || !G_ClearLOS( self, self->client->renderInfo.eyePoint, enemy ) )
		&& ( DistanceHorizontalSquared( enemy->currentOrigin, self->currentOrigin ) > 65536 || fabs(enemy->currentOrigin[2]-self->currentOrigin[2]) > 384 )  )
	{//(not in front or not clear LOS) & greater than 256 away
		return qfalse;
	}

	if ( enemy->client->playerTeam == self->client->playerTeam )
	{//on same team
		return qfalse;
	}

	//LOS?

	return qtrue;
}

float WP_SaberRateEnemy( gentity_t *enemy, vec3_t center, vec3_t forward, float radius )
{
	float rating;
	vec3_t	dir;

	VectorSubtract( enemy->currentOrigin, center, dir );
	rating = (1.0f-(VectorNormalize( dir )/radius));
	rating *= DotProduct( forward, dir );
	return rating;
}

gentity_t *WP_SaberFindEnemy( gentity_t *self, gentity_t *saber )
{
//FIXME: should be a more intelligent way of doing this, like auto aim?
//closest, most in front... did damage to... took damage from?  How do we know who the player is focusing on?
	gentity_t	*ent, *bestEnt = NULL;
	gentity_t	*entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		center, mins, maxs, fwdangles, forward;
	int			i, e;
	float		radius = 400;
	float		rating, bestRating = 0.0f;

	//FIXME: no need to do this in 1st person?
	fwdangles[1] = self->client->ps.viewangles[1];
	AngleVectors( fwdangles, forward, NULL, NULL );

	VectorCopy( saber->currentOrigin, center );

	for ( i = 0 ; i < 3 ; i++ )
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}

	//if the saber has an enemy from the last time it looked, init to that one
	if ( WP_SaberValidateEnemy( self, saber->enemy ) )
	{
		if ( gi.inPVS( self->currentOrigin, saber->enemy->currentOrigin ) )
		{//potentially visible
			if ( G_ClearLOS( self, self->client->renderInfo.eyePoint, saber->enemy ) )
			{//can see him
				bestEnt = saber->enemy;
				bestRating = WP_SaberRateEnemy( bestEnt, center, forward, radius );
			}
		}
	}

	//If I have an enemy, see if that's even better
	if ( WP_SaberValidateEnemy( self, self->enemy ) )
	{
		float myEnemyRating = WP_SaberRateEnemy( self->enemy, center, forward, radius );
		if ( myEnemyRating > bestRating )
		{
			if ( gi.inPVS( self->currentOrigin, self->enemy->currentOrigin ) )
			{//potentially visible
				if ( G_ClearLOS( self, self->client->renderInfo.eyePoint, self->enemy ) )
				{//can see him
					bestEnt = self->enemy;
					bestRating = myEnemyRating;
				}
			}
		}
	}

	numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	if ( !numListedEntities )
	{//should we clear the enemy?
		return bestEnt;
	}

	for ( e = 0 ; e < numListedEntities ; e++ )
	{
		ent = entityList[ e ];

		if ( ent == self || ent == saber || ent == bestEnt )
		{
			continue;
		}
		if ( !WP_SaberValidateEnemy( self, ent ) )
		{//doesn't meet criteria of valid look enemy (don't check current since we would have done that before this func's call
			continue;
		}
		if ( !gi.inPVS( self->currentOrigin, ent->currentOrigin ) )
		{//not even potentially visible
			continue;
		}
		if ( !G_ClearLOS( self, self->client->renderInfo.eyePoint, ent ) )
		{//can't see him
			continue;
		}
		//rate him based on how close & how in front he is
		rating = WP_SaberRateEnemy( ent, center, forward, radius );
		if ( rating > bestRating )
		{
			bestEnt = ent;
			bestRating = rating;
		}
	}
	return bestEnt;
}

void WP_RunSaber( gentity_t *self, gentity_t *saber )
{
	vec3_t		origin, oldOrg;
	trace_t		tr;

	VectorCopy( saber->currentOrigin, oldOrg );
	// get current position
	EvaluateTrajectory( &saber->s.pos, level.time, origin );
	// get current angles
	EvaluateTrajectory( &saber->s.apos, level.time, saber->currentAngles );

	// trace a line from the previous position to the current position,
	// ignoring interactions with the missile owner
	int clipmask = saber->clipmask;
	if ( !self || !self->client || self->client->ps.SaberLength() <= 0 )
	{//don't keep hitting other sabers when turned off
		clipmask &= ~CONTENTS_LIGHTSABER;
	}
	gi.trace( &tr, saber->currentOrigin, saber->mins, saber->maxs, origin,
		saber->owner ? saber->owner->s.number : ENTITYNUM_NONE, clipmask, (EG2_Collision)0, 0 );

	VectorCopy( tr.endpos, saber->currentOrigin );

	if ( self->client->ps.SaberActive() )
	{
		if ( self->client->ps.saberInFlight || (self->client->ps.weaponTime&&!Q_irand( 0, 100 )) )
		{//make enemies run from a lit saber in flight or from me when I'm attacking
			if ( !Q_irand( 0, 10 ) )
			{//not so often...
				AddSightEvent( self, saber->currentOrigin, self->client->ps.SaberLength()*3, AEL_DANGER, 100 );
			}
		}
	}

	if ( tr.startsolid )
	{
		tr.fraction = 0;
	}

	gi.linkentity( saber );

	//touch push triggers?

	if ( tr.fraction != 1 )
	{
		WP_SaberImpact( self, saber, &tr );
	}

	if ( saber->s.pos.trType == TR_LINEAR )
	{//home
		//figure out where saber should be
		vec3_t	forward, saberHome, saberDest, fwdangles = {0};

		VectorCopy( self->client->ps.viewangles, fwdangles );
		if ( self->s.number )
		{
			fwdangles[0] -= 8;
		}
		else if ( cg.renderingThirdPerson )
		{
			fwdangles[0] -= 5;
		}

		if ( self->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_1
			|| self->client->ps.saberEntityState == SES_RETURNING
			|| VectorCompare( saber->s.pos.trDelta, vec3_origin ) )
		{//control if it's returning or just starting
			float	saberSpeed = 500;//FIXME: based on force level?
			float	dist;
			gentity_t *enemy = NULL;

			AngleVectors( fwdangles, forward, NULL, NULL );

			if ( self->client->ps.saberEntityDist < 100 )
			{//make the saber head to my hand- the bolt it was attached to
				VectorCopy( self->client->renderInfo.handRPoint, saberHome );
			}
			else
			{//aim saber from eyes
				VectorCopy( self->client->renderInfo.eyePoint, saberHome );
			}
			VectorMA( saberHome, self->client->ps.saberEntityDist, forward, saberDest );

			if ( self->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_2
				&& self->client->ps.saberEntityState == SES_LEAVING )
			{//max level
				if ( self->enemy &&
					!WP_SaberValidateEnemy( self, self->enemy ) )
				{//if my enemy isn't valid to auto-aim at, don't autoaim
				}
				else
				{
					//pick an enemy
					enemy = WP_SaberFindEnemy( self, saber );
					if ( enemy )
					{//home in on enemy
						float enemyDist = Distance( self->client->renderInfo.handRPoint, enemy->currentOrigin );
						VectorCopy( enemy->currentOrigin, saberDest );
						saberDest[2] += enemy->maxs[2]/2.0f;//FIXME: when in a knockdown anim, the saber float above them... do we care?
						self->client->ps.saberEntityDist = enemyDist;
						//once you pick an enemy, stay with it!
						saber->enemy = enemy;
						//FIXME: lock onto that enemy for a minimum amount of time (unless they become invalid?)
					}
				}
			}


			//Make the saber head there
			VectorSubtract( saberDest, saber->currentOrigin, saber->s.pos.trDelta );
			dist = VectorNormalize( saber->s.pos.trDelta );
			if ( self->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_2 && self->client->ps.saberEntityState == SES_LEAVING && !enemy )
			{
				if ( dist < 200 )
				{
					saberSpeed = 400 - (dist*2);
				}
			}
			else if ( self->client->ps.saberEntityState == SES_LEAVING && dist < 50 )
			{
				saberSpeed = dist * 2 + 30;
				if ( (enemy && dist > enemy->maxs[0]) || (!enemy && dist > 24) )
				{//auto-tracking an enemy and we can't hit him
					if ( saberSpeed < 120 )
					{//clamp to a minimum speed
						saberSpeed = 120;
					}
				}
			}
			/*
			if ( self->client->ps.saberEntityState == SES_RETURNING )
			{//FIXME: if returning, move faster?
				saberSpeed = 800;
				if ( dist < 200 )
				{
					saberSpeed -= 400 - (dist*2);
				}
			}
			*/
			VectorScale( saber->s.pos.trDelta, saberSpeed, saber->s.pos.trDelta );
			//SnapVector( saber->s.pos.trDelta );	// save net bandwidth
			VectorCopy( saber->currentOrigin, saber->s.pos.trBase );
			saber->s.pos.trTime = level.time;
			saber->s.pos.trType = TR_LINEAR;
		}
		else
		{
			VectorCopy( saber->currentOrigin, saber->s.pos.trBase );
			saber->s.pos.trTime = level.time;
			saber->s.pos.trType = TR_LINEAR;
		}

		//if it's heading back, point it's base at us
		if ( self->client->ps.saberEntityState == SES_RETURNING
			&& !(self->client->ps.saber[0].saberFlags&SFL_RETURN_DAMAGE) )//type != SABER_STAR )
		{
			fwdangles[0] += SABER_PITCH_HACK;
			VectorCopy( fwdangles, saber->s.apos.trBase );
			saber->s.apos.trTime = level.time;
			saber->s.apos.trType = TR_INTERPOLATE;
			VectorClear( saber->s.apos.trDelta );
		}
	}
}


qboolean WP_SaberLaunch( gentity_t *self, gentity_t *saber, qboolean thrown, qboolean noFail = qfalse )
{//FIXME: probably need a debounce time
	vec3_t	saberMins={-3.0f,-3.0f,-3.0f};
	vec3_t	saberMaxs={3.0f,3.0f,3.0f};
	trace_t	trace;

	if ( self->client->NPC_class == CLASS_SABER_DROID )
	{//saber droids can't drop their saber
		return qfalse;
	}
	if ( !noFail )
	{
		if ( thrown )
		{//this is a regular throw, so see if it's legal
			if ( self->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_2 )
			{
				if ( !WP_ForcePowerUsable( self, FP_SABERTHROW, 20 ) )
				{
					return qfalse;
				}
			}
			else
			{
				if ( !WP_ForcePowerUsable( self, FP_SABERTHROW, 0 ) )
				{
					return qfalse;
				}
			}
		}
		if ( !self->s.number && (cg.zoomMode || in_camera) )
		{//can't saber throw when zoomed in or in cinematic
			return qfalse;
		}
		//make sure it won't start in solid
		gi.trace( &trace, self->client->renderInfo.handRPoint, saberMins, saberMaxs, self->client->renderInfo.handRPoint, saber->s.number, MASK_SOLID, (EG2_Collision)0, 0 );
		if ( trace.startsolid || trace.allsolid )
		{
			return qfalse;
		}
		//make sure I'm not throwing it on the other side of a door or wall or whatever
		gi.trace( &trace, self->currentOrigin, vec3_origin, vec3_origin, self->client->renderInfo.handRPoint, self->s.number, MASK_SOLID, (EG2_Collision)0, 0 );
		if ( trace.startsolid || trace.allsolid || trace.fraction < 1.0f )
		{
			return qfalse;
		}

		if ( thrown )
		{//this is a regular throw, so take force power
			if ( self->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_2 )
			{//at max skill, the cost increases as keep it out
				WP_ForcePowerStart( self, FP_SABERTHROW, 10 );
			}
			else
			{
				WP_ForcePowerStart( self, FP_SABERTHROW, 0 );
			}
		}
	}
	//clear the enemy
	saber->enemy = NULL;

//===FIXME!!!==============================================================================================
	//We should copy the right-hand saber's g2 instance to the thrown saber
	//Then back again when you catch it!!!
//===FIXME!!!==============================================================================================

	//draw it
	saber->s.eFlags &= ~EF_NODRAW;
	saber->svFlags |= SVF_BROADCAST;
	saber->svFlags &= ~SVF_NOCLIENT;

	//place it
	VectorCopy( self->client->renderInfo.handRPoint, saber->currentOrigin );//muzzlePoint
	VectorCopy( saber->currentOrigin, saber->s.pos.trBase );
	saber->s.pos.trTime = level.time;
	saber->s.pos.trType = TR_LINEAR;
	VectorClear( saber->s.pos.trDelta );
	gi.linkentity( saber );

	//spin it
	VectorClear( saber->s.apos.trBase );
	saber->s.apos.trTime = level.time;
	saber->s.apos.trType = TR_LINEAR;
	if ( self->health > 0 && thrown )
	{//throwing it
		saber->s.apos.trBase[1] = self->client->ps.viewangles[1];
		saber->s.apos.trBase[0] = SABER_PITCH_HACK;
	}
	else
	{//dropping it
		vectoangles( self->client->renderInfo.muzzleDir, saber->s.apos.trBase );
	}
	VectorClear( saber->s.apos.trDelta );

	switch ( self->client->ps.forcePowerLevel[FP_SABERTHROW] )
	{//FIXME: make a table?
	default:
	case FORCE_LEVEL_1:
		saber->s.apos.trDelta[1] = 600;
		break;
	case FORCE_LEVEL_2:
		saber->s.apos.trDelta[1] = 800;
		break;
	case FORCE_LEVEL_3:
		saber->s.apos.trDelta[1] = 1200;
		break;
	}

	//Take it out of my hand
	self->client->ps.saberInFlight = qtrue;
	self->client->ps.saberEntityState = SES_LEAVING;
	self->client->ps.saberEntityDist = saberThrowDist[self->client->ps.forcePowerLevel[FP_SABERTHROW]];
	self->client->ps.saberThrowTime = level.time;
	//if ( self->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_2 )
	{
		self->client->ps.forcePowerDebounce[FP_SABERTHROW] = level.time + 1000;//so we can keep it out for a minimum amount of time
	}

	if ( thrown )
	{//this is a regular throw, so turn the saber on
		//turn saber on
		if ( (self->client->ps.saber[0].saberFlags&SFL_SINGLE_BLADE_THROWABLE) )//SaberStaff() )
		{//only first blade can be on
			if ( !self->client->ps.saber[0].blade[0].active )
			{//turn on first one
				self->client->ps.SaberBladeActivate( 0, 0 );
			}
			for ( int i = 1; i < self->client->ps.saber[0].numBlades; i++ )
			{//turn off all others
				if ( self->client->ps.saber[0].blade[i].active )
				{
					self->client->ps.SaberBladeActivate( 0, i, qfalse );
				}
			}
		}
		else
		{//turn the sabers, all blades...?
			self->client->ps.saber[0].Activate();
			//self->client->ps.SaberActivate();
		}
		//turn on the saber trail
		self->client->ps.saber[0].ActivateTrail( 150 );
	}

	//reset the mins
	VectorCopy( saberMins, saber->mins );
	VectorCopy( saberMaxs, saber->maxs );
	saber->contents = 0;//CONTENTS_LIGHTSABER;
	saber->clipmask = MASK_SOLID | CONTENTS_LIGHTSABER;

	// remove the ghoul2 right-hand saber model on the player
	if ( self->weaponModel[0] > 0 )
	{
		gi.G2API_RemoveGhoul2Model(self->ghoul2, self->weaponModel[0]);
		self->weaponModel[0] = -1;
	}

	return qtrue;
}

qboolean WP_SaberLose( gentity_t *self, vec3_t throwDir )
{
	if ( !self || !self->client || self->client->ps.saberEntityNum <= 0 )
	{//WTF?!!  We lost it already?
		return qfalse;
	}
	if ( self->client->NPC_class == CLASS_SABER_DROID )
	{//saber droids can't drop their saber
		return qfalse;
	}
	gentity_t *dropped = &g_entities[self->client->ps.saberEntityNum];
	if ( !self->client->ps.saberInFlight )
	{//not alreay in air
		/*
		qboolean noForceThrow = qfalse;
		//make it so we can throw it
		self->client->ps.forcePowersKnown |= (1<<FP_SABERTHROW);
		if ( self->client->ps.forcePowerLevel[FP_SABERTHROW] < FORCE_LEVEL_1 )
		{
			noForceThrow = qtrue;
			self->client->ps.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_1;
		}
		*/
		//throw it
		if ( !WP_SaberLaunch( self, dropped, qfalse ) )
		{//couldn't throw it
			return qfalse;
		}
		/*
		if ( noForceThrow )
		{
			self->client->ps.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_0;
		}
		*/
	}
	if ( self->client->ps.saber[0].Active() )
	{//on
		//drop it instantly
		WP_SaberDrop( self, dropped );
	}
	//optionally give it some thrown velocity
	if ( throwDir && !VectorCompare( throwDir, vec3_origin ) )
	{
		VectorCopy( throwDir, dropped->s.pos.trDelta );
	}
	//don't pull it back on the next frame
	if ( self->NPC )
	{
		self->NPC->last_ucmd.buttons &= ~BUTTON_ATTACK;
		if ( self->health > 0 && (!self->client->ps.dualSabers || !self->client->ps.saber[1].Active())
			&& !G_IsSaberBoss( self, qtrue ) )
		{//no backup saber left -- react immediately instead of waiting for an AI tick to notice
		 //(bosses don't run: they stand and pull their saber back - the Jedi AI's rearm)
			G_StartFlee( self, self->enemy, self->enemy ? self->enemy->currentOrigin : self->currentOrigin, AEL_DANGER_GREAT, 5000, 10000 );
		}
	}
	return qtrue;
}

void WP_SetSaberOrigin( gentity_t *self, vec3_t newOrg )
{
	if ( !self || !self->client )
	{
		return;
	}
	if ( self->client->ps.saberEntityNum <= 0 || self->client->ps.saberEntityNum >= ENTITYNUM_WORLD )
	{//no saber ent to reposition
		return;
	}
	if ( self->client->NPC_class == CLASS_SABER_DROID )
	{//saber droids can't drop their saber
		return;
	}
	gentity_t *dropped = &g_entities[self->client->ps.saberEntityNum];
	if ( !self->client->ps.saberInFlight )
	{//not already in air
		qboolean noForceThrow = qfalse;
		//make it so we can throw it
		self->client->ps.forcePowersKnown |= (1<<FP_SABERTHROW);
		if ( self->client->ps.forcePowerLevel[FP_SABERTHROW] < FORCE_LEVEL_1 )
		{
			noForceThrow = qtrue;
			self->client->ps.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_1;
		}
		//throw it
		if ( !WP_SaberLaunch( self, dropped, qfalse, qtrue ) )
		{//couldn't throw it
			return;
		}
		if ( noForceThrow )
		{
			self->client->ps.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_0;
		}
	}
	VectorCopy( newOrg, dropped->s.origin );
	VectorCopy( newOrg, dropped->currentOrigin );
	VectorCopy( newOrg, dropped->s.pos.trBase );
	//drop it instantly
	WP_SaberDrop( self, dropped );
	//don't pull it back on the next frame
	if ( self->NPC )
	{
		self->NPC->last_ucmd.buttons &= ~BUTTON_ATTACK;
	}
}

void WP_SaberCatch( gentity_t *self, gentity_t *saber, qboolean switchToSaber )
{//FIXME: probably need a debounce time
	if ( self->health > 0 && !PM_SaberInBrokenParry( self->client->ps.saberMove ) && self->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
	{
		//clear the enemy
		saber->enemy = NULL;
//===FIXME!!!==============================================================================================
	//We should copy the thrown saber's g2 instance to the right-hand saber
	//When you catch it, and vice-versa when you throw it!!!
//===FIXME!!!==============================================================================================
		//don't draw it
		saber->s.eFlags |= EF_NODRAW;
		saber->svFlags &= SVF_BROADCAST;
		saber->svFlags |= SVF_NOCLIENT;

		//take off any gravity stuff if we'd dropped it
		saber->s.pos.trType = TR_LINEAR;
		saber->s.eFlags &= ~EF_BOUNCE_HALF;

		//Put it in my hand
		self->client->ps.saberInFlight = qfalse;
		self->client->ps.saberEntityState = SES_LEAVING;

		//turn off the saber trail
		self->client->ps.saber[0].DeactivateTrail( 75 );

		//reset its contents/clipmask
		saber->contents = CONTENTS_LIGHTSABER;// | CONTENTS_SHOTCLIP;
		saber->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		//play catch sound
		G_Sound( saber, G_SoundIndex( "sound/weapons/saber/saber_catch.wav" ) );
		//FIXME: if an NPC, don't turn it back on if no enemy or enemy is dead...
		//if it's not our current weapon, make it our current weapon
		if ( self->client->ps.weapon == WP_SABER )
		{//only do the first saber since we only throw the first one
			WP_SaberAddG2SaberModels( self, qfalse );
		}
		if ( switchToSaber )
		{
			if ( self->client->ps.weapon != WP_SABER )
			{
				CG_ChangeWeapon( WP_SABER );
			}
			else
			{//if it's not active, turn it on
				if ( (self->client->ps.saber[0].saberFlags&SFL_SINGLE_BLADE_THROWABLE) )//SaberStaff() )
				{//only first blade can be on
					if ( !self->client->ps.saber[0].blade[0].active )
					{//only turn it on if first blade is off, otherwise, leave as-is
						self->client->ps.saber[0].Activate();
					}
				}
				else
				{//turn all blades on
					self->client->ps.saber[0].Activate();
				}
			}
		}
	}
}


void WP_SaberReturn( gentity_t *self, gentity_t *saber )
{
	if ( PM_SaberInBrokenParry( self->client->ps.saberMove ) || self->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN )
	{
		return;
	}
	if ( self && self->client )
	{//still alive and stuff
		//FIXME: when it's returning, flies butt first, but seems to do a lot of damage when going through people... hmm...
		self->client->ps.saberEntityState = SES_RETURNING;
		//turn down the saber trail
		if ( !(self->client->ps.saber[0].saberFlags&SFL_RETURN_DAMAGE) )//type != SABER_STAR )
		{
			self->client->ps.saber[0].DeactivateTrail( 75 );
		}
	}
	if ( !(saber->s.eFlags&EF_BOUNCE) )
	{
		saber->s.eFlags |= EF_BOUNCE;
		saber->bounceCount = 300;
	}
}


void WP_SaberDrop( gentity_t *self, gentity_t *saber )
{
	//clear the enemy
	saber->enemy = NULL;
	saber->s.eFlags &= ~EF_BOUNCE;
	saber->bounceCount = 0;
	//make it fall
	saber->s.pos.trType = TR_GRAVITY;
	//make it bounce some
	saber->s.eFlags |= EF_BOUNCE_HALF;
	//make it spin
	VectorCopy( saber->currentAngles, saber->s.apos.trBase );
	saber->s.apos.trType = TR_LINEAR;
	saber->s.apos.trTime = level.time;
	VectorSet( saber->s.apos.trDelta, Q_irand( -300, 300 ), saber->s.apos.trDelta[1], Q_irand( -300, 300 ) );
	if ( !saber->s.apos.trDelta[1] )
	{
		saber->s.apos.trDelta[1] = Q_irand( -300, 300 );
	}
	//force it to be ready to return
	self->client->ps.saberEntityDist = 0;
	self->client->ps.saberEntityState = SES_RETURNING;
	//turn it off
	self->client->ps.saber[0].Deactivate();
	//turn off the saber trail
	self->client->ps.saber[0].DeactivateTrail( 75 );
	//play the saber turning off sound
	G_SoundIndexOnEnt( saber, CHAN_AUTO, self->client->ps.saber[0].soundOff );

	if ( self->health <= 0 )
	{//owner is dead!
		saber->s.time = level.time;//will make us free ourselves after a time
	}
}


void WP_SaberPull( gentity_t *self, gentity_t *saber )
{
	if ( PM_SaberInBrokenParry( self->client->ps.saberMove ) || self->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN )
	{
		return;
	}
	if ( self->health > 0 )
	{
		//take off gravity
		saber->s.pos.trType = TR_LINEAR;
		//take off bounce
		saber->s.eFlags &= EF_BOUNCE_HALF;
		//play sound
		G_Sound( self, G_SoundIndex( "sound/weapons/force/pull.wav" ) );
	}
}

const char *saberColorStringForColor[SABER_PURPLE+1] =
{
	"red",//SABER_RED
	"orange",//SABER_ORANGE
	"yellow",//SABER_YELLOW
	"green",//SABER_GREEN
	"blue",//SABER_BLUE
	"purple"//SABER_PURPLE
};

// Check if we are throwing it, launch it if needed, update position if needed.
void WP_SaberThrow( gentity_t *self, usercmd_t *ucmd )
{
	vec3_t			saberDiff;
	trace_t			tr;
	//static float	SABER_SPEED = 10;

	gentity_t *saberent;

	if ( self->client->ps.saberEntityNum <= 0 || self->client->ps.saberEntityNum >= ENTITYNUM_WORLD )
	{//WTF?!!  We lost it?
		return;
	}

	if ( self->client->ps.torsoAnim == BOTH_LOSE_SABER )
	{//can't catch it while it's being yanked from your hand!
		return;
	}

	if ( !g_saberNewControlScheme->integer )
	{
		if ( PM_SaberInKata( (saberMoveName_t)self->client->ps.saberMove ) )
		{//don't throw saber when in special attack (alt+attack)
			return;
		}
		if ( (ucmd->buttons&BUTTON_ATTACK)
			&& (ucmd->buttons&BUTTON_ALT_ATTACK)
			&& !self->client->ps.saberInFlight )
		{//trying to do special attack, don't throw it
			return;
		}
		else if ( self->client->ps.torsoAnim == BOTH_A1_SPECIAL
			|| self->client->ps.torsoAnim == BOTH_A2_SPECIAL
			|| self->client->ps.torsoAnim == BOTH_A3_SPECIAL )
		{//don't throw in these anims!
			return;
		}
	}
	saberent = &g_entities[self->client->ps.saberEntityNum];

	VectorSubtract( self->client->renderInfo.handRPoint, saberent->currentOrigin, saberDiff );

	//is our saber in flight?
	if ( !self->client->ps.saberInFlight )
	{//saber is not in flight right now
		if ( self->client->ps.weapon != WP_SABER )
		{//don't even have it out
			return;
		}
		else if ( (ucmd->buttons&BUTTON_SABERTHROW) && !(self->client->ps.pm_flags&PMF_SABERTHROW_HELD) )
		{//still holding it, not still holding attack from a previous throw, so throw it.
			if ( !(self->client->ps.saberEventFlags&SEF_INWATER) && WP_SaberLaunch( self, saberent, qtrue ) )
			{
				if ( self->client && !self->s.number )
				{
					self->client->sess.missionStats.saberThrownCnt++;
				}
				//need to recalc this because we just moved it
				VectorSubtract( self->client->renderInfo.handRPoint, saberent->currentOrigin, saberDiff );
			}
			else
			{//couldn't throw it
				return;
			}
		}
		else
		{//holding it, don't want to throw it, go away.
			return;
		}
	}
	else
	{//inflight
		//is our saber currently on it's way back to us?
		if ( self->client->ps.saberEntityState == SES_RETURNING )
		{//see if we're close enough to pick it up
			if ( VectorLengthSquared( saberDiff ) <= 256 )//16 squared//G_BoundsOverlap( self->absmin, self->absmax, saberent->absmin, saberent->absmax ) )//
			{//caught it
				vec3_t	axisPoint;
				trace_t	trace;
				VectorCopy( self->currentOrigin, axisPoint );
				axisPoint[2] = self->client->renderInfo.handRPoint[2];
				gi.trace( &trace, axisPoint, vec3_origin, vec3_origin, self->client->renderInfo.handRPoint, self->s.number, MASK_SOLID, (EG2_Collision)0, 0 );
				if ( !trace.startsolid && trace.fraction >= 1.0f )
				{//our hand isn't through a wall
					WP_SaberCatch( self, saberent, qtrue );
					//NPC_SetAnim( self, SETANIM_TORSO, TORSO_HANDRETRACT1, SETANIM_FLAG_OVERRIDE );
				}
				return;
			}
		}

		if ( saberent->s.pos.trType != TR_STATIONARY )
		{//saber is in flight, lerp it
			if ( self->health <= 0 )//&& level.time > saberent->s.time + 5000 )
			{//make us free ourselves after a time
				if ( g_saberPickuppableDroppedSabers->integer
					&& G_DropSaberItem( self->client->ps.saber[0].name, self->client->ps.saber[0].blade[0].color, saberent->currentOrigin, saberent->s.pos.trDelta, saberent->currentAngles ) != NULL )
				{//dropped it
					//free it
					G_FreeEntity( saberent );
					//forget it
					self->client->ps.saberEntityNum = ENTITYNUM_NONE;
					return;
				}
			}
			WP_RunSaber( self, saberent );
		}
		else
		{//it fell on the ground
			if ( self->health <= 0 )//&& level.time > saberent->s.time + 5000 )
			{//make us free ourselves after a time
				if ( g_saberPickuppableDroppedSabers->integer )
				{//spawn an item
					G_DropSaberItem( self->client->ps.saber[0].name, self->client->ps.saber[0].blade[0].color, saberent->currentOrigin, saberent->s.pos.trDelta, saberent->currentAngles );
				}
				//free it
				G_FreeEntity( saberent );
				//forget it
				self->client->ps.saberEntityNum = ENTITYNUM_NONE;
				return;
			}
			if ( (!self->s.number && level.time - saberent->aimDebounceTime > 15000)
				|| (self->s.number && level.time - saberent->aimDebounceTime > 5000) )
			{//(only for player) been missing for 15 seconds, automagicially return
				WP_SaberCatch( self, saberent, qfalse );
				return;
			}
		}
	}

	//are we still trying to use the saber?
	if ( self->client->ps.weapon != WP_SABER )
	{//switched away
		if ( !self->client->ps.saberInFlight )
		{//wasn't throwing saber
			return;
		}
		else if ( saberent->s.pos.trType == TR_LINEAR )
		{//switched away while controlling it, just drop the saber
			WP_SaberDrop( self, saberent );
			return;
		}
		else
		{//it's on the ground, see if it's inside us (touching)
			if ( G_PointInBounds( saberent->currentOrigin, self->absmin, self->absmax ) )
			{//it's in us, pick it up automatically
				WP_SaberPull( self, saberent );
			}
		}
	}
	else if ( saberent->s.pos.trType != TR_LINEAR )
	{//weapon is saber and not flying
		if ( self->client->ps.saberInFlight )
		{//we dropped it
			if ( ucmd->buttons & BUTTON_ATTACK )//|| self->client->ps.weaponstate == WEAPON_RAISING )//ucmd->buttons & BUTTON_ALT_ATTACK ||
			{//we actively want to pick it up or we just switched to it, so pull it back
				gi.trace( &tr, saberent->currentOrigin, saberent->mins, saberent->maxs, self->client->renderInfo.handRPoint, self->s.number, MASK_SOLID, (EG2_Collision)0, 0 );

				if ( tr.allsolid || tr.startsolid || tr.fraction < 1.0f )
				{//can't pick it up yet, no LOS
					return;
				}
				//clear LOS, pick it up
				WP_SaberPull( self, saberent );
			}
			else
			{//see if it's inside us (touching)
				if ( G_PointInBounds( saberent->currentOrigin, self->absmin, self->absmax ) )
				{//it's in us, pick it up automatically
					WP_SaberPull( self, saberent );
				}
			}
		}
	}
	else if ( self->health <= 0 && self->client->ps.saberInFlight )
	{//we died, drop it
		WP_SaberDrop( self, saberent );
		return;
	}
	else if ( !self->client->ps.saber[0].Active() && self->client->ps.saberEntityState != SES_RETURNING )
	{//we turned it off, drop it
		WP_SaberDrop( self, saberent );
		return;
	}

	//TODO: if deactivate saber in flight, should it drop?

	if ( saberent->s.pos.trType != TR_LINEAR )
	{//don't home
		return;
	}

	float saberDist = VectorLength( saberDiff );
	if ( self->client->ps.saberEntityState == SES_LEAVING )
	{//saber still flying forward
		if ( self->client->ps.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_2 )
		{//still holding it out
			if ( !(ucmd->buttons&BUTTON_SABERTHROW) && self->client->ps.forcePowerDebounce[FP_SABERTHROW] < level.time )
			{//done throwing, return to me
				if ( self->client->ps.saber[0].Active() )
				{//still on
					WP_SaberReturn( self, saberent );
				}
			}
			else if ( level.time - self->client->ps.saberThrowTime >= 100 )
			{
				if ( WP_ForcePowerAvailable( self, FP_SABERTHROW, 1 ) )
				{
					WP_ForcePowerDrain( self, FP_SABERTHROW, 1 );
					self->client->ps.saberThrowTime = level.time;
				}
				else
				{//out of force power, return to me
					WP_SaberReturn( self, saberent );
				}
			}
		}
		else
		{
			if ( !(ucmd->buttons&BUTTON_SABERTHROW) && self->client->ps.forcePowerDebounce[FP_SABERTHROW] < level.time )
			{//not holding button and has been out at least 1 second, return to me
				if ( self->client->ps.saber[0].Active() )
				{//still on
					WP_SaberReturn( self, saberent );
				}
			}
			else if ( level.time - self->client->ps.saberThrowTime > 3000
				|| (self->client->ps.forcePowerLevel[FP_SABERTHROW]==FORCE_LEVEL_1&&saberDist>=self->client->ps.saberEntityDist) )
			{//been out too long, or saber throw 1 went too far, return to me
				if ( self->client->ps.saber[0].Active() )
				{//still on
					WP_SaberReturn( self, saberent );
				}
			}
		}
	}
	if ( self->client->ps.saberEntityState == SES_RETURNING )
	{
		if ( self->client->ps.saberEntityDist > 0 )
		{
			self->client->ps.saberEntityDist -= 25;
		}
		if ( self->client->ps.saberEntityDist < 0 )
		{
			self->client->ps.saberEntityDist = 0;
		}
		else if ( saberDist < self->client->ps.saberEntityDist )
		{//if it's coming back to me, never push it away
			self->client->ps.saberEntityDist = saberDist;
		}
	}
}

extern void	G_CreateG2AttachedWeaponModel( gentity_t *ent, const char *weaponModel, int boltNum, int weaponNum );
void WP_SaberFireGun( gentity_t *self, usercmd_t *ucmd, int whichGun )
{
	int addTime, oldWeapon;
	qboolean chargedShot = qfalse;
	
	if ( self->health <= 0 )
	{
		return;
	}
	
	if ( !self->s.number && (cg.zoomMode || in_camera) )
	{//can't shoot when zoomed in or in cinematic
		return;
	}
	
	if ( self->client->ps.leanofs )
	{
		return;
	}
	
	if ( self->client->ps.weaponTime > 0 )
	{
		return;
	}
	
	if ( self->client->ps.saberAnimLevel != SS_KATARN )
	{
		return;
	}
	
	if ( self->s.weapon != WP_SABER )
	{
		return;
	}
	
	if ( !(self->client->ps.weapons[whichGun]) )
	{
		return;
	}
	
	if ( !(ucmd->buttons & BUTTON_ALT_ATTACK) )
	{
		return;
	}
	
	if ( !self->weaponModel[1] || (self->weaponModel[1] == -1) )
	{
		G_CreateG2AttachedWeaponModel( self, weaponData[whichGun].worldModel, self->handLBolt, 1 );
	}
	
	addTime = weaponData[whichGun].fireTime;
	self->client->ps.weaponTime += addTime;
	self->client->ps.lastShotTime = level.time;
//TODO:proper addTime scaling
	
	if ( whichGun == WP_BRYAR_PISTOL && self->client->ps.saberLockTime > level.time )
	{
		//shoot your way out of saberlocks!
		G_StartMatrixEffect( self );
		chargedShot = qtrue;
		self->client->ps.weaponChargeTime = level.time - 10*BRYAR_CHARGE_UNIT;
	}
	
	self->client->ps.saberBlocked = BLOCKED_NONE;
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;
	NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCELIGHTNING, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART );
	
	oldWeapon = self->s.weapon;
	self->s.weapon = whichGun;
	FireWeapon(self, chargedShot);
	
	cg_entities[self->s.number].muzzleFlashTime = level.time;
	cg_entities[self->s.number].muzzleFlashWeapon = whichGun;
	
	self->s.weapon = oldWeapon;
}



//SABER BLOCKING============================================================================
//SABER BLOCKING============================================================================
//SABER BLOCKING============================================================================
//SABER BLOCKING============================================================================
//SABER BLOCKING============================================================================
static int blockForceCost[] = {
	0,   // SS_NONE
	5,   // SS_FAST
	5,   // SS_MEDIUM
	10,  // SS_STRONG
	10,  // SS_DESANN
	5,   // SS_TAVION
	10,  // SS_KATARN
	10,  // SS_DUAL
	10,  // SS_STAFF
};

// Shared cost formula for a non-perfect block, used both by WP_SaberBlockNonRandom (saber-vs-saber)
// and by g_missile.cpp's ordinary (non-perfect) bolt-deflect resolution -- kept here since
// blockForceCost[] is file-static.
int WP_SaberBlockForceCost( gentity_t *self, qboolean missileBlock )
{
	int defLevel = self->client->ps.forcePowerLevel[FP_SABER_DEFENSE];
	int cost = blockForceCost[self->client->ps.saberAnimLevel];
	if ( defLevel >= FORCE_LEVEL_3 )      cost /= 2;
	else if ( defLevel >= FORCE_LEVEL_2 ) cost = cost * 3 / 4;
	if ( missileBlock )
	{
		cost = cost * 3 / 2;  // 1.5x for blaster bolt blocks
		if ( self->client->ps.saberAnimLevel == SS_STAFF )
		{//Staff has superior missile defense
			cost /= 2;
		}
	}
	else                cost = cost * 5;       // 5x for saber attack blocks -- non-perfect blocks should sting
	if ( !missileBlock && g_saberDeterministic && g_saberDeterministic->integer
		&& self->enemy && self->enemy->client && self->enemy->client->ps.weapon == WP_SABER )
	{//JKEnhanced: harder swings cost more to block - blocks hold, Force is what wears you down
		const int pow = PM_PowerLevelForSaberAnim( &self->enemy->client->ps, 0 );
		float scale = pow >= FORCE_LEVEL_3 ? 1.35f : ( pow == FORCE_LEVEL_2 ? 1.0f : 0.75f );
		if ( WP_IsHeavyAttack( self->enemy ) )
		{
			scale = 1.6f;
		}
		cost = (int)( cost * scale );
	}
	return cost;
}

// Perfect-parry refund: half of what a non-perfect block would cost at zero Defense investment.
// Deliberately NOT scaled down by the defender's own Defense level (unlike WP_SaberBlockForceCost)
// so a high-Defense character's reward doesn't shrink toward an imperceptible 1 point.
int WP_SaberPerfectParryRefund( gentity_t *self, qboolean missileBlock )
{
	int cost = blockForceCost[self->client->ps.saberAnimLevel];
	if ( missileBlock )
	{
		cost = cost * 3 / 2;
		if ( self->client->ps.saberAnimLevel == SS_STAFF )
		{
			cost /= 2;
		}
	}
	else
	{
		cost = cost * 5;
	}
	return cost / 2;
}

int WP_MissileBlockForBlock( int saberBlock )
{
	switch( saberBlock )
	{
	case BLOCKED_UPPER_RIGHT:
		return BLOCKED_UPPER_RIGHT_PROJ;
		break;
	case BLOCKED_UPPER_LEFT:
		return BLOCKED_UPPER_LEFT_PROJ;
		break;
	case BLOCKED_LOWER_RIGHT:
		return BLOCKED_LOWER_RIGHT_PROJ;
		break;
	case BLOCKED_LOWER_LEFT:
		return BLOCKED_LOWER_LEFT_PROJ;
		break;
	case BLOCKED_TOP:
		return BLOCKED_TOP_PROJ;
		break;
	}
	return saberBlock;
}

/*
WP_PlayerBlockChargeNew (JKEnhanced)

The player's saber-block cost is paid once per enemy swing. The block runs again on every frame of
blade contact, and the Defense 2+ debounce (37-130 ms) is shorter than a frame at low fps, so one
swing was billed two or three times. A new swing = a different attacker or a different saberMove,
or 1 s since the last charge.
*/
static qboolean WP_PlayerBlockChargeNew( gentity_t *self )
{
	static int s_lastAttacker = ENTITYNUM_NONE;
	static int s_lastMove = LS_NONE;
	static int s_lastTime = 0;
	const gentity_t *att = self->enemy;
	const int attNum = att ? att->s.number : ENTITYNUM_NONE;
	const int attMove = ( att && att->client ) ? att->client->ps.saberMove : LS_NONE;
	if ( attNum == s_lastAttacker && attMove == s_lastMove
		&& level.time >= s_lastTime && level.time - s_lastTime < 1000 )
	{
		return qfalse;
	}
	s_lastAttacker = attNum;
	s_lastMove = attMove;
	s_lastTime = level.time;
	return qtrue;
}

void WP_SaberBlockNonRandom( gentity_t *self, vec3_t hitloc, qboolean missileBlock )
{
	vec3_t diff, fwdangles={0,0,0}, right;
	float rightdot;
	float zdiff;

	if ( self->client->ps.weaponstate == WEAPON_DROPPING ||
		self->client->ps.weaponstate == WEAPON_RAISING )
	{//don't block while changing weapons
		return;
	}
	if ( PM_SuperBreakLoseAnim( self->client->ps.torsoAnim )
		|| PM_SuperBreakWinAnim( self->client->ps.torsoAnim ) )
	{
		return;
	}
	// No force power = cannot block
	if ( self->s.number == 0 && !g_saberAutoBlocking->integer
		&& self->client->ps.forcePower <= 0 )
	{
		if ( self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] < level.time )
		{
			G_SoundOnEnt( self, CHAN_AUTO, "sound/weapons/force/noforce.mp3" );
			self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 1000;
		}
		return;
	}
	//NPCs don't auto-block
	if ( !missileBlock && self->s.number != 0 && self->client->ps.saberBlocked != BLOCKED_NONE )
	{
		return;
	}

	VectorSubtract( hitloc, self->client->renderInfo.eyePoint, diff );
	diff[2] = 0;
	VectorNormalize( diff );

	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors( fwdangles, NULL, right, NULL );

	rightdot = DotProduct(right, diff);
	zdiff = hitloc[2] - self->client->renderInfo.eyePoint[2];

	//FIXME: take torsoAngles into account?
	if ( zdiff > -5 )//0 )//40 )
	{
		if ( rightdot > 0.3 )
		{
			self->client->ps.saberBlocked = BLOCKED_UPPER_RIGHT;
		}
		else if ( rightdot < -0.3 )
		{
			self->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
		}
		else
		{
			self->client->ps.saberBlocked = BLOCKED_TOP;
		}
	}
	else if ( zdiff > -22 )//-20 )//20 )
	{
		if ( zdiff < -10 )//30 )
		{//hmm, pretty low, but not low enough to use the low block, so we need to duck
			//NPC should duck, but NPC should never get here
		}
		if ( rightdot > 0.1 )
		{
			self->client->ps.saberBlocked = BLOCKED_UPPER_RIGHT;
		}
		else if ( rightdot < -0.1 )
		{
			self->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
		}
		else
		{//FIXME: this looks really weird if the shot is too low!
			self->client->ps.saberBlocked = BLOCKED_TOP;
		}
	}
	else
	{
		if ( rightdot >= 0 )
		{
			self->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
		}
		else
		{
			self->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
		}
	}

#ifndef FINAL_BUILD
	if ( d_saberCombat->integer )
	{
		if ( !self->s.number )
		{
			gi.Printf( "EyeZ: %4.2f  HitZ: %4.2f  zdiff: %4.2f  rdot: %4.2f\n", self->client->renderInfo.eyePoint[2], hitloc[2], zdiff, rightdot );
			switch ( self->client->ps.saberBlocked )
			{
			case BLOCKED_TOP:
				gi.Printf( "BLOCKED_TOP\n" );
				break;
			case BLOCKED_UPPER_RIGHT:
				gi.Printf( "BLOCKED_UPPER_RIGHT\n" );
				break;
			case BLOCKED_UPPER_LEFT:
				gi.Printf( "BLOCKED_UPPER_LEFT\n" );
				break;
			case BLOCKED_LOWER_RIGHT:
				gi.Printf( "BLOCKED_LOWER_RIGHT\n" );
				break;
			case BLOCKED_LOWER_LEFT:
				gi.Printf( "BLOCKED_LOWER_LEFT\n" );
				break;
			default:
				break;
			}
		}
	}
#endif

	if ( missileBlock )
	{
		self->client->ps.saberBlocked = WP_MissileBlockForBlock( self->client->ps.saberBlocked );
	}

	if ( self->client->ps.saberBlocked != BLOCKED_NONE )
	{
		if ( !missileBlock	//missile-block cost is charged once at actual impact resolution in g_missile.cpp
							//instead: this function gets called predictively, once per frame, while an
							//incoming bolt is still approaching -- charging here could bill an early,
							//pre-timing-window frame even though the player goes on to land a perfect parry
			&& self->s.number == 0 && !g_saberAutoBlocking->integer
			&& !WP_SaberRules()	//saber rules: WP_SaberGuardStop charges the stop
			&& self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] < level.time
			&& WP_PlayerBlockChargeNew( self ) )
		{
			if ( WP_InPerfectParryWindowVs( self, self->enemy, missileBlock ) )
			{//perfect parries stay free (WP_PerfectParryReward gives the Force back - the old half-cost
			 //refund here paid it twice)
			}
			else
			{
				int cost = WP_SaberBlockForceCost( self, missileBlock );
				WP_ForcePowerDrain( self, FP_SABER_DEFENSE, cost );
				// Longer regen pause for saber attacks since they drain significantly more FP
				self->client->ps.forcePowerRegenDebounceTime = level.time + 3200;
			}
		}
		int parryReCalcTime = Jedi_ReCalcParryTime( self, missileBlock ? EVASION_MISSILE_PARRY : EVASION_PARRY );
		if ( self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime )
		{
			self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
		}
	}
}

void WP_SaberStartMissileBlockCheck( gentity_t *self, usercmd_t *ucmd  )
{
	float		dist;
	gentity_t	*ent, *incoming = NULL;
	gentity_t	*entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	int			i, e;
	float		closestDist, radius = 256;
	vec3_t		forward, dir, missile_dir, fwdangles = {0};
	trace_t		trace;
	vec3_t		traceTo, entDir;
	qboolean	dodgeOnlySabers = qfalse;

	if ( self->client && self->client->ps.saberAnimLevel == SS_STAFF )
	{//Staff has superior missile defense -- wider threat-detection radius
		radius = 320;
	}

	// Lightweight "is anything currently threatening me" signal for perfect-parry cooldown
	// selection (missile vs saber, see PM_AdjustAttackStates) — runs regardless of the
	// early-returns below, since those gate the actual block *attempt*, not whether the
	// player is still under fire while on cooldown. Deliberately simple: doesn't need to
	// match the full blockability filtering further down, just a rough proximity signal.
	if ( self->s.number == 0 && self->client )
	{
		vec3_t threatMins, threatMaxs;
		gentity_t *threatList[MAX_GENTITIES];
		for ( int ti = 0; ti < 3; ti++ )
		{
			threatMins[ti] = self->currentOrigin[ti] - 256;
			threatMaxs[ti] = self->currentOrigin[ti] + 256;
		}
		int numThreats = gi.EntitiesInBox( threatMins, threatMaxs, threatList, MAX_GENTITIES );
		for ( int ti = 0; ti < numThreats; ti++ )
		{
			gentity_t *tent = threatList[ti];
			if ( tent->inuse && tent->s.eType == ET_MISSILE
				&& !(tent->owner && OnSameTeam( self, tent->owner )) )
			{
				self->client->ps.lastNearbyMissileTime = level.time;
				break;
			}
		}
	}

	if ( self->NPC && (self->NPC->scriptFlags&SCF_IGNORE_ALERTS) )
	{//don't react to things flying at me...
		return;
	}
	if ( self->health <= 0 )
	{//dead don't try to block (NOTE: actual deflection happens in missile code)
		return;
	}

	if ( PM_InKnockDown( &self->client->ps ) )
	{//can't block when knocked down
		return;
	}

	if ( PM_SuperBreakLoseAnim( self->client->ps.torsoAnim )
		|| PM_SuperBreakWinAnim( self->client->ps.torsoAnim ) )
	{//can't block while in break anim
		return;
	}

	if ( Rosh_BeingHealed( self ) )
	{
		return;
	}

	if ( self->client->NPC_class == CLASS_ROCKETTROOPER )
	{//rockettrooper
		if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE )
		{//must be in air
			return;
		}
		if ( Q_irand( 0, 4-(g_spskill->integer*2) ) )
		{//easier level guys do this less
			return;
		}
		if ( Q_irand( 0, 3 ) )
		{//base level: 25% chance of looking for something to dodge
			if ( Q_irand( 0, 1 ) )
			{//dodge sabers twice as frequently as other projectiles
				dodgeOnlySabers = qtrue;
			}
			else
			{
				return;
			}
		}
	}

	if ( self->client->NPC_class == CLASS_BOBAFETT )
	{//Boba doesn't dodge quite as much
		if ( Q_irand( 0, 2-g_spskill->integer) )
		{//easier level guys do this less
			return;
		}
	}

	if ( self->client->NPC_class != CLASS_BOBAFETT
		&& (self->client->NPC_class != CLASS_REBORN || self->s.weapon == WP_SABER)
		&& (self->client->NPC_class != CLASS_ROCKETTROOPER||!self->NPC||self->NPC->rank<RANK_LT)//if a rockettrooper, but not an officer, do these normal checks
		)
	{
		if ( g_debugMelee->integer
			&& (ucmd->buttons & BUTTON_USE)
			&& cg.renderingThirdPerson
			&& G_OkayToLean( &self->client->ps, ucmd, qfalse )
			&& (self->client->ps.forcePowersActive&(1<<FP_SPEED)) )
		{
		}
		else
		{
			if ( self->client->ps.weapon != WP_SABER )
			{
				return;
			}

			if ( self->client->ps.saberInFlight )
			{
				return;
			}

			if ( self->s.number < MAX_CLIENTS )
			{
				if ( !self->client->ps.SaberLength() )
				{//player doesn't auto-activate
					return;
				}

				if ( !g_saberAutoBlocking->integer && self->client->ps.saberBlockingTime<level.time )
				{
					return;
				}
				// No force power = cannot deflect missiles
				if ( !g_saberAutoBlocking->integer && self->client->ps.forcePower <= 0 )
				{
					if ( self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] < level.time )
					{
						G_SoundOnEnt( self, CHAN_AUTO, "sound/weapons/force/noforce.mp3" );
						self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 1000;
					}
					return;
				}
			}

			if ( (self->client->ps.saber[0].saberFlags&SFL_NOT_ACTIVE_BLOCKING) )
			{//can't actively block with this saber type
				return;
			}
		}

		if ( !self->s.number )
		{//don't do this if already attacking!
			if ( ucmd->buttons & BUTTON_ATTACK
				|| PM_SaberInAttack( self->client->ps.saberMove )
				|| PM_SaberInSpecialAttack( self->client->ps.torsoAnim )
				|| PM_SaberInTransitionAny( self->client->ps.saberMove ))
			{
				return;
			}
		}

		if ( self->client->ps.forcePowerLevel[FP_SABER_DEFENSE] < FORCE_LEVEL_1 )
		{//you have not the SKILLZ
			return;
		}

		if ( self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] > level.time )
		{//can't block while already blocking
			return;
		}

		if ( self->client->ps.forcePowersActive&(1<<FP_LIGHTNING) )
		{//can't block while zapping
			return;
		}

		if ( self->client->ps.forcePowersActive&(1<<FP_DRAIN) )
		{//can't block while draining
			return;
		}

		if ( self->client->ps.forcePowersActive&(1<<FP_PUSH) )
		{//can't block while shoving
			return;
		}

		if ( self->client->ps.forcePowersActive&(1<<FP_GRIP) )
		{//can't block while gripping (FIXME: or should it break the grip?  Pain should break the grip, I think...)
			return;
		}
	}

	fwdangles[1] = self->client->ps.viewangles[1];
	AngleVectors( fwdangles, forward, NULL, NULL );

	for ( i = 0 ; i < 3 ; i++ )
	{
		mins[i] = self->currentOrigin[i] - radius;
		maxs[i] = self->currentOrigin[i] + radius;
	}

	numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	closestDist = radius;

	for ( e = 0 ; e < numListedEntities ; e++ )
	{
		ent = entityList[ e ];

		if (ent == self)
			continue;
		if (ent->owner == self)
			continue;
		if ( ent->owner && ent->owner->client && OnSameTeam( self, ent->owner ) )
			continue;
		if ( !(ent->inuse) )
			continue;
		if ( dodgeOnlySabers )
		{//only care about thrown sabers
			if ( ent->client
				|| ent->s.weapon != WP_SABER
				|| !ent->classname
				|| !ent->classname[0]
				|| Q_stricmp( "lightsaber", ent->classname ) )
			{//not a lightsaber, ignore it
				continue;
			}
		}
		if ( ent->s.eType != ET_MISSILE && !(ent->s.eFlags&EF_MISSILE_STICK) )
		{//not a normal projectile
			if ( ent->client || ent->s.weapon != WP_SABER )
			{//FIXME: wake up bad guys?
				continue;
			}
			if ( ent->s.eFlags & EF_NODRAW )
			{
				continue;
			}
			if ( Q_stricmp( "lightsaber", ent->classname ) != 0 )
			{//not a lightsaber
				//FIXME: what about general objects that are small in size- like rocks, etc...
				continue;
			}
			//a lightsaber.. make sure it's on and inFlight
			if ( !ent->owner || !ent->owner->client )
			{
				continue;
			}
			if ( !ent->owner->client->ps.saberInFlight )
			{//not in flight
				continue;
			}
			if ( ent->owner->client->ps.SaberLength() <= 0 )
			{//not on
				continue;
			}
			if ( ent->owner->health <= 0 && g_saberRealisticCombat->integer < 2 )
			{//it's not doing damage, so ignore it
				continue;
			}
		}
		else
		{
			if ( ent->s.pos.trType == TR_STATIONARY && !self->s.number )
			{//nothing you can do with a stationary missile if you're the player
				continue;
			}
		}

		float		dot1, dot2;
		//see if they're in front of me
		VectorSubtract( ent->currentOrigin, self->currentOrigin, dir );
		dist = VectorNormalize( dir );
		//FIXME: handle detpacks, proximity mines and tripmines
		if ( ent->s.weapon == WP_THERMAL )
		{//thermal detonator!
			if ( self->NPC && dist < ent->splashRadius )
			{
				if ( dist < ent->splashRadius &&
					ent->nextthink < level.time + 600 &&
					ent->count &&
					self->client->ps.groundEntityNum != ENTITYNUM_NONE &&
						(ent->s.pos.trType == TR_STATIONARY||
						ent->s.pos.trType == TR_INTERPOLATE||
						(dot1 = DotProduct( dir, forward )) < SABER_REFLECT_MISSILE_CONE||
						!WP_ForcePowerUsable( self, FP_PUSH, 0 )) )
				{//TD is close enough to hurt me, I'm on the ground and the thing is at rest or behind me and about to blow up, or I don't have force-push so force-jump!
					//FIXME: sometimes this might make me just jump into it...?
					self->client->ps.forceJumpCharge = 480;
				}
				else if ( self->client->NPC_class != CLASS_BOBAFETT
					&& (self->client->NPC_class != CLASS_REBORN || self->s.weapon == WP_SABER)
					&& self->client->NPC_class != CLASS_ROCKETTROOPER )
				{//FIXME: check forcePushRadius[NPC->client->ps.forcePowerLevel[FP_PUSH]]
					if ( !ent->owner || !OnSameTeam( self, ent->owner ) )
					{
						ForceThrow( self, qfalse );
					}
				}
			}
			continue;
		}
		else if ( ent->splashDamage && ent->splashRadius && !(ent->s.powerups & (1<<PW_FORCE_PROJECTILE)) )
		{//exploding missile
			//FIXME: handle tripmines and detpacks somehow...
			//			maybe do a force-gesture that makes them explode?
			//			But what if we're within it's splashradius?
			if ( !self->s.number )
			{//players don't auto-handle these at all
				continue;
			}
			else
			{
				if ( self->client->NPC_class == CLASS_BOBAFETT
					|| self->client->NPC_class == CLASS_ROCKETTROOPER )
				{
					/*
					if ( ent->s.pos.trType == TR_STATIONARY && (ent->s.eFlags&EF_MISSILE_STICK) )
					{//sorry, you're scrooged here
						//FIXME: maybe jump or go up if on ground?
						continue;
					}
					//else it's a rocket, try to evade it
					*/
					//HMM... let's see what happens if these guys try to avoid tripmines and detpacks, too...?
				}
				else
				{//normal Jedi
					if ( ent->s.pos.trType == TR_STATIONARY && (ent->s.eFlags&EF_MISSILE_STICK)
						&& (self->client->NPC_class != CLASS_REBORN || self->s.weapon == WP_SABER) )
					{//a placed explosive like a tripmine or detpack
						if ( InFOV( ent->currentOrigin, self->client->renderInfo.eyePoint, self->client->ps.viewangles, 90, 90 ) )
						{//in front of me
							if ( G_ClearLOS( self, ent ) )
							{//can see it
								vec3_t throwDir;
								//make the gesture
								ForceThrow( self, qfalse );
								//take it off the wall and toss it
								ent->s.pos.trType = TR_GRAVITY;
								ent->s.eType = ET_MISSILE;
								ent->s.eFlags &= ~EF_MISSILE_STICK;
								ent->s.eFlags |= EF_BOUNCE_HALF;
								AngleVectors( ent->currentAngles, throwDir, NULL, NULL );
								VectorMA( ent->currentOrigin, ent->maxs[0]+4, throwDir, ent->currentOrigin );
								VectorCopy( ent->currentOrigin, ent->s.pos.trBase );
								VectorScale( throwDir, 300, ent->s.pos.trDelta );
								ent->s.pos.trDelta[2] += 150;
								VectorMA( ent->s.pos.trDelta, 800, dir, ent->s.pos.trDelta );
								ent->s.pos.trTime = level.time;		// move a bit on the very first frame
								VectorCopy( ent->currentOrigin, ent->s.pos.trBase );
								ent->owner = self;
								// make it explode, but with less damage
								ent->splashDamage /= 3;
								ent->splashRadius /= 3;
								ent->e_ThinkFunc = thinkF_WP_Explode;
								ent->nextthink = level.time + Q_irand( 500, 3000 );
							}
						}
					}
					else if ( dist < ent->splashRadius
						&& self->client->ps.groundEntityNum != ENTITYNUM_NONE
						&& ( DotProduct( dir, forward ) < SABER_REFLECT_MISSILE_CONE
							|| !WP_ForcePowerUsable( self, FP_PUSH, 0 ) ) )
					{//NPCs try to evade it
						self->client->ps.forceJumpCharge = 480;
					}
					else if ( (self->client->NPC_class != CLASS_REBORN || self->s.weapon == WP_SABER) )
					{//else, try to force-throw it away
						if ( !ent->owner || !OnSameTeam( self, ent->owner ) )
						{
							//FIXME: check forcePushRadius[NPC->client->ps.forcePowerLevel[FP_PUSH]]
							ForceThrow( self, qfalse );
						}
					}
					//otherwise, can't block it, so we're screwed
					continue;
				}
			}
		}

		if ( ent->s.weapon != WP_SABER )
		{//only block shots coming from behind
			if ( (dot1 = DotProduct( dir, forward )) < SABER_REFLECT_MISSILE_CONE )
				continue;
		}
		else if ( !self->s.number )
		{//player never auto-blocks thrown sabers
			continue;
		}//NPCs always try to block sabers coming from behind!

		//see if they're heading towards me
		VectorCopy( ent->s.pos.trDelta, missile_dir );
		VectorNormalize( missile_dir );
		if ( (dot2 = DotProduct( dir, missile_dir )) > 0 )
			continue;

		//FIXME: must have a clear trace to me, too...
		if ( dist < closestDist )
		{
			VectorCopy( self->currentOrigin, traceTo );
			traceTo[2] = self->absmax[2] - 4;
			gi.trace( &trace, ent->currentOrigin, ent->mins, ent->maxs, traceTo, ent->s.number, ent->clipmask, (EG2_Collision)0, 0 );
			if ( trace.allsolid || trace.startsolid || (trace.fraction < 1.0f && trace.entityNum != self->s.number && trace.entityNum != self->client->ps.saberEntityNum) )
			{//okay, try one more check
				VectorNormalize2( ent->s.pos.trDelta, entDir );
				VectorMA( ent->currentOrigin, radius, entDir, traceTo );
				gi.trace( &trace, ent->currentOrigin, ent->mins, ent->maxs, traceTo, ent->s.number, ent->clipmask, (EG2_Collision)0, 0 );
				if ( trace.allsolid || trace.startsolid || (trace.fraction < 1.0f && trace.entityNum != self->s.number && trace.entityNum != self->client->ps.saberEntityNum) )
				{//can't hit me, ignore it
					continue;
				}
			}
			if ( self->s.number != 0 )
			{//An NPC
				if ( self->NPC && !self->enemy && ent->owner )
				{
					if ( ent->owner->health >= 0 && (!ent->owner->client || ent->owner->client->playerTeam != self->client->playerTeam) )
					{
						G_SetEnemy( self, ent->owner );
					}
				}
			}
			//FIXME: if NPC, predict the intersection between my current velocity/path and the missile's, see if it intersects my bounding box (+/-saberLength?), don't try to deflect unless it does?
			closestDist = dist;
			incoming = ent;
		}
	}

	if ( incoming )
	{
		if ( self->NPC && !G_ControlledByPlayer( self ) )
		{
			if ( Jedi_WaitingAmbush( self ) )
			{
				Jedi_Ambush( self );
			}
			if ( ( self->client->NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_ROCKETTROOPER )
				&& self->client->moveType == MT_FLYSWIM
				&& incoming->methodOfDeath != MOD_ROCKET_ALT )
			{//a hovering Boba Fett, not a tracking rocket
				if ( !Q_irand( 0, 1 ) )
				{//strafe
					self->NPC->standTime = 0;
					self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + Q_irand( 1000, 2000 );
				}
				if ( !Q_irand( 0, 1 ) )
				{//go up/down
					TIMER_Set( self, "heightChange", Q_irand( 1000, 3000 ) );
					self->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + Q_irand( 1000, 2000 );
				}
			}
			else if ( self->client->NPC_class != CLASS_ROCKETTROOPER
				&& Jedi_SaberBlockGo( self, &self->NPC->last_ucmd, NULL, NULL, incoming ) != EVASION_NONE )
			{//make sure to turn on your saber if it's not on
				if ( self->client->NPC_class != CLASS_BOBAFETT
					&& (self->client->NPC_class != CLASS_REBORN || self->s.weapon == WP_SABER) )
				{
					self->client->ps.SaberActivate();
				}
			}
		}
		else//player
		{
			if ( !(ucmd->buttons & BUTTON_USE) )//self->s.weapon == WP_SABER && self->client->ps.SaberActive() )
			{
				// Predict where the bolt will be when it reaches the player rather than using
				// its current position — prevents quadrant mismatch at oblique angles.
				vec3_t predictedHitLoc;
				float boltSpeed = VectorLength( incoming->s.pos.trDelta );
				if ( boltSpeed > 1.0f )
				{
					float boltDist = Distance( incoming->currentOrigin, self->client->renderInfo.eyePoint );
					VectorMA( incoming->currentOrigin, boltDist / boltSpeed, incoming->s.pos.trDelta, predictedHitLoc );
				}
				else
				{
					VectorCopy( incoming->currentOrigin, predictedHitLoc );
				}
				WP_SaberBlockNonRandom( self, predictedHitLoc, qtrue );
			}
			else
			{
				vec3_t diff, start, end;
				float dist;
				VectorSubtract( incoming->currentOrigin, self->currentOrigin, diff );
				dist = VectorLength( diff );
				VectorNormalize2( incoming->s.pos.trDelta, entDir );
				VectorMA( incoming->currentOrigin, dist, entDir, start );
				VectorCopy( self->currentOrigin, end );
				end[2] += self->maxs[2]*0.75f;
				gi.trace( &trace, start, incoming->mins, incoming->maxs, end, incoming->s.number, MASK_SHOT, G2_COLLIDE, 10 );

				Jedi_DodgeEvasion( self, incoming->owner, &trace, HL_NONE );
			}
			if ( incoming->owner && incoming->owner->client && (!self->enemy || self->enemy->s.weapon != WP_SABER) )//keep enemy jedi over shooters
			{
				self->enemy = incoming->owner;
				NPC_SetLookTarget( self, incoming->owner->s.number, level.time+1000 );
			}
		}
	}
}


//GENERAL SABER============================================================================
//GENERAL SABER============================================================================
//GENERAL SABER============================================================================
//GENERAL SABER============================================================================
//GENERAL SABER============================================================================

void WP_SetSaberMove(gentity_t *self, short blocked)
{
	self->client->ps.saberBlocked = blocked;
}

extern void CG_CubeOutline( vec3_t mins, vec3_t maxs, int time, unsigned int color, float alpha );
void WP_SaberUpdate( gentity_t *self, usercmd_t *ucmd )
{
	//float	swap;
	float	minsize = 16;

	if(0)//	if ( self->s.number != 0 )
	{//for now only the player can do this		// not anymore
		return;
	}

	if ( !self->client )
	{
		return;
	}

	WP_SaberTrackSwingRun( self );

	if ( self->client->ps.saberEntityNum < 0 || self->client->ps.saberEntityNum >= ENTITYNUM_WORLD )
	{//never got one
		return;
	}

	// Check if we want to fire the pistol
	WP_SaberFireGun(self, ucmd, WP_BRYAR_PISTOL);
	
	// Check if we are throwing it, launch it if needed, update position if needed.
	WP_SaberThrow(self, ucmd);


	//vec3_t saberloc;
	//vec3_t sabermins={-8,-8,-8}, sabermaxs={8,8,8};

	gentity_t *saberent;

	if (self->client->ps.saberEntityNum <= 0)
	{//WTF?!!  We lost it?
		return;
	}

	saberent = &g_entities[self->client->ps.saberEntityNum];

	//FIXME: Based on difficulty level/jedi saber combat skill, make this bounding box fatter/smaller
	if ( self->client->ps.saberBlocked != BLOCKED_NONE )
	{//we're blocking, increase min size
		minsize = 32;
	}

	if ( G_InCinematicSaberAnim( self ) )
	{//fake some blocking
		self->client->ps.saberBlocking = BLK_TIGHT;
		if ( self->client->ps.saber[0].Active() )
		{
			self->client->ps.saber[0].ActivateTrail( 150 );
		}
		if ( self->client->ps.saber[1].Active() )
		{
			self->client->ps.saber[1].ActivateTrail( 150 );
		}
	}

	if ( WP_SaberRules() && !self->client->ps.saberBlocking && WP_SaberGuardUp( self ) )
	{//JKEnhanced: a raised guard always has its box, even in a saber move that has none
		self->client->ps.saberBlocking = BLK_WIDE;
	}

	//is our saber in flight?
	if ( !self->client->ps.saberInFlight )
	{	// It isn't, which means we can update its position as we will.
		qboolean alwaysBlock[MAX_SABERS][MAX_BLADES];
		qboolean forceBlock = qfalse;
		qboolean noBlocking = qfalse;

		//clear out last frame's numbers
		VectorClear(saberent->mins);
		VectorClear(saberent->maxs);

		Vehicle_t *pVeh = G_IsRidingVehicle( self );
		if ( !self->client->ps.SaberActive()
			|| !self->client->ps.saberBlocking
			|| PM_InKnockDown( &self->client->ps )
			|| PM_SuperBreakLoseAnim( self->client->ps.torsoAnim )
			|| (pVeh && pVeh->m_pVehicleInfo && pVeh->m_pVehicleInfo->type != VH_ANIMAL && pVeh->m_pVehicleInfo->type != VH_FLIER) )//riding a vehicle that you cannot block shots on
		{//can't block if saber isn't on
			int i, j;
			for ( i = 0; i < MAX_SABERS; i++ )
			{
				//initialize to not blocking
				for ( j = 0; j < MAX_BLADES; j++ )
				{
					alwaysBlock[i][j] = qfalse;
				}
				if ( i > 0 && !self->client->ps.dualSabers )
				{//not using a second saber, leave it not blocking
				}
				else
				{
					if ( (self->client->ps.saber[i].saberFlags2&SFL2_ALWAYS_BLOCK) )
					{
						for ( j = 0; j < self->client->ps.saber[i].numBlades; j++ )
						{
							alwaysBlock[i][j] = qtrue;
							forceBlock = qtrue;
						}
					}
					if ( self->client->ps.saber[i].bladeStyle2Start > 0 )
					{
						for ( j = self->client->ps.saber[i].bladeStyle2Start; j < self->client->ps.saber[i].numBlades; j++ )
						{
							if ( (self->client->ps.saber[i].saberFlags2&SFL2_ALWAYS_BLOCK2) )
							{
								alwaysBlock[i][j] = qtrue;
								forceBlock = qtrue;
							}
							else
							{
								alwaysBlock[i][j] = qfalse;
							}
						}
					}
				}
			}
			if ( !forceBlock )
			{
				noBlocking = qtrue;
			}
			else if ( !self->client->ps.saberBlocking )
			{//turn blocking on!
				self->client->ps.saberBlocking = BLK_TIGHT;
			}
		}
		if ( noBlocking )
		{
			//VectorClear(saberent->mins);
			//VectorClear(saberent->maxs);
			G_SetOrigin(saberent, self->currentOrigin);
		}
		else if ( self->client->ps.saberBlocking == BLK_TIGHT
			|| self->client->ps.saberBlocking == BLK_WIDE )
		{//FIXME: keep bbox in front of player, even when wide?
			vec3_t	saberOrg;
			//JKEnhanced saber rules: a raised guard covers the body whatever the saber anim is doing
			//(a parry's recovery used to shrink this to the blade, so the next swing of a flurry got through)
			const qboolean guardBox = (qboolean)( WP_SaberRules() && WP_SaberGuardUp( self ) );
			if ( !forceBlock
				&& ( guardBox
					|| ( ( (self->s.number&&!Jedi_SaberBusy(self)&&!g_saberRealisticCombat->integer) || (self->s.number == 0 && self->client->ps.saberBlocking == BLK_WIDE && (g_saberAutoBlocking->integer||(self->client->ps.saberBlockingTime>level.time&&self->client->ps.forcePower>0))) )
						&& self->client->ps.weaponTime <= 0 ) )
				&& !G_InCinematicSaberAnim( self ) )
			{//full-size blocking for non-attacking player with g_saberAutoBlocking on
				vec3_t saberang={0,0,0}, fwd, sabermins={-8,-8,-8}, sabermaxs={8,8,8};

				saberang[YAW] = self->client->ps.viewangles[YAW];
				AngleVectors( saberang, fwd, NULL, NULL );

				VectorMA( self->currentOrigin, 12, fwd, saberOrg );

				VectorAdd( self->mins, sabermins, saberent->mins );
				VectorAdd( self->maxs, sabermaxs, saberent->maxs );

				saberent->contents = CONTENTS_LIGHTSABER;

				G_SetOrigin( saberent, saberOrg );
			}
			else
			{
				vec3_t	saberBase, saberTip;
				int numSabers = 1;
				if ( self->client->ps.dualSabers )
				{
					numSabers = 2;
				}
				for ( int saberNum = 0; saberNum < numSabers; saberNum++ )
				{
					for ( int bladeNum = 0; bladeNum < self->client->ps.saber[saberNum].numBlades; bladeNum++ )
					{
						if ( self->client->ps.saber[saberNum].blade[bladeNum].length <= 0.0f )
						{//don't include blades that are not on...
							continue;
						}
						if ( forceBlock )
						{//doing blade-specific bbox-sizing only, see if this blade should be counted
							if ( !alwaysBlock[saberNum][bladeNum] )
							{//this blade doesn't count right now
								continue;
							}
						}
						VectorCopy( self->client->ps.saber[saberNum].blade[bladeNum].muzzlePoint, saberBase );
						VectorMA( saberBase, self->client->ps.saber[saberNum].blade[bladeNum].length, self->client->ps.saber[saberNum].blade[bladeNum].muzzleDir, saberTip );
						VectorMA( saberBase, self->client->ps.saber[saberNum].blade[bladeNum].length*0.5, self->client->ps.saber[saberNum].blade[bladeNum].muzzleDir, saberOrg );
						for ( int i = 0; i < 3; i++ )
						{
							/*
							if ( saberTip[i] > self->client->ps.saber[saberNum].blade[bladeNum].muzzlePoint[i] )
							{
								saberent->maxs[i] = saberTip[i] - saberOrg[i] + 8;//self->client->renderInfo.muzzlePoint[i];
								saberent->mins[i] = self->client->ps.saber[saberNum].blade[bladeNum].muzzlePoint[i] - saberOrg[i] - 8;
							}
							else //if ( saberTip[i] < self->client->renderInfo.muzzlePoint[i] )
							{
								saberent->maxs[i] = self->client->ps.saber[saberNum].blade[bladeNum].muzzlePoint[i] - saberOrg[i] + 8;
								saberent->mins[i] = saberTip[i] - saberOrg[i] - 8;//self->client->ps.saber[saberNum].blade[bladeNum].muzzlePoint[i];
							}
							*/
							float newSizeTip = (saberTip[i] - saberOrg[i]);
							newSizeTip += (newSizeTip>=0)?8:-8;
							float newSizeBase = (saberBase[i] - saberOrg[i]);
							newSizeBase += (newSizeBase>=0)?8:-8;
							if ( newSizeTip > saberent->maxs[i] )
							{
								saberent->maxs[i] = newSizeTip;
							}
							if ( newSizeBase > saberent->maxs[i] )
							{
								saberent->maxs[i] = newSizeBase;
							}
							if ( newSizeTip < saberent->mins[i] )
							{
								saberent->mins[i] = newSizeTip;
							}
							if ( newSizeBase < saberent->mins[i] )
							{
								saberent->mins[i] = newSizeBase;
							}
						}
					}
				}
				if ( !forceBlock )
				{//not doing special "alwaysBlock" bbox
					if ( self->client->ps.weaponTime > 0
						|| self->s.number
						|| g_saberAutoBlocking->integer
						|| (self->client->ps.saberBlockingTime > level.time && self->client->ps.forcePower > 0) )
					{//if attacking or blocking (or an NPC), inflate to a minimum size
						for ( int i = 0; i < 3; i++ )
						{
							if ( saberent->maxs[i] < minsize )
							{
								saberent->maxs[i] = minsize;
							}
							if ( saberent->mins[i] > -minsize )
							{
								saberent->mins[i] = -minsize;
							}
						}
					}
				}
				saberent->contents = CONTENTS_LIGHTSABER;
				G_SetOrigin( saberent, saberOrg );
			}
		}
		/*
		else if (self->client->ps.saberBlocking == BLK_WIDE)
		{	// Assuming that we are not swinging, the saber's bounding box should be around the player.
			vec3_t saberang={0,0,0}, fwd;

			saberang[YAW] = self->client->ps.viewangles[YAW];
			AngleVectors( saberang, fwd, NULL, NULL );

			VectorMA(self->currentOrigin, 12, fwd, saberloc);

			VectorAdd(self->mins, sabermins, saberent->mins);
			VectorAdd(self->maxs, sabermaxs, saberent->maxs);

			saberent->contents = CONTENTS_LIGHTSABER;

			G_SetOrigin( saberent, saberloc);
		}
		else if (self->client->ps.saberBlocking == BLK_TIGHT)
		{	// If the player is swinging, the bbox is around just the saber
			VectorCopy(self->client->renderInfo.muzzlePoint, sabermins);
			// Put the limits of the bbox around the saber size.
			VectorMA(sabermins, self->client->ps.saberLength, self->client->renderInfo.muzzleDir, sabermaxs);

			// Now make the mins into mins and the maxs into maxs
			if (sabermins[0] > sabermaxs[0])
			{
				swap = sabermins[0];
				sabermins[0] = sabermaxs[0];
				sabermaxs[0] = swap;
			}
			if (sabermins[1] > sabermaxs[1])
			{
				swap = sabermins[1];
				sabermins[1] = sabermaxs[1];
				sabermaxs[1] = swap;
			}
			if (sabermins[2] > sabermaxs[2])
			{
				swap = sabermins[2];
				sabermins[2] = sabermaxs[2];
				sabermaxs[2] = swap;
			}

			// Now the loc is halfway between the (absolute) mins and maxs
			VectorAdd(sabermins, sabermaxs, saberloc);
			VectorScale(saberloc, 0.5, saberloc);

			// Finally, turn the mins and maxs, which are absolute, into relative mins and maxs.
			VectorSubtract(sabermins, saberloc, saberent->mins);
			VectorSubtract(sabermaxs, saberloc, saberent->maxs);

			saberent->contents = CONTENTS_LIGHTSABER;// | CONTENTS_SHOTCLIP;

			G_SetOrigin( saberent, saberloc);
		}
		*/
		else
		{	// Otherwise there is no blocking possible.
			//VectorClear(saberent->mins);
			//VectorClear(saberent->maxs);
			G_SetOrigin(saberent, self->currentOrigin);
		}
		saberent->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
		gi.linkentity(saberent);
	}
	else
	{
		WP_SaberInFlightReflectCheck( self, ucmd );
	}
#ifndef FINAL_BUILD
	if ( d_saberCombat->integer > 2 )
	{
		CG_CubeOutline( saberent->absmin, saberent->absmax, 50, WPDEBUG_SaberColor( self->client->ps.saber[0].blade[0].color ), 1 );
	}
#endif
}

#define	MAX_RADIUS_ENTS			256	//NOTE: This can cause entities to be lost
qboolean G_CheckEnemyPresence( gentity_t *ent, int dir, float radius, float tolerance )
{
	gentity_t	*radiusEnts[ MAX_RADIUS_ENTS ];
	vec3_t		mins, maxs;
	int			numEnts;
	vec3_t		checkDir, dir2checkEnt;
	float		dist;
	int			i;

	switch( dir )
	{
	case DIR_RIGHT:
		AngleVectors( ent->currentAngles, NULL, checkDir, NULL );
		break;
	case DIR_LEFT:
		AngleVectors( ent->currentAngles, NULL, checkDir, NULL );
		VectorScale( checkDir, -1, checkDir );
		break;
	case DIR_FRONT:
		AngleVectors( ent->currentAngles, checkDir, NULL, NULL );
		break;
	case DIR_BACK:
		AngleVectors( ent->currentAngles, checkDir, NULL, NULL );
		VectorScale( checkDir, -1, checkDir );
		break;
	}
	//Get all ents in range, see if they're living clients and enemies, then check dot to them...

	//Setup the bbox to search in
	for ( i = 0; i < 3; i++ )
	{
		mins[i] = ent->currentOrigin[i] - radius;
		maxs[i] = ent->currentOrigin[i] + radius;
	}

	//Get a number of entities in a given space
	numEnts = gi.EntitiesInBox( mins, maxs, radiusEnts, MAX_RADIUS_ENTS );

	for ( i = 0; i < numEnts; i++ )
	{
		//Don't consider self
		if ( radiusEnts[i] == ent )
			continue;

		//Must be valid
		if ( G_ValidEnemy( ent, radiusEnts[i] ) == qfalse )
			continue;

		VectorSubtract( radiusEnts[i]->currentOrigin, ent->currentOrigin, dir2checkEnt );
		dist = VectorNormalize( dir2checkEnt );
		if ( dist <= radius
			&& DotProduct( dir2checkEnt, checkDir ) >= tolerance )
		{
			//stop on the first one
			return qtrue;
		}
	}

	return qfalse;
}
//OTHER JEDI POWERS=========================================================================
//OTHER JEDI POWERS=========================================================================
//OTHER JEDI POWERS=========================================================================
//OTHER JEDI POWERS=========================================================================
//OTHER JEDI POWERS=========================================================================
extern gentity_t *TossClientItems( gentity_t *self );
extern void ChangeWeapon( gentity_t *ent, int newWeapon );
void WP_DropWeapon( gentity_t *dropper, vec3_t velocity )
{
	if ( !dropper || !dropper->client )
	{
		return;
	}
	qboolean neutralize = qfalse;
	if ( dropper->NPC && dropper->health > 0 )
	{//losing your gun is a bad moment
		AI_NerveEvent( dropper, NE_DISARMED, 1.0f, NULL );
		AI_NerveWitness( dropper, NE_DISARMED, NULL );
		if ( dropper->s.number >= MAX_CLIENTS && !G_IsNeutralized( dropper )
			&& AI_NerveTier( dropper ) != NT_STEADY
			&& dropper->s.weapon != WP_NONE && dropper->s.weapon != WP_MELEE && dropper->s.weapon != WP_SABER
			&& G_CanNeutralize( dropper, NH_DISARM ) )
		{//disarmed and already rattled: that's the end of their fight (shock, then hands up or run).
		 //Done below, after the toss: G_Neutralize drops the gun itself, which used to leave nothing
		 //here to throw (a pulled gun didn't fly to you) and the weapon slot uncleared
			neutralize = qtrue;
		}
	}
	int	replaceWeap = WP_NONE;
	int oldWeap = dropper->s.weapon;
	gentity_t *weapon = TossClientItems( dropper );
	if ( oldWeap == WP_THERMAL && dropper->NPC )
	{//Hmm, maybe all NPCs should go into melee?  Not too many, though, or they mob you and look silly
		replaceWeap = WP_MELEE;
	}
	if ( dropper->ghoul2.IsValid() )
	{
		if ( dropper->weaponModel[0] > 0 )
		{//NOTE: guess you never drop the left-hand weapon, eh?
			gi.G2API_RemoveGhoul2Model( dropper->ghoul2, dropper->weaponModel[0] );
			dropper->weaponModel[0] = -1;
		}
	}
	//FIXME: does this work on the player?
	dropper->client->ps.weapons[replaceWeap] = 1;
	if ( !dropper->s.number )
	{
		if ( oldWeap == WP_THERMAL )
		{
			dropper->client->ps.ammo[weaponData[oldWeap].ammoIndex] -= weaponData[oldWeap].energyPerShot;
		}
		else
		{
			dropper->client->ps.weapons[oldWeap] = 0;
		}
		CG_ChangeWeapon( replaceWeap );
	}
	else
	{
		dropper->client->ps.weapons[oldWeap] = 0;
	}
	ChangeWeapon( dropper, replaceWeap );
	dropper->s.weapon = replaceWeap;
	if ( dropper->NPC )
	{
		dropper->NPC->last_ucmd.weapon = replaceWeap;
	}
	if ( weapon != NULL && velocity && !VectorCompare( velocity, vec3_origin ) )
	{//weapon should have a direction to it's throw
		VectorScale( velocity, 3, weapon->s.pos.trDelta );//NOTE: Presumes it is moving already...?
		if ( weapon->s.pos.trDelta[2] < 150 )
		{//this is presuming you don't want them to drop the weapon down on you...
			weapon->s.pos.trDelta[2] = 150;
		}
		//FIXME: gets stuck inside it's former owner...
		weapon->forcePushTime = level.time + 600; // let the push effect last for 600 ms
	}
	if ( neutralize )
	{//(the flee below would override their shock / hands-up)
		G_Neutralize( dropper, dropper->enemy, NH_DISARM );
	}
	else if ( dropper->NPC )
	{
		if ( replaceWeap == WP_NONE && dropper->health > 0 )
		{//actually left unarmed (not the thermal->melee swap) -- react immediately instead of
		//waiting for a class-specific AI tick to eventually notice weapon==WP_NONE
			G_StartFlee( dropper, dropper->enemy, dropper->enemy ? dropper->enemy->currentOrigin : dropper->currentOrigin, AEL_DANGER_GREAT, 5000, 10000 );
		}
	}
}

void WP_KnockdownTurret( gentity_t *self, gentity_t *pas )
{
	//knock it over
	VectorCopy( pas->currentOrigin, pas->s.pos.trBase );
	pas->s.pos.trType = TR_LINEAR_STOP;
	pas->s.pos.trDuration = 250;
	pas->s.pos.trTime = level.time;
	pas->s.pos.trDelta[2] = ( 12.0f / ( pas->s.pos.trDuration * 0.001f ) );

	VectorCopy( pas->currentAngles, pas->s.apos.trBase );
	pas->s.apos.trType = TR_LINEAR_STOP;
	pas->s.apos.trDuration = 250;
	pas->s.apos.trTime = level.time;
	//FIXME: pick pitch/roll that always tilts it directly away from pusher
	pas->s.apos.trDelta[PITCH] = ( 100.0f / ( pas->s.apos.trDuration * 0.001f ) );

	//kill it
	pas->count = 0;
	pas->nextthink = -1;
	G_Sound( pas, G_SoundIndex( "sound/chars/turret/shutdown.wav" ));
	//push effect?
	pas->forcePushTime = level.time + 600; // let the push effect last for 600 ms
}

void WP_ForceThrowHazardTrooper( gentity_t *self, gentity_t *trooper, qboolean pull )
{
	if ( !self || !self->client )
	{
		return;
	}
	if ( !trooper || !trooper->client )
	{
		return;
	}

	//all levels: see effect on them, they notice us
	trooper->forcePushTime = level.time + 600; // let the push effect last for 600 ms

	if ( (pull&&self->client->ps.forcePowerLevel[FP_PULL]>FORCE_LEVEL_1)
		|| (!pull&&self->client->ps.forcePowerLevel[FP_PUSH]>FORCE_LEVEL_1) )
	{//level 2: they stop for a couple seconds and make a sound
		trooper->painDebounceTime = level.time + Q_irand( 1500, 2500 );
		G_AddVoiceEvent( trooper, Q_irand(EV_PUSHED1, EV_PUSHED3), Q_irand( 1000, 3000 ) );
		GEntity_PainFunc( trooper, self, self, trooper->currentOrigin, 0, MOD_MELEE );

		if ( (pull&&self->client->ps.forcePowerLevel[FP_PULL]>FORCE_LEVEL_2)
			|| (!pull&&self->client->ps.forcePowerLevel[FP_PUSH]>FORCE_LEVEL_2) )
		{//level 3: they actually play a pushed anim and stumble a bit
			vec3_t hazAngles = {0,trooper->currentAngles[YAW],0};
			int anim = -1;
			if ( InFront( self->currentOrigin, trooper->currentOrigin, hazAngles ) )
			{//I'm on front of him
				if ( pull )
				{
					anim = BOTH_PAIN4;
				}
				else
				{
					anim = BOTH_PAIN1;
				}
			}
			else
			{//I'm behind him
				if ( pull )
				{
					anim = BOTH_PAIN1;
				}
				else
				{
					anim = BOTH_PAIN4;
				}
			}
			if ( anim != -1 )
			{
				if ( anim == BOTH_PAIN1 )
				{//make them take a couple steps back
					AngleVectors( hazAngles, trooper->client->ps.velocity, NULL, NULL );
					VectorScale( trooper->client->ps.velocity, -40.0f, trooper->client->ps.velocity );
					trooper->client->ps.pm_flags |= PMF_TIME_NOFRICTION;
				}
				else if ( anim == BOTH_PAIN4 )
				{//make them stumble forward
					AngleVectors( hazAngles, trooper->client->ps.velocity, NULL, NULL );
					VectorScale( trooper->client->ps.velocity, 80.0f, trooper->client->ps.velocity );
					trooper->client->ps.pm_flags |= PMF_TIME_NOFRICTION;
				}
				NPC_SetAnim( trooper, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				trooper->painDebounceTime += trooper->client->ps.torsoAnimTimer;
				trooper->client->ps.pm_time = trooper->client->ps.torsoAnimTimer;
			}
		}
		if ( trooper->NPC )
		{
			if ( trooper->NPC->shotTime < trooper->painDebounceTime )
			{
				trooper->NPC->shotTime = trooper->painDebounceTime;
			}
		}
		trooper->client->ps.weaponTime = trooper->painDebounceTime-level.time;
	}
	else
	{//level 1: no pain reaction, but they should still notice
		if ( trooper->enemy == NULL//not mad at anyone
			&& trooper->client->playerTeam != self->client->playerTeam//not on our team
			&& !(trooper->svFlags&SVF_LOCKEDENEMY)//not locked on an enemy
			&& !(trooper->svFlags&SVF_IGNORE_ENEMIES)//not ignoring enemie
			&& !(self->flags&FL_NOTARGET) )//I'm not in notarget
		{//not already mad at them and can get mad at them, do so
			G_SetEnemy( trooper, self );
		}
	}
}

void WP_ResistForcePush( gentity_t *self, gentity_t *pusher, qboolean noPenalty )
{
	int parts;
	qboolean runningResist = qfalse;

	if ( !self || self->health <= 0 || !self->client || !pusher || !pusher->client )
	{
		return;
	}

	if ( !noPenalty && WP_SaberRules() && self->client->ps.weapon == WP_SABER && self->client->ps.SaberActive()
		&& !self->client->ps.saberInFlight && WP_SaberSwinging( &self->client->ps )
		&& !PM_SaberInSpecialAttack( self->client->ps.torsoAnim ) && TIMER_Done( self, "bossStageRecover" ) )
	{//JKEnhanced saber rules: a push or pull that catches a saber fighter mid-swing knocks them Open
	 //(standing or guarding they hold, and it costs them Force below)
		WP_SaberSetOpen( self, g_saberOpenMedium ? g_saberOpenMedium->integer : 800 );
		return;
	}

	//NOTE: don't interrupt big anims with this!
	if ( !PM_SaberCanInterruptMove( self->client->ps.saberMove, self->client->ps.torsoAnim ) )
	{//can't interrupt my current torso anim/sabermove with this, so ignore it entirely!
		return;
	}

	if ( !noPenalty )
	{//even a successful resist wears down saber guard — Push pays off against duelists
		G_DamageGuard( self, pusher, 15 );
	}

	if ( (!self->s.number
			||( self->NPC && (self->NPC->aiFlags&NPCAI_BOSS_CHARACTER) )
			||( self->client && self->client->NPC_class == CLASS_SHADOWTROOPER )
			/*
			|| self->client->NPC_class == CLASS_DESANN
			|| !Q_stricmp("Yoda",self->NPC_type)
			|| self->client->NPC_class == CLASS_LUKE*/
		  )
		&& (VectorLengthSquared( self->client->ps.velocity ) > 10000 || self->client->ps.forcePowerLevel[FP_PUSH] >= FORCE_LEVEL_3 || self->client->ps.forcePowerLevel[FP_PULL] >= FORCE_LEVEL_3 ) )
	{
		runningResist = qtrue;
	}

	if ( !runningResist
		&& self->client->ps.groundEntityNum != ENTITYNUM_NONE
		&& !PM_SpinningSaberAnim( self->client->ps.legsAnim )
		&& !PM_FlippingAnim( self->client->ps.legsAnim )
		&& !PM_RollingAnim( self->client->ps.legsAnim )
		&& !PM_InKnockDown( &self->client->ps )
		&& !PM_CrouchAnim( self->client->ps.legsAnim ))
	{//if on a surface and not in a spin or flip, play full body resist
		parts = SETANIM_BOTH;
	}
	else
	{//play resist just in torso
		parts = SETANIM_TORSO;
	}
	NPC_SetAnim( self, parts, BOTH_RESISTPUSH, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );

	if ( !noPenalty )
	{
		if ( !runningResist )
		{
			VectorClear( self->client->ps.velocity );
			//still stop them from attacking or moving for a bit, though
			//FIXME: maybe push just a little (like, slide)?
			self->client->ps.weaponTime = 1000;
			if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
			{
				self->client->ps.weaponTime = floor( self->client->ps.weaponTime * g_timescale->value );
			}
			self->client->ps.pm_time = self->client->ps.weaponTime;
			self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			//play the full body push effect on me
			self->forcePushTime = level.time + 600; // let the push effect last for 600 ms
		}
		else
		{
			self->client->ps.weaponTime = 600;
			if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
			{
				self->client->ps.weaponTime = floor( self->client->ps.weaponTime * g_timescale->value );
			}
		}
	}
	//play my force push effect on my hand
	//self->client->ps.powerups[PW_FORCE_PUSH] = level.time + self->client->ps.torsoAnimTimer + 500;

	//reset to 0 in case it's still > 0 from a previous push
	//self->client->pushEffectFadeTime = 0;
	if ( !pusher //???
		|| pusher == self->enemy//my enemy tried to push me
		|| (pusher->client && pusher->client->playerTeam != self->client->playerTeam) )//someone not on my team tried to push me
	{
		Jedi_PlayBlockedPushSound( self );
	}
}

extern qboolean Boba_StopKnockdown( gentity_t *self, gentity_t *pusher, const vec3_t pushDir, qboolean forceKnockdown );
extern qboolean Jedi_StopKnockdown( gentity_t *self, gentity_t *pusher, const vec3_t pushDir );
void WP_ForceKnockdown( gentity_t *self, gentity_t *pusher, qboolean pull, qboolean strongKnockdown, qboolean breakSaberLock )
{
	if ( !self || !self->client || !pusher || !pusher->client )
	{
		return;
	}

	if ( self->client->NPC_class == CLASS_ROCKETTROOPER )
	{
		return;
	}
	else if ( PM_LockedAnim( self->client->ps.legsAnim ) )
	{//stuck doing something else
		return;
	}
	else if ( Rosh_BeingHealed( self ) )
	{
		return;
	}
	else if ( self->health > 0 && G_SpeedLevel( self ) >= FORCE_LEVEL_2 )
	{//JKEnhanced 4B.6: Speed 2+ (Flow) keeps its feet against Push / Pull / Repulse - thrown, not floored
		if ( g_debugForce && g_debugForce->integer )
		{
			gi.Printf( "  SPEED %s Flow shrugged off a Force knockdown\n", WP_ForceDebugWho( self ) );
		}
		return;
	}

	//break out of a saberLock?
	if ( self->client->ps.saberLockTime > level.time )
	{
		if ( breakSaberLock
			|| (pusher && self->client->ps.saberLockEnemy == pusher->s.number) )
		{
			self->client->ps.saberLockTime = 0;
			self->client->ps.saberLockEnemy = ENTITYNUM_NONE;
		}
		else
		{
			return;
		}
	}

	if ( self->health > 0 )
	{
		if ( !self->s.number )
		{
			NPC_SetPainEvent( self );
		}
		else
		{
			GEntity_PainFunc( self, pusher, pusher, self->currentOrigin, 0, MOD_MELEE );
		}

		vec3_t	pushDir;
		if ( pull )
		{
			VectorSubtract( pusher->currentOrigin, self->currentOrigin, pushDir );
		}
		else
		{
			VectorSubtract( self->currentOrigin, pusher->currentOrigin, pushDir );
		}

		//FIXME: sometimes do this for some NPC force-users, too!
		if ( Boba_StopKnockdown( self, pusher, pushDir, qtrue ) )
		{//He can backflip instead of be knocked down
			return;
		}
		else if ( Jedi_StopKnockdown( self, pusher, pushDir ) )
		{//They can backflip instead of be knocked down
			return;
		}

		G_CheckLedgeDive( self, 72, pushDir, qfalse, qfalse );

		if ( !PM_SpinningSaberAnim( self->client->ps.legsAnim )
			&& !PM_FlippingAnim( self->client->ps.legsAnim )
			&& !PM_RollingAnim( self->client->ps.legsAnim )
			&& !PM_InKnockDown( &self->client->ps ) )
		{
			int knockAnim = BOTH_KNOCKDOWN1;//default knockdown
			if ( pusher->client->NPC_class == CLASS_DESANN && self->client->NPC_class != CLASS_LUKE )
			{//desann always knocks down, unless you're Luke
				strongKnockdown = qtrue;
			}
			if ( !self->s.number
				&& !strongKnockdown
				&& ( (!pull&&(self->client->ps.forcePowerLevel[FP_PUSH]>FORCE_LEVEL_1||!g_spskill->integer)) || (pull&&(self->client->ps.forcePowerLevel[FP_PULL]>FORCE_LEVEL_1||!g_spskill->integer)) )	)
			{//player only knocked down if pushed *hard*
				if ( self->s.weapon == WP_SABER )
				{//temp HACK: these are the only 2 pain anims that look good when holding a saber
					knockAnim = PM_PickAnim( self, BOTH_PAIN2, BOTH_PAIN3 );
				}
				else
				{
					knockAnim = PM_PickAnim( self, BOTH_PAIN1, BOTH_PAIN18 );
				}
			}
			else if ( PM_CrouchAnim( self->client->ps.legsAnim ) )
			{//crouched knockdown
				knockAnim = BOTH_KNOCKDOWN4;
			}
			else
			{//plain old knockdown
				vec3_t pLFwd, pLAngles = {0,self->client->ps.viewangles[YAW],0};
				vec3_t sFwd, sAngles = {0,pusher->client->ps.viewangles[YAW],0};
				AngleVectors( pLAngles, pLFwd, NULL, NULL );
				AngleVectors( sAngles, sFwd, NULL, NULL );
				if ( DotProduct( sFwd, pLFwd ) > 0.2f )
				{//pushing him from behind
					//FIXME: check to see if we're aiming below or above the waist?
					if ( pull )
					{
						knockAnim = BOTH_KNOCKDOWN1;
					}
					else
					{
						knockAnim = BOTH_KNOCKDOWN3;
					}
				}
				else
				{//pushing him from front
					if ( pull )
					{
						knockAnim = BOTH_KNOCKDOWN3;
					}
					else
					{
						knockAnim = BOTH_KNOCKDOWN1;
					}
				}
			}
			if ( knockAnim == BOTH_KNOCKDOWN1 && strongKnockdown )
			{//push *hard*
				knockAnim = BOTH_KNOCKDOWN2;
			}
			NPC_SetAnim( self, SETANIM_BOTH, knockAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			if ( self->s.number >= MAX_CLIENTS )
			{//randomize getup times - but not for boba
				int addTime;
				if ( self->client->NPC_class == CLASS_BOBAFETT )
				{
					addTime = Q_irand( -500, 0 );
				}
				else
				{
					addTime = Q_irand( -300, 300 );
				}
				self->client->ps.legsAnimTimer += addTime;
				self->client->ps.torsoAnimTimer += addTime;
			}
			else
			{//player holds extra long so you have more time to decide to do the quick getup
				if ( PM_KnockDownAnim( self->client->ps.legsAnim ) )
				{
					self->client->ps.legsAnimTimer += PLAYER_KNOCKDOWN_HOLD_EXTRA_TIME;
					self->client->ps.torsoAnimTimer += PLAYER_KNOCKDOWN_HOLD_EXTRA_TIME;
				}
			}
			//
			if ( pusher->NPC && pusher->enemy == self )
			{//pushed pushed down his enemy
				G_AddVoiceEvent( pusher, Q_irand( EV_GLOAT1, EV_GLOAT3 ), 3000 );
				pusher->NPC->blockedSpeechDebounceTime = level.time + 3000;
			}
		}
	}
	self->forcePushTime = level.time + 600; // let the push effect last for 600 ms
}

qboolean WP_ForceThrowable( gentity_t *ent, gentity_t *forwardEnt, gentity_t *self, qboolean pull, float cone, float radius, vec3_t forward )
{
	if (ent == self)
		return qfalse;
	if ( ent->owner == self && ent->s.weapon != WP_THERMAL )//can push your own thermals
		return qfalse;
	if ( !(ent->inuse) )
		return qfalse;
	if ( ent->NPC && ent->NPC->scriptFlags & SCF_NO_FORCE )
	{
		if ( ent->s.weapon == WP_SABER )
		{//Hmm, should jedi do the resist behavior?  If this is on, perhaps it's because of a cinematic?
			WP_ResistForcePush( ent, self, qtrue );
		}
		return qfalse;
	}
	if ( (ent->flags&FL_FORCE_PULLABLE_ONLY) && !pull )
	{//simple HACK: cannot force-push ammo rack items (because they may start in solid)
		return qfalse;
	}
	//FIXME: don't push it if I already pushed it a little while ago
	if ( ent->s.eType != ET_MISSILE )
	{
		if ( ent->client )
		{
			if ( ent->client->ps.pullAttackTime > level.time )
			{
				return qfalse;
			}
		}
		if ( cone >= 1.0f && Q_stricmp( "limb", ent->classname ) != 0 )
		{//must be pointing right at them (gibs are exempt - too small/flat to reliably land a pixel-precise crosshair trace on)
			if ( ent != forwardEnt )
			{//must be the person I'm looking right at
				if ( ent->client && !pull
					&& ent->client->ps.forceGripEntityNum == self->s.number
					&& (self->s.eFlags&EF_FORCE_GRIPPED) )
				{//this is the guy that's force-gripping me, use a wider cone regardless of force power level
				}
				else
				{
					if ( ent->client && !pull
						&& ent->client->ps.forceDrainEntityNum == self->s.number
						&& (self->s.eFlags&EF_FORCE_DRAINED) )
					{//this is the guy that's force-draining me, use a wider cone regardless of force power level
					}
					else
					{
						return qfalse;
					}
				}
			}
		}
		if ( ent->s.eType != ET_ITEM && ent->e_ThinkFunc != thinkF_G_RunObject )//|| !(ent->flags&FL_DROPPED_ITEM) )//was only dropped items
		{
			//FIXME: need pushable objects
			if ( ent->s.eFlags & EF_NODRAW )
			{
				return qfalse;
			}
			if ( !ent->client )
			{
				if ( Q_stricmp( "lightsaber", ent->classname ) != 0 )
				{//not a lightsaber
					if ( !(ent->svFlags&SVF_GLASS_BRUSH) )
					{//and not glass
						if ( Q_stricmp( "func_door", ent->classname ) != 0 || !(ent->spawnflags & 2/*MOVER_FORCE_ACTIVATE*/) )
						{//not a force-usable door
							if ( Q_stricmp( "func_static", ent->classname ) != 0 || (!(ent->spawnflags&1/*F_PUSH*/)&&!(ent->spawnflags&2/*F_PULL*/)) || (ent->spawnflags&32/*SOLITARY*/) )
							{//not a force-usable func_static or, it is one, but it's solitary, so you only press it when looking right at it
								if ( Q_stricmp( "limb", ent->classname ) )
								{//not a limb
									if ( ent->s.weapon == WP_TURRET && !Q_stricmp( "PAS", ent->classname ) && ent->s.apos.trType == TR_STATIONARY )
									{//can knock over placed turrets
										if ( !self->s.number || self->enemy != ent )
										{//only NPCs who are actively mad at this turret can push it over
											return qfalse;
										}
									}
									else
									{
										return qfalse;
									}
								}
							}
						}
						else if ( ent->moverState != MOVER_POS1 && ent->moverState != MOVER_POS2 )
						{//not at rest
							return qfalse;
						}
					}
				}
				//return qfalse;
			}
			else if ( ent->client->NPC_class == CLASS_MARK1 )
			{//can't push Mark1 unless push 3
				if ( pull || self->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_3 )
				{
					return qfalse;
				}
			}
			else if ( ent->client->NPC_class == CLASS_GALAKMECH
				|| ent->client->NPC_class == CLASS_ATST
				|| ent->client->NPC_class == CLASS_RANCOR
				|| ent->client->NPC_class == CLASS_WAMPA
				|| ent->client->NPC_class == CLASS_SAND_CREATURE )
			{//can't push ATST or Galak or Rancor or Wampa
				return qfalse;
			}
			else if ( ent->s.weapon == WP_EMPLACED_GUN && ent->client->ps.eFlags & EF_LOCKED_TO_WEAPON )
			{//FIXME: maybe can pull them out?
				return qfalse;
			}
			else if ( ent->client->playerTeam == self->client->playerTeam && self->enemy && self->enemy != ent )
			{//can't accidently push a teammate while in combat
				return qfalse;
			}
			else if ( G_IsRidingVehicle( ent )
				&& (ent->s.eFlags&EF_NODRAW) )
			{//can't push/pull anyone riding *inside* vehicle
				return qfalse;
			}
		}
		else if ( ent->s.eType == ET_ITEM )
		{
			if ( (ent->flags&FL_NO_KNOCKBACK) )
			{
				return qfalse;
			}
			if ( ent->item
				&& ent->item->giType == IT_HOLDABLE
				&& ent->item->giTag == INV_SECURITY_KEY )
				//&& (ent->flags&FL_DROPPED_ITEM) ???
			{//dropped security keys can't be pushed?  But placed ones can...?  does this make any sense?
				if ( !pull || self->s.number )
				{//can't push, NPC's can't do anything to it
					return qfalse;
				}
				else
				{
					if ( g_crosshairEntNum != ent->s.number )
					{//player can pull it if looking *right* at it
						if ( cone >= 1.0f )
						{//we did a forwardEnt trace
							if ( forwardEnt != ent )
							{//must be pointing right at them
								return qfalse;
							}
						}
						else if ( forward )
						{//do a forwardEnt trace
							trace_t tr;
							vec3_t end;
							VectorMA( self->client->renderInfo.eyePoint, radius, forward, end );
							gi.trace( &tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, end, self->s.number, MASK_OPAQUE|CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_ITEM|CONTENTS_CORPSE, (EG2_Collision)0, 0 );//was MASK_SHOT, changed to match crosshair trace
							if ( tr.entityNum != ent->s.number )
							{//last chance
								return qfalse;
							}
						}
					}
				}
			}
		}
	}
	else
	{
		switch ( ent->s.weapon )
		{//only missiles with mass are force-pushable
		case WP_CONCUSSION:
			//Don't push Destruction projectiles
			if (ent->s.powerups & (1<<PW_FORCE_PROJECTILE))
			{
				return qfalse;
			}
			break;
		case WP_SABER:
		case WP_FLECHETTE:
		case WP_ROCKET_LAUNCHER:
		case WP_THERMAL:
		case WP_TRIP_MINE:
		case WP_DET_PACK:
			break;
		//only alt-fire of this weapon is force-pushable
		case WP_REPEATER:
			if ( ent->methodOfDeath != MOD_REPEATER_ALT )
			{//not an alt-fire missile
				return qfalse;
			}
			break;
		//everything else cannot be pushed
		case WP_ATST_SIDE:
			if ( ent->methodOfDeath != MOD_EXPLOSIVE )
			{//not a rocket
				return qfalse;
			}
			break;
		default:
			return qfalse;
			break;
		}

		if ( ent->s.pos.trType == TR_STATIONARY && (ent->s.eFlags&EF_MISSILE_STICK) )
		{//can't force-push/pull stuck missiles (detpacks, tripmines)
			return qfalse;
		}
		if ( ent->s.pos.trType == TR_STATIONARY && ent->s.weapon != WP_THERMAL )
		{//only thermal detonators can be pushed once stopped
			return qfalse;
		}
	}
	return qtrue;
}

qboolean ShouldPlayerResistForceThrow( gentity_t *player, gentity_t *attacker, qboolean pull )
{
	if ( player->health <= 0 )
	{
		return qfalse;
	}

	if ( !player->client )
	{
		return qfalse;
	}

	if ( player->client->ps.forceRageRecoveryTime >= level.time )
	{
		return qfalse;
	}

	//wasn't trying to grip/drain anyone
	if ( player->client->ps.torsoAnim == BOTH_FORCEGRIP_HOLD ||
			player->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_START ||
			player->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_HOLD )
	{
		return qfalse;
	}

	//only 30% chance of resisting a Desann or yoda push
	if ( (attacker->client->NPC_class == CLASS_DESANN || Q_stricmp("Yoda",attacker->NPC_type) == 0) && Q_irand( 0, 2 ) > 0 )
	{
		return qfalse;
	}

	//on the ground
	if ( player->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{
		return qfalse;
	}

	//not knocked down already
	if ( PM_InKnockDown( &player->client->ps ) )
	{
		return qfalse;
	}

	//not involved in a saberLock
	if ( player->client->ps.saberLockTime >= level.time )
	{
		return qfalse;
	}

	//not attacking or otherwise busy
	if ( player->client->ps.weaponTime >= level.time )
	{
		return qfalse;
	}

	//using saber or fists
	if ( player->client->ps.weapon != WP_SABER && player->client->ps.weapon != WP_MELEE )
	{
		return qfalse;
	}

	forcePowers_t forcePower = (pull ? FP_PULL : FP_PUSH);
	int attackingForceLevel = attacker->client->ps.forcePowerLevel[forcePower];
	int defendingForceLevel = player->client->ps.forcePowerLevel[forcePower];

	if ( player->client->ps.powerups[PW_FORCE_PUSH] > level.time ||
		Q_irand( 0, Q_max(0, defendingForceLevel - attackingForceLevel)*2 + 1 ) > 0 )
	{
		// player was pushing, or player's force push/pull is high enough to try to stop me
		if ( InFront( attacker->currentOrigin, player->client->renderInfo.eyePoint, player->client->ps.viewangles, 0.3f ) )
		{
			//I'm in front of player
			return qtrue;
		}
	}

	return qfalse;
}

extern void G_GetAimRay( gentity_t *ent, vec3_t org, vec3_t dir );
void ForceThrow( gentity_t *self, qboolean pull, qboolean fake )
{//FIXME: pass in a target ent so we (an NPC) can push/pull just one targeted ent.
	//shove things in front of you away
	float		dist;
	vec3_t		aimOrg, aimDir;
	float		aimRange;
	gentity_t	*ent, *forwardEnt = NULL;
	gentity_t	*entityList[MAX_GENTITIES];
	gentity_t	*push_list[MAX_GENTITIES];
	int			numListedEntities = 0;
	vec3_t		mins, maxs;
	vec3_t		v;
	int			i, e;
	int			ent_count = 0;
	int			maxVictimRecovery = 0;//longest recovery inflicted on any target this throw, used below to keep our own follow-up lockout honest
	int			radius;
	vec3_t		center, ent_org, size, forward, right, end, dir, fwdangles = {0};
	float		dot1, cone;
	trace_t		tr;
	int			anim, hold, soundIndex, cost, actualCost;
	qboolean	noResist = qfalse;

	if ( self->health <= 0 )
	{
		return;
	}
	if ( self->client->ps.leanofs )
	{//can't force-throw while leaning
		return;
	}
	if ( self->client->ps.forcePowerDebounce[FP_PUSH] > level.time )
	{//already pushing- now you can't haul someone across the room, sorry
		return;
	}
	if ( self->client->ps.forcePowerDebounce[FP_PULL] > level.time )
	{//already pulling- now you can't haul someone across the room, sorry
		return;
	}
	if ( self->client->ps.pullAttackTime > level.time )
	{//already pull-attacking
		return;
	}
	if ( !self->s.number && (cg.zoomMode || in_camera) )
	{//can't force throw/pull when zoomed in or in cinematic
		return;
	}
	if ( self->client->ps.saberLockTime > level.time )
	{
		if ( pull || self->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_3 )
		{//this can be a way to break out
			return;
		}
		//else, I'm breaking my half of the saberlock
		self->client->ps.saberLockTime = 0;
		self->client->ps.saberLockEnemy = ENTITYNUM_NONE;
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
	if ( pull )
	{
		radius = forcePushPullRadius[self->client->ps.forcePowerLevel[FP_PULL]];
	}
	else
	{
		radius = forcePushPullRadius[self->client->ps.forcePowerLevel[FP_PUSH]];
	}

	if ( !radius )
	{//no ability to do this yet
		return;
	}

	if ( pull )
	{
		cost = forcePowerNeeded[FP_PULL];
		if ( !WP_ForcePowerUsable( self, FP_PULL, cost ) )
		{
			return;
		}
		//make sure this plays and that you cannot press fire for about 200ms after this
		anim = BOTH_FORCEPULL;
		soundIndex = G_SoundIndex( "sound/weapons/force/pull.wav" );
		hold = 200;
	}
	else
	{
		cost = forcePowerNeeded[FP_PUSH];
		if ( !WP_ForcePowerUsable( self, FP_PUSH, cost ) )
		{
			return;
		}
		//make sure this plays and that you cannot press fire for about 1 second after this
		anim = BOTH_FORCEPUSH;
		soundIndex = G_SoundIndex( "sound/weapons/force/push.wav" );
		hold = 650;
	}

	int parts = SETANIM_TORSO;
	if ( !PM_InKnockDown( &self->client->ps ) )
	{
		if ( self->client->ps.saberLockTime > level.time )
		{
			self->client->ps.saberLockTime = 0;
			self->painDebounceTime = level.time + 2000;
			hold += 1000;
			parts = SETANIM_BOTH;
		}
		else if ( !VectorLengthSquared( self->client->ps.velocity ) && !(self->client->ps.pm_flags&PMF_DUCKED))
		{
			parts = SETANIM_BOTH;
		}
	}
	NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART );
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;
	if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
	{
		hold = floor( hold*g_timescale->value );
	}
	self->client->ps.weaponTime = hold;//was 1000, but want to swing sooner
	//do effect... FIXME: build-up or delay this until in proper part of anim
	self->client->ps.powerups[PW_FORCE_PUSH] = level.time + self->client->ps.torsoAnimTimer + 500;
	//reset to 0 in case it's still > 0 from a previous push
	self->client->pushEffectFadeTime = 0;

	G_Sound( self, soundIndex );

	if ( (!pull && self->client->ps.forcePowersForced&(1<<FP_PUSH))
		|| (pull && self->client->ps.forcePowersForced&(1<<FP_PULL))
		|| (pull&&self->client->NPC_class==CLASS_KYLE&&(self->spawnflags&1)&&TIMER_Done( self, "kyleTakesSaber" )) )
	{
		noResist = qtrue;
	}

	VectorCopy( self->client->ps.viewangles, fwdangles );
	//fwdangles[1] = self->client->ps.viewangles[1];
	AngleVectors( fwdangles, forward, right, NULL );
	VectorCopy( self->currentOrigin, center );

	// "Pointing right at it" traces start where the crosshair does (the camera in third
	// person), so the target under the crosshair is the one pushed/pulled. Extend the range
	// by the camera's distance from the eye so the offset doesn't shorten it.
	G_GetAimRay( self, aimOrg, aimDir );
	aimRange = radius + Distance( aimOrg, self->client->renderInfo.eyePoint );

	if ( pull )
	{
		cone = forcePullCone[self->client->ps.forcePowerLevel[FP_PULL]];
	}
	else
	{
		cone = forcePushCone[self->client->ps.forcePowerLevel[FP_PUSH]];
	}

	//	if ( cone >= 1.0f )
	{//must be pointing right at them
		VectorMA( aimOrg, aimRange, aimDir, end );
		gi.trace( &tr, aimOrg, vec3_origin, vec3_origin, end, self->s.number, MASK_OPAQUE|CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_ITEM|CONTENTS_CORPSE, (EG2_Collision)0, 0 );//was MASK_SHOT, changed to match crosshair trace
		if ( tr.entityNum < ENTITYNUM_WORLD )
		{//found something right in front of self,
			forwardEnt = &g_entities[tr.entityNum];
			if ( !forwardEnt->client && !Q_stricmp( "func_static", forwardEnt->classname ) )
			{
				if ( (forwardEnt->spawnflags&1/*F_PUSH*/)||(forwardEnt->spawnflags&2/*F_PULL*/) )
				{//push/pullable
					if ( (forwardEnt->spawnflags&32/*SOLITARY*/) )
					{//can only push/pull ME, ignore all others
						if ( forwardEnt->NPC_targetname == NULL
							|| (self->targetname&&Q_stricmp( forwardEnt->NPC_targetname, self->targetname ) == 0) )
						{//anyone can push it or only 1 person can push it and it's me
							push_list[0] = forwardEnt;
							ent_count = numListedEntities = 1;
						}
					}
				}
			}
		}
	}

	if ( forwardEnt )
	{
		if ( G_TryingPullAttack( self, &self->client->usercmd, qtrue ) )
		{//we're going to try to do a pull attack on our forwardEnt
			if ( WP_ForceThrowable( forwardEnt, forwardEnt, self, pull, cone, radius, forward ) )
			{//we will actually pull-attack him, so don't pull him or anything else here
				//activate the power, here, though, so the later check that actually does the pull attack knows we tried to pull
				self->client->ps.forcePowersActive |= (1<<FP_PULL);
				self->client->ps.forcePowerDebounce[FP_PULL] = level.time + 100; //force-pulling
				return;
			}
		}
	}

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

			if ( !WP_ForceThrowable( ent, forwardEnt, self, pull, cone, radius, forward ) )
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
			if ( cone < 1.0f )
			{//must be within the forward cone
				if ( ent->client && !pull
					&& ent->client->ps.forceGripEntityNum == self->s.number
					&& (self->s.eFlags&EF_FORCE_GRIPPED) )
				{//this is the guy that's force-gripping me, use a wider cone regardless of force power level
					if ( (dot1 = DotProduct( dir, forward )) < cone-0.3f )
						continue;
				}
				else if ( ent->client && !pull
					&& ent->client->ps.forceDrainEntityNum == self->s.number
					&& (self->s.eFlags&EF_FORCE_DRAINED) )
				{//this is the guy that's force-draining me, use a wider cone regardless of force power level
					if ( (dot1 = DotProduct( dir, forward )) < cone-0.3f )
						continue;
				}
				else if ( ent->s.eType == ET_MISSILE )//&& ent->s.eType != ET_ITEM && ent->e_ThinkFunc != thinkF_G_RunObject )
				{//missiles are easier to force-push, never require direct trace (FIXME: maybe also items and general physics objects)
					if ( (dot1 = DotProduct( dir, forward )) < cone-0.3f )
						continue;
				}
				else if ( (dot1 = DotProduct( dir, forward )) < cone )
				{
					continue;
				}
			}
			else if ( ent->s.eType == ET_MISSILE )
			{//a missile and we're at force level 1... just use a small cone, but not ridiculously small
				if ( (dot1 = DotProduct( dir, forward )) < 0.75f )
				{
					continue;
				}
			}//else is an NPC or brush entity that our forward trace would have to hit

			dist = VectorLength( v );

			//Now check and see if we can actually deflect it
			//method1
			//if within a certain range, deflect it
			if ( ent->s.eType == ET_MISSILE && cone >= 1.0f )
			{//smaller radius on missile checks at force push 1
				if ( dist >= 192 )
				{
					continue;
				}
			}
			else if ( dist >= radius )
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

	if ( ent_count )
	{
		for ( int x = 0; x < ent_count; x++ )
		{
			if ( push_list[x]->client )
			{
				vec3_t	pushDir;
				float	knockback = pull?0:200;

				//SIGH band-aid...
				if ( push_list[x]->s.number >= MAX_CLIENTS
					&& self->s.number < MAX_CLIENTS )
				{
					if ( (push_list[x]->client->ps.forcePowersActive&(1<<FP_GRIP))
						//&& push_list[x]->client->ps.forcePowerDebounce[FP_GRIP] < level.time
						&& push_list[x]->client->ps.forceGripEntityNum == self->s.number )
					{
						WP_ForcePowerStop( push_list[x], FP_GRIP );
					}
					if ( (push_list[x]->client->ps.forcePowersActive&(1<<FP_DRAIN))
						//&& push_list[x]->client->ps.forcePowerDebounce[FP_DRAIN] < level.time
						&& push_list[x]->client->ps.forceDrainEntityNum == self->s.number )
					{
						WP_ForcePowerStop( push_list[x], FP_DRAIN );
					}
				}

				if ( Rosh_BeingHealed( push_list[x] ) )
				{
					continue;
				}
				if ( push_list[x]->client->NPC_class == CLASS_HAZARD_TROOPER
					&& push_list[x]->health > 0 )
				{//living hazard troopers resist push/pull
					WP_ForceThrowHazardTrooper( self, push_list[x], pull );
					continue;
				}
				if ( fake )
				{//always resist
					WP_ResistForcePush( push_list[x], self, qfalse );
					continue;
				}
//FIXMEFIXMEFIXMEFIXMEFIXME: extern a lot of this common code when I have the time!!!
				int powerLevel, powerUse;
				if (pull)
				{
					powerLevel = self->client->ps.forcePowerLevel[FP_PULL];
					powerUse = FP_PULL;
				}
				else
				{
					powerLevel = self->client->ps.forcePowerLevel[FP_PUSH];
					powerUse = FP_PUSH;
				}
				int modPowerLevel = WP_AbsorbConversion( push_list[x], push_list[x]->client->ps.forcePowerLevel[FP_ABSORB], self, powerUse, powerLevel, forcePowerNeeded[powerUse] );//JKEnhanced: was indexed by LEVEL (Absorb refunded e.g. Speed's 50)
				if (push_list[x]->client->NPC_class==CLASS_ASSASSIN_DROID ||
					push_list[x]->client->NPC_class==CLASS_HAZARD_TROOPER)
				{
					modPowerLevel = 0;	// devides throw by 10
				}

				//First, if this is the player we're push/pulling, see if he can counter it
				if ( modPowerLevel != -1
					&& !noResist
					&& InFront( self->currentOrigin, push_list[x]->client->renderInfo.eyePoint, push_list[x]->client->ps.viewangles, 0.3f ) )
				{//absorbed and I'm in front of them
					//counter it
					if ( push_list[x]->client->ps.forcePowerLevel[FP_ABSORB] > FORCE_LEVEL_2 )
					{//no reaction at all
					}
					else
					{
						WP_ResistForcePush( push_list[x], self, qfalse );
						push_list[x]->client->ps.saberMove = push_list[x]->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
						push_list[x]->client->ps.saberBlocked = BLOCKED_NONE;
					}
					continue;
				}
				else if ( !push_list[x]->s.number )
				{//player
					if ( !noResist && ShouldPlayerResistForceThrow(push_list[x], self, pull) )
					{
						WP_ResistForcePush( push_list[x], self, qfalse );
						push_list[x]->client->ps.saberMove = push_list[x]->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
						push_list[x]->client->ps.saberBlocked = BLOCKED_NONE;
						continue;
					}
				}
				else if ( push_list[x]->client && Jedi_WaitingAmbush( push_list[x] ) )
				{
					WP_ForceKnockdown( push_list[x], self, pull, qtrue, qfalse );
					continue;
				}

				if ( push_list[x]->NPC && !OnSameTeam( self, push_list[x] ) )
				{//being thrown around by the Force is unnerving, for them and for whoever saw it
					AI_NerveEvent( push_list[x], NE_FORCE_ON_ME, 1.0f, self );
					AI_NerveWitness( push_list[x], NE_FORCE_ON_ME, self );
				}

				G_KnockOffVehicle( push_list[x], self, pull );

				if ( !pull
					&& push_list[x]->client->ps.forceDrainEntityNum == self->s.number
					&& (self->s.eFlags&EF_FORCE_DRAINED) )
				{//stop them from draining me now, dammit!
					WP_ForcePowerStop( push_list[x], FP_DRAIN );
				}

				//okay, everyone else (or player who couldn't resist it)...
				if ( ((self->s.number == 0 && Q_irand( 0, 2 ) ) || Q_irand( 0, 2 ) ) && push_list[x]->client && push_list[x]->health > 0 //a living client
						&& push_list[x]->client->ps.weapon == WP_SABER //Jedi
						&& push_list[x]->health > 0 //alive
						&& push_list[x]->client->ps.forceRageRecoveryTime < level.time //not recobering from rage
						&& ((self->client->NPC_class != CLASS_DESANN&&Q_stricmp("Yoda",self->NPC_type)) || !Q_irand( 0, 2 ) )//only 30% chance of resisting a Desann push
						&& push_list[x]->client->ps.groundEntityNum != ENTITYNUM_NONE //on the ground
						&& InFront( self->currentOrigin, push_list[x]->currentOrigin, push_list[x]->client->ps.viewangles, 0.3f ) //I'm in front of him
						&& ( push_list[x]->client->ps.powerups[PW_FORCE_PUSH] > level.time ||//he's pushing too
								(push_list[x]->s.number != 0 && push_list[x]->client->ps.weaponTime < level.time)//not the player and not attacking (NPC jedi auto-defend against pushes)
						   )
					)
				{//Jedi don't get pushed, they resist as long as they aren't already attacking and are on the ground
					if ( push_list[x]->client->ps.saberLockTime > level.time )
					{//they're in a lock
						if ( push_list[x]->client->ps.saberLockEnemy != self->s.number )
						{//they're not in a lock with me
							continue;
						}
						else if ( pull || self->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_3 ||
							push_list[x]->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2 )
						{//they're in a lock with me, but my push is too weak
							continue;
						}
						else
						{//we will knock them down
							self->painDebounceTime = 0;
							self->client->ps.weaponTime = 500;
							if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
							{
								self->client->ps.weaponTime = floor( self->client->ps.weaponTime * g_timescale->value );
							}
						}
					}
					int resistChance = Q_irand(0, 2);
					if ( push_list[x]->s.number >= MAX_CLIENTS )
					{//NPC
						if ( g_spskill->integer == 1 )
						{//stupid tweak for graham
							resistChance = Q_irand(0, 3);
						}
					}
					if ( noResist ||
							( !pull
							&& modPowerLevel == -1
							&& self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2
							&& !resistChance
							&& push_list[x]->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_3 )
						)
					{//a level 3 push can even knock down a jedi
						if ( PM_InKnockDown( &push_list[x]->client->ps ) )
						{//can't knock them down again
							continue;
						}
						WP_ForceKnockdown( push_list[x], self, pull, qfalse, qtrue );
					}
					else
					{
						WP_ResistForcePush( push_list[x], self, qfalse );
					}
				}
				else
				{
					//UGH: FIXME: for enemy jedi, they should probably always do force pull 3, and not your weapon (if player?)!
					//shove them
					if ( push_list[x]->NPC
						&& push_list[x]->NPC->jumpState == JS_JUMPING )
					{//don't interrupt a scripted jump
						//WP_ResistForcePush( push_list[x], self, qfalse );
						push_list[x]->forcePushTime = level.time + 600; // let the push effect last for 600 ms
						continue;
					}

					if ( push_list[x]->s.number
						&& (push_list[x]->message || (push_list[x]->flags&FL_NO_KNOCKBACK)) )
					{//an NPC who has a key
						//don't push me... FIXME: maybe can pull the key off me?
						WP_ForceKnockdown( push_list[x], self, pull, qfalse, qfalse );
						continue;
					}
					if ( pull )
					{
						VectorSubtract( self->currentOrigin, push_list[x]->currentOrigin, pushDir );
						if ( self->client->ps.forcePowerLevel[FP_PULL] >= FORCE_LEVEL_3
							&& self->client->NPC_class == CLASS_KYLE
							&& (self->spawnflags&1)
							&& TIMER_Done( self, "kyleTakesSaber" )
							&& push_list[x]->client
							&& push_list[x]->client->ps.weapon == WP_SABER
							&& !push_list[x]->client->ps.saberInFlight
							&& push_list[x]->client->ps.saberEntityNum < ENTITYNUM_WORLD
							&& !PM_InOnGroundAnim( &push_list[x]->client->ps ) )
						{
							vec3_t throwVec;
							VectorScale( pushDir, 10.0f, throwVec );
							WP_SaberLose( push_list[x], throwVec );
							NPC_SetAnim( push_list[x], SETANIM_BOTH, BOTH_LOSE_SABER, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
							push_list[x]->client->ps.torsoAnimTimer += 500;
							push_list[x]->client->ps.pm_time = push_list[x]->client->ps.weaponTime = push_list[x]->client->ps.torsoAnimTimer;
							push_list[x]->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
							push_list[x]->client->ps.saberMove = LS_NONE;
							push_list[x]->aimDebounceTime = level.time + push_list[x]->client->ps.torsoAnimTimer;
							VectorClear( push_list[x]->client->ps.velocity );
							VectorClear( push_list[x]->client->ps.moveDir );
							//Kyle will stand around for a bit, too...
							self->client->ps.pm_time = self->client->ps.weaponTime = 2000;
							self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
							self->painDebounceTime = level.time + self->client->ps.weaponTime;
							TIMER_Set( self, "kyleTakesSaber", Q_irand( 60000, 180000 ) );//don't do this again for a while
							G_AddVoiceEvent( self, Q_irand(EV_TAUNT1,EV_TAUNT3), Q_irand( 4000, 6000 ) );
							VectorClear( self->client->ps.velocity );
							VectorClear( self->client->ps.moveDir );
							continue;
						}
						else if ( push_list[x]->NPC
							&& (push_list[x]->NPC->scriptFlags&SCF_DONT_FLEE) )
						{//*SIGH*... if an NPC can't flee, they can't run after and pick up their weapon, do don't drop it
						}
						else if ( self->client->ps.forcePowerLevel[FP_PULL] > FORCE_LEVEL_1
							&& push_list[x]->client->NPC_class != CLASS_ROCKETTROOPER//rockettroopers never drop their weapon
							&& push_list[x]->client->NPC_class != CLASS_VEHICLE
							&& push_list[x]->client->NPC_class != CLASS_BOBAFETT
							&& push_list[x]->client->NPC_class != CLASS_TUSKEN
							&& push_list[x]->client->NPC_class != CLASS_HAZARD_TROOPER
							&& push_list[x]->client->NPC_class != CLASS_ASSASSIN_DROID
							&& push_list[x]->s.weapon != WP_SABER
							&& push_list[x]->s.weapon != WP_MELEE
							&& push_list[x]->s.weapon != WP_THERMAL
							&& push_list[x]->s.weapon != WP_CONCUSSION	// so rax can't drop his
							)
						{//yank the weapon - NOTE: level 1 just knocks them down, not take weapon
							//FIXME: weapon yank anim if not a knockdown?
							if ( InFront( self->currentOrigin, push_list[x]->currentOrigin, push_list[x]->client->ps.viewangles, 0.0f ) )
							{//enemy has to be facing me, too...
								WP_DropWeapon( push_list[x], pushDir );
							}
						}
						knockback += VectorNormalize( pushDir );
						if ( knockback > 200 )
						{
							knockback = 200;
						}
						if ( self->client->ps.forcePowerLevel[FP_PULL] < FORCE_LEVEL_3 )
						{//maybe just knock them down
							knockback /= 3;
						}
					}
					else
					{
						VectorSubtract( push_list[x]->currentOrigin, self->currentOrigin, pushDir );
						knockback -= VectorNormalize( pushDir );
						if ( knockback < 100 )
						{
							knockback = 100;
						}
						//scale for push level
						if ( self->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_2 )
						{//maybe just knock them down
							knockback /= 3;
						}
						else if ( self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2 )
						{//super-hard push
							//Hmm, maybe in this case can even nudge/knockdown a jedi?  Especially if close?
							//knockback *= 5;
						}
					}

					if ( modPowerLevel != -1 )
					{
						if ( !modPowerLevel )
						{
							knockback /= 10.0f;
						}
						else if ( modPowerLevel == 1 )
						{
							knockback /= 6.0f;
						}
						else// if ( modPowerLevel == 2 )
						{
							knockback /= 2.0f;
						}
					}
					//actually push/pull the enemy
					G_Throw( push_list[x], pushDir, knockback );
					//make it so they don't actually hurt me when pulled at me...
					push_list[x]->forcePuller = self->s.number;

					if ( push_list[x]->client->ps.groundEntityNum != ENTITYNUM_NONE )
					{//if on the ground, make sure they get shoved up some
						if ( push_list[x]->client->ps.velocity[2] < knockback )
						{
							push_list[x]->client->ps.velocity[2] = knockback;
						}
					}

					if ( push_list[x]->health > 0 )
					{//target is still alive
						if ( (push_list[x]->s.number||(cg.renderingThirdPerson&&!cg.zoomMode)) //NPC or 3rd person player
							&& ((!pull&&self->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_2 && push_list[x]->client->ps.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_1) //level 1 push
								|| (pull && self->client->ps.forcePowerLevel[FP_PULL] < FORCE_LEVEL_2 && push_list[x]->client->ps.forcePowerLevel[FP_PULL] < FORCE_LEVEL_1)) )//level 1 pull
						{//NPC or third person player (without force push/pull skill), and force push/pull level is at 1
							WP_ForceKnockdown( push_list[x], self, pull, (qboolean)(!pull&&knockback>150), qfalse );
						}
						else if ( !push_list[x]->s.number )
						{//player, have to force an anim on him
							WP_ForceKnockdown( push_list[x], self, pull, (qboolean)(!pull&&knockback>150), qfalse );
						}
						else
						{//NPC and force-push/pull at level 2 or higher
							WP_ForceKnockdown( push_list[x], self, pull, (qboolean)(!pull&&knockback>100), qfalse );
						}
						if ( push_list[x]->client->ps.torsoAnimTimer > maxVictimRecovery )
						{//track the longest recovery we just inflicted, so our own lockout below can respect it
							maxVictimRecovery = push_list[x]->client->ps.torsoAnimTimer;
						}
					}
					push_list[x]->forcePushTime = level.time + 600; // let the push effect last for 600 ms
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
					if ( pull )
					{//deflect rather than reflect?
					}
					else
					{
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
						{//it's heading towards me
							int reflectChance = 100; // players always reflect
							if ( self->s.number > 0 )
							{//NPC: reflect chance based on force push level
								int pushLevel = self->client->ps.forcePowerLevel[FP_PUSH];
								reflectChance = (pushLevel >= FORCE_LEVEL_3) ? 60 :
								               (pushLevel >= FORCE_LEVEL_2) ? 40 : 25;
							}
							if ( Q_irand( 0, 99 ) < reflectChance )
							{
								G_ReflectMissile( self, push_list[x], forward );
							}
						}
						else
						{
							VectorScale( push_list[x]->s.pos.trDelta, 1.25f, push_list[x]->s.pos.trDelta );
						}
						//deflect sound
						//G_Sound( push_list[x], G_SoundIndex( va("sound/weapons/blaster/reflect%d.wav", Q_irand( 1, 3 ) ) ) );
						//push_list[x]->forcePushTime = level.time + 600; // let the push effect last for 600 ms
					}
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
					VectorMA( aimOrg, aimRange, aimDir, end );
					gi.trace( &tr, aimOrg, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT, (EG2_Collision)0, 0 );
					if ( tr.entityNum != push_list[x]->s.number || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
					{//must be pointing right at it
						continue;
					}

					if ( pull )
					{
						VectorSubtract( aimOrg, tr.endpos, pushDir );
					}
					else
					{
						VectorSubtract( tr.endpos, aimOrg, pushDir );
					}
					/*
					VectorSubtract( push_list[x]->absmax, push_list[x]->absmin, size );
					VectorMA( push_list[x]->absmin, 0.5, size, center );
					if ( pull )
					{
						VectorSubtract( self->client->renderInfo.eyePoint, center, pushDir );
					}
					else
					{
						VectorSubtract( center, self->client->renderInfo.eyePoint, pushDir );
					}
					*/
					damage -= VectorNormalize( pushDir );
					if ( damage < 200 )
					{
						damage = 200;
					}
					VectorScale( pushDir, damage, pushDir );

					G_Damage( push_list[x], self, self, pushDir, tr.endpos, damage, 0, MOD_UNKNOWN );
				}
				else if ( !Q_stricmp( "func_static", push_list[x]->classname ) )
				{//force-usable func_static
					if ( !pull && (push_list[x]->spawnflags&1/*F_PUSH*/) )
					{
						if ( push_list[x]->NPC_targetname == NULL
							|| (self->targetname&&Q_stricmp( push_list[x]->NPC_targetname, self->targetname ) == 0) )
						{//anyone can pull it or only 1 person can push it and it's me
							GEntity_UseFunc( push_list[x], self, self );
						}
					}
					else if ( pull && (push_list[x]->spawnflags&2/*F_PULL*/) )
					{
						if ( push_list[x]->NPC_targetname == NULL
							|| (self->targetname&&Q_stricmp( push_list[x]->NPC_targetname, self->NPC_targetname ) == 0) )
						{//anyone can push it or only 1 person can push it and it's me
							GEntity_UseFunc( push_list[x], self, self );
						}
					}
				}
				else if ( !Q_stricmp( "func_door", push_list[x]->classname ) && (push_list[x]->spawnflags&2/*MOVER_FORCE_ACTIVATE*/) )
				{//push/pull the door
					vec3_t	pos1, pos2;

					AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
					VectorNormalize( forward );
					VectorMA( aimOrg, aimRange, aimDir, end );
					gi.trace( &tr, aimOrg, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT, (EG2_Collision)0, 0 );
					if ( tr.entityNum != push_list[x]->s.number || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
					{//must be pointing right at it
						continue;
					}

					if ( VectorCompare( vec3_origin, push_list[x]->s.origin ) )
					{//does not have an origin brush, so pos1 & pos2 are relative to world origin, need to calc center
						VectorSubtract( push_list[x]->absmax, push_list[x]->absmin, size );
						VectorMA( push_list[x]->absmin, 0.5, size, center );
						if ( (push_list[x]->spawnflags&1) && push_list[x]->moverState == MOVER_POS1 )
						{//if at pos1 and started open, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
							VectorSubtract( center, push_list[x]->pos1, center );
						}
						else if ( !(push_list[x]->spawnflags&1) && push_list[x]->moverState == MOVER_POS2 )
						{//if at pos2, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
							VectorSubtract( center, push_list[x]->pos2, center );
						}
						VectorAdd( center, push_list[x]->pos1, pos1 );
						VectorAdd( center, push_list[x]->pos2, pos2 );
					}
					else
					{//actually has an origin, pos1 and pos2 are absolute
						VectorCopy( push_list[x]->currentOrigin, center );
						VectorCopy( push_list[x]->pos1, pos1 );
						VectorCopy( push_list[x]->pos2, pos2 );
					}

					if ( Distance( pos1, self->client->renderInfo.eyePoint ) < Distance( pos2, self->client->renderInfo.eyePoint ) )
					{//pos1 is closer
						if ( push_list[x]->moverState == MOVER_POS1 )
						{//at the closest pos
							if ( pull )
							{//trying to pull, but already at closest point, so screw it
								continue;
							}
						}
						else if ( push_list[x]->moverState == MOVER_POS2 )
						{//at farthest pos
							if ( !pull )
							{//trying to push, but already at farthest point, so screw it
								continue;
							}
						}
					}
					else
					{//pos2 is closer
						if ( push_list[x]->moverState == MOVER_POS1 )
						{//at the farthest pos
							if ( !pull )
							{//trying to push, but already at farthest point, so screw it
								continue;
							}
						}
						else if ( push_list[x]->moverState == MOVER_POS2 )
						{//at closest pos
							if ( pull )
							{//trying to pull, but already at closest point, so screw it
								continue;
							}
						}
					}
					GEntity_UseFunc( push_list[x], self, self );
				}
				else if ( push_list[x]->s.eType == ET_MISSILE/*thermal resting on ground*/
					|| push_list[x]->s.eType == ET_ITEM
					|| push_list[x]->e_ThinkFunc == thinkF_G_RunObject || Q_stricmp( "limb", push_list[x]->classname ) == 0 )
				{//general object, toss it
					vec3_t	pushDir, kvel;
					float	knockback = pull?0:200;
					float	mass = 200;

					if ( pull )
					{
						if ( push_list[x]->s.eType == ET_ITEM )
						{//pull it to a little higher point
							vec3_t	adjustedOrg;
							VectorCopy( self->currentOrigin, adjustedOrg );
							adjustedOrg[2] += self->maxs[2]/3;
							VectorSubtract( adjustedOrg, push_list[x]->currentOrigin, pushDir );
						}
						else if ( self->enemy //I have an enemy
							//&& push_list[x]->s.eType != ET_ITEM //not an item
							&& self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2 //have push 3 or greater
							&& InFront(push_list[x]->currentOrigin, self->currentOrigin, self->currentAngles, 0.25f)//object is generally in front of me
							&& InFront(self->enemy->currentOrigin, self->currentOrigin, self->currentAngles, 0.75f)//enemy is pretty much right in front of me
							&& !InFront(push_list[x]->currentOrigin, self->enemy->currentOrigin, self->enemy->currentAngles, -0.25f)//object is generally behind enemy
							//FIXME: check dist to enemy and clear LOS to enemy and clear Path between object and enemy?
							&& ( (self->NPC&&(noResist||Q_irand(0,RANK_CAPTAIN)<self->NPC->rank) )//NPC with enough skill
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
							VectorSubtract( self->currentOrigin, push_list[x]->currentOrigin, pushDir );
						}
						knockback += VectorNormalize( pushDir );
						if ( knockback > 200 )
						{
							knockback = 200;
						}
						if ( push_list[x]->s.eType == ET_ITEM
							&& push_list[x]->item
							&& push_list[x]->item->giType == IT_HOLDABLE
							&& push_list[x]->item->giTag == INV_SECURITY_KEY )
						{//security keys are pulled with less enthusiasm
							if ( knockback > 100 )
							{
								knockback = 100;
							}
						}
						else if ( knockback > 200 )
						{
							knockback = 200;
						}
					}
					else
					{
						if ( self->enemy //I have an enemy
							&& push_list[x]->s.eType != ET_ITEM //not an item
							&& self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2 //have push 3 or greater
							&& InFront(push_list[x]->currentOrigin, self->currentOrigin, self->currentAngles, 0.25f)//object is generally in front of me
							&& InFront(self->enemy->currentOrigin, self->currentOrigin, self->currentAngles, 0.75f)//enemy is pretty much right in front of me
							&& InFront(push_list[x]->currentOrigin, self->enemy->currentOrigin, self->enemy->currentAngles, 0.25f)//object is generally in front of enemy
							//FIXME: check dist to enemy and clear LOS to enemy and clear Path between object and enemy?
							&& ( (self->NPC&&(noResist||Q_irand(0,RANK_CAPTAIN)<self->NPC->rank) )//NPC with enough skill
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
					}
					if ( Q_stricmp( "limb", push_list[x]->classname ) == 0 )
					{//gibs are small and light - don't let them go flying as hard as a full ragdoll/prop
						knockback *= 0.4f;
						//spread multiple gibs out instead of letting them all converge on the exact same point
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
		//JKEnhanced 4B: hitting several costs the base + force.cfg multiTarget.extra per extra target,
		//capped at multiTarget.cap (15/20/25/30 by default). Was base x targets, capped at 50 (L3)
		//and base x targets / 1.5 (Push L2) - a single Push could cost a third of your bar.
		const forceTuning_t *ft = G_ForceTuning();
		const int multiCost = Q_max( cost, Q_min( ft->multiTargetCap, cost + ft->multiTargetExtra * ( ent_count - 1 ) ) );
		if ( pull )
		{
			actualCost = ( self->client->ps.forcePowerLevel[FP_PULL] > FORCE_LEVEL_2 ) ? multiCost : cost;//L3 pulls several
			WP_ForcePowerStart( self, FP_PULL, actualCost );
		}
		else
		{
			actualCost = ( self->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_1 ) ? multiCost : cost;//L2+ pushes several
			WP_ForcePowerStart( self, FP_PUSH, actualCost );
		}
	}
	else
	{//didn't push or pull anything?  don't penalize them too much
		if ( pull )
		{
			WP_ForcePowerStart( self, FP_PULL, 5 );
		}
		else
		{
			WP_ForcePowerStart( self, FP_PUSH, 5 );
		}
	}
	if ( pull )
	{
		if ( self->NPC )
		{//NPCs can push more often
			//FIXME: vary by rank and game skill?
			self->client->ps.forcePowerDebounce[FP_PULL] = level.time + 200;
		}
		else
		{
			self->client->ps.forcePowerDebounce[FP_PULL] = level.time + self->client->ps.torsoAnimTimer + 500;
		}
	}
	else
	{
		if ( self->NPC )
		{//NPCs can push more often
			//FIXME: vary by rank and game skill?
			self->client->ps.forcePowerDebounce[FP_PUSH] = level.time + 200;
		}
		else
		{
			self->client->ps.forcePowerDebounce[FP_PUSH] = level.time + self->client->ps.torsoAnimTimer + 500;
		}
	}
	if ( ent_count && maxVictimRecovery > 0 )
	{//don't let us get back to attacking well before whoever we just knocked down can defend themselves
		int scaledHold = (int)( maxVictimRecovery * g_forceStaggerLockoutScale->value );
		self->client->ps.weaponTime = Q_max( self->client->ps.weaponTime, scaledHold );
	}
}

void WP_DebounceForceDeactivateTime( gentity_t *self )
{
	//FIXME: if these are interruptable, should they also drain power at a constant rate
	//		rather than just taking one lump sum of force power upfront?
	if ( self && self->client )
	{
		if ( self->client->ps.forcePowersActive&(1<<FP_SPEED)
			|| self->client->ps.forcePowersActive&(1<<FP_PROTECT)
			|| self->client->ps.forcePowersActive&(1<<FP_ABSORB)
			|| self->client->ps.forcePowersActive&(1<<FP_RAGE)
			|| self->client->ps.forcePowersActive&(1<<FP_SEE))
		{//already running another power that can be manually, stopped don't debounce so long
			self->client->ps.forceAllowDeactivateTime = level.time + 500;
		}
		else
		{//not running one of the interruptable powers
			//FIXME: this should be shorter for force speed and rage (because of timescaling)
			self->client->ps.forceAllowDeactivateTime = level.time + 1500;
		}
	}
}

//JKEnhanced 4B.3: casting an offensive power (Grip / Lightning / Drain / Repulse) drops Absorb and a
//timed L1-2 Protect, as before; an L3 Protect toggle stays up
void WP_StopShieldsForAttack( gentity_t *self )
{
	if ( ( self->client->ps.forcePowersActive & ( 1 << FP_PROTECT ) ) && !G_ForceShieldIsToggle( self, FP_PROTECT ) )
	{
		WP_ForcePowerStop( self, FP_PROTECT );
	}
	if ( ( self->client->ps.forcePowersActive & ( 1 << FP_ABSORB ) ) && !G_ForceShieldIsToggle( self, FP_ABSORB ) )
	{
		WP_ForcePowerStop( self, FP_ABSORB );
	}
}

void ForceSpeed( gentity_t *self, int duration )
{
	if ( self->health <= 0 )
	{
		return;
	}
	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.forcePowersActive & (1 << FP_SPEED)) )
	{//stop using it
		WP_ForcePowerStop( self, FP_SPEED );
		return;
	}
	if ( !WP_ForcePowerUsable( self, FP_SPEED, 0 ) )
	{
		return;
	}
	if ( self->client->ps.saberLockTime > level.time )
	{//FIXME: can this be a way to break out?
		return;
	}

	WP_DebounceForceDeactivateTime( self );

	WP_ForcePowerStart( self, FP_SPEED, 0 );
	if ( duration )
	{
		self->client->ps.forcePowerDuration[FP_SPEED] = level.time + duration;
	}
	G_Sound( self, G_SoundIndex( "sound/weapons/force/speed.wav" ) );
}

void WP_StartForceHealEffects( gentity_t *self )
{
	if ( self->ghoul2.size() )
	{
		if ( self->chestBolt != -1 )
		{
			G_PlayEffect( G_EffectIndex( "force/heal2" ), self->playerModel, self->chestBolt, self->s.number, self->currentOrigin, 3000, qtrue );
		}
		/*
		if ( self->headBolt != -1 )
		{
			G_PlayEffect( G_EffectIndex( "force/heal_joint" ), self->playerModel, self->headBolt, self->s.number, self->currentOrigin, 3000, qtrue );
		}
		if ( self->cervicalBolt != -1 )
		{
			G_PlayEffect( G_EffectIndex( "force/heal_joint" ), self->playerModel, self->cervicalBolt, self->s.number, self->currentOrigin, 3000, qtrue );
		}
		if ( self->chestBolt != -1 )
		{
			G_PlayEffect( G_EffectIndex( "force/heal_joint" ), self->playerModel, self->chestBolt, self->s.number, self->currentOrigin, 3000, qtrue );
		}
		if ( self->gutBolt != -1 )
		{
			G_PlayEffect( G_EffectIndex( "force/heal_joint" ), self->playerModel, self->gutBolt, self->s.number, self->currentOrigin, 3000, qtrue );
		}
		if ( self->kneeLBolt != -1 )
		{
			G_PlayEffect( G_EffectIndex( "force/heal_joint" ), self->playerModel, self->kneeLBolt, self->s.number, self->currentOrigin, 3000, qtrue );
		}
		if ( self->kneeRBolt != -1 )
		{
			G_PlayEffect( G_EffectIndex( "force/heal_joint" ), self->playerModel, self->kneeRBolt, self->s.number, self->currentOrigin, 3000, qtrue );
		}
		if ( self->elbowLBolt != -1 )
		{
			G_PlayEffect( G_EffectIndex( "force/heal_joint" ), self->playerModel, self->elbowLBolt, self->s.number, self->currentOrigin, 3000, qtrue );
		}
		if ( self->elbowRBolt != -1 )
		{
			G_PlayEffect( G_EffectIndex( "force/heal_joint" ), self->playerModel, self->elbowRBolt, self->s.number, self->currentOrigin, 3000, qtrue );
		}
		*/
	}
}

void WP_StopForceHealEffects( gentity_t *self )
{
	if ( self->ghoul2.size() )
	{
		if ( self->chestBolt != -1 )
		{
			G_StopEffect( G_EffectIndex( "force/heal2" ), self->playerModel, self->chestBolt, self->s.number );
		}
		/*
		if ( self->headBolt != -1 )
		{
			G_StopEffect( G_EffectIndex( "force/heal_joint" ), self->playerModel, self->headBolt, self->s.number );
		}
		if ( self->cervicalBolt != -1 )
		{
			G_StopEffect( G_EffectIndex( "force/heal_joint" ), self->playerModel, self->cervicalBolt, self->s.number );
		}
		if ( self->chestBolt != -1 )
		{
			G_StopEffect( G_EffectIndex( "force/heal_joint" ), self->playerModel, self->chestBolt, self->s.number );
		}
		if ( self->gutBolt != -1 )
		{
			G_StopEffect( G_EffectIndex( "force/heal_joint" ), self->playerModel, self->gutBolt, self->s.number );
		}
		if ( self->kneeLBolt != -1 )
		{
			G_StopEffect( G_EffectIndex( "force/heal_joint" ), self->playerModel, self->kneeLBolt, self->s.number );
		}
		if ( self->kneeRBolt != -1 )
		{
			G_StopEffect( G_EffectIndex( "force/heal_joint" ), self->playerModel, self->kneeRBolt, self->s.number );
		}
		if ( self->elbowLBolt != -1 )
		{
			G_StopEffect( G_EffectIndex( "force/heal_joint" ), self->playerModel, self->elbowLBolt, self->s.number );
		}
		if ( self->elbowRBolt != -1 )
		{
			G_StopEffect( G_EffectIndex( "force/heal_joint" ), self->playerModel, self->elbowRBolt, self->s.number );
		}
		*/
	}
}

int FP_MaxForceHeal( gentity_t *self )
{
	if ( self->s.number >= MAX_CLIENTS )
	{
		return MAX_FORCE_HEAL_HARD;
	}
	switch ( g_spskill->integer )
	{
	case 0://easy
		return MAX_FORCE_HEAL_EASY;
		break;
	case 1://medium
		return MAX_FORCE_HEAL_MEDIUM;
		break;
	case 2://hard
	default:
		return MAX_FORCE_HEAL_HARD;
		break;
	}
}

int FP_ForceHealInterval( gentity_t *self )
{
	return (self->client->ps.forcePowerLevel[FP_HEAL]>FORCE_LEVEL_2)?50:FORCE_HEAL_INTERVAL;
}

void ForceHeal( gentity_t *self )
{
	if ( self->health <= 0 || self->client->ps.stats[STAT_MAX_HEALTH] <= self->health )
	{
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_HEAL, 20 ) )
	{//must have enough force power for at least 5 points of health
		return;
	}

	if ( self->painDebounceTime > level.time || (self->client->ps.weaponTime&&self->client->ps.weapon!=WP_NONE) )
	{//can't initiate a heal while taking pain or attacking
		return;
	}

	if ( self->client->ps.saberLockTime > level.time )
	{//FIXME: can this be a way to break out?
		return;
	}
	/*
	if ( self->client->ps.forcePowerLevel[FP_HEAL] > FORCE_LEVEL_2 )
	{//instant heal
		//no more than available force power
		int max = self->client->ps.forcePower;
		if ( max > MAX_FORCE_HEAL )
		{//no more than max allowed
			max = MAX_FORCE_HEAL;
		}
		if ( max > self->client->ps.stats[STAT_MAX_HEALTH] - self->health )
		{//no more than what's missing
			max = self->client->ps.stats[STAT_MAX_HEALTH] - self->health;
		}
		self->health += max;
		WP_ForcePowerDrain( self, FP_HEAL, max );
		G_SoundOnEnt( self, CHAN_VOICE, va( "sound/weapons/force/heal%d.mp3", Q_irand( 1, 4 ) ) );
	}
	else
	*/
	{
		//start health going up
		//NPC_SetAnim( self, SETANIM_TORSO, ?, SETANIM_FLAG_OVERRIDE );
		WP_ForcePowerStart( self, FP_HEAL, 0 );
		if ( self->client->ps.forcePowerLevel[FP_HEAL] < FORCE_LEVEL_2 )
		{//must meditate
			//FIXME: holster weapon (select WP_NONE?)
			//FIXME: BOTH_FORCEHEAL_START
			NPC_SetAnim( self, SETANIM_BOTH, BOTH_FORCEHEAL_START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
			self->client->ps.saberBlocked = BLOCKED_NONE;
			self->client->ps.torsoAnimTimer = self->client->ps.legsAnimTimer = FP_ForceHealInterval(self)*FP_MaxForceHeal(self) + 2000;//???
			WP_DeactivateSaber( self );//turn off saber when meditating
		}
		else
		{//just a quick gesture
			/*
			//Can't get an anim that looks good...
			NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCEHEAL_QUICK, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
			self->client->ps.saberBlocked = BLOCKED_NONE;
			*/
		}
	}

	//FIXME: always play healing effect
	G_SoundOnEnt( self, CHAN_ITEM, "sound/weapons/force/heal.mp3" );
}

extern void NPC_PlayConfusionSound( gentity_t *self );
extern void NPC_Jedi_PlayConfusionSound( gentity_t *self );
qboolean WP_CheckBreakControl( gentity_t *self )
{
	if ( !self )
	{
		return qfalse;
	}
	if ( !self->s.number )
	{//player
		if ( self->client && self->client->ps.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_3 )
		{//control-level
			if ( self->client->ps.viewEntity > 0 && self->client->ps.viewEntity < ENTITYNUM_WORLD )
			{//we are in a viewentity
				gentity_t *controlled = &g_entities[self->client->ps.viewEntity];
				if ( controlled->NPC && controlled->NPC->controlledTime > level.time )
				{//it is an NPC we controlled
					//clear it and return
					G_ClearViewEntity( self );
					return qtrue;
				}
			}
		}
	}
	else
	{//NPC
		if ( self->NPC && self->NPC->controlledTime > level.time )
		{//being controlled
			gentity_t *controller = &g_entities[0];
			if ( controller->client && controller->client->ps.viewEntity == self->s.number )
			{//we are being controlled by player
				if ( controller->client->ps.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_3 )
				{//control-level mind trick
					//clear the control and return
					G_ClearViewEntity( controller );
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}
extern bool	Pilot_AnyVehiclesRegistered();

void ForceTelepathy( gentity_t *self )
{
	trace_t	tr;
	vec3_t	end, forward;
	gentity_t	*traceEnt;
	qboolean	targetLive = qfalse;

	if ( WP_CheckBreakControl( self ) )
	{
		return;
	}
	if ( self->health <= 0 )
	{
		return;
	}
	//FIXME: if mind trick 3 and aiming at an enemy need more force power
	if ( !WP_ForcePowerUsable( self, FP_TELEPATHY, 0 ) )
	{
		return;
	}

	if ( self->client->ps.weaponTime >= 800 )
	{//just did one!
		return;
	}
	if ( self->client->ps.saberLockTime > level.time )
	{//FIXME: can this be a way to break out?
		return;
	}

	//JKEnhanced: along the crosshair ray (in third person that's the camera, not ps.viewangles)
	vec3_t	aimOrg;
	G_GetAimRay( self, aimOrg, forward );
	VectorNormalize( forward );
	VectorMA( aimOrg, 2048, forward, end );

	//Cause a distraction if enemy is not fighting
	gi.trace( &tr, aimOrg, vec3_origin, vec3_origin, end, self->s.number, MASK_OPAQUE|CONTENTS_BODY, (EG2_Collision)0, 0 );
	if ( tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
	{
		return;
	}

	traceEnt = &g_entities[tr.entityNum];

	if( traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE )
	{
		return;
	}

	if ( traceEnt && traceEnt->client  )
	{
		switch ( traceEnt->client->NPC_class )
		{
		case CLASS_GALAKMECH://cant grip him, he's in armor
		case CLASS_ATST://much too big to grip!
		//no droids either
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
			break;
		case CLASS_RANCOR:
			if ( !(traceEnt->spawnflags&1) )
			{
				targetLive = qtrue;
			}
			break;
		default:
			targetLive = qtrue;
			break;
		}
	}
	if ( targetLive
		&& traceEnt->NPC
		&& traceEnt->health > 0
		&& traceEnt->NPC->darkCharmedTime < level.time
		&& traceEnt->NPC->insanityTime < level.time )
	{//hit an organic non-player
		if ( G_ActivateBehavior( traceEnt, BSET_MINDTRICK ) )
		{//activated a script on him
			//FIXME: do the visual sparkles effect on their heads, still?
			WP_ForcePowerStart( self, FP_TELEPATHY, 0 );
		}
		else if ( traceEnt->client->playerTeam != self->client->playerTeam )
		{//an enemy
			int override = 0;
			if ( (traceEnt->NPC->scriptFlags&SCF_NO_MIND_TRICK) )
			{
				if ( traceEnt->client->NPC_class == CLASS_GALAKMECH )
				{
					G_AddVoiceEvent( traceEnt, Q_irand( EV_CONFUSE1, EV_CONFUSE3 ), Q_irand( 3000, 5000 ) );
				}
			}
			else if ( G_IsNeutralized( traceEnt ) && !G_IsDownedBody( traceEnt ) )
			{//JKEnhanced: already gave up (hands up, running, maimed) - L3 puts them to sleep. They're already
			 //counted as subdued and unarmed, so it's the plain cost, not the 75 of a fresh pacify
				if ( self->client->ps.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_2 && g_mindTrickPacify->integer )
				{
					const int wasState = traceEnt->NPC->neutralState;
					G_NeutralEnterState( traceEnt, NS_OUT_COLD );
					if ( traceEnt->ghoul2.size() && traceEnt->headBolt != -1 )
					{
						G_PlayEffect( G_EffectIndex( "force/confusion" ), traceEnt->playerModel, traceEnt->headBolt, traceEnt->s.number, traceEnt->currentOrigin, 2000, qtrue );
					}
					if ( g_debugForce && g_debugForce->integer )
					{
						gi.Printf( "  MINDTRICK player -> %s: was %s, now out cold\n", traceEnt->NPC_type, G_NeutralStateName( wasState ) );
					}
				}
				else
				{
					NPC_PlayConfusionSound( traceEnt );
				}
			}
			else if ( self->client->ps.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_3 )
			{//control them, even jedi
				G_SetViewEntity( self, traceEnt );
				traceEnt->NPC->controlledTime = level.time + 30000;
			}
			else if ( traceEnt->s.weapon != WP_SABER
				&& traceEnt->client->NPC_class != CLASS_REBORN )
			{//haha!  Jedi aren't easily confused!
				if ( self->client->ps.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_2
					&& traceEnt->s.weapon != WP_NONE		//don't charm people who aren't capable of fighting... like ugnaughts and droids, just confuse them
					&& traceEnt->client->NPC_class != CLASS_TUSKEN//don't charm them, just confuse them
					&& traceEnt->client->NPC_class != CLASS_NOGHRI//don't charm them, just confuse them
					&& !Pilot_AnyVehiclesRegistered()		//also, don't charm guys when bikes are near
					)
				{//turn them to our side
					//if mind trick 3 and aiming at an enemy need more force power
					const qboolean pacify = (qboolean)( g_mindTrickPacify->integer && G_CanNeutralize( traceEnt, NH_PACIFY ) );
					override = pacify ? 75 : 50;//putting someone out for good costs most of the pool
					if ( self->client->ps.forcePower < override )
					{
						return;
					}
					if ( traceEnt->enemy )
					{
						G_ClearEnemy( traceEnt );
					}
					if ( pacify )
					{//they're done: out cold where they stand (counts as subdued)
						G_Neutralize( traceEnt, self, NH_PACIFY );
					}
					else
					{//the old way: they fight for us for a while
						if ( traceEnt->NPC )
						{
							//traceEnt->NPC->tempBehavior = BS_FOLLOW_LEADER;
							traceEnt->client->leader = self;
						}
						//FIXME: maybe pick an enemy right here?
						//FIXME: does nothing to TEAM_FREE and TEAM_NEUTRALs!!!
						team_t	saveTeam = traceEnt->client->enemyTeam;
						traceEnt->client->enemyTeam = traceEnt->client->playerTeam;
						traceEnt->client->playerTeam = saveTeam;
						//FIXME: need a *charmed* timer on this...?  Or do TEAM_PLAYERS assume that "confusion" means they should switch to team_enemy when done?
						traceEnt->NPC->charmedTime = level.time + mindTrickTime[self->client->ps.forcePowerLevel[FP_TELEPATHY]];
					}
					if ( traceEnt->ghoul2.size() && traceEnt->headBolt != -1 )
					{//FIXME: what if already playing effect?
						G_PlayEffect( G_EffectIndex( "force/confusion" ), traceEnt->playerModel, traceEnt->headBolt, traceEnt->s.number, traceEnt->currentOrigin, mindTrickTime[self->client->ps.forcePowerLevel[FP_TELEPATHY]], qtrue );
					}
				}
				else
				{//just confuse them
					//somehow confuse them?  Set don't fire to true for a while?  Drop their aggression?  Maybe just take their enemy away and don't let them pick one up for a while unless shot?
					traceEnt->NPC->confusionTime = level.time + mindTrickTime[self->client->ps.forcePowerLevel[FP_TELEPATHY]];//confused for about 10 seconds
					AI_AwarenessReset( traceEnt, 0 );//and whatever they'd noticed of you is gone
					if ( traceEnt->ghoul2.size() && traceEnt->headBolt != -1 )
					{//FIXME: what if already playing effect?
						G_PlayEffect( G_EffectIndex( "force/confusion" ), traceEnt->playerModel, traceEnt->headBolt, traceEnt->s.number, traceEnt->currentOrigin, mindTrickTime[self->client->ps.forcePowerLevel[FP_TELEPATHY]], qtrue );
					}
					NPC_PlayConfusionSound( traceEnt );
					if ( traceEnt->enemy )
					{
						G_ClearEnemy( traceEnt );
					}
				}
			}
			else
			{
				NPC_Jedi_PlayConfusionSound( traceEnt );
			}
			WP_ForcePowerStart( self, FP_TELEPATHY, override );
		}
		else if ( traceEnt->client->playerTeam == self->client->playerTeam )
		{//an ally
			//maybe just have him look at you?  Respond?  Take your enemy?
			if ( traceEnt->client->ps.pm_type < PM_DEAD && traceEnt->NPC!=NULL && !(traceEnt->NPC->scriptFlags&SCF_NO_RESPONSE) )
			{
				NPC_UseResponse( traceEnt, self, qfalse );
				WP_ForcePowerStart( self, FP_TELEPATHY, 1 );
			}
		}//NOTE: no effect on TEAM_NEUTRAL?
		vec3_t	eyeDir;
		AngleVectors( traceEnt->client->renderInfo.eyeAngles, eyeDir, NULL, NULL );
		VectorNormalize( eyeDir );
		G_PlayEffect( "force/force_touch", traceEnt->client->renderInfo.eyePoint, eyeDir );

		//make sure this plays and that you cannot press fire for about 1 second after this
		//FIXME: BOTH_FORCEMINDTRICK or BOTH_FORCEDISTRACT
		NPC_SetAnim( self, SETANIM_TORSO, BOTH_MINDTRICK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_RESTART|SETANIM_FLAG_HOLD );
		//FIXME: build-up or delay this until in proper part of anim
	}
	else
	{
		if ( self->client->ps.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_1 && DistanceSquared( tr.endpos, self->currentOrigin ) > 64 * 64 )
		{//don't create a diversion less than 64 from you of if at power level 1
			//use distraction anim instead
			G_PlayEffect( G_EffectIndex( "force/force_touch" ), tr.endpos, tr.plane.normal );
			//FIXME: these events don't seem to always be picked up...?
			AddSoundEvent( self, tr.endpos, 512, AEL_SUSPICIOUS, qtrue, qtrue );
			AddSightEvent( self, tr.endpos, 512, AEL_SUSPICIOUS, 50 );
			WP_ForcePowerStart( self, FP_TELEPATHY, 0 );
		}
		NPC_SetAnim( self, SETANIM_TORSO, BOTH_MINDTRICK2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_RESTART|SETANIM_FLAG_HOLD );
	}
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;
	self->client->ps.weaponTime = 1000;
	if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
	{
		self->client->ps.weaponTime = floor( self->client->ps.weaponTime * g_timescale->value );
	}
}

//rww - RAGDOLL_BEGIN
//#define JK2_RAGDOLL_GRIPNOHEALTH
//rww - RAGDOLL_END

/*
JKEnhanced 4B.5: the player's Grip also takes bodies, cut limbs, items and loose props. L2 lifts them,
L3 carries them, and attacking while carrying throws whatever's held (G_GripThrow) - a thrown body hits
like a pushed one (ClientImpacts), a thrown object hits the first person in its path
(WP_GripThrownUpdate). Holding one costs a slow trickle, not the choke cost.
*/
#define GRIP_THROW_SPEED		900.0f
#define GRIP_THROW_TRACK_MS		1500
static qboolean	s_gripHeldObject[MAX_GENTITIES];	// [gripper]: what we hold is a body / object, not a live target
static struct
{
	int		entNum;		// ENTITYNUM_NONE / 0 = nothing in flight
	int		until;
	vec3_t	lastPos;
} s_gripThrown[MAX_CLIENTS];

static qboolean WP_GripGrabbableObject( const gentity_t *ent )
{
	if ( !ent || !ent->inuse || ent->bmodel || ent->client
		|| (ent->s.eFlags&EF_NODRAW) || (ent->svFlags&SVF_NOCLIENT) )
	{
		return qfalse;
	}
	return (qboolean)( ent->s.eType == ET_ITEM
		|| !Q_stricmp( "limb", ent->classname )
		|| ent->e_ThinkFunc == thinkF_G_RunObject );
}

//a corpse, or a knocked-out NPC (carried like one: no choke, no waking up)
static qboolean WP_GripBody( const gentity_t *ent )
{
	return (qboolean)( ent && ent->inuse && ent->client && G_IsDownedBody( ent )
		&& !(ent->s.eFlags&EF_NODRAW) && !(ent->s.eFlags&EF_DISINTEGRATION)
		&& ent->client->NPC_class != CLASS_VEHICLE && ent->client->NPC_class != CLASS_ATST
		&& ent->client->NPC_class != CLASS_RANCOR && ent->client->NPC_class != CLASS_WAMPA
		&& ent->client->NPC_class != CLASS_SAND_CREATURE );
}

static qboolean WP_GripLivingTarget( const gentity_t *self, const gentity_t *ent )
{
	return (qboolean)( ent->client && ent->health > 0 && !G_IsDownedBody( ent )
		&& !(ent->s.eFlags&EF_NODRAW) && !(ent->flags&FL_NOTARGET)
		&& !( self->enemy && ent != self->enemy && ent->client->playerTeam == self->client->playerTeam ) );
}

//the best target near where the player aims: whatever is closest to the crosshair, not the last enemy
//(that one could be off at the edge of the screen). The straight trace misses items, and fresh bodies
//have no contents for a moment. Bodies / objects only count at Grip 2+.
static gentity_t *WP_GripFindObject( gentity_t *self )
{
	const qboolean objects = (qboolean)( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 );
	gentity_t	*list[MAX_GENTITIES];
	vec3_t		mins, maxs, fwd, dir, center;
	gentity_t	*best = NULL;
	float		bestDot = 0.93f;
	vec3_t		aimOrg, aimDir;

	//pick along the crosshair ray (in third person that's the camera, not ps.viewangles - Push already
	//aims this way); reach and line of sight are still from the body's eyes
	G_GetAimRay( self, aimOrg, fwd );
	for ( int i = 0; i < 3; i++ )
	{
		mins[i] = self->currentOrigin[i] - FORCE_GRIP_DIST;
		maxs[i] = self->currentOrigin[i] + FORCE_GRIP_DIST;
	}
	const int num = gi.EntitiesInBox( mins, maxs, list, MAX_GENTITIES );
	for ( int i = 0; i < num; i++ )
	{
		gentity_t *ent = list[i];
		if ( ent == self
			|| !( WP_GripLivingTarget( self, ent ) || ( objects && ( WP_GripGrabbableObject( ent ) || WP_GripBody( ent ) ) ) ) )
		{
			continue;
		}
		VectorAdd( ent->absmin, ent->absmax, center );
		VectorScale( center, 0.5f, center );
		VectorSubtract( center, self->client->renderInfo.eyePoint, dir );
		if ( VectorNormalize( dir ) > FORCE_GRIP_DIST )
		{
			continue;
		}
		VectorSubtract( center, aimOrg, aimDir );
		VectorNormalize( aimDir );
		const float dot = DotProduct( aimDir, fwd );
		if ( dot <= bestDot )
		{
			continue;
		}
		trace_t tr;
		gi.trace( &tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, center, self->s.number, MASK_FORCE_PUSH, (EG2_Collision)0, 0 );
		if ( tr.fraction < 1.0f && tr.entityNum != ent->s.number )
		{
			continue;
		}
		best = ent;
		bestDot = dot;
	}
	return best;
}

//non-client objects run their own physics: hand them back to it
static void WP_GripObjectPhysics( gentity_t *ent )
{
	if ( ent->s.eType == ET_ITEM )
	{//G_RunFrame runs items itself
		return;
	}
	ent->e_ThinkFunc = !Q_stricmp( "limb", ent->classname ) ? thinkF_LimbThink : thinkF_G_RunObject;
	ent->nextthink = level.time + FRAMETIME;
}

//a held object is moved here, not by its own physics: re-basing a linear trajectory every frame never
//moved it on the server (jitter), and a limb resting in the floor started every trace solid (stuck).
//TR_INTERPOLATE makes the client lerp between frames; trDelta keeps the carry speed for the drop.
static void WP_GripMoveObject( gentity_t *self, gentity_t *ent, const vec3_t gripOrg, qboolean carry )
{
	vec3_t	step, newOrg, oldOrg;
	VectorCopy( ent->currentOrigin, oldOrg );
	VectorSubtract( gripOrg, oldOrg, step );
	VectorScale( step, carry ? 0.5f : 0.25f, step );//eases in instead of snapping
	const float len = VectorLength( step );
	if ( len > 64.0f )
	{
		VectorScale( step, 64.0f / len, step );
	}
	VectorAdd( oldOrg, step, newOrg );
	const int mask = ent->clipmask ? ent->clipmask : MASK_SOLID;
	trace_t tr;
	gi.trace( &tr, oldOrg, ent->mins, ent->maxs, newOrg, ent->s.number, mask, (EG2_Collision)0, 0 );
	if ( tr.startsolid || tr.allsolid )
	{//embedded (a limb lying in the floor): take the move if where it's going is clear
		gi.trace( &tr, newOrg, ent->mins, ent->maxs, newOrg, ent->s.number, mask, (EG2_Collision)0, 0 );
		if ( !tr.startsolid && !tr.allsolid )
		{
			VectorCopy( newOrg, tr.endpos );
		}
		else
		{//still stuck: lift it free of the floor
			VectorCopy( oldOrg, tr.endpos );
			tr.endpos[2] += 4.0f;
		}
	}
	VectorCopy( tr.endpos, ent->currentOrigin );
	VectorCopy( tr.endpos, ent->s.pos.trBase );
	VectorSubtract( tr.endpos, oldOrg, ent->s.pos.trDelta );
	VectorScale( ent->s.pos.trDelta, 1000.0f / FRAMETIME, ent->s.pos.trDelta );
	ent->s.pos.trType = TR_INTERPOLATE;
	ent->s.pos.trTime = level.time;
	if ( ent->s.eType == ET_ITEM )
	{//G_RunItem turns an item with no ground into a falling one
		ent->s.groundEntityNum = ENTITYNUM_WORLD;
	}
	gi.linkentity( ent );
	if ( ent->s.eType != ET_ITEM )
	{//its own physics would bounce / stop it under us; WP_GripObjectPhysics hands it back on the drop
	 //(this also clears a resting limb's removal timer)
		ent->e_ThinkFunc = thinkF_NULL;
		ent->nextthink = 0;
	}
}

//JKEnhanced: weapon / Force cycling (the mouse wheel) while carrying something at Grip 3 moves it
//closer or farther instead. Called from cgame (one DLL).
static float s_gripCarryDist[MAX_CLIENTS];	// 0 = keep it where it was picked up

/*
WP_GripLoadReset (JKEnhanced)

What Grip is holding (s_gripHeldObject / s_gripCarryDist) isn't saved, so after a load a carried body
or knocked-out NPC was treated as a live target (choke anim, choke damage, the victim turning to face
you). A load lets go of any grip in progress instead.
*/
void WP_GripLoadReset( gentity_t *self )
{
	if ( self && self->client && ( self->client->ps.forcePowersActive & ( 1 << FP_GRIP ) ) )
	{
		WP_ForcePowerStop( self, FP_GRIP );
	}
	memset( s_gripHeldObject, 0, sizeof( s_gripHeldObject ) );
	memset( s_gripCarryDist, 0, sizeof( s_gripCarryDist ) );
}
#define GRIP_SCROLL_STEP		32.0f
#define GRIP_SCROLL_MIN_DIST	64.0f
#define GRIP_SCROLL_MAX_DIST	384.0f
static int s_gripPickupTime[MAX_CLIENTS];	// when Grip last pulled an item in and picked it up
#define GRIP_PICKUP_SCROLL_BLOCK	600
qboolean WP_GripScroll( gentity_t *self, int dir )
{
	if ( self && self->s.number >= 0 && self->s.number < MAX_CLIENTS
		&& s_gripPickupTime[self->s.number] <= level.time//(later = stale, from before a load)
		&& level.time - s_gripPickupTime[self->s.number] < GRIP_PICKUP_SCROLL_BLOCK )
	{//just reeled an item in with the wheel: the next few clicks don't switch weapons / powers
		return qtrue;
	}
	if ( !self || !self->client || self->s.number >= MAX_CLIENTS || self->health <= 0
		|| !(self->client->ps.forcePowersActive&(1<<FP_GRIP))
		|| self->client->ps.forcePowerLevel[FP_GRIP] < FORCE_LEVEL_3
		|| self->client->ps.forceGripEntityNum >= ENTITYNUM_WORLD )
	{
		return qfalse;
	}
	const gentity_t *held = &g_entities[self->client->ps.forceGripEntityNum];
	if ( held->client && ( held->message || (held->flags&FL_NO_KNOCKBACK) ) )
	{//only lifted in place, not carried
		return qfalse;
	}
	float d = s_gripCarryDist[self->s.number];
	if ( d <= 0.0f )
	{
		d = Q_min( FORCE_GRIP_3_MAX_DIST, Q_max( FORCE_GRIP_3_MIN_DIST, Distance( self->client->renderInfo.handLPoint, held->currentOrigin ) ) );
	}
	s_gripCarryDist[self->s.number] = Q_min( GRIP_SCROLL_MAX_DIST, Q_max( GRIP_SCROLL_MIN_DIST, d + dir * GRIP_SCROLL_STEP ) );
	return qtrue;
}

static void G_GripThrow( gentity_t *self, gentity_t *ent )
{
	vec3_t	fwd, aimEnd, from, aimOrg;
	//the crosshair ray (in third person the camera's, which isn't ps.viewangles)
	G_GetAimRay( self, aimOrg, fwd );

	WP_ForcePowerStop( self, FP_GRIP );	//lets go (and drops a body / object)
	if ( !ent || !ent->inuse )
	{
		return;
	}
	//it flies at what the crosshair is on, not just parallel to the view (it's off to one side of it)
	VectorMA( aimOrg, 4096.0f, fwd, aimEnd );
	trace_t aimTr;
	gi.trace( &aimTr, aimOrg, vec3_origin, vec3_origin, aimEnd, self->s.number, MASK_SHOT, (EG2_Collision)0, 0 );
	VectorAdd( ent->absmin, ent->absmax, from );
	VectorScale( from, 0.5f, from );
	vec3_t toAim;
	VectorSubtract( aimTr.endpos, from, toAim );
	if ( aimTr.entityNum != ent->s.number//aiming right at it: straight ahead
		&& VectorNormalize( toAim ) > 48.0f && DotProduct( toAim, fwd ) > 0.5f )
	{
		VectorCopy( toAim, fwd );
	}
	NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCEPUSH, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART );
	G_Sound( self, G_SoundIndex( "sound/weapons/force/push.wav" ) );
	if ( ent->client )
	{
		if ( ent->health > 0 && !G_IsDownedBody( ent ) )
		{//goes flying in a knockdown (the knocked out just fly, limp)
			G_Knockdown( ent, self, fwd, 300, qtrue );
		}
		VectorScale( fwd, GRIP_THROW_SPEED, ent->client->ps.velocity );
		ent->client->ps.velocity[2] += 150.0f;
		ent->client->ps.groundEntityNum = ENTITYNUM_NONE;
		ent->forcePuller = self->s.number;//a body hits like a pushed one (ClientImpacts), but not me
		ent->forcePushTime = level.time + 600;
	}
	else
	{
		VectorCopy( ent->currentOrigin, ent->s.pos.trBase );
		VectorScale( fwd, GRIP_THROW_SPEED, ent->s.pos.trDelta );
		ent->s.pos.trDelta[2] += 100.0f;
		ent->s.pos.trType = TR_GRAVITY;
		ent->s.pos.trTime = level.time;
		ent->forcePuller = self->s.number;
		ent->forcePushTime = level.time + 600;
		WP_GripObjectPhysics( ent );
		if ( self->s.number < MAX_CLIENTS )
		{
			s_gripThrown[self->s.number].entNum = ent->s.number;
			s_gripThrown[self->s.number].until = level.time + GRIP_THROW_TRACK_MS;
			VectorCopy( ent->currentOrigin, s_gripThrown[self->s.number].lastPos );
		}
	}
	WP_ForcePowerDrain( self, FP_GRIP, 5 );
	if ( g_debugForce && g_debugForce->integer )
	{
		gi.Printf( "  GRIP %s threw %s (%s)\n", WP_ForceDebugWho( self ), ent->client ? WP_ForceDebugWho( ent ) : ent->classname,
			ent->client ? ( ent->health > 0 ? "alive" : "body" ) : "object" );
	}
}

//a thrown object hits the first person in its path
static void WP_GripThrownUpdate( gentity_t *self )
{
	if ( self->s.number >= MAX_CLIENTS || !s_gripThrown[self->s.number].entNum )
	{
		return;
	}
	const int n = s_gripThrown[self->s.number].entNum;
	gentity_t *obj = &g_entities[n];
	if ( n <= 0 || n >= ENTITYNUM_WORLD || !obj->inuse || obj->client
		|| level.time > s_gripThrown[self->s.number].until
		|| s_gripThrown[self->s.number].until > level.time + GRIP_THROW_TRACK_MS )//stale (loaded save)
	{
		s_gripThrown[self->s.number].entNum = 0;
		return;
	}
	static const vec3_t boxMins = { -8, -8, -8 }, boxMaxs = { 8, 8, 8 };
	trace_t tr;
	gi.trace( &tr, s_gripThrown[self->s.number].lastPos, boxMins, boxMaxs, obj->currentOrigin, n, MASK_SHOT, (EG2_Collision)0, 0 );
	VectorCopy( obj->currentOrigin, s_gripThrown[self->s.number].lastPos );
	if ( tr.fraction >= 1.0f || tr.entityNum >= ENTITYNUM_WORLD || tr.entityNum == self->s.number )
	{
		return;
	}
	gentity_t *victim = &g_entities[tr.entityNum];
	if ( !victim->client || victim->health <= 0 )
	{
		return;
	}
	vec3_t dir;
	VectorCopy( obj->s.pos.trDelta, dir );
	VectorNormalize( dir );
	const int dmg = ( obj->e_ThinkFunc == thinkF_G_RunObject ) ? 25 : 10;	//props hit harder than items / limbs
	G_Damage( victim, obj, self, dir, tr.endpos, dmg, DAMAGE_NO_ARMOR, MOD_IMPACT );
	if ( victim->health > 0 )
	{
		G_Knockdown( victim, self, dir, 200, qtrue );
	}
	VectorScale( obj->s.pos.trDelta, 0.3f, obj->s.pos.trDelta );	//it stops short on them
	VectorCopy( obj->currentOrigin, obj->s.pos.trBase );
	obj->s.pos.trTime = level.time;
	s_gripThrown[self->s.number].entNum = 0;
	if ( g_debugForce && g_debugForce->integer )
	{
		gi.Printf( "  GRIP thrown %s hit %s for %d\n", obj->classname, WP_ForceDebugWho( victim ), dmg );
	}
}

void ForceGrip( gentity_t *self )
{//FIXME: make enemy Jedi able to use this
	trace_t	tr;
	vec3_t	end, forward;
	gentity_t	*traceEnt = NULL;

	if ( self->health <= 0 )
	{
		return;
	}
	if ( !self->s.number && (cg.zoomMode || in_camera) )
	{//can't force grip when zoomed in or in cinematic
		return;
	}
	if ( self->client->ps.leanofs )
	{//can't force-grip while leaning
		return;
	}

	if ( self->client->ps.forceGripEntityNum <= ENTITYNUM_WORLD )
	{//already gripping
		if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
		{
			self->client->ps.forcePowerDuration[FP_GRIP] = level.time + 100;
			self->client->ps.weaponTime = 1000;
			if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
			{
				self->client->ps.weaponTime = floor( self->client->ps.weaponTime * g_timescale->value );
			}
		}
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_GRIP, 0 ) )
	{//can't use it right now
		return;
	}

	if ( self->client->ps.forcePower < 26 )
	{//need 20 to start, 6 to hold it for any decent amount of time...
		return;
	}

	if ( self->client->ps.weaponTime )
	{//busy
		return;
	}

	if ( self->client->ps.saberLockTime > level.time )
	{//FIXME: can this be a way to break out?
		return;
	}
	//Cause choking anim + health drain in ent in front of me
	NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCEGRIP_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;

	self->client->ps.weaponTime = 1000;
	if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
	{
		self->client->ps.weaponTime = floor( self->client->ps.weaponTime * g_timescale->value );
	}

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );
	VectorMA( self->client->renderInfo.handLPoint, FORCE_GRIP_DIST, forward, end );

	if ( self->enemy && self->s.number >= MAX_CLIENTS )
	{//I have an enemy (JKEnhanced: NPCs only - the player gets what's nearest the crosshair, below; this
	 //used to grab the last enemy anywhere within ~78 degrees of the view)
		if ( !self->enemy->message
			&& !(self->flags&FL_NO_KNOCKBACK) )
		{//don't auto-pickup guys with keys
			if ( DistanceSquared( self->enemy->currentOrigin, self->currentOrigin ) < FORCE_GRIP_DIST_SQUARED )
			{//close enough to grab
				float minDot = 0.5f;
				if ( self->s.number < MAX_CLIENTS )
				{//player needs to be facing more directly
					minDot = 0.2f;
				}
				if ( InFront( self->enemy->currentOrigin, self->client->renderInfo.eyePoint, self->client->ps.viewangles, minDot ) ) //self->s.number || //NPCs can always lift enemy since we assume they're looking at them...?
				{//need to be facing the enemy
					if ( gi.inPVS( self->enemy->currentOrigin, self->client->renderInfo.eyePoint ) )
					{//must be in PVS
						gi.trace( &tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, self->enemy->currentOrigin, self->s.number, MASK_SHOT, (EG2_Collision)0, 0 );
						if ( tr.fraction == 1.0f || tr.entityNum == self->enemy->s.number )
						{//must have clear LOS
							traceEnt = self->enemy;
						}
					}
				}
			}
		}
	}
	if ( !traceEnt )
	{//okay, trace straight ahead and see what's there
		gi.trace( &tr, self->client->renderInfo.handLPoint, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT, (EG2_Collision)0, 0 );
		if ( !( tr.entityNum >= ENTITYNUM_WORLD || tr.fraction == 1.0 || tr.allsolid || tr.startsolid ) )
		{
			traceEnt = &g_entities[tr.entityNum];
		}
		if ( self->s.number < MAX_CLIENTS
			&& ( !traceEnt || ( !traceEnt->client && !WP_GripGrabbableObject( traceEnt ) ) ) )
		{//JKEnhanced 4B.5: nothing grippable dead ahead - whoever / whatever is nearest the crosshair
			gentity_t *obj = WP_GripFindObject( self );
			if ( obj )
			{
				traceEnt = obj;
			}
		}
		if ( !traceEnt )
		{
			return;
		}
	}
	//JKEnhanced 4B.5: the player can hold a body or a loose object (no choke, no resist)
	const qboolean heldObject = (qboolean)( self->s.number < MAX_CLIENTS && self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1//L1 only chokes in place
		&& ( WP_GripBody( traceEnt ) || WP_GripGrabbableObject( traceEnt ) ) );
//rww - RAGDOLL_BEGIN
#ifdef JK2_RAGDOLL_GRIPNOHEALTH
	if ( !traceEnt || traceEnt == self/*???*/ || traceEnt->bmodel || (traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE) )
	{
		return;
	}
#else
//rww - RAGDOLL_END
	if ( !traceEnt || traceEnt == self/*???*/ || traceEnt->bmodel || (traceEnt->health <= 0 && traceEnt->takedamage && !heldObject) || (traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE && !heldObject) )
	{
		return;
	}
//rww - RAGDOLL_BEGIN
#endif
//rww - RAGDOLL_END

	if ( !heldObject && traceEnt->m_pVehicle != NULL )
	{//is it a vehicle
		//grab pilot if there is one
		if ( traceEnt->m_pVehicle->m_pPilot != NULL
			&& traceEnt->m_pVehicle->m_pPilot->client != NULL )
		{//grip the pilot
			traceEnt = traceEnt->m_pVehicle->m_pPilot;
		}
		else
		{//can't grip a vehicle
			return;
		}
	}
	if ( heldObject )
	{//a body / object: none of the live-target checks below (resists, Absorb, classes)
	}
	else if ( traceEnt->client )
	{
		if ( traceEnt->client->ps.forceJumpZStart )
		{//can't catch them in mid force jump - FIXME: maybe base it on velocity?
			return;
		}
		if ( traceEnt->client->ps.pullAttackTime > level.time )
		{//can't grip someone who is being pull-attacked or is pull-attacking
			return;
		}
		if ( !Q_stricmp("Yoda",traceEnt->NPC_type) )
		{
			Jedi_PlayDeflectSound( traceEnt );
			ForceThrow( traceEnt, qfalse );
			return;
		}

		if ( G_IsRidingVehicle( traceEnt )
			&& (traceEnt->s.eFlags&EF_NODRAW) )
		{//riding *inside* vehicle
			return;
		}

		switch ( traceEnt->client->NPC_class )
		{
		case CLASS_GALAKMECH://cant grip him, he's in armor
			G_AddVoiceEvent( traceEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), Q_irand( 3000, 5000 ) );
			return;
			break;
		case CLASS_HAZARD_TROOPER://cant grip him, he's in armor
			return;
			break;
		case CLASS_ATST://much too big to grip!
		case CLASS_RANCOR://much too big to grip!
		case CLASS_WAMPA://much too big to grip!
		case CLASS_SAND_CREATURE://much too big to grip!
			return;
			break;
		//no droids either...?
		case CLASS_GONK:
		case CLASS_R2D2:
		case CLASS_R5D2:
		case CLASS_MARK1:
		case CLASS_MARK2:
		case CLASS_MOUSE://?
		case CLASS_PROTOCOL:
			//*sigh*... in JK3, you'll be able to grab and move *anything*...
			return;
			break;
		//not even combat droids?  (No animation for being gripped...)
		case CLASS_SABER_DROID:
		case CLASS_ASSASSIN_DROID:
			//*sigh*... in JK3, you'll be able to grab and move *anything*...
			return;
			break;
		case CLASS_PROBE:
		case CLASS_SEEKER:
		case CLASS_REMOTE:
		case CLASS_SENTRY:
		case CLASS_INTERROGATOR:
			//*sigh*... in JK3, you'll be able to grab and move *anything*...
			return;
			break;
		case CLASS_DESANN://Desann cannot be gripped, he just pushes you back instantly
		case CLASS_KYLE:
		case CLASS_TAVION:
		case CLASS_LUKE:
			Jedi_PlayDeflectSound( traceEnt );
			ForceThrow( traceEnt, qfalse );
			return;
			break;
		case CLASS_REBORN:
		case CLASS_SHADOWTROOPER:
		case CLASS_ALORA:
		case CLASS_JEDI:
			if ( traceEnt->NPC && traceEnt->NPC->rank > RANK_CIVILIAN && self->client->ps.forcePowerLevel[FP_GRIP] < FORCE_LEVEL_2 )
			{
				Jedi_PlayDeflectSound( traceEnt );
				ForceThrow( traceEnt, qfalse );
				return;
			}
			break;
		default:
			break;
		}
		if ( traceEnt->s.weapon == WP_EMPLACED_GUN && traceEnt->client->ps.eFlags & EF_LOCKED_TO_WEAPON )
		{//FIXME: maybe can pull them out?
			return;
		}
		if ( self->enemy && traceEnt != self->enemy && traceEnt->client->playerTeam == self->client->playerTeam )
		{//can't accidently grip your teammate in combat
			return;
		}
//=CHECKABSORB===
		if ( -1 != WP_AbsorbConversion( traceEnt, traceEnt->client->ps.forcePowerLevel[FP_ABSORB], self, FP_GRIP, self->client->ps.forcePowerLevel[FP_GRIP], 20) )//JKEnhanced: the grip start cost (was forcePowerNeeded indexed by LEVEL)
		{
			//WP_ForcePowerStop( self, FP_GRIP );
			return;
		}
//===============
	}
	else
	{//can't grip non-clients... right?
		//FIXME: Make it so objects flagged as "grabbable" are let through
		//if ( Q_stricmp( "misc_model_breakable", traceEnt->classname ) || !(traceEnt->s.eFlags&EF_BOUNCE_HALF) || !traceEnt->physicsBounce )
		{
			return;
		}
	}

	//JKEnhanced 4B.3: drops timed (L1-2) Protect / Absorb; an L3 toggle stays up
	WP_StopShieldsForAttack( self );

	WP_ForcePowerStart( self, FP_GRIP, heldObject ? 5 : 20 );
	//FIXME: rule out other things?
	//FIXME: Jedi resist, like the push and pull?
	self->client->ps.forceGripEntityNum = traceEnt->s.number;
	s_gripHeldObject[self->s.number] = heldObject;
	if ( self->s.number < MAX_CLIENTS )
	{
		s_gripCarryDist[self->s.number] = 0.0f;
	}
	if ( heldObject )
	{
		if ( g_debugForce && g_debugForce->integer )
		{
			gi.Printf( "  GRIP %s picked up %s\n", WP_ForceDebugWho( self ), traceEnt->client ? va( "the body of %s", WP_ForceDebugWho( traceEnt ) ) : traceEnt->classname );
		}
	}
	if ( traceEnt->NPC && !OnSameTeam( self, traceEnt ) && traceEnt->health > 0 && !heldObject )
	{
		AI_NerveEvent( traceEnt, NE_FORCE_ON_ME, 1.0f, self );
		AI_NerveWitness( traceEnt, NE_FORCE_ON_ME, self );
	}
	if ( traceEnt->client && !heldObject )
	{
		Vehicle_t *pVeh;
		if ( ( pVeh = G_IsRidingVehicle( traceEnt ) ) != NULL )
		{//riding vehicle? pull him off!
			//FIXME: if in an AT-ST or X-Wing, shouldn't do this... :)
			//pull him off of it
			//((CVehicleNPC *)traceEnt->NPC)->Eject( traceEnt );
			pVeh->m_pVehicleInfo->Eject( pVeh, traceEnt, qtrue );
			//G_DriveVehicle( traceEnt, NULL, NULL );
		}
		G_AddVoiceEvent( traceEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000 );
		if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2 || traceEnt->s.weapon == WP_SABER )
		{//if we pick up & carry, drop their weap
			if ( traceEnt->s.weapon
				&& traceEnt->client->NPC_class != CLASS_ROCKETTROOPER
				&& traceEnt->client->NPC_class != CLASS_VEHICLE
				&& traceEnt->client->NPC_class != CLASS_HAZARD_TROOPER
				&& traceEnt->client->NPC_class != CLASS_TUSKEN
				&& traceEnt->client->NPC_class != CLASS_BOBAFETT
				&& traceEnt->client->NPC_class != CLASS_ASSASSIN_DROID
				&& traceEnt->s.weapon != WP_CONCUSSION	// so rax can't drop his
				)
			{
				if ( traceEnt->client->NPC_class == CLASS_BOBAFETT )
				{//he doesn't drop them, just puts it away
					ChangeWeapon( traceEnt, WP_MELEE );
				}
				else if ( traceEnt->s.weapon == WP_MELEE )
				{//they can't take that away from me, oh no...
				}
				else if ( traceEnt->NPC
					&& (traceEnt->NPC->scriptFlags&SCF_DONT_FLEE) )
				{//*SIGH*... if an NPC can't flee, they can't run after and pick up their weapon, do don't drop it
				}
				else if ( traceEnt->s.weapon != WP_SABER )
				{
					WP_DropWeapon( traceEnt, NULL );
				}
				else
				{
					//turn it off?
					traceEnt->client->ps.SaberDeactivate();
					G_SoundOnEnt( traceEnt, CHAN_WEAPON, "sound/weapons/saber/saberoffquick.wav" );
				}
			}
		}
		//else FIXME: need a one-armed choke if we're not on a high enough level to make them drop their gun
		VectorCopy( traceEnt->client->renderInfo.headPoint, self->client->ps.forceGripOrg );
	}
	else
	{
		VectorCopy( traceEnt->currentOrigin, self->client->ps.forceGripOrg );
	}
	self->client->ps.forceGripOrg[2] += 48;//FIXME: define?
	if ( self->client->ps.forcePowerLevel[FP_GRIP] < FORCE_LEVEL_2 )
	{//just a duration
		self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + 250;
		self->client->ps.forcePowerDuration[FP_GRIP] = level.time + 5000;

		if ( self->m_pVehicle && self->m_pVehicle->m_pVehicleInfo->Inhabited( self->m_pVehicle ) )
		{//empty vehicles don't make gripped noise
			traceEnt->s.loopSound = G_SoundIndex( "sound/weapons/force/grip.mp3" );
		}
	}
	else
	{
		if ( self->client->ps.forcePowerLevel[FP_GRIP] == FORCE_LEVEL_2 )
		{//lifting sound?  or always?
		}
		//if ( traceEnt->s.number )
		{//picks them up for a second first
			self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + 1000;
		}
		/*
		else
		{//player should take damage right away
			self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + 250;
		}
		*/
	// force grip sound should only play when the target is alive?
	//	if (traceEnt->health>0)
	//	{
			self->s.loopSound = G_SoundIndex( "sound/weapons/force/grip.mp3" );
	//	}
	}
}

qboolean ForceLightningCheck2Handed( gentity_t *self )
{
	if ( self && self->client )
	{
		if ( self->s.weapon == WP_NONE
			||  self->s.weapon == WP_MELEE
			|| (self->s.weapon == WP_SABER && !self->client->ps.SaberActive()) )
		{
			return qtrue;
		}
	}
	return qfalse;
}

void ForceLightningAnim( gentity_t *self )
{
	if ( !self || !self->client )
	{
		return;
	}

	//one-handed lightning 2 and above
	int startAnim = BOTH_FORCELIGHTNING_START;
	int holdAnim = BOTH_FORCELIGHTNING_HOLD;

	if ( self->client->ps.forcePowerLevel[FP_LIGHTNING] >= FORCE_LEVEL_3
		&& ForceLightningCheck2Handed( self ) )
	{//empty handed lightning 3
		startAnim = BOTH_FORCE_2HANDEDLIGHTNING_START;
		holdAnim = BOTH_FORCE_2HANDEDLIGHTNING_HOLD;
	}

	//FIXME: if standing still, play on whole body?  Especially 2-handed version
	if ( self->client->ps.torsoAnim == startAnim )
	{
		if ( !self->client->ps.torsoAnimTimer )
		{
			NPC_SetAnim( self, SETANIM_TORSO, holdAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
		else
		{
			NPC_SetAnim( self, SETANIM_TORSO, startAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
	}
	else
	{
		NPC_SetAnim( self, SETANIM_TORSO, holdAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	}
}

void ForceLightning( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}
	if ( !self->s.number && (cg.zoomMode || in_camera) )
	{//can't force lightning when zoomed in or in cinematic
		return;
	}
	if ( self->client->ps.leanofs )
	{//can't force-lightning while leaning
		return;
	}
	if ( self->client->ps.forcePower < 25 || !WP_ForcePowerUsable( self, FP_LIGHTNING, 0 ) )
	{
		return;
	}
	if ( self->client->ps.forcePowerDebounce[FP_LIGHTNING] > level.time )
	{//stops it while using it and also after using it, up to 3 second delay
		return;
	}
	if ( self->client->ps.saberLockTime > level.time )
	{//FIXME: can this be a way to break out?
		return;
	}
	//JKEnhanced 4B.3: drops timed (L1-2) Protect / Absorb; an L3 toggle stays up
	WP_StopShieldsForAttack( self );
	//Shoot lightning from hand
	//make sure this plays and that you cannot press fire for about 1 second after this
	if ( self->client->ps.forcePowerLevel[FP_LIGHTNING] < FORCE_LEVEL_2 )
	{
		NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCELIGHTNING, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	}
	else
	{
		ForceLightningAnim( self );
		/*
		if ( ForceLightningCheck2Handed( self ) )
		{//empty handed lightning 3
			NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCE_2HANDEDLIGHTNING_START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
		else
		{//one-handed lightning 3
			NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCELIGHTNING_START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
		*/
	}
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;

	G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/lightning.wav" );
	if ( self->client->ps.forcePowerLevel[FP_LIGHTNING] < FORCE_LEVEL_2 )
	{//short burst
		//G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/lightning.wav" );
	}
	else
	{//holding it
		self->s.loopSound = G_SoundIndex( "sound/weapons/force/lightning2.wav" );
	}

	//FIXME: build-up or delay this until in proper part of anim
	self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
	WP_ForcePowerStart( self, FP_LIGHTNING, self->client->ps.torsoAnimTimer );
}

void ForceLightningDamage( gentity_t *self, gentity_t *traceEnt, vec3_t dir, float dist, float dot, vec3_t impactPoint )
{
	
	qboolean blockedWithSaber = qfalse;
	qboolean blockedWithHand = qfalse;

	if (traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE)
	{
		return;
	}

	if ( traceEnt && traceEnt->takedamage )
	{
		if ( !traceEnt->client || traceEnt->client->playerTeam != self->client->playerTeam || self->enemy == traceEnt || traceEnt->enemy == self )
		{//an enemy or object
			G_DamageGuard( traceEnt, self, 4 );	//lightning wears down saber guard per zap tick
			if ( traceEnt->NPC )
			{//(debounced, so the per-tick calls don't pile up)
				AI_NerveEvent( traceEnt, NE_FORCE_ON_ME, 1.0f, self );
				AI_NerveWitness( traceEnt, NE_FORCE_ON_ME, self );
			}
			int	dmg;
			//FIXME: check for client using FP_ABSORB
			if ( self->client->ps.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_2 )
			{//more damage if closer and more in front
				dmg = 1;
				if ( self->client->NPC_class == CLASS_REBORN
					&& self->client->ps.weapon == WP_NONE )
				{//Cultist: looks fancy, but does less damage
				}
				else
				{
					if ( dist < 100 )
					{
						dmg += 2;
					}
					else if ( dist < 200 )
					{
						dmg += 1;
					}
					if ( dot > 0.9f )
					{
						dmg += 2;
					}
					else if ( dot > 0.7f )
					{
						dmg += 1;
					}
				}
				if ( self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
					|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE )
				{//jackin' 'em up, Palpatine-style
					dmg *= 2;
				}
			}
			else
			{
				dmg = Q_irand( 1, 3 );//*self->client->ps.forcePowerLevel[FP_LIGHTNING];
			}

			if ( traceEnt->client
				&& traceEnt->health > 0
				&& traceEnt->NPC
				&& (traceEnt->NPC->aiFlags&NPCAI_BOSS_CHARACTER) )
			{//Luke, Desann Tavion and Kyle can shield themselves from the attack
				//FIXME: shield effect or something?
				int parts;
				if ( traceEnt->client->ps.groundEntityNum != ENTITYNUM_NONE && !PM_SpinningSaberAnim( traceEnt->client->ps.legsAnim ) && !PM_FlippingAnim( traceEnt->client->ps.legsAnim ) && !PM_RollingAnim( traceEnt->client->ps.legsAnim ) )
				{//if on a surface and not in a spin or flip, play full body resist
					parts = SETANIM_BOTH;
				}
				else
				{//play resist just in torso
					parts = SETANIM_TORSO;
				}
				//FIXME: don't interrupt big anims with this!
				NPC_SetAnim( traceEnt, parts, BOTH_RESISTPUSH, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				Jedi_PlayDeflectSound( traceEnt );
				dmg = Q_irand(0,1);
				blockedWithHand = (dmg == 0) ? qtrue : qfalse;
				{//JKEnhanced: the resist pose is re-held every tick, so a held stream stun-locked the boss
				 //(AI looked dead). After g_bossLightningBreak ms of resisting they Push the caster away.
					extern cvar_t *g_bossLightningBreak;
					static int s_resistStart[MAX_GENTITIES];
					static int s_resistLast[MAX_GENTITIES];
					const int n = traceEnt->s.number;
					if ( level.time - s_resistLast[n] > 300 || level.time < s_resistLast[n] )
					{//a new stream (or a stale entry after a load)
						s_resistStart[n] = level.time;
					}
					s_resistLast[n] = level.time;
					if ( g_bossLightningBreak->integer > 0
						&& level.time - s_resistStart[n] >= g_bossLightningBreak->integer
						&& traceEnt->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_0
						&& !PM_InKnockDown( &traceEnt->client->ps ) )
					{
						vec3_t toCaster, ang;
						VectorSubtract( self->currentOrigin, traceEnt->currentOrigin, toCaster );
						vectoangles( toCaster, ang );
						ang[PITCH] = 0;
						SetClientViewAngle( traceEnt, ang );
						traceEnt->client->ps.forcePowerDebounce[FP_PUSH] = 0;
						ForceThrow( traceEnt, qfalse, qfalse );
						s_resistStart[n] = level.time;	//another full window before the next one
						if ( g_debugForce && g_debugForce->integer )
						{
							gi.Printf( "  %s broke out of %s's Lightning with a Push\n", WP_ForceDebugWho( traceEnt ), WP_ForceDebugWho( self ) );
						}
					}
				}
			}
			else if ( traceEnt->s.weapon == WP_SABER )
			{//saber can block lightning
				if ( traceEnt->client //a client
					&& !traceEnt->client->ps.saberInFlight//saber in hand
					&& ( traceEnt->client->ps.saberMove == LS_READY || PM_SaberInParry( traceEnt->client->ps.saberMove ) || PM_SaberInReturn( traceEnt->client->ps.saberMove ) )//not attacking with saber
					&& (traceEnt->s.number || g_saberAutoBlocking->integer || traceEnt->client->ps.saberBlockingTime > level.time)//actually holding block (NPCs have no manual block button, so the pose check above is their whole signal)
					&& (traceEnt->s.number != 0 || traceEnt->client->ps.forcePower > 0)//player with no force power left can't block it
					&& InFOV( self->currentOrigin, traceEnt->currentOrigin, traceEnt->client->ps.viewangles, 20, 35 ) //I'm in front of them
					&& !PM_InKnockDown( &traceEnt->client->ps ) //they're not in a knockdown
					&& !PM_SuperBreakLoseAnim( traceEnt->client->ps.torsoAnim )
					&& !PM_SuperBreakWinAnim( traceEnt->client->ps.torsoAnim )
					&& !PM_SaberInSpecialAttack( traceEnt->client->ps.torsoAnim )
					&& !PM_InSpecialJump( traceEnt->client->ps.torsoAnim )
					&& (!traceEnt->s.number||(traceEnt->NPC&&traceEnt->NPC->rank>=RANK_LT_COMM)) )//the player or a tough jedi/reborn
				{
					if ( Q_irand( 0, traceEnt->client->ps.forcePowerLevel[FP_SABER_DEFENSE]*3 ) > 0 )//more of a chance of defending if saber defense is high
					{
						dmg = 0;
					}
					if ( (traceEnt->client->ps.forcePowersActive&(1<<FP_ABSORB))
						&& traceEnt->client->ps.forcePowerLevel[FP_ABSORB] > FORCE_LEVEL_2 )
					{//no parry, just absorb
					}
					else
					{
						blockedWithSaber = qtrue;
						//make them do a parry
						traceEnt->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
						int parryReCalcTime = Jedi_ReCalcParryTime( traceEnt, EVASION_PARRY );
						if ( traceEnt->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime )
						{
							traceEnt->client->ps.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
						}
						traceEnt->client->ps.weaponTime = Q_irand( 100, 300 );//hold this move - can't attack! - FIXME: unless dual sabers?
						if ( traceEnt->s.number == 0 )
						{//tanking the stream costs the player force power per tick, same rate the caster pays to sustain it (forcePowerNeeded[FP_LIGHTNING])
							WP_ForcePowerDrain( traceEnt, FP_SABER_DEFENSE, forcePowerNeeded[FP_LIGHTNING] );
						}
					}
				}
				else if ( Q_irand( 0, 1 ) )
				{//jedi less likely to be damaged
					blockedWithSaber = qtrue;
					dmg = 0;
				}
				else
				{
					dmg = 1;
				}
			}
			if ( traceEnt && traceEnt->client && traceEnt->client->ps.powerups[PW_GALAK_SHIELD] )
			{
				//has shield up
				dmg = 0;
			}
			int modPowerLevel = -1;

			if (traceEnt->client)
			{
				modPowerLevel = WP_AbsorbConversion(traceEnt, traceEnt->client->ps.forcePowerLevel[FP_ABSORB], self, FP_LIGHTNING, self->client->ps.forcePowerLevel[FP_LIGHTNING], 1);
			}

			if (modPowerLevel != -1)
			{
				if ( !modPowerLevel )
				{
					dmg = 0;
				}
				else if ( modPowerLevel == 1 )
				{
					dmg = floor((float)dmg/4.0f);
				}
				else if ( modPowerLevel == 2 )
				{
					dmg = floor((float)dmg/2.0f);
				}
			}
			if ( dmg > 0 )
			{//JKEnhanced: g_lightningDamage scales every tick (20 ticks/s); random rounding keeps the average
				extern cvar_t *g_lightningDamage;
				const float scaled = dmg * g_lightningDamage->value;
				dmg = (int)scaled;
				if ( Q_flrand( 0.0f, 1.0f ) < scaled - dmg )
				{
					dmg++;
				}
			}
			//FIXME: if ForceDrain, sap force power and add health to self, use different sound & effects
			if ( dmg )
			{
				G_Damage( traceEnt, self, self, dir, impactPoint, dmg, 0, MOD_FORCE_LIGHTNING );
			}
			if ( traceEnt->client )
			{
				if ( !Q_irand( 0, 2 ) )
				{
					G_Sound( traceEnt, G_SoundIndex( va( "sound/weapons/force/lightninghit%d.wav", Q_irand( 1, 3 ) ) ) );
				}
				traceEnt->s.powerups |= ( 1 << PW_SHOCKED );

				// If we are dead or we are a bot, we can do the full version
				class_t npc_class = traceEnt->client->NPC_class;
				if ( traceEnt->health <= 0 || ( npc_class == CLASS_SEEKER || npc_class == CLASS_PROBE ||
					 npc_class == CLASS_MOUSE || npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class == CLASS_REMOTE ||
					 npc_class == CLASS_R5D2 || npc_class == CLASS_PROTOCOL || npc_class == CLASS_MARK1 ||
					 npc_class == CLASS_MARK2 || npc_class == CLASS_INTERROGATOR || npc_class == CLASS_ATST ) ||
					 npc_class == CLASS_SENTRY )
				{
					traceEnt->client->ps.powerups[PW_SHOCKED] = level.time + 4000;
				}
				else if (blockedWithSaber)
				{
					traceEnt->client->ps.powerups[PW_SHOCKED] = 0;
					vec3_t	end, fwd, right, up;
					VectorNegate(dir, fwd);
					
					//randomise direction a bit
					MakeNormalVectors(fwd, right, up);
					VectorMA(fwd, Q_flrand(0.0f, 1.0f), right, fwd);
					VectorMA(fwd, Q_flrand(0.0f, 1.0f), up, fwd);
					VectorNormalize(fwd);
					
					VectorMA( traceEnt->client->ps.saber[0].blade[0].muzzlePoint, traceEnt->client->ps.saber[0].blade[0].length*Q_flrand(0, 1), traceEnt->client->ps.saber[0].blade[0].muzzleDir, end );//FIXME: pick a random blade?
					G_PlayEffect( G_EffectIndex("force/lightning"), end, fwd);
				}
				else if (blockedWithHand)
				{
					//probably play an effect on the hand.
					traceEnt->client->ps.powerups[PW_SHOCKED] = 0;
				}
				else //short version
				{
					traceEnt->client->ps.powerups[PW_SHOCKED] = level.time + 500;
				}
			}
		}
	}
}

void ForceShootLightning( gentity_t *self )
{
	trace_t	tr;
	vec3_t	end, forward;
	gentity_t	*traceEnt;

	if ( self->health <= 0 )
	{
		return;
	}
	if ( !self->s.number && cg.zoomMode )
	{//can't force lightning when zoomed in
		return;
	}

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );

	//FIXME: if lightning hits water, do water-only-flagged radius damage from that point
	if ( self->client->ps.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_2 )
	{//arc
		vec3_t	center, mins, maxs, dir, ent_org, size, v;
		float	radius = 512, dot, dist;
		gentity_t	*entityList[MAX_GENTITIES];
		int		e, numListedEntities, i;

		VectorCopy( self->currentOrigin, center );
		for ( i = 0 ; i < 3 ; i++ )
		{
			mins[i] = center[i] - radius;
			maxs[i] = center[i] + radius;
		}
		numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

		for ( e = 0 ; e < numListedEntities ; e++ )
		{
			traceEnt = entityList[e];

			if ( !traceEnt )
				continue;
			if ( traceEnt == self )
				continue;
			if ( traceEnt->owner == self && traceEnt->s.weapon != WP_THERMAL )//can push your own thermals
				continue;
			if ( !traceEnt->inuse )
				continue;
			if ( !traceEnt->takedamage )
				continue;
			/*
			if ( traceEnt->health <= 0 )//no torturing corpses
				continue;
			*/
			//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
			// find the distance from the edge of the bounding box
			for ( i = 0 ; i < 3 ; i++ )
			{
				if ( center[i] < traceEnt->absmin[i] )
				{
					v[i] = traceEnt->absmin[i] - center[i];
				} else if ( center[i] > traceEnt->absmax[i] )
				{
					v[i] = center[i] - traceEnt->absmax[i];
				} else
				{
					v[i] = 0;
				}
			}

			VectorSubtract( traceEnt->absmax, traceEnt->absmin, size );
			VectorMA( traceEnt->absmin, 0.5, size, ent_org );

			//see if they're in front of me
			//must be within the forward cone
			VectorSubtract( ent_org, center, dir );
			VectorNormalize( dir );
			if ( (dot = DotProduct( dir, forward )) < 0.5 )
				continue;

			//must be close enough
			dist = VectorLength( v );
			if ( dist >= radius )
			{
				continue;
			}

			//in PVS?
			if ( !traceEnt->bmodel && !gi.inPVS( ent_org, self->client->renderInfo.handLPoint ) )
			{//must be in PVS
				continue;
			}

			//Now check and see if we can actually hit it
			gi.trace( &tr, self->client->renderInfo.handLPoint, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT, (EG2_Collision)0, 0 );
			if ( tr.fraction < 1.0f && tr.entityNum != traceEnt->s.number )
			{//must have clear LOS
				continue;
			}

			// ok, we are within the radius, add us to the incoming list
			//FIXME: maybe add up the ents and do more damage the less ents there are
			//		as if we're spreading out the damage?
			ForceLightningDamage( self, traceEnt, dir, dist, dot, ent_org );
		}

	}
	else
	{//trace-line
		int ignore = self->s.number;
		int traces = 0;
		vec3_t	start;

		VectorCopy( self->client->renderInfo.handLPoint, start );
		VectorMA( self->client->renderInfo.handLPoint, 2048, forward, end );

		while ( traces < 10 )
		{//need to loop this in case we hit a Jedi who dodges the shot
			gi.trace( &tr, start, vec3_origin, vec3_origin, end, ignore, MASK_SHOT, G2_RETURNONHIT, 10 );
			if ( tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
			{
				return;
			}

			traceEnt = &g_entities[tr.entityNum];
			//NOTE: only NPCs do this auto-dodge
			if ( traceEnt
				&& traceEnt->s.number >= MAX_CLIENTS
				&& traceEnt->client
				&& traceEnt->client->ps.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0 )//&& traceEnt->NPC
			{//FIXME: need a more reliable way to know we hit a jedi?
				if ( !Jedi_DodgeEvasion( traceEnt, self, &tr, HL_NONE ) )
				{//act like we didn't even hit him
					VectorCopy( tr.endpos, start );
					ignore = tr.entityNum;
					traces++;
					continue;
				}
			}
			//a Jedi is not dodging this shot
			break;
		}

		traceEnt = &g_entities[tr.entityNum];
		ForceLightningDamage( self, traceEnt, forward, 0, 0, tr.endpos );
	}
}

void WP_DeactivateSaber( gentity_t *self, qboolean clearLength )
{
	if ( !self || !self->client )
	{
		return;
	}
	//keep my saber off!
	if ( self->client->ps.SaberActive() )
	{
		self->client->ps.SaberDeactivate();
		if ( clearLength )
		{
			self->client->ps.SetSaberLength( 0 );
		}
		G_SoundIndexOnEnt( self, CHAN_WEAPON, self->client->ps.saber[0].soundOff );
	}
}

static void ForceShootDrain( gentity_t *self );

void ForceDrainGrabStart( gentity_t *self )
{
	NPC_SetAnim( self, SETANIM_BOTH, BOTH_FORCE_DRAIN_GRAB_START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;

	self->client->ps.weaponTime = 1000;
	if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
	{
		self->client->ps.weaponTime = floor( self->client->ps.weaponTime * g_timescale->value );
	}
	//actually grabbing someone, so turn off the saber!
	WP_DeactivateSaber( self, qtrue );
}

qboolean ForceDrain2( gentity_t *self )
{//FIXME: make enemy Jedi able to use this
	trace_t	tr;
	vec3_t	end, forward;
	gentity_t	*traceEnt = NULL;

	if ( self->health <= 0 )
	{
		return qtrue;
	}

	if ( !self->s.number && (cg.zoomMode || in_camera) )
	{//can't force grip when zoomed in or in cinematic
		return qtrue;
	}

	if ( self->client->ps.leanofs )
	{//can't force-drain while leaning
		return qtrue;
	}

	/*
	if ( self->client->ps.SaberLength() > 0 )
	{//can't do this if saber is on!
		return qfalse;
	}
	*/

	if ( self->client->ps.forceDrainEntityNum <= ENTITYNUM_WORLD )
	{//already draining
		//keep my saber off!
		WP_DeactivateSaber( self, qtrue );
		if ( self->client->ps.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_1 )
		{
			self->client->ps.forcePowerDuration[FP_DRAIN] = level.time + 100;
			self->client->ps.weaponTime = 1000;
			if ( self->client->ps.forcePowersActive&(1<<FP_SPEED) )
			{
				self->client->ps.weaponTime = floor( self->client->ps.weaponTime * g_timescale->value );
			}
		}
		return qtrue;
	}

	if ( self->client->ps.forcePowerDebounce[FP_DRAIN] > level.time )
	{//stops it while using it and also after using it, up to 3 second delay
		return qtrue;
	}

	if ( self->client->ps.weaponTime > 0 )
	{//busy
		return qtrue;
	}

	if ( self->client->ps.forcePower < 25 || !WP_ForcePowerUsable( self, FP_DRAIN, 0 ) )
	{
		return qtrue;
	}

	if ( self->client->ps.saberLockTime > level.time )
	{//in saberlock
		return qtrue;
	}

	//NOTE: from here on, if it fails, it's okay to try a normal drain, so return qfalse
	if ( self->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//in air
		return qfalse;
	}

	//Cause choking anim + health drain in ent in front of me
	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );
	VectorMA( self->client->renderInfo.eyePoint, FORCE_DRAIN_DIST, forward, end );

	//okay, trace straight ahead and see what's there
	gi.trace( &tr, self->client->renderInfo.eyePoint, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT, (EG2_Collision)0, 0 );
	if ( tr.entityNum >= ENTITYNUM_WORLD || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
	{
		return qfalse;
	}
	traceEnt = &g_entities[tr.entityNum];
	if ( !traceEnt || traceEnt == self/*???*/ || traceEnt->bmodel || (traceEnt->health <= 0 && traceEnt->takedamage) || (traceEnt->NPC && traceEnt->NPC->scriptFlags & SCF_NO_FORCE) )
	{
		return qfalse;
	}

	if ( traceEnt->client )
	{
		if ( traceEnt->client->ps.forceJumpZStart )
		{//can't catch them in mid force jump - FIXME: maybe base it on velocity?
			return qfalse;
		}
		if ( traceEnt->client->ps.groundEntityNum == ENTITYNUM_NONE )
		{//can't catch them in mid air
			return qfalse;
		}
		if ( !Q_stricmp("Yoda",traceEnt->NPC_type) )
		{
			Jedi_PlayDeflectSound( traceEnt );
			ForceThrow( traceEnt, qfalse );
			return qtrue;
		}
		switch ( traceEnt->client->NPC_class )
		{
		case CLASS_GALAKMECH://cant grab him, he's in armor
			G_AddVoiceEvent( traceEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), Q_irand( 3000, 5000 ) );
			return qfalse;
			break;
		case CLASS_ROCKETTROOPER://cant grab him, he's in armor
		case CLASS_HAZARD_TROOPER://cant grab him, he's in armor
			return qfalse;
			break;
		case CLASS_ATST://much too big to grab!
			return qfalse;
			break;
		//no droids either
		case CLASS_GONK:
		case CLASS_R2D2:
		case CLASS_R5D2:
		case CLASS_MARK1:
		case CLASS_MARK2:
		case CLASS_MOUSE:
		case CLASS_PROTOCOL:
		case CLASS_SABER_DROID:
		case CLASS_ASSASSIN_DROID:
			return qfalse;
			break;
		case CLASS_PROBE:
		case CLASS_SEEKER:
		case CLASS_REMOTE:
		case CLASS_SENTRY:
		case CLASS_INTERROGATOR:
			return qfalse;
			break;
		case CLASS_DESANN://Desann cannot be gripped, he just pushes you back instantly
		case CLASS_KYLE:
		case CLASS_TAVION:
		case CLASS_LUKE:
			Jedi_PlayDeflectSound( traceEnt );
			ForceThrow( traceEnt, qfalse );
			return qtrue;
			break;
		case CLASS_REBORN:
		case CLASS_SHADOWTROOPER:
		//case CLASS_ALORA:
		case CLASS_JEDI:
			if ( traceEnt->NPC
				&& traceEnt->NPC->rank > RANK_CIVILIAN
				&& self->client->ps.forcePowerLevel[FP_DRAIN] < FORCE_LEVEL_2
				&& traceEnt->client->ps.weaponTime <= 0 )
			{
				ForceDrainGrabStart( self );
				Jedi_PlayDeflectSound( traceEnt );
				ForceThrow( traceEnt, qfalse );
				return qtrue;
			}
			break;
		default:
			break;
		}
		if ( traceEnt->s.weapon == WP_EMPLACED_GUN && traceEnt->client->ps.eFlags & EF_LOCKED_TO_WEAPON )
		{//FIXME: maybe can pull them out?
			return qfalse;
		}
		if ( traceEnt != self->enemy && OnSameTeam(self, traceEnt) )
		{//can't accidently grip-drain your teammate
			return qfalse;
		}
//=CHECKABSORB===
		//JKEnhanced (4A bug fix): this was commented out, so the close-range drain grab went straight
		//through Absorb. An Absorbing target isn't grabbed; Absorb gives them Force back (and plays
		//its sound) and the caster pays and has to wait before trying again.
		//(10 = what the grab costs, WP_ForcePowerStart below; stock indexed forcePowerNeeded by LEVEL here)
		if ( -1 != WP_AbsorbConversion( traceEnt, traceEnt->client->ps.forcePowerLevel[FP_ABSORB], self, FP_DRAIN, self->client->ps.forcePowerLevel[FP_DRAIN], 10 ) )
		{
			WP_ForcePowerDrain( self, FP_DRAIN, 10 );
			self->client->ps.forcePowerDebounce[FP_DRAIN] = level.time + 1500;
			return qtrue;
		}
//===============
		if ( !FP_ForceDrainGrippableEnt( traceEnt ) )
		{
			return qfalse;
		}
	}
	else
	{//can't drain non-clients
		return qfalse;
	}

	ForceDrainGrabStart( self );

	WP_ForcePowerStart( self, FP_DRAIN, 10 );
	self->client->ps.forceDrainEntityNum = traceEnt->s.number;

//	G_AddVoiceEvent( traceEnt, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000 );
	G_AddVoiceEvent( traceEnt, Q_irand(EV_CHOKE1, EV_CHOKE3), 2000 );
	if ( /*self->client->ps.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_2 ||*/ traceEnt->s.weapon == WP_SABER )
	{//if we pick up, turn off their weapon
		WP_DeactivateSaber( traceEnt, qtrue );
	}

	/*
	if ( self->client->ps.forcePowerLevel[FP_DRAIN] < FORCE_LEVEL_2 )
	{//just a duration
		self->client->ps.forcePowerDebounce[FP_DRAIN] = level.time + 250;
		self->client->ps.forcePowerDuration[FP_DRAIN] = level.time + 5000;
	}
	*/

	G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/drain.mp3" );

//	NPC_SetAnim( traceEnt, SETANIM_BOTH, BOTH_HUGGEE1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	NPC_SetAnim( traceEnt, SETANIM_BOTH, BOTH_FORCE_DRAIN_GRABBED, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );

	WP_SabersCheckLock2( self, traceEnt, LOCK_FORCE_DRAIN );

	return qtrue;
}

void ForceDrain( gentity_t *self, qboolean triedDrain2 )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if ( !triedDrain2 && self->client->ps.weaponTime > 0 )
	{
		return;
	}

	if ( self->client->ps.forcePower < 25 || !WP_ForcePowerUsable( self, FP_DRAIN, 0 ) )
	{
		return;
	}
	if ( self->client->ps.forcePowerDebounce[FP_DRAIN] > level.time )
	{//stops it while using it and also after using it, up to 3 second delay
		return;
	}

	if ( self->client->ps.saberLockTime > level.time )
	{//FIXME: can this be a way to break out?
		return;
	}

	//JKEnhanced 4B.3: drops timed (L1-2) Protect / Absorb; an L3 toggle stays up
	WP_StopShieldsForAttack( self );

	G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/drain.mp3" );

	WP_ForcePowerStart( self, FP_DRAIN, 0 );
}


qboolean FP_ForceDrainableEnt( gentity_t *victim )
{
	if ( !victim || !victim->client )
	{
		return qfalse;
	}
	switch ( victim->client->NPC_class )
	{
	case CLASS_SAND_CREATURE://??
	case CLASS_ATST:				// technically droid...
	case CLASS_GONK:				// droid
	case CLASS_INTERROGATOR:		// droid
	case CLASS_MARK1:			// droid
	case CLASS_MARK2:			// droid
	case CLASS_GALAKMECH:		// droid
	case CLASS_MINEMONSTER:
	case CLASS_MOUSE:			// droid
	case CLASS_PROBE:			// droid
	case CLASS_PROTOCOL:			// droid
	case CLASS_R2D2:				// droid
	case CLASS_R5D2:				// droid
	case CLASS_REMOTE:
	case CLASS_SEEKER:			// droid
	case CLASS_SENTRY:
	case CLASS_SABER_DROID:
	case CLASS_ASSASSIN_DROID:
	case CLASS_VEHICLE:
		return qfalse;
	default:
		break;
	}
	return qtrue;
}

qboolean FP_ForceDrainGrippableEnt( gentity_t *victim )
{
	if ( !victim || !victim->client )
	{
		return qfalse;
	}
	if ( !FP_ForceDrainableEnt( victim ) )
	{
		return qfalse;
	}
	switch ( victim->client->NPC_class )
	{
	case CLASS_RANCOR:
	case CLASS_SAND_CREATURE:
	case CLASS_WAMPA:
	case CLASS_LIZARD:
	case CLASS_MINEMONSTER:
	case CLASS_MURJJ:
	case CLASS_SWAMP:
	case CLASS_ROCKETTROOPER:
	case CLASS_HAZARD_TROOPER:
		return qfalse;
	default:
		break;
	}
	return qtrue;
}

void ForceDrainDamage( gentity_t *self, gentity_t *traceEnt, vec3_t dir, vec3_t impactPoint )
{
	if ( traceEnt
		&& traceEnt->health > 0
		&& traceEnt->takedamage
		&& FP_ForceDrainableEnt( traceEnt ) )
	{
		if ( traceEnt->client
			&& (!OnSameTeam(self, traceEnt)||self->enemy==traceEnt)//don't drain an ally unless that is actually my current enemy
			&& self->client->ps.forceDrainTime < level.time )
		{//an enemy or object
			if ( traceEnt->NPC )
			{
				AI_NerveEvent( traceEnt, NE_FORCE_ON_ME, 1.0f, self );
				AI_NerveWitness( traceEnt, NE_FORCE_ON_ME, self );
			}
			int modPowerLevel = -1;
			int	dmg = self->client->ps.forcePowerLevel[FP_DRAIN] + 1;
			int dflags = (DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK|DAMAGE_NO_HIT_LOC);//|DAMAGE_NO_KILL);
			if ( traceEnt->s.number == self->client->ps.forceDrainEntityNum )
			{//grabbing hold of them does more damage/drains more, and can actually kill them
				dmg += 3;
				dflags |= DAMAGE_IGNORE_TEAM;
				//dflags &= ~DAMAGE_NO_KILL;
			}

			if (traceEnt->client)
			{
				//check for client using FP_ABSORB
				//JKEnhanced (4A bug fix): pass the tick's drain as the Force spent so Absorb converts it into
				//Force for the target, as Absorb's description says (stock passed 0: no Force back at all)
				modPowerLevel = WP_AbsorbConversion(traceEnt, traceEnt->client->ps.forcePowerLevel[FP_ABSORB], self, FP_DRAIN, self->client->ps.forcePowerLevel[FP_DRAIN], dmg);
			}

			if ( modPowerLevel != -1 )
			{
				if ( !modPowerLevel )
				{
					dmg = 0;
				}
				else if ( modPowerLevel == 1 )
				{
					dmg = 1;
				}
				else if ( modPowerLevel == 2 )
				{
					dmg = 2;
				}
			}

			if ( dmg )
			{
				int	drain = 0;
				if ( traceEnt->client->ps.forcePower )
				{
					if ( dmg > traceEnt->client->ps.forcePower )
					{
						drain = traceEnt->client->ps.forcePower;
						dmg -= drain;
						traceEnt->client->ps.forcePower = 0;
					}
					else
					{
						drain = dmg;
						traceEnt->client->ps.forcePower -= (dmg);
						dmg = 0;
					}
				}

				/*
				if ( (dflags&DAMAGE_NO_KILL) )
				{//must cap damage
					if ( traceEnt->health <= 1 )
					{//can't drain more than they have
						dmg = 0;
					}
					else if ( dmg >= traceEnt->health )
					{//no more than they have, leaving one for them
						dmg = traceEnt->health-1;
					}
				}
				*/

				int maxHealth = self->client->ps.stats[STAT_MAX_HEALTH];
				if ( self->client->ps.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_2 )
				{//overcharge health
					maxHealth = floor( (float)self->client->ps.stats[STAT_MAX_HEALTH] * 1.25f );
				}
				{//JKEnhanced: a staged boss can't drain back up into a stage it already lost
					extern int G_BossHealthCeiling( const gentity_t *ent );
					maxHealth = Q_min( maxHealth, G_BossHealthCeiling( self ) );
				}
				if (self->client->ps.stats[STAT_HEALTH] < maxHealth &&
					self->health > 0 && self->client->ps.stats[STAT_HEALTH] > 0)
				{
					self->health += (drain+dmg);
					if (self->health > maxHealth )
					{
						self->health = maxHealth;
					}
					self->client->ps.stats[STAT_HEALTH] = self->health;
					if ( self->health > self->client->ps.stats[STAT_MAX_HEALTH] )
					{
						self->flags |= FL_OVERCHARGED_HEALTH;
					}
				}

				if ( dmg )
				{//do damage, too
					G_Damage( traceEnt, self, self, dir, impactPoint, dmg, dflags, MOD_FORCE_DRAIN );
				}
				else if ( drain )
				{
					/*
					if ( traceEnt->s.number == self->client->ps.forceDrainEntityNum
						|| traceEnt->s.number < MAX_CLIENTS )
					{//grip-draining (or player - only does sound)
					*/
						NPC_SetPainEvent( traceEnt );
					/*
					}
					else
					{
						GEntity_PainFunc( traceEnt, self, self, impactPoint, 0, MOD_FORCE_DRAIN );
					}
					*/
				}

				if ( !Q_irand( 0, 2 ) )
				{
					G_Sound( traceEnt, G_SoundIndex( "sound/weapons/force/drained.mp3" ) );
				}

				traceEnt->client->ps.forcePowerRegenDebounceTime = level.time + 800; //don't let the client being drained get force power back right away
			}
		}
	}
}

qboolean WP_CheckForceDraineeStopMe( gentity_t *self, gentity_t *drainee )
{
	if ( drainee->NPC
		&& drainee->client
		&& (drainee->client->ps.forcePowersKnown&(1<<FP_PUSH))
		&& level.time-(self->client->ps.forcePowerDebounce[FP_DRAIN]>self->client->ps.forcePowerLevel[FP_DRAIN]*500)//at level 1, I always get at least 500ms of drain, at level 3 I get 1500ms
		&& !Q_irand( 0, 100-(drainee->NPC->stats.evasion*10)-(g_spskill->integer*12) ) )
	{//a jedi who broke free
		ForceThrow( drainee, qfalse );
		//FIXME: I need to go into some pushed back anim...
		WP_ForcePowerStop( self, FP_DRAIN );
		//can't drain again for 2 seconds
		self->client->ps.forcePowerDebounce[FP_DRAIN] = level.time + 4000;
		return qtrue;
	}
	return qfalse;
}

void ForceShootDrain( gentity_t *self )
{
	trace_t	tr;
	vec3_t	end, forward;
	gentity_t	*traceEnt;
	int			numDrained = 0;

	if ( self->health <= 0 )
	{
		return;
	}

	if ( self->client->ps.forcePowerDebounce[FP_DRAIN] <= level.time )
	{
		AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
		VectorNormalize( forward );

		if ( self->client->ps.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_2 )
		{//arc
			vec3_t	center, mins, maxs, dir, ent_org, size, v;
			float	radius = MAX_DRAIN_DISTANCE, dot, dist;
			gentity_t	*entityList[MAX_GENTITIES];
			int		e, numListedEntities, i;

			VectorCopy( self->client->ps.origin, center );
			for ( i = 0 ; i < 3 ; i++ )
			{
				mins[i] = center[i] - radius;
				maxs[i] = center[i] + radius;
			}
			numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

			for ( e = 0 ; e < numListedEntities ; e++ )
			{
				traceEnt = entityList[e];

				if ( !traceEnt )
					continue;
				if ( traceEnt == self )
					continue;
				if ( !traceEnt->inuse )
					continue;
				if ( !traceEnt->takedamage )
					continue;
				if ( traceEnt->health <= 0 )//no torturing corpses
					continue;
				if ( !traceEnt->client )
					continue;
				/*
				if ( !traceEnt->client->ps.forcePower )
					continue;
				*/
	//			if (traceEnt->client->ps.forceSide == FORCE_DARKSIDE)	// We no longer care if the victim is dark or light
	//				continue;
				if (self->enemy != traceEnt//not my enemy
					&& OnSameTeam(self, traceEnt))//on my team
					continue;
				//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
				// find the distance from the edge of the bounding box
				for ( i = 0 ; i < 3 ; i++ )
				{
					if ( center[i] < traceEnt->absmin[i] )
					{
						v[i] = traceEnt->absmin[i] - center[i];
					} else if ( center[i] > traceEnt->absmax[i] )
					{
						v[i] = center[i] - traceEnt->absmax[i];
					} else
					{
						v[i] = 0;
					}
				}

				VectorSubtract( traceEnt->absmax, traceEnt->absmin, size );
				VectorMA( traceEnt->absmin, 0.5, size, ent_org );

				//see if they're in front of me
				//must be within the forward cone
				VectorSubtract( ent_org, center, dir );
				VectorNormalize( dir );
				if ( (dot = DotProduct( dir, forward )) < 0.5 )
					continue;

				//must be close enough
				dist = VectorLength( v );
				if ( dist >= radius )
				{
					continue;
				}

				//in PVS?
				if ( !traceEnt->bmodel && !gi.inPVS( ent_org, self->client->renderInfo.handLPoint ) )
				{//must be in PVS
					continue;
				}

				//Now check and see if we can actually hit it
				gi.trace( &tr, self->client->ps.origin, vec3_origin, vec3_origin, ent_org, self->s.number, MASK_SHOT, G2_RETURNONHIT, 10 );
				if ( tr.fraction < 1.0f && tr.entityNum != traceEnt->s.number )
				{//must have clear LOS
					continue;
				}

				if ( traceEnt
					&& traceEnt->s.number >= MAX_CLIENTS
					&& traceEnt->client
					&& traceEnt->client->ps.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0 )//&& traceEnt->NPC
				{
					if ( !Q_irand( 0, 4 ) && !Jedi_DodgeEvasion( traceEnt, self, &tr, HL_NONE ) )
					{//act like we didn't even hit him
						continue;
					}
				}

				// ok, we are within the radius, add us to the incoming list
				if ( WP_CheckForceDraineeStopMe( self, traceEnt ) )
				{
					continue;
				}
				ForceDrainDamage( self, traceEnt, dir, ent_org );
				numDrained++;
			}

		}
		else
		{//trace-line
			int ignore = self->s.number;
			int traces = 0;
			vec3_t	start;

			VectorCopy( self->client->renderInfo.handLPoint, start );
			VectorMA( start, MAX_DRAIN_DISTANCE, forward, end );

			while ( traces < 10 )
			{//need to loop this in case we hit a Jedi who dodges the shot
				gi.trace( &tr, start, vec3_origin, vec3_origin, end, ignore, MASK_SHOT, G2_RETURNONHIT, 10 );
				if ( tr.entityNum == ENTITYNUM_NONE || tr.fraction == 1.0 || tr.allsolid || tr.startsolid )
				{
					//always take 1 force point per frame that we're shooting this
					WP_ForcePowerDrain( self, FP_DRAIN, 1 );
					return;
				}

				traceEnt = &g_entities[tr.entityNum];
				//NOTE: only NPCs do this auto-dodge
				if ( traceEnt
					&& traceEnt->s.number >= MAX_CLIENTS
					&& traceEnt->client
					&& traceEnt->client->ps.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0 )//&& traceEnt->NPC
				{
					if ( !Q_irand( 0, 2 ) && !Jedi_DodgeEvasion( traceEnt, self, &tr, HL_NONE ) )
					{//act like we didn't even hit him
						VectorCopy( tr.endpos, start );
						ignore = tr.entityNum;
						traces++;
						continue;
					}
				}
				//a Jedi is not dodging this shot
				break;
			}
			traceEnt = &g_entities[tr.entityNum];
			if ( !WP_CheckForceDraineeStopMe( self, traceEnt ) )
			{
				ForceDrainDamage( self, traceEnt, forward, tr.endpos );
			}
			numDrained = 1;
		}

		self->client->ps.forcePowerDebounce[FP_DRAIN] = level.time + 200;//so we don't drain so damn fast!
	}
	self->client->ps.forcePowerRegenDebounceTime = level.time + 500;

	if ( !numDrained )
	{//always take 1 force point per frame that we're shooting this
		WP_ForcePowerDrain( self, FP_DRAIN, 1 );
	}
	else
	{
		WP_ForcePowerDrain( self, FP_DRAIN, numDrained );//was 2, but...
	}

	return;
}

void ForceDrainEnt( gentity_t *self, gentity_t *drainEnt )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if ( self->client->ps.forcePowerDebounce[FP_DRAIN] <= level.time )
	{
		if ( !drainEnt )
			return;
		if ( drainEnt == self )
			return;
		if ( !drainEnt->inuse )
			return;
		if ( !drainEnt->takedamage )
			return;
		if ( drainEnt->health <= 0 )//no torturing corpses
			return;
		if ( !drainEnt->client )
			return;
		if (OnSameTeam(self, drainEnt))
			return;

		vec3_t fwd;
		AngleVectors( self->client->ps.viewangles, fwd, NULL, NULL );

		drainEnt->painDebounceTime = 0;
		ForceDrainDamage( self, drainEnt, fwd, drainEnt->currentOrigin );
		drainEnt->painDebounceTime = level.time + 2000;

		if ( drainEnt->s.number )
		{
			if ( self->client->ps.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_2 )
			{//do damage faster at level 3
				self->client->ps.forcePowerDebounce[FP_DRAIN] = level.time + Q_irand( 100, 500 );
			}
			else
			{
				self->client->ps.forcePowerDebounce[FP_DRAIN] = level.time + Q_irand( 200, 800 );
			}
		}
		else
		{//player takes damage faster
			self->client->ps.forcePowerDebounce[FP_DRAIN] = level.time + Q_irand( 100, 500 );
		}
	}

	self->client->ps.forcePowerRegenDebounceTime = level.time + 500;
}

void ForceSeeing( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.forcePowersActive & (1 << FP_SEE)) )
	{
		WP_ForcePowerStop( self, FP_SEE );
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_SEE, 0 ) )
	{
		return;
	}

	WP_DebounceForceDeactivateTime( self );

	WP_ForcePowerStart( self, FP_SEE, 0 );

	G_SoundOnEnt( self, CHAN_ITEM, "sound/weapons/force/see.wav" );
}

void ForceProtect( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.forcePowersActive & (1 << FP_PROTECT)) )
	{
		WP_ForcePowerStop( self, FP_PROTECT );
		return;
	}

	//JKEnhanced 4B.3: L3 is a toggle - a small cost to turn on, then it pays per hit it stops
	const int toggleCost = ( self->client->ps.forcePowerLevel[FP_PROTECT] >= FORCE_LEVEL_3 ) ? G_ForceTuning()->protectToggleCost : 0;
	if ( !WP_ForcePowerUsable( self, FP_PROTECT, toggleCost ) )
	{
		return;
	}

	// Make sure to turn off Force Rage and Force Absorb.
	if (self->client->ps.forcePowersActive & (1 << FP_RAGE) )
	{
		WP_ForcePowerStop( self, FP_RAGE );
	}

	WP_DebounceForceDeactivateTime( self );

	WP_ForcePowerStart( self, FP_PROTECT, toggleCost );

	if ( self->client->ps.saberLockTime < level.time )
	{
		if ( self->client->ps.forcePowerLevel[FP_PROTECT] < FORCE_LEVEL_3 )
		{//animate
			int parts = SETANIM_BOTH;
			int anim = BOTH_FORCE_PROTECT;
			if ( self->client->ps.forcePowerLevel[FP_PROTECT] > FORCE_LEVEL_1 )
			{//level 2 only does it on torso (can keep running)
				parts = SETANIM_TORSO;
				anim = BOTH_FORCE_PROTECT_FAST;
			}
			else
			{
				if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{
					VectorClear( self->client->ps.velocity );
				}
				if ( self->NPC )
				{
					VectorClear( self->client->ps.moveDir );
					self->client->ps.speed = 0;
				}
				//FIXME: what if in air?
			}
			NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			//don't move or attack during this anim
			if ( self->client->ps.forcePowerLevel[FP_PROTECT] < FORCE_LEVEL_2 )
			{
				self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
				self->client->ps.pm_time = self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
				if ( self->s.number )
				{//NPC
					self->painDebounceTime = level.time + self->client->ps.torsoAnimTimer;
				}
				else
				{//player
					self->aimDebounceTime = level.time + self->client->ps.torsoAnimTimer;
				}
			}
			else
			{
				self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
			}
		}
	}
}

void ForceAbsorb( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.forcePowersActive & (1 << FP_ABSORB)) )
	{
		WP_ForcePowerStop( self, FP_ABSORB );
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_ABSORB, 0 ) )
	{
		return;
	}

	// Make sure to turn off Force Rage and Force Protection.
	if (self->client->ps.forcePowersActive & (1 << FP_RAGE) )
	{
		WP_ForcePowerStop( self, FP_RAGE );
	}

	WP_DebounceForceDeactivateTime( self );

	WP_ForcePowerStart( self, FP_ABSORB, 0 );

	if ( self->client->ps.saberLockTime < level.time )
	{
		if ( self->client->ps.forcePowerLevel[FP_ABSORB] < FORCE_LEVEL_3 )
		{//must animate
			int parts = SETANIM_BOTH;
			if ( self->client->ps.forcePowerLevel[FP_ABSORB] > FORCE_LEVEL_1 )
			{//level 2 only does it on torso (can keep running)
				parts = SETANIM_TORSO;
			}
			else
			{
				if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{
					VectorClear( self->client->ps.velocity );
				}
				if ( self->NPC )
				{
					VectorClear( self->client->ps.moveDir );
					self->client->ps.speed = 0;
				}
				//FIXME: what if in air?
			}
			/*
			//if in air, only do on torso - NOTE: or moving?
			if ( self->client->ps.groundEntityNum == ENTITYNUM_NONE )//|| !VectorCompare( self->client->ps.velocity, vec3_origin ) )
			{
				parts = SETANIM_TORSO;
			}
			*/
			NPC_SetAnim( self, parts, BOTH_FORCE_ABSORB, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
			if ( parts == SETANIM_BOTH )
			{//can't move
				self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
				self->client->ps.pm_time = self->client->ps.legsAnimTimer = self->client->ps.torsoAnimTimer;// = self->client->ps.forcePowerDuration[FP_ABSORB];
				if ( self->s.number )
				{//NPC
					self->painDebounceTime = level.time + self->client->ps.pm_time;//self->client->ps.forcePowerDuration[FP_ABSORB];
				}
				else
				{//player
					self->aimDebounceTime = level.time + self->client->ps.pm_time;//self->client->ps.forcePowerDuration[FP_ABSORB];
				}
			}
			//stop saber
			//WP_DeactivateSaber( self );//turn off saber when meditating
			self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
			self->client->ps.saberBlocked = BLOCKED_NONE;
		}
	}
}

void ForceRage( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}

	//FIXME: prevent them from using any other force powers when raging?

	if (self->client->ps.forceAllowDeactivateTime < level.time &&
		(self->client->ps.forcePowersActive & (1 << FP_RAGE)) )
	{
		WP_ForcePowerStop( self, FP_RAGE );
		return;
	}

	if ( !WP_ForcePowerUsable( self, FP_RAGE, 0 ) )
	{
		return;
	}

	if (self->client->ps.forceRageRecoveryTime >= level.time)
	{
		return;
	}

	if ( self->s.number < MAX_CLIENTS
		&& self->health < 25 )
	{//have to have at least 25 health to start it
		return;
	}

	if (self->health < 10)
	{
		return;
	}

	// Make sure to turn off Force Protection and Force Absorb.
	if (self->client->ps.forcePowersActive & (1 << FP_PROTECT) )
	{
		WP_ForcePowerStop( self, FP_PROTECT );
	}
	if (self->client->ps.forcePowersActive & (1 << FP_ABSORB) )
	{
		WP_ForcePowerStop( self, FP_ABSORB );
	}

	WP_DebounceForceDeactivateTime( self );

	WP_ForcePowerStart( self, FP_RAGE, 0 );

	if ( self->client->ps.saberLockTime < level.time )
	{
		if ( self->client->ps.forcePowerLevel[FP_RAGE] < FORCE_LEVEL_3 )
		{//must animate
			if ( self->client->ps.forcePowerLevel[FP_RAGE] < FORCE_LEVEL_2 )
			{//have to stand still for whole length of anim
				//FIXME: if in air, only do on torso?
				NPC_SetAnim( self, SETANIM_BOTH, BOTH_FORCE_RAGE, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				//don't attack during this anim
				self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
				self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
				self->client->ps.pm_time = self->client->ps.torsoAnimTimer;
				if ( self->s.number )
				{//NPC
					self->painDebounceTime = level.time + self->client->ps.torsoAnimTimer;
				}
				else
				{//player
					self->aimDebounceTime = level.time + self->client->ps.torsoAnimTimer;
				}
			}
			else
			{
				NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCE_RAGE, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				//don't attack during this anim
				self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
			}
			//stop saber
			self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
			self->client->ps.saberBlocked = BLOCKED_NONE;
		}
	}
}

void ForceJumpCharge( gentity_t *self, usercmd_t *ucmd )
{
	float forceJumpChargeInterval = forceJumpStrength[0] / (FORCE_JUMP_CHARGE_TIME/FRAMETIME);

	if ( self->health <= 0 )
	{
		return;
	}
	if ( !self->s.number && cg.zoomMode )
	{//can't force jump when zoomed in
		return;
	}

	//need to play sound
	if ( !self->client->ps.forceJumpCharge )
	{//FIXME: this should last only as long as the actual charge-up
		G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/jumpbuild.wav" );
	}
	//Increment
	self->client->ps.forceJumpCharge += forceJumpChargeInterval;

	//clamp to max strength for current level
	if ( self->client->ps.forceJumpCharge > forceJumpStrength[self->client->ps.forcePowerLevel[FP_LEVITATION]] )
	{
		self->client->ps.forceJumpCharge = forceJumpStrength[self->client->ps.forcePowerLevel[FP_LEVITATION]];
	}

	//clamp to max available force power
	if ( self->client->ps.forceJumpCharge/forceJumpChargeInterval/(FORCE_JUMP_CHARGE_TIME/FRAMETIME)*forcePowerNeeded[FP_LEVITATION] > self->client->ps.forcePower )
	{//can't use more than you have
		self->client->ps.forceJumpCharge = self->client->ps.forcePower*forceJumpChargeInterval/(FORCE_JUMP_CHARGE_TIME/FRAMETIME);
	}
	//FIXME: a simple tap should always do at least a normal height's jump?
}

int WP_GetVelocityForForceJump( gentity_t *self, vec3_t jumpVel, usercmd_t *ucmd )
{
	float pushFwd = 0, pushRt = 0;
	vec3_t	view, forward, right;
	VectorCopy( self->client->ps.viewangles, view );
	view[0] = 0;
	AngleVectors( view, forward, right, NULL );
	if ( ucmd->forwardmove && ucmd->rightmove )
	{
		if ( ucmd->forwardmove > 0 )
		{
			pushFwd = 50;
		}
		else
		{
			pushFwd = -50;
		}
		if ( ucmd->rightmove > 0 )
		{
			pushRt = 50;
		}
		else
		{
			pushRt = -50;
		}
	}
	else if ( ucmd->forwardmove || ucmd->rightmove )
	{
		if ( ucmd->forwardmove > 0 )
		{
			pushFwd = 100;
		}
		else if ( ucmd->forwardmove < 0 )
		{
			pushFwd = -100;
		}
		else if ( ucmd->rightmove > 0 )
		{
			pushRt = 100;
		}
		else if ( ucmd->rightmove < 0 )
		{
			pushRt = -100;
		}
	}
	VectorMA( self->client->ps.velocity, pushFwd, forward, jumpVel );
	VectorMA( self->client->ps.velocity, pushRt, right, jumpVel );
	jumpVel[2] += self->client->ps.forceJumpCharge;//forceJumpStrength;
	if ( pushFwd > 0 && self->client->ps.forceJumpCharge > 200 )
	{
		return FJ_FORWARD;
	}
	else if ( pushFwd < 0 && self->client->ps.forceJumpCharge > 200 )
	{
		return FJ_BACKWARD;
	}
	else if ( pushRt > 0 && self->client->ps.forceJumpCharge > 200 )
	{
		return FJ_RIGHT;
	}
	else if ( pushRt < 0 && self->client->ps.forceJumpCharge > 200 )
	{
		return FJ_LEFT;
	}
	else
	{//FIXME: jump straight up anim
		return FJ_UP;
	}
}

void ForceJump( gentity_t *self, usercmd_t *ucmd )
{
	if ( self->client->ps.forcePowerDuration[FP_LEVITATION] > level.time )
	{
		return;
	}
	if ( !WP_ForcePowerUsable( self, FP_LEVITATION, 0 ) )
	{
		return;
	}
	if ( self->s.groundEntityNum == ENTITYNUM_NONE )
	{
		return;
	}
	if ( self->client->ps.pm_flags&PMF_JUMP_HELD )
	{
		return;
	}
	if ( self->health <= 0 )
	{
		return;
	}
	if ( !self->s.number && (cg.zoomMode || in_camera) )
	{//can't force jump when zoomed in or in cinematic
		return;
	}
	if ( self->client->ps.saberLockTime > level.time )
	{//FIXME: can this be a way to break out?
		return;
	}

	if ( self->client->NPC_class == CLASS_BOBAFETT
		|| self->client->NPC_class == CLASS_ROCKETTROOPER )
	{
		if ( self->client->ps.forceJumpCharge > 300 )
		{
			JET_FlyStart(NPC);
		}
		else
		{
			G_AddEvent( self, EV_JUMP, 0 );
		}
	}
	else
	{
		G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/jump.wav" );
	}

	float forceJumpChargeInterval = forceJumpStrength[self->client->ps.forcePowerLevel[FP_LEVITATION]]/(FORCE_JUMP_CHARGE_TIME/FRAMETIME);

	int anim;
	vec3_t	jumpVel;

	switch( WP_GetVelocityForForceJump( self, jumpVel, ucmd ) )
	{
	case FJ_FORWARD:
		if ( ((self->client->NPC_class == CLASS_BOBAFETT||self->client->NPC_class == CLASS_ROCKETTROOPER) && self->client->ps.forceJumpCharge > 300 )
			|| (self->client->ps.saber[0].saberFlags&SFL_NO_FLIPS)
			|| (self->client->ps.dualSabers && (self->client->ps.saber[1].saberFlags&SFL_NO_FLIPS) )
			|| ( self->NPC &&
				self->NPC->rank != RANK_CREWMAN &&
				self->NPC->rank <= RANK_LT_JG ) )
		{//can't do acrobatics
			anim = BOTH_FORCEJUMP1;
		}
		else
		{
			if ( self->client->NPC_class == CLASS_ALORA && Q_irand( 0, 3 ) )
			{
				anim = Q_irand( BOTH_ALORA_FLIP_1, BOTH_ALORA_FLIP_3 );
			}
			else
			{
				anim = BOTH_FLIP_F;
			}
		}
		break;
	case FJ_BACKWARD:
		if ( ((self->client->NPC_class == CLASS_BOBAFETT||self->client->NPC_class == CLASS_ROCKETTROOPER) && self->client->ps.forceJumpCharge > 300 )
			|| (self->client->ps.saber[0].saberFlags&SFL_NO_FLIPS)
			|| (self->client->ps.dualSabers && (self->client->ps.saber[1].saberFlags&SFL_NO_FLIPS) )
			|| ( self->NPC &&
				self->NPC->rank != RANK_CREWMAN &&
				self->NPC->rank <= RANK_LT_JG ) )
		{//can't do acrobatics
			anim = BOTH_FORCEJUMPBACK1;
		}
		else
		{
			anim = BOTH_FLIP_B;
		}
		break;
	case FJ_RIGHT:
		if ( ((self->client->NPC_class == CLASS_BOBAFETT||self->client->NPC_class == CLASS_ROCKETTROOPER) && self->client->ps.forceJumpCharge > 300 )
			|| (self->client->ps.saber[0].saberFlags&SFL_NO_FLIPS)
			|| (self->client->ps.dualSabers && (self->client->ps.saber[1].saberFlags&SFL_NO_FLIPS) )
			|| ( self->NPC &&
				self->NPC->rank != RANK_CREWMAN &&
				self->NPC->rank <= RANK_LT_JG ) )
		{//can't do acrobatics
			anim = BOTH_FORCEJUMPRIGHT1;
		}
		else
		{
			anim = BOTH_FLIP_R;
		}
		break;
	case FJ_LEFT:
		if ( ((self->client->NPC_class == CLASS_BOBAFETT||self->client->NPC_class == CLASS_ROCKETTROOPER) && self->client->ps.forceJumpCharge > 300 )
			|| (self->client->ps.saber[0].saberFlags&SFL_NO_FLIPS)
			|| (self->client->ps.dualSabers && (self->client->ps.saber[1].saberFlags&SFL_NO_FLIPS) )
			|| ( self->NPC &&
				self->NPC->rank != RANK_CREWMAN &&
				self->NPC->rank <= RANK_LT_JG ) )
		{//can't do acrobatics
			anim = BOTH_FORCEJUMPLEFT1;
		}
		else
		{
			anim = BOTH_FLIP_L;
		}
		break;
	default:
	case FJ_UP:
		anim = BOTH_JUMP1;
		break;
	}

	int	parts = SETANIM_BOTH;
	if ( self->client->ps.weaponTime )
	{//FIXME: really only care if we're in a saber attack anim.. maybe trail length?
		parts = SETANIM_LEGS;
	}

	NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );

	//FIXME: sound effect
	self->client->ps.forceJumpZStart = self->currentOrigin[2];//remember this for when we land
	VectorCopy( jumpVel, self->client->ps.velocity );
	//wasn't allowing them to attack when jumping, but that was annoying
	//self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;

	WP_ForcePowerStart( self, FP_LEVITATION, self->client->ps.forceJumpCharge/forceJumpChargeInterval/(FORCE_JUMP_CHARGE_TIME/FRAMETIME)*forcePowerNeeded[FP_LEVITATION] );
	//self->client->ps.forcePowerDuration[FP_LEVITATION] = level.time + self->client->ps.weaponTime;
	self->client->ps.forceJumpCharge = 0;
}

#define	DESTRUCTION_VELOCITY		600
#define	DESTRUCTION_DAMAGE			40
#define DESTRUCTION_SPLASH_DAMAGE	30
#define	DESTRUCTION_SPLASH_RADIUS		160
#define DESTRUCTION_NPC_DAMAGE_EASY		10
#define DESTRUCTION_NPC_DAMAGE_NORMAL	20
#define DESTRUCTION_NPC_DAMAGE_HARD		30
#define DESTRUCTION_SIZE				3

gentity_t *CreateMissile( vec3_t org, vec3_t dir, float vel, int life, gentity_t *owner, qboolean altFire = qfalse );
//---------------------------------------------------------
void WP_FireDestruction( gentity_t *ent, int forceLevel )
//---------------------------------------------------------
{
	vec3_t	start, forward;
	int		damage	= DESTRUCTION_DAMAGE;
	float	vel = DESTRUCTION_VELOCITY;
	
	if ( forceLevel == FORCE_LEVEL_2 )
	{
		vel *= 1.5f;
	}
	else if ( forceLevel == FORCE_LEVEL_3 )
	{
		vel *= 2.0f;
	}
	
	AngleVectors( ent->client->ps.viewangles, forward, NULL, NULL );
	VectorNormalize( forward );
	
	VectorCopy( ent->client->renderInfo.eyePoint, start );
	
	gentity_t *missile = CreateMissile( start, forward, vel, 10000, ent, qfalse );
	
	missile->classname = "rocket_proj";
	missile->s.weapon = WP_CONCUSSION;
	missile->s.powerups |= (1<<PW_FORCE_PROJECTILE);
	missile->mass = 10;
	
	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = DESTRUCTION_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = DESTRUCTION_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = DESTRUCTION_NPC_DAMAGE_HARD;
		}
	}
	
	// Make it easier to hit things
	VectorSet( missile->maxs, DESTRUCTION_SIZE, DESTRUCTION_SIZE, DESTRUCTION_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );
	
	missile->damage = damage * (1.0f + forceLevel)/2.0f;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	
	missile->methodOfDeath = MOD_DESTRUCTION;
	missile->splashMethodOfDeath = MOD_DESTRUCTION;// ?SPLASH;
	
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = DESTRUCTION_SPLASH_DAMAGE * (1.0f + forceLevel)/2.0f;
	missile->splashRadius = DESTRUCTION_SPLASH_RADIUS * (1.0f + forceLevel)/2.0f;;
	
	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

extern qboolean PM_HasAnimation( gentity_t *ent, int animation );
void ForceInsanity( gentity_t *self );	//fp_insanity.cpp (JKEnhanced 4B.8 - Terror, a nerve cone)

qboolean PlayerAffectedByStasis( void )
{
	gentity_t *ent = &g_entities[0];
	if (ent && ent->client && ent->client->ps.stasisTime > (cg.time ? cg.time : level.time))
	{
		return qtrue;
	}
	return qfalse;
}

/*
ForceRepulse (JKEnhanced 4B: player Repulse restored from 45210704)

Hold to charge, release to throw (WP_ForcePowerStop -> ForceRepulseThrow). A neutral core power
levelled like Push / Pull: the level sets the radius (forcePushPullRadius) and how far the charge
can build (WP_RepulseChargeTime, force.cfg repulse.charge). 4B.4: clears space - saber users can block it,
the hurt comes from slamming into things; not castable gripped / drained / in a saber lock.
NPCs cast it outright through G_NPCRepulse (fp_repulse.cpp).
*/
//JKEnhanced 4B.4: the same charge at every level (force.cfg repulse.charge, 2 s; the level sets the
//radius); ForceRepulseThrow turns the charge into a 0-1 strength against this
int WP_RepulseFullCharge( const gentity_t *self )
{
	return G_ForceTuning()->repulseCharge;
}

//JKEnhanced 4B.4: no Repulse out of a saber lock, a Grip or a Drain - Push is the way out of those
qboolean WP_RepulseHeld( const gentity_t *self )
{
	return (qboolean)( self->client->ps.saberLockTime > level.time
		|| ( self->client->ps.eFlags & EF_FORCE_GRIPPED ) || ( self->s.eFlags & EF_FORCE_GRIPPED )
		|| ( self->client->ps.eFlags & EF_FORCE_DRAINED ) || ( self->s.eFlags & EF_FORCE_DRAINED ) );
}

//What the release pays. The player pays as they charge instead (force.cfg repulse.startCost up front,
//then draining to repulse.totalCost by a full charge - WP_RepulseChargeDrain), so 0 here; NPCs pay
//cost.repulse on release (a boss's stage Repulse must go off)
int WP_RepulseCost( const gentity_t *self )
{
	return ( self->s.number < MAX_CLIENTS ) ? 0 : forcePowerNeeded[FP_REPULSE];
}

//what the player has paid on the charge so far (not saved: a charge doesn't survive a load anyway)
static int s_repulseSpent[MAX_CLIENTS];

//the player's charge drains toward repulse.totalCost by a full charge; returns qfalse when the Force
//has run out (it goes off early)
static qboolean WP_RepulseChargeDrain( gentity_t *self, int chargeTime )
{
	if ( self->s.number >= MAX_CLIENTS )
	{
		return qtrue;
	}
	const forceTuning_t *ft = G_ForceTuning();
	const int fullCharge = Q_max( 1, WP_RepulseFullCharge( self ) );
	const int target = ft->repulseStartCost
		+ (int)( ( ft->repulseTotalCost - ft->repulseStartCost ) * Q_min( 1.0f, (float)chargeTime / fullCharge ) + 0.5f );
	const int owed = target - s_repulseSpent[self->s.number];
	if ( owed > 0 )
	{
		const int pay = Q_min( owed, self->client->ps.forcePower );
		if ( pay > 0 )
		{
			WP_ForcePowerDrain( self, FP_REPULSE, pay );
			s_repulseSpent[self->s.number] += pay;
		}
	}
	return (qboolean)( self->client->ps.forcePower > 0 );
}

int WP_RepulseChargeTime( const gentity_t *self )
{
	if ( !self || !self->client || !self->client->ps.repulseChargeStart )
	{
		return 0;
	}
	return Q_min( level.time - self->client->ps.repulseChargeStart, WP_RepulseFullCharge( self ) );
}

void ForceRepulse( gentity_t *self )
{
	if ( self->health <= 0 )
	{
		return;
	}
	if ( !self->s.number && (cg.zoomMode || in_camera) )
	{//can't repulse when zoomed in or in cinematic
		return;
	}
	if ( self->client->ps.leanofs )
	{//can't repulse while leaning
		return;
	}
	if ( self->client->ps.repulseChargeStart )
	{//already charging
		return;
	}
	if ( self->client->ps.forcePowerDebounce[FP_REPULSE] > level.time )
	{
		return;
	}
	//the player needs only the start cost (the rest drains while charging); NPCs pay the cost on release
	const int startCost = ( self->s.number < MAX_CLIENTS ) ? G_ForceTuning()->repulseStartCost : forcePowerNeeded[FP_REPULSE] + 5;
	if ( !WP_ForcePowerUsable( self, FP_REPULSE, Q_max( 1, startCost ) ) )
	{
		return;
	}
	if ( WP_RepulseHeld( self ) )
	{
		return;
	}
	if ( PM_InKnockDown( &self->client->ps ) )
	{
		return;
	}
	//JKEnhanced 4B.3: drops timed (L1-2) Protect / Absorb; an L3 toggle stays up
	WP_StopShieldsForAttack( self );

	self->client->ps.repulseChargeStart = level.time;

	NPC_SetAnim( self, SETANIM_BOTH, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );

	self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
	self->client->ps.saberBlocked = BLOCKED_NONE;

	self->s.loopSound = G_SoundIndex( "sound/weapons/force/lightning2.wav" );

	self->client->ps.weaponTime = self->client->ps.torsoAnimTimer;
	if ( self->s.number < MAX_CLIENTS )
	{
		WP_ForcePowerStart( self, FP_REPULSE, -1 );
		s_repulseSpent[self->s.number] = 0;
		WP_RepulseChargeDrain( self, 0 );//the start cost
		if ( g_debugForce && g_debugForce->integer )
		{
			gi.Printf( "  REPULSE charge start at %d (paid %d, Force %d)\n", level.time, s_repulseSpent[self->s.number], self->client->ps.forcePower );
		}
	}
	else
	{
		WP_ForcePowerStart( self, FP_REPULSE, -1 );//(nothing up front: the release pays it)
	}
}

extern void PM_SetTorsoAnimTimer( gentity_t *ent, int *torsoAnimTimer, int time );
void ForceStasis( gentity_t *self );	//fp_stasis.cpp (JKEnhanced 4B.7 - the cone)

qboolean WP_AbsorbablePower( int power )
{
	return (qboolean)( power == FP_LIGHTNING || power == FP_DRAIN || power == FP_GRIP || power == FP_PUSH
		|| power == FP_PULL || power == FP_REPULSE || power == FP_STASIS );
}

int WP_AbsorbConversion(gentity_t *attacked, int atdAbsLevel, gentity_t *attacker, int atPower, int atPowerLevel, int atForceSpent)
{
	int getLevel = 0;
	int addTot = 0;

	if ( !WP_AbsorbablePower( atPower ) )
	{ //Only these powers can be absorbed
		return -1;
	}

	if (!atdAbsLevel)
	{ //looks like attacker doesn't have any absorb power
		return -1;
	}

	if (!(attacked->client->ps.forcePowersActive & (1 << FP_ABSORB)))
	{ //absorb is not active
		return -1;
	}

	//Subtract absorb power level from the offensive force power
	getLevel = atPowerLevel;
	getLevel -= atdAbsLevel;

	if (getLevel < 0)
	{
		getLevel = 0;
	}

	//let the attacker absorb an amount of force used in this attack based on his level of absorb
	addTot = (atForceSpent/3)*attacked->client->ps.forcePowerLevel[FP_ABSORB];

	if (addTot < 1 && atForceSpent >= 1)
	{
		addTot = 1;
	}
	attacked->client->ps.forcePower += addTot;
	if (attacked->client->ps.forcePower > attacked->client->ps.forcePowerMax)
	{
		attacked->client->ps.forcePower = attacked->client->ps.forcePowerMax;
	}
	if ( attacker && attacker->NPC && attacker->enemy == attacked && getLevel <= FORCE_LEVEL_0 )
	{//JKEnhanced 4B.3: fully absorbed - the AI stops casting absorbable powers at them for a while
	 //(sandbox: Tavion pushed an Absorb 3 player 23 times, refunding them every time)
		TIMER_Set( attacker, "enemyAbsorbs", Q_irand( 8000, 12000 ) );
	}

	G_SoundOnEnt( attacked, CHAN_ITEM, "sound/weapons/force/absorbhit.wav" );

	if ( g_debugForce && g_debugForce->integer )
	{
		gi.Printf( "  ABSORB %s vs %s's %s L%d: works as L%d, +%d Force (now %d)\n", WP_ForceDebugWho( attacked ),
			WP_ForceDebugWho( attacker ), WP_ForcePowerName( atPower ), atPowerLevel, getLevel, addTot, attacked->client->ps.forcePower );
	}
	return getLevel;
}

void WP_ForcePowerRegenerate( gentity_t *self, int overrideAmt )
{
	if ( !self->client )
	{
		return;
	}

	if ( self->client->ps.forcePower < self->client->ps.forcePowerMax )
	{
		if ( overrideAmt )
		{
			self->client->ps.forcePower += overrideAmt;
		}
		else
		{
			self->client->ps.forcePower++;
		}
		if ( self->client->ps.forcePower > self->client->ps.forcePowerMax )
		{
			self->client->ps.forcePower = self->client->ps.forcePowerMax;
		}
	}
}

/*
g_debugForce / forcetally (JKEnhanced, Step 4A audit)

g_debugForce 1: one console line per Force cast (WP_ForcePowerStart): who, power, level, cost, the
caster's Force; plus Force damage taken (G_Damage, MOD_FORCE_*) and Absorb results. 2: also every
per-tick Force spend (Grip/Lightning/Drain/holds). NPCs pay nothing (WP_ForcePowerDrain), so their
lines show the cost they would have paid.
forcetally: how many times each NPC cast each power since the map started (or "forcetally reset").
Not saved.
*/
extern cvar_t *g_debugForce;
static const char *s_forcePowerNames[NUM_FORCE_POWERS] =
{
	"Heal", "Jump", "Speed", "Push", "Pull", "MindTrick", "Grip", "Lightning", "SaberThrow",
	"SaberDefense", "SaberOffense", "Rage", "Protect", "Absorb", "Drain", "Sight", "Insanity",
	"Stasis", "Repulse"
};
static int s_forceTally[MAX_GENTITIES][NUM_FORCE_POWERS];
static int s_forceTallyStart = 0;

const char *WP_ForcePowerName( int power )
{
	return ( power >= 0 && power < NUM_FORCE_POWERS ) ? s_forcePowerNames[power] : "?";
}

static const char *WP_ForceDebugWho( const gentity_t *ent )
{
	if ( !ent )
	{
		return "?";
	}
	if ( ent->s.number == 0 )
	{
		return "player";
	}
	return ent->NPC_type ? va( "%s#%d", ent->NPC_type, ent->s.number ) : va( "ent#%d", ent->s.number );
}

void WP_ForceDebugCast( const gentity_t *self, int power, int cost )
{
	if ( !self || !self->client || power < 0 || power >= NUM_FORCE_POWERS )
	{
		return;
	}
	if ( self->s.number > 0 && self->s.number < MAX_GENTITIES )
	{
		s_forceTally[self->s.number][power]++;
	}
	if ( g_debugForce && g_debugForce->integer )
	{
		const char *freeReason = WP_NPCForceFreeReason( self, power );
		gi.Printf( S_COLOR_CYAN "FORCE %s: %s L%d, cost %d, Force %d/%d%s%s%s\n", WP_ForceDebugWho( self ),
			s_forcePowerNames[power], self->client->ps.forcePowerLevel[power], cost,
			self->client->ps.forcePower, self->client->ps.forcePowerMax,
			freeReason ? " (free: " : "", freeReason ? freeReason : "", freeReason ? ")" : "" );
	}
}

void WP_ForceDebugDamage( const gentity_t *attacker, const gentity_t *targ, int take, int mod )
{
	if ( !g_debugForce || !g_debugForce->integer || ( mod != MOD_FORCE_GRIP && mod != MOD_FORCE_LIGHTNING && mod != MOD_FORCE_DRAIN ) )
	{
		return;
	}
	gi.Printf( "  %s -> %s: %d damage (%s), health now %d\n", WP_ForceDebugWho( attacker ), WP_ForceDebugWho( targ ), take,
		mod == MOD_FORCE_GRIP ? "Grip" : ( mod == MOD_FORCE_LIGHTNING ? "Lightning" : "Drain" ), targ ? targ->health : 0 );
}

void WP_ForceTally_f( void )
{
	if ( gi.argc() > 1 && !Q_stricmp( gi.argv( 1 ), "reset" ) )
	{
		memset( s_forceTally, 0, sizeof( s_forceTally ) );
		s_forceTallyStart = level.time;
		gi.Printf( "forcetally: reset\n" );
		return;
	}
	gi.Printf( "Force casts per NPC over the last %d s:\n", ( level.time - s_forceTallyStart ) / 1000 );
	for ( int i = 1; i < MAX_GENTITIES; i++ )
	{
		char line[1024] = "";
		for ( int p = 0; p < NUM_FORCE_POWERS; p++ )
		{
			if ( s_forceTally[i][p] )
			{
				Q_strcat( line, sizeof( line ), va( " %s %d,", s_forcePowerNames[p], s_forceTally[i][p] ) );
			}
		}
		if ( line[0] )
		{
			const gentity_t *ent = &g_entities[i];
			gi.Printf( "  %s:%s\n", ent->inuse ? WP_ForceDebugWho( ent ) : va( "(gone)#%d", i ), line );
		}
	}
}

void WP_ForceTallyReset( void )
{
	memset( s_forceTally, 0, sizeof( s_forceTally ) );
	s_forceTallyStart = level.time;
}

/*
WP_NPCForceFree (JKEnhanced 4B)

What an NPC doesn't pay for. g_npcForcePool 0: everything (stock infinite NPC Force). On: blocking
(the guard meter covers it), jumps (a low bar mustn't strand their pathing) and any power a script
forced (ICARUS / cinematics can't fail for lack of Force).
*/
static const char *WP_NPCForceFreeReason( const gentity_t *self, int forcePower )
{
	if ( !self || !self->NPC || !self->client )
	{
		return NULL;
	}
	extern cvar_t *g_npcForcePool;
	if ( !g_npcForcePool || !g_npcForcePool->integer )
	{
		return "NPC pool off";
	}
	if ( forcePower == FP_SABER_DEFENSE )
	{
		return "block";
	}
	if ( forcePower == FP_LEVITATION )
	{
		return "jump";
	}
	if ( forcePower >= 0 && forcePower < NUM_FORCE_POWERS && ( self->client->ps.forcePowersForced & ( 1 << forcePower ) ) )
	{
		return "scripted";
	}
	return NULL;
}

qboolean WP_NPCForceFree( const gentity_t *self, int forcePower )
{
	return (qboolean)( WP_NPCForceFreeReason( self, forcePower ) != NULL );
}

void WP_ForcePowerDrain( gentity_t *self, forcePowers_t forcePower, int overrideAmt )
{
	const char *freeReason = WP_NPCForceFreeReason( self, forcePower );
	//take away the power
	int	drain = overrideAmt;
	if ( !drain )
	{
		drain = forcePowerNeeded[forcePower];
	}
	if ( self->NPC && !freeReason && drain > 0 )
	{//JKEnhanced 4B: NPCs pay (force.cfg npc.costScale), never less than 1
		drain = Q_max( 1, (int)( drain * G_ForceTuning()->npcCostScale + 0.5f ) );
	}
	if ( g_debugForce && g_debugForce->integer > 1 && self && self->client )
	{//per-tick / follow-up spends
		gi.Printf( "  FORCE %s spends %d on %s (Force %d)%s%s%s\n", WP_ForceDebugWho( self ),
			drain, WP_ForcePowerName( forcePower ), self->client->ps.forcePower,
			freeReason ? " (free: " : "", freeReason ? freeReason : "", freeReason ? ")" : "" );
	}
	if ( freeReason )
	{
		return;
	}
	if ( !drain )
	{
		return;
	}
	self->client->ps.forcePower -= drain;
	if ( self->client->ps.forcePower < 0 )
	{
		self->client->ps.forcePower = 0;
	}
}

void WP_ForcePowerStart( gentity_t *self, forcePowers_t forcePower, int overrideAmt )
{
	int	duration = 0;

	//JKEnhanced audit: log/tally the cast (Lightning's override is its duration, not a cost)
	WP_ForceDebugCast( self, forcePower, ( forcePower == FP_REPULSE ) ? ( self->s.number < MAX_CLIENTS ? G_ForceTuning()->repulseStartCost : WP_RepulseCost( self ) )
		: ( forcePower == FP_LIGHTNING || !overrideAmt ) ? forcePowerNeeded[forcePower] : overrideAmt );//(Repulse: the player's start cost; NPCs pay on release)

	//FIXME: debounce some of these?
	self->client->ps.forcePowerDebounce[forcePower] = 0;

	//and it in
	//set up duration time
	switch( (int)forcePower )
	{
	case FP_HEAL:
		self->client->ps.forcePowersActive |= ( 1 << forcePower );
		self->client->ps.forceHealCount = 0;
		WP_StartForceHealEffects( self );
		break;
	case FP_LEVITATION:
		self->client->ps.forcePowersActive |= ( 1 << forcePower );
		break;
	case FP_SPEED:
		{//JKEnhanced 4B.6: force.cfg speed.duration1-3, in real time (Burst 4 s / Flow 7 s / Focus 10 s). The
		 //player's own slow-mo runs game time at forceSpeedValue, so game time = real x that; was 10 s at
		 //every level
			const int speedLevel = Q_max( FORCE_LEVEL_1, Q_min( FORCE_LEVEL_3, self->client->ps.forcePowerLevel[FP_SPEED] ) );
			const float realMs = (float)G_ForceTuning()->speedDuration[speedLevel - 1];
			duration = ( self->s.number < MAX_CLIENTS ) ? (int)ceil( realMs * forceSpeedValue[speedLevel] ) : (int)realMs;
			G_SpeedCastStart( self );
			if ( g_debugForce && g_debugForce->integer )
			{
				gi.Printf( "  SPEED %s: %s for %.1f s%s\n", WP_ForceDebugWho( self ),
					speedLevel >= FORCE_LEVEL_3 ? "Focus (L3)" : speedLevel == FORCE_LEVEL_2 ? "Flow (L2)" : "Burst (L1)",
					realMs / 1000.0f, speedLevel >= FORCE_LEVEL_3 ? va( ", %d reflex dodge(s)", G_ForceTuning()->speedDodges ) : "" );
			}
		}
		self->client->ps.forcePowersActive |= ( 1 << forcePower );
		self->s.loopSound = G_SoundIndex( "sound/weapons/force/speedloop.wav" );
		if ( self->client->ps.forcePowerLevel[FP_SPEED] > FORCE_LEVEL_2 )
		{//HACK: just using this as a timestamp for when the power started, setting debounce to current time shouldn't adversely affect anything else
			self->client->ps.forcePowerDebounce[FP_SPEED] = level.time;
		}
		break;
	case FP_PUSH:
		break;
	case FP_PULL:
		self->client->ps.forcePowersActive |= ( 1 << forcePower );
		break;
	case FP_TELEPATHY:
		break;
	case FP_GRIP:
		duration = 1000;
		self->client->ps.forcePowersActive |= ( 1 << forcePower );
		//HACK: just using this as a timestamp for when the power started, setting debounce to current time shouldn't adversely affect anything else
		//self->client->ps.forcePowerDebounce[forcePower] = level.time;
		break;
	case FP_LIGHTNING:
		duration = overrideAmt;
		overrideAmt = 0;
		self->client->ps.forcePowersActive |= ( 1 << forcePower );
		break;
	//new Jedi Academy force powers
	case FP_RAGE:
		//duration is always 5 seconds, player time
		duration = ceil(FORCE_RAGE_DURATION*forceSpeedValue[self->client->ps.forcePowerLevel[FP_RAGE]-1]);//FIXME: because the timescale scales down (not instant), this doesn't end up being exactly right...
		self->client->ps.forcePowersActive |= ( 1 << forcePower );
		G_SoundOnEnt( self, CHAN_ITEM, "sound/weapons/force/rage.mp3" );
		self->s.loopSound = G_SoundIndex( "sound/weapons/force/rageloop.wav" );
		if ( self->chestBolt != -1 )
		{
			G_PlayEffect( G_EffectIndex( "force/rage2" ), self->playerModel, self->chestBolt, self->s.number, self->currentOrigin, duration, qtrue );
		}
		break;
	case FP_DRAIN:
		if ( self->client->ps.forcePowerLevel[forcePower] > FORCE_LEVEL_1
			&& self->client->ps.forceDrainEntityNum >= ENTITYNUM_WORLD )
		{
			duration = overrideAmt;
			overrideAmt = 0;
			//HACK: just using this as a timestamp for when the power started, setting debounce to current time shouldn't adversely affect anything else
			self->client->ps.forcePowerDebounce[forcePower] = level.time;
		}
		else
		{
			duration = 1000;
		}
		self->client->ps.forcePowersActive |= ( 1 << forcePower );
		break;
	case FP_PROTECT:
		//JKEnhanced 4B.3: L1-2 run for force.cfg's duration; L3 is a toggle (no duration) and stays
		//silent and invisible until it stops something (G_ForceShieldFlash)
		{
			const int lvl = self->client->ps.forcePowerLevel[forcePower];
			self->client->ps.forcePowersActive |= ( 1 << forcePower );
			G_SoundOnEnt( self, CHAN_ITEM, "sound/weapons/force/protect.mp3" );
			if ( lvl < FORCE_LEVEL_3 )
			{
				duration = G_ForceTuning()->protectDuration[( lvl > FORCE_LEVEL_1 ) ? 1 : 0];
				self->s.loopSound = G_SoundIndex( "sound/weapons/force/protectloop.wav" );
			}
		}
		break;
	case FP_ABSORB:
		duration = 20000;
		self->client->ps.forcePowersActive |= ( 1 << forcePower );
		G_SoundOnEnt( self, CHAN_ITEM, "sound/weapons/force/absorb.mp3" );
		self->s.loopSound = G_SoundIndex( "sound/weapons/force/absorbloop.wav" );
		break;
	case FP_SEE:
		if ( self->client->ps.forcePowerLevel[FP_SEE] == FORCE_LEVEL_1 )
		{
			duration = 5000;
		}
		else if ( self->client->ps.forcePowerLevel[FP_SEE] == FORCE_LEVEL_2 )
		{
			duration = 10000;
		}
		else// if ( self->client->ps.forcePowerLevel[FP_SEE] == FORCE_LEVEL_3 )
		{
			duration = 20000;
		}

		self->client->ps.forcePowersActive |= ( 1 << forcePower );
		G_SoundOnEnt( self, CHAN_ITEM, "sound/weapons/force/see.mp3" );
		self->s.loopSound = G_SoundIndex( "sound/weapons/force/seeloop.wav" );
		break;
	case FP_INSANITY:
		break;
	case FP_STASIS:
		break;
	case FP_REPULSE://JKEnhanced (restored)
		self->client->ps.forcePowersActive |= ( 1 << forcePower );
		self->client->ps.powerups[PW_FORCE_REPULSE] = Q3_INFINITE;
		self->client->pushEffectFadeTime = 0;
		break;
	default:
		break;
	}
	if ( duration )
	{
		self->client->ps.forcePowerDuration[forcePower] = level.time + duration;
	}
	else
	{
		self->client->ps.forcePowerDuration[forcePower] = 0;
	}

	if ( overrideAmt >= 0 )
	{//JKEnhanced: -1 = nothing up front (Repulse pays on release)
		WP_ForcePowerDrain( self, forcePower, overrideAmt );
	}

	if ( !self->s.number )
	{
		self->client->sess.missionStats.forceUsed[(int)forcePower]++;
	}
}

qboolean WP_ForcePowerAvailable( gentity_t *self, forcePowers_t forcePower, int overrideAmt )
{
	if ( forcePower == FP_LEVITATION )
	{
		return qtrue;
	}
	int	drain = overrideAmt?overrideAmt:forcePowerNeeded[forcePower];
	if ( !drain )
	{
		return qtrue;
	}
	extern cvar_t *g_npcForcePool;
	if ( g_npcForcePool && g_npcForcePool->integer && WP_NPCForceFree( self, forcePower ) )
	{//JKEnhanced 4B: costs this NPC nothing (block, jump, scripted). Pool off: stock check below
	 //(their bar stays full unless the player Drains it)
		return qtrue;
	}
	if ( self->NPC && g_npcForcePool && g_npcForcePool->integer )
	{//what they'll actually pay (npc.costScale)
		drain = Q_max( 1, (int)( drain * G_ForceTuning()->npcCostScale + 0.5f ) );
	}
	if ( self->NPC && WP_SaberRules() && g_npcForcePool && g_npcForcePool->integer
		&& self->client->ps.weapon == WP_SABER && self->client->ps.forcePowerMax > 0
		&& forcePower != FP_SABER_DEFENSE && forcePower != FP_SABER_OFFENSE
		&& self->enemy && self->enemy->client && self->enemy->client->ps.weapon == WP_SABER
		&& self->client->ps.forcePower - drain < self->client->ps.forcePowerMax / 2 )
	{//JKEnhanced saber rules: Force is a saber fighter's guard. Facing another saber they keep half of
	 //it back for that: a power that would take them under half isn't started, and one that is
	 //running (Drain, lightning, grip) stops when it gets there.
		return qfalse;
	}
	if ( self->client->ps.forcePower < drain )
	{
		//G_AddEvent( self, EV_NOAMMO, 0 );
		return qfalse;
	}
	return qtrue;
}

extern void CG_PlayerLockedWeaponSpeech( int jumping );
extern qboolean Rosh_TwinNearBy( gentity_t *self );
qboolean WP_ForcePowerUsable( gentity_t *self, forcePowers_t forcePower, int overrideAmt )
{
	if ( !(self->client->ps.forcePowersKnown & ( 1 << forcePower )) )
	{//don't know this power
		return qfalse;
	}
	else if ( self->NPC && (self->NPC->aiFlags&NPCAI_ROSH) )
	{
		if ( ((1<<forcePower)&FORCE_POWERS_ROSH_FROM_TWINS) )
		{//this is a force power we can only use when a twin is near us
			if ( !Rosh_TwinNearBy( self ) )
			{
				return qfalse;
			}
		}
	}

	if ( self->client->ps.forcePowerLevel[forcePower] <= 0 )
	{//can't use this power
		return qfalse;
	}

	if( (self->flags&FL_LOCK_PLAYER_WEAPONS) ) // yes this locked weapons check also includes force powers, if we need a separate check later I'll make one
	{
		if ( self->s.number < MAX_CLIENTS )
		{
			CG_PlayerLockedWeaponSpeech( qfalse );
		}
		return qfalse;
	}

	if ( in_camera && self->s.number < MAX_CLIENTS )
	{//player can't turn on force powers duing cinematics
		return qfalse;
	}

	if ( PM_LockedAnim( self->client->ps.torsoAnim ) && self->client->ps.torsoAnimTimer )
	{//no force powers during these special anims
		return qfalse;
	}
	if ( PM_SuperBreakLoseAnim( self->client->ps.torsoAnim )
		|| PM_SuperBreakWinAnim( self->client->ps.torsoAnim ) )
	{
		return qfalse;
	}

	if ( (self->client->ps.forcePowersActive & ( 1 << forcePower )) )
	{//already using this power
		return qfalse;
	}
	/*
	if ( !self->client->ps.forcePowerLevel[(int)(forcePower)] )
	{
		return qfalse;
	}
	*/
	if ( self->NPC && !( self->client->ps.forcePowersForced & ( 1 << forcePower ) )
		&& WP_AbsorbablePower( forcePower ) && forcePower != FP_REPULSE && !TIMER_Done( self, "enemyAbsorbs" )
		&& self->enemy && self->enemy->client && ( self->enemy->client->ps.forcePowersActive & ( 1 << FP_ABSORB ) ) )
	{//JKEnhanced 4B.3: their enemy just absorbed one of these - don't feed them Force until it lapses
	 //(timer set in WP_AbsorbConversion; once it runs out they'll test it again)
		return qfalse;
	}
	if ( self->client->NPC_class == CLASS_ATST )
	{//Doh!  No force powers in an AT-ST!
		return qfalse;
	}
	Vehicle_t *pVeh = NULL;
	if ( (pVeh = G_IsRidingVehicle( self )) != NULL )
	{//Doh!  No force powers when flying a vehicle!
		if ( pVeh->m_pVehicleInfo->numHands > 1 )
		{//if in a two-handed vehicle
			return qfalse;
		}
	}
	if ( self->client->ps.viewEntity > 0 && self->client->ps.viewEntity < ENTITYNUM_WORLD )
	{//Doh!  No force powers when controlling an NPC
		return qfalse;
	}
	if ( self->client->ps.eFlags & EF_LOCKED_TO_WEAPON )
	{//Doh!  No force powers when in an emplaced gun!
		return qfalse;
	}

	if ( (self->client->ps.saber[0].saberFlags&SFL_SINGLE_BLADE_THROWABLE)//SaberStaff() //using staff
		&& !self->client->ps.dualSabers //only 1, in right hand
		&& !self->client->ps.saber[0].blade[1].active )//only first blade is on
	{//allow power
		//FIXME: externalize this condition seperately?
	}
	else
	{
		if ( forcePower == FP_SABERTHROW && (self->client->ps.saber[0].saberFlags&SFL_NOT_THROWABLE) )
		{//cannot throw this kind of saber
			return qfalse;
		}

		if ( self->client->ps.saber[0].Active() )
		{
			if ( (self->client->ps.saber[0].saberFlags&SFL_TWO_HANDED) )
			{
				if ( g_saberRestrictForce->integer )
				{
					switch ( forcePower )
					{
					case FP_PUSH:
					case FP_PULL:
					case FP_TELEPATHY:
					case FP_GRIP:
					case FP_LIGHTNING:
					case FP_DRAIN:
						return qfalse;
					default:
						break;
					}
				}
			}
			if ( (self->client->ps.saber[0].saberFlags&SFL_TWO_HANDED)
				|| (self->client->ps.dualSabers && self->client->ps.saber[1].Active()) )
			{//this saber requires the use of two hands OR our other hand is using an active saber too
				if ( (self->client->ps.saber[0].forceRestrictions&(1<<forcePower)) )
				{//this power is verboten when using this saber
					return qfalse;
				}
			}
		}
		if ( self->client->ps.dualSabers && self->client->ps.saber[1].Active() )
		{
			if ( g_saberRestrictForce->integer )
			{
				switch ( forcePower )
				{
				case FP_PUSH:
				case FP_PULL:
				case FP_TELEPATHY:
				case FP_GRIP:
				case FP_LIGHTNING:
				case FP_DRAIN:
					return qfalse;
				default:
					break;
				}
			}
			if ( (self->client->ps.saber[1].forceRestrictions&(1<<forcePower)) )
			{//this power is verboten when using this saber
				return qfalse;
			}
		}
	}

	return WP_ForcePowerAvailable( self, forcePower, overrideAmt );
}

void WP_ForcePowerStop( gentity_t *self, forcePowers_t forcePower )
{
	gentity_t	*gripEnt;
	gentity_t	*drainEnt;

	if ( !(self->client->ps.forcePowersActive&(1<<forcePower)) )
	{//umm, wasn't doing it, so...
		return;
	}

	self->client->ps.forcePowersActive &= ~( 1 << forcePower );

	switch( (int)forcePower )
	{
	case FP_HEAL:
		//if ( self->client->ps.forcePowerLevel[FP_HEAL] < FORCE_LEVEL_3 )
		{//wasn't an instant heal and heal is now done
			if ( self->client->ps.forcePowerLevel[FP_HEAL] < FORCE_LEVEL_2 )
			{//if in meditation pose, must come out of it
				//FIXME: BOTH_FORCEHEAL_STOP
				if ( self->client->ps.legsAnim == BOTH_FORCEHEAL_START )
				{
					NPC_SetAnim( self, SETANIM_LEGS, BOTH_FORCEHEAL_STOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				}
				if ( self->client->ps.torsoAnim == BOTH_FORCEHEAL_START )
				{
					NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCEHEAL_STOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				}
				self->client->ps.saberMove = self->client->ps.saberBounceMove = LS_READY;//don't finish whatever saber anim you may have been in
				self->client->ps.saberBlocked = BLOCKED_NONE;
			}
		}
		WP_StopForceHealEffects( self );
		if (self->health >= self->client->ps.stats[STAT_MAX_HEALTH]/3)
		{
			gi.G2API_ClearSkinGore(self->ghoul2);
		}
		break;
	case FP_LEVITATION:
		self->client->ps.forcePowerDebounce[FP_LEVITATION] = 0;
		break;
	case FP_SPEED:
		G_SpeedCastEnd( self );//JKEnhanced 4B.6: unused Focus dodges go with it
		if ( !self->s.number )
		{//player using force speed
			if ( g_timescale->value != 1.0 )
			{
				if ( !(self->client->ps.forcePowersActive&(1<<FP_RAGE))||self->client->ps.forcePowerLevel[FP_RAGE] < FORCE_LEVEL_2 )
				{//not slowed down because of force rage
					gi.cvar_set("timescale", "1");
				}
			}
		}
		//FIXME: reset my current anim, keeping current frame, but with proper anim speed
		//		otherwise, the anim will continue playing at high speed
		self->s.loopSound = 0;
		break;
	case FP_PUSH:
		break;
	case FP_PULL:
		break;
	case FP_TELEPATHY:
		break;
	case FP_GRIP:
		if ( self->NPC )
		{
			TIMER_Set( self, "gripping", -level.time );
		}
		if ( self->client->ps.forceGripEntityNum < ENTITYNUM_WORLD )
		{
			gripEnt = &g_entities[self->client->ps.forceGripEntityNum];
			if ( gripEnt )
			{
				gripEnt->s.loopSound = 0;
				if ( gripEnt->client )
				{
					gripEnt->client->ps.eFlags &= ~EF_FORCE_GRIPPED;
					if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
					{//sanity-cap the velocity
						float gripVel = VectorNormalize( gripEnt->client->ps.velocity );
						if ( gripVel > 500.0f )
						{
							gripVel = 500.0f;
						}
						VectorScale( gripEnt->client->ps.velocity, gripVel, gripEnt->client->ps.velocity );
					}

					//FIXME: they probably dropped their weapon, should we make them flee?  Or should AI handle no-weapon behavior?
//rww - RAGDOLL_BEGIN
#ifndef JK2_RAGDOLL_GRIPNOHEALTH
//rww - RAGDOLL_END
					if ( gripEnt->health > 0 && !s_gripHeldObject[self->s.number] )//JKEnhanced: a knocked-out body doesn't come to and raise the alarm
//rww - RAGDOLL_BEGIN
#endif
//rww - RAGDOLL_END
					{
						int	holdTime = 500;
						if ( gripEnt->health > 0 )
						{
							G_AddEvent( gripEnt, EV_WATER_CLEAR, 0 );
						}
						if ( gripEnt->client->ps.forcePowerDebounce[FP_PUSH] > level.time )
						{//they probably pushed out of it
							holdTime = 0;
						}
						else if ( gripEnt->s.weapon == WP_SABER )
						{//jedi recover faster
							holdTime = self->client->ps.forcePowerLevel[FP_GRIP]*200;
						}
						else
						{
							holdTime = self->client->ps.forcePowerLevel[FP_GRIP]*500;
						}
						//stop the anims soon, keep them locked in place for a bit
						if ( gripEnt->client->ps.torsoAnim == BOTH_CHOKE1 || gripEnt->client->ps.torsoAnim == BOTH_CHOKE3 )
						{//stop choking anim on torso
							if ( gripEnt->client->ps.torsoAnimTimer > holdTime )
							{
								gripEnt->client->ps.torsoAnimTimer = holdTime;
							}
						}
						if ( gripEnt->client->ps.legsAnim == BOTH_CHOKE1 || gripEnt->client->ps.legsAnim == BOTH_CHOKE3 )
						{//stop choking anim on legs
							gripEnt->client->ps.legsAnimTimer = 0;
							if ( holdTime )
							{
								//lock them in place for a bit
								gripEnt->client->ps.pm_time = gripEnt->client->ps.torsoAnimTimer;
								gripEnt->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
								if ( gripEnt->s.number )
								{//NPC
									gripEnt->painDebounceTime = level.time + gripEnt->client->ps.torsoAnimTimer;
								}
								else
								{//player
									gripEnt->aimDebounceTime = level.time + gripEnt->client->ps.torsoAnimTimer;
								}
							}
						}
						if ( gripEnt->NPC )
						{
							if ( !(gripEnt->NPC->aiFlags&NPCAI_DIE_ON_IMPACT) )
							{//not falling to their death
								gripEnt->NPC->nextBStateThink = level.time + holdTime;
							}
							//if still alive after stopped gripping, let them wake others up
							if ( gripEnt->health > 0 )
							{
								G_AngerAlert( gripEnt );
							}
						}
						if ( holdTime > 0 )
						{//don't let us get back to attacking well before they can defend themselves again
							int scaledHold = (int)( holdTime * g_forceStaggerLockoutScale->value );
							self->client->ps.weaponTime = Q_max( self->client->ps.weaponTime, scaledHold );
						}
					}
				}
				else
				{
					gripEnt->s.eFlags &= ~EF_FORCE_GRIPPED;
					if ( gripEnt->s.eType == ET_MISSILE )
					{//continue normal movement
						if ( gripEnt->s.weapon == WP_THERMAL )
						{
							gripEnt->s.pos.trType = TR_INTERPOLATE;
						}
						else
						{
							gripEnt->s.pos.trType = TR_LINEAR;//FIXME: what about gravity-effected projectiles?
						}
						VectorCopy( gripEnt->currentOrigin, gripEnt->s.pos.trBase );
						gripEnt->s.pos.trTime = level.time;
					}
					else
					{//drop it (JKEnhanced 4B.5: items / limbs back to their own physics, not G_RunObject)
						WP_GripObjectPhysics( gripEnt );
						gripEnt->s.pos.trType = TR_GRAVITY;
						VectorCopy( gripEnt->currentOrigin, gripEnt->s.pos.trBase );
						gripEnt->s.pos.trTime = level.time;
						if ( gripEnt->s.eType == ET_ITEM )
						{//held off the ground while carried (WP_GripMoveObject)
							gripEnt->s.groundEntityNum = ENTITYNUM_NONE;
						}
						if ( VectorLengthSquared( gripEnt->s.pos.trDelta ) > 600.0f*600.0f )
						{//keeps the carry momentum, within reason
							VectorNormalize( gripEnt->s.pos.trDelta );
							VectorScale( gripEnt->s.pos.trDelta, 600.0f, gripEnt->s.pos.trDelta );
						}
					}
				}
			}
			self->s.loopSound = 0;
			self->client->ps.forceGripEntityNum = ENTITYNUM_NONE;
		}
		if ( self->client->ps.torsoAnim == BOTH_FORCEGRIP_HOLD )
		{
			NPC_SetAnim( self, SETANIM_BOTH, BOTH_FORCEGRIP_RELEASE, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
		break;
	case FP_LIGHTNING:
		if ( self->NPC )
		{
			TIMER_Set( self, "holdLightning", -level.time );
		}
		if ( self->client->ps.torsoAnim == BOTH_FORCELIGHTNING_HOLD
			|| self->client->ps.torsoAnim == BOTH_FORCELIGHTNING_START )
		{
			NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCELIGHTNING_RELEASE, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
		else if ( self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
			|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START )
		{
			NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCE_2HANDEDLIGHTNING_RELEASE, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
		if ( self->client->ps.forcePowerLevel[FP_LIGHTNING] < FORCE_LEVEL_2 )
		{//don't do it again for 3 seconds, minimum... FIXME: this should be automatic once regeneration is slower (normal)
			self->client->ps.forcePowerDebounce[FP_LIGHTNING] = level.time + 3000;//FIXME: define?
		}
		else
		{//stop the looping sound
			self->client->ps.forcePowerDebounce[FP_LIGHTNING] = level.time + 1000;//FIXME: define?
			self->s.loopSound = 0;
		}
		break;
	case FP_RAGE:
		self->client->ps.forceRageRecoveryTime = level.time + 10000;//recover for 10 seconds
		if ( self->client->ps.forcePowerDuration[FP_RAGE] > level.time )
		{//still had time left, we cut it short
			self->client->ps.forceRageRecoveryTime -= (self->client->ps.forcePowerDuration[FP_RAGE] - level.time);//minus however much time you had left when you cut it short
		}
		if ( !self->s.number )
		{//player using force speed
			if ( g_timescale->value != 1.0 )
			{
				if ( !(self->client->ps.forcePowersActive&(1<<FP_SPEED)) )
				{//not slowed down because of force speed
					gi.cvar_set("timescale", "1");
				}
			}
		}
		//FIXME: reset my current anim, keeping current frame, but with proper anim speed
		//		otherwise, the anim will continue playing at high speed
		self->s.loopSound = 0;
		if ( self->NPC )
		{
			Jedi_RageStop( self );
		}
		if ( self->chestBolt != -1 )
		{
			G_StopEffect("force/rage2", self->playerModel, self->chestBolt, self->s.number );
		}
		break;
	case FP_DRAIN:
		if ( self->NPC )
		{
			TIMER_Set( self, "draining", -level.time );
		}
		if ( self->client->ps.forcePowerLevel[FP_DRAIN] < FORCE_LEVEL_2 )
		{//don't do it again for 3 seconds, minimum... FIXME: this should be automatic once regeneration is slower (normal)
			self->client->ps.forcePowerDebounce[FP_DRAIN] = level.time + 3000;//FIXME: define?
		}
		else
		{//stop the looping sound
			self->client->ps.forcePowerDebounce[FP_DRAIN] = level.time + 1000;//FIXME: define?
			self->s.loopSound = 0;
		}
		//drop them
		if ( self->client->ps.forceDrainEntityNum < ENTITYNUM_WORLD )
		{
			drainEnt = &g_entities[self->client->ps.forceDrainEntityNum];
			if ( drainEnt )
			{
				if ( drainEnt->client )
				{
					drainEnt->client->ps.eFlags &= ~EF_FORCE_DRAINED;
					//VectorClear( drainEnt->client->ps.velocity );
					if ( drainEnt->health > 0 )
					{
						if ( drainEnt->client->ps.forcePowerDebounce[FP_PUSH] > level.time )
						{//they probably pushed out of it
						}
						else
						{
							//NPC_SetAnim( drainEnt, SETANIM_BOTH, BOTH_HUGGEESTOP1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
							if ( drainEnt->client->ps.torsoAnim != BOTH_FORCEPUSH )
							{//don't stop the push
								drainEnt->client->ps.torsoAnimTimer = 0;
							}
							drainEnt->client->ps.legsAnimTimer = 0;
						}
						if ( drainEnt->NPC )
						{//if still alive after stopped draining, let them wake others up
							G_AngerAlert( drainEnt );
						}
					}
					else
					{//leave the effect playing on them for a few seconds
						//drainEnt->client->ps.eFlags |= EF_FORCE_DRAINED;
						drainEnt->s.powerups |= ( 1 << PW_DRAINED );
						drainEnt->client->ps.powerups[PW_DRAINED] = level.time + Q_irand( 1000, 4000 );
					}
				}
			}
			self->client->ps.forceDrainEntityNum = ENTITYNUM_NONE;
		}
		if ( self->client->ps.torsoAnim == BOTH_HUGGER1 )
		{//old anim
			NPC_SetAnim( self, SETANIM_BOTH, BOTH_HUGGERSTOP1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
		else if ( self->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_START
			|| self->client->ps.torsoAnim ==  BOTH_FORCE_DRAIN_GRAB_HOLD )
		{//new anim
			NPC_SetAnim( self, SETANIM_BOTH, BOTH_FORCE_DRAIN_GRAB_END, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
		else if ( self->client->ps.torsoAnim == BOTH_FORCE_DRAIN_HOLD
			|| self->client->ps.torsoAnim == BOTH_FORCE_DRAIN_START )
		{
			NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCE_DRAIN_RELEASE, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
		break;
	case FP_PROTECT:
		self->s.loopSound = 0;
		break;
	case FP_ABSORB:
		self->s.loopSound = 0;
		if ( self->client->ps.legsAnim == BOTH_FORCE_ABSORB_START )
		{
			NPC_SetAnim( self, SETANIM_LEGS, BOTH_FORCE_ABSORB_END, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
		if ( self->client->ps.torsoAnim == BOTH_FORCE_ABSORB_START )
		{
			NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCE_ABSORB_END, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		}
		if ( self->client->ps.forcePowerLevel[FP_ABSORB] < FORCE_LEVEL_2 )
		{//was stuck, free us in case we interrupted it or something
			self->client->ps.weaponTime = 0;
			self->client->ps.pm_flags &= ~PMF_TIME_KNOCKBACK;
			self->client->ps.pm_time = 0;
			if ( self->s.number )
			{//NPC
				self->painDebounceTime = 0;
			}
			else
			{//player
				self->aimDebounceTime = 0;
			}
		}
		break;
	case FP_SEE:
		self->s.loopSound = 0;
		break;
	case FP_INSANITY:
		break;
	case FP_STASIS:
		break;
	case FP_REPULSE://JKEnhanced (restored): end the charge visual; a charge in progress throws now
		self->client->ps.powerups[PW_FORCE_REPULSE] = 0;
		if ( self->client->ps.repulseChargeStart )
		{
			extern void ForceRepulseThrow( gentity_t *self, int chargeTime );
			self->s.loopSound = 0;
			ForceRepulseThrow( self, WP_RepulseChargeTime( self ) );
			self->client->ps.repulseChargeStart = 0;
		}
		self->s.loopSound = 0;
		{//anything still held from the charge (it didn't go off) carries on
			extern void G_RepulseReleaseShots( gentity_t *self );
			G_RepulseReleaseShots( self );
		}
		break;
	default:
		break;
	}
}

void WP_ForceForceThrow( gentity_t *thrower )
{
	if ( !thrower || !thrower->client )
	{
		return;
	}
	qboolean relock = qfalse;
	if ( !(thrower->client->ps.forcePowersKnown&(1<<FP_PUSH)) )
	{//give them push just for this specific purpose
		thrower->client->ps.forcePowersKnown |= (1<<FP_PUSH);
		thrower->client->ps.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_1;
	}

	if ( thrower->NPC
		&& (thrower->NPC->aiFlags&NPCAI_HEAL_ROSH)
		&& (thrower->flags&FL_LOCK_PLAYER_WEAPONS) )
	{
		thrower->flags &= ~FL_LOCK_PLAYER_WEAPONS;
		relock = qtrue;
	}

	ForceThrow( thrower, qfalse );

	if ( relock )
	{
		thrower->flags |= FL_LOCK_PLAYER_WEAPONS;
	}

	if ( thrower )
	{//take it back off
		thrower->client->ps.forcePowersKnown &= ~(1<<FP_PUSH);
		thrower->client->ps.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_0;
	}
}

extern qboolean PM_ForceJumpingUp( gentity_t *gent );
static void WP_ForcePowerRun( gentity_t *self, forcePowers_t forcePower, usercmd_t *cmd )
{
	float				speed, newSpeed;
	gentity_t			*gripEnt, *drainEnt;
	vec3_t				angles, dir, gripOrg, gripEntOrg;
	float				dist;
	extern usercmd_t	ucmd;

	switch( (int)forcePower )
	{
	case FP_HEAL:
		if ( self->client->ps.forceHealCount >= FP_MaxForceHeal(self) || self->health >= self->client->ps.stats[STAT_MAX_HEALTH] )
		{//fully healed or used up all 25
			if ( !Q3_TaskIDPending( self, TID_CHAN_VOICE ) )
			{
				int index = Q_irand( 1, 4 );
				if ( self->s.number < MAX_CLIENTS )
				{
					G_SoundOnEnt( self, CHAN_VOICE, va( "sound/weapons/force/heal%d_%c.mp3", index, g_sex->string[0] ) );
				}
				else if ( self->NPC )
				{
					if ( self->NPC->blockedSpeechDebounceTime <= level.time )
					{//enough time has passed since our last speech
						if ( Q3_TaskIDPending( self, TID_CHAN_VOICE ) )
						{//not playing a scripted line
							//say "Ahhh...."
							if ( self->NPC->stats.sex == SEX_MALE
								|| self->NPC->stats.sex == SEX_NEUTRAL )
							{
								G_SoundOnEnt( self, CHAN_VOICE, va( "sound/weapons/force/heal%d_m.mp3", index ) );
							}
							else//all other sexes use female sounds
							{
								G_SoundOnEnt( self, CHAN_VOICE, va( "sound/weapons/force/heal%d_f.mp3", index ) );
							}
						}
					}
				}
			}
			WP_ForcePowerStop( self, forcePower );
		}
		else if ( self->client->ps.forcePowerLevel[FP_HEAL] < FORCE_LEVEL_3 && ( (cmd->buttons&BUTTON_ATTACK) || (cmd->buttons&BUTTON_ALT_ATTACK) || self->painDebounceTime > level.time || (self->client->ps.weaponTime&&self->client->ps.weapon!=WP_NONE) ) )
		{//attacked or was hit while healing...
			//stop healing
			WP_ForcePowerStop( self, forcePower );
		}
		else if ( self->client->ps.forcePowerLevel[FP_HEAL] < FORCE_LEVEL_2 && ( cmd->rightmove || cmd->forwardmove || cmd->upmove > 0 ) )
		{//moved while healing... FIXME: also, in WP_ForcePowerStart, stop healing if any other force power is used
			//stop healing
			WP_ForcePowerStop( self, forcePower );
		}
		else if ( self->client->ps.forcePowerDebounce[FP_HEAL] < level.time )
		{//time to heal again
			if ( WP_ForcePowerAvailable( self, forcePower, 4 ) )
			{//have available power
				int healInterval = FP_ForceHealInterval( self );
				int	healAmount = 1;//hard, normal healing rate
				if ( self->s.number < MAX_CLIENTS )
				{
					if ( g_spskill->integer == 1 )
					{//medium, heal twice as fast
						healAmount *= 2;
					}
					else if ( g_spskill->integer == 0 )
					{//easy, heal 3 times as fast...
						healAmount *= 3;
					}
				}
				if ( self->health + healAmount > self->client->ps.stats[STAT_MAX_HEALTH] )
				{
					healAmount = self->client->ps.stats[STAT_MAX_HEALTH] - self->health;
				}
				{//JKEnhanced: a staged boss can't heal back up into a stage it already lost
					extern int G_BossHealthCeiling( const gentity_t *ent );
					const int ceiling = G_BossHealthCeiling( self );
					if ( self->health + healAmount > ceiling )
					{
						healAmount = Q_max( 0, ceiling - self->health );
					}
				}
				self->health += healAmount;
				self->client->ps.forceHealCount += healAmount;
				self->client->ps.forcePowerDebounce[FP_HEAL] = level.time + healInterval;
				WP_ForcePowerDrain( self, forcePower, 4 );
			}
			else
			{//stop
				WP_ForcePowerStop( self, forcePower );
			}
		}
		break;
	case FP_LEVITATION:
		if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE && !self->client->ps.forceJumpZStart )
		{//done with jump
			WP_ForcePowerStop( self, forcePower );
		}
		else
		{
			if ( PM_ForceJumpingUp( self ) )
			{//holding jump in air
				if ( cmd->upmove > 10 )
				{//still trying to go up
					if ( WP_ForcePowerAvailable( self, FP_LEVITATION, 1 ) )
					{
						if ( self->client->ps.forcePowerDebounce[FP_LEVITATION] < level.time )
						{
							WP_ForcePowerDrain( self, FP_LEVITATION, 5 );
							self->client->ps.forcePowerDebounce[FP_LEVITATION] = level.time + 100;
						}
						self->client->ps.forcePowersActive |= ( 1 << FP_LEVITATION );
						self->client->ps.forceJumpCharge = 1;//just used as a flag for the player, cleared when he lands
					}
					else
					{//cut the jump short
						WP_ForcePowerStop( self, forcePower );
					}
				}
				else
				{//cut the jump short
					WP_ForcePowerStop( self, forcePower );
				}
			}
			else
			{
				WP_ForcePowerStop( self, forcePower );
			}
		}
		break;
	case FP_SPEED:
		speed = forceSpeedValue[self->client->ps.forcePowerLevel[FP_SPEED]];
		if ( !self->s.number )
		{//player using force speed
			if ( !(self->client->ps.forcePowersActive&(1<<FP_RAGE))
				|| self->client->ps.forcePowerLevel[FP_SPEED] >= self->client->ps.forcePowerLevel[FP_RAGE] )
			{//either not using rage or rage is at a lower level than speed
				gi.cvar_set("timescale", va("%4.2f", speed));
				if ( g_timescale->value > speed )
				{
					newSpeed = g_timescale->value - 0.05;
					if ( newSpeed < speed )
					{
						newSpeed = speed;
					}
					gi.cvar_set("timescale", va("%4.2f", newSpeed));
				}
			}
		}
		break;
	case FP_PUSH:
		break;
	case FP_PULL:
		break;
	case FP_TELEPATHY:
		break;
	case FP_GRIP:
		if ( !WP_ForcePowerAvailable( self, FP_GRIP, 0 )
			|| (self->client->ps.forcePowerLevel[FP_GRIP]>FORCE_LEVEL_1&&!self->s.number&&!(cmd->buttons&BUTTON_FORCEGRIP)) )
		{
			WP_ForcePowerStop( self, FP_GRIP );
			return;
		}
		else if ( self->client->ps.forceGripEntityNum >= 0 && self->client->ps.forceGripEntityNum < ENTITYNUM_WORLD )
		{
			gripEnt = &g_entities[self->client->ps.forceGripEntityNum];

			if ( !gripEnt || !gripEnt->inuse )
			{//invalid or freed ent
				WP_ForcePowerStop( self, FP_GRIP );
				return;
			}
			else
//rww - RAGDOLL_BEGIN
#ifndef JK2_RAGDOLL_GRIPNOHEALTH
//rww - RAGDOLL_END
			if ( gripEnt->health <= 0 && gripEnt->takedamage && !s_gripHeldObject[self->s.number] )//FIXME: what about things that never had health or lose takedamage when they die?
			{//either invalid ent, or dead ent (one we picked up already dead is fine - 4B.5)
				WP_ForcePowerStop( self, FP_GRIP );
				return;
			}
			else
//rww - RAGDOLL_BEGIN
#endif
//rww - RAGDOLL_END
			if ( self->client->ps.forcePowerLevel[FP_GRIP] == FORCE_LEVEL_1
				&& gripEnt->client
				&& gripEnt->client->ps.groundEntityNum == ENTITYNUM_NONE
				&& gripEnt->client->moveType != MT_FLYSWIM )
			{
				WP_ForcePowerStop( self, FP_GRIP );
				return;
			}
			else if ( gripEnt->client && gripEnt->client->moveType == MT_FLYSWIM && VectorLengthSquared( gripEnt->client->ps.velocity ) > (300*300) )
			{//flying creature broke free
				WP_ForcePowerStop( self, FP_GRIP );
				return;
			}
			else if ( gripEnt->client
				&& gripEnt->health>0	//dead dudes don't fly
				&& (gripEnt->client->NPC_class == CLASS_BOBAFETT || gripEnt->client->NPC_class == CLASS_ROCKETTROOPER)
				&& self->client->ps.forcePowerDebounce[FP_GRIP] < level.time
				&& !Q_irand( 0, 3 )
				)
			{//boba fett - fly away!
				gripEnt->client->ps.forceJumpCharge = 0;//so we don't play the force flip anim
				gripEnt->client->ps.velocity[2] = 250;
				gripEnt->client->ps.forceJumpZStart = gripEnt->currentOrigin[2];//so we don't take damage if we land at same height
				gripEnt->client->ps.pm_flags |= PMF_JUMPING;
				G_AddEvent( gripEnt, EV_JUMP, 0 );
				JET_FlyStart( gripEnt );
				WP_ForcePowerStop( self, FP_GRIP );
				return;
			}
			else if ( gripEnt->NPC
				&& gripEnt->client
				&& gripEnt->health > 0	//JKEnhanced 4B.5: bodies don't break free
				&& !s_gripHeldObject[self->s.number]	//nor do the knocked out
				&& gripEnt->client->ps.forcePowersKnown
				&& (gripEnt->client->NPC_class==CLASS_REBORN||gripEnt->client->ps.weapon==WP_SABER)
				&& !Jedi_CultistDestroyer(gripEnt)
				&& !Q_irand( 0, 100-(gripEnt->NPC->stats.evasion*8)-(g_spskill->integer*20) ) )
			{//a jedi who broke free FIXME: maybe have some minimum grip length- a reaction time?
				WP_ForceForceThrow( gripEnt );
				//FIXME: I need to go into some pushed back anim...
				WP_ForcePowerStop( self, FP_GRIP );
				return;
			}
			else if ( self->s.number < MAX_CLIENTS && (cmd->buttons&(BUTTON_ATTACK|BUTTON_ALT_ATTACK))
				&& ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2
					|| ( s_gripHeldObject[self->s.number] && self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 ) )
				&& (!gripEnt->client || (!gripEnt->message&&!(gripEnt->flags&FL_NO_KNOCKBACK))) )
			{//JKEnhanced 4B.5: either attack while carrying throws it (a body / object from Grip 2, someone alive at 3)
				G_GripThrow( self, gripEnt );
				return;
			}
			else if ( PM_SaberInAttack( self->client->ps.saberMove )
				|| PM_SaberInStart( self->client->ps.saberMove ) )
			{//started an attack
				WP_ForcePowerStop( self, FP_GRIP );
				return;
			}
			else
			{
				int gripLevel = self->client->ps.forcePowerLevel[FP_GRIP];
				if ( gripEnt->client )
				{
					gripLevel = WP_AbsorbConversion( gripEnt, gripEnt->client->ps.forcePowerLevel[FP_ABSORB], self, FP_GRIP, self->client->ps.forcePowerLevel[FP_GRIP], forcePowerNeeded[FP_GRIP] );//JKEnhanced: was indexed by LEVEL
				}
				if ( !gripLevel )
				{
					WP_ForcePowerStop( self, forcePower );
					return;
				}

				if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
				{//holding it
					NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCEGRIP_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					if ( self->client->ps.torsoAnimTimer < 100 ){//we were already playing this anim, we didn't want to restart it, but we want to hold it for at least 100ms, sooo....

						self->client->ps.torsoAnimTimer = 100;
					}
				}
				//get their org
				VectorCopy( self->client->ps.viewangles, angles );
				angles[0] -= 10;
				AngleVectors( angles, dir, NULL, NULL );
				if ( gripEnt->client )
				{//move
					VectorCopy( gripEnt->client->renderInfo.headPoint, gripEntOrg );
				}
				else
				{
					VectorCopy( gripEnt->currentOrigin, gripEntOrg );
				}

				//how far are they
				dist = Distance( self->client->renderInfo.handLPoint, gripEntOrg );
				if ( self->client->ps.forcePowerLevel[FP_GRIP] == FORCE_LEVEL_2 &&
					(!InFront( gripEntOrg, self->client->renderInfo.handLPoint, self->client->ps.viewangles, 0.3f ) ||
						DistanceSquared( gripEntOrg, self->client->renderInfo.handLPoint ) > FORCE_GRIP_DIST_SQUARED))
				{//must face them
					WP_ForcePowerStop( self, FP_GRIP );
					return;
				}

				//check for lift or carry
				if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2
					&& (!gripEnt->client || (!gripEnt->message&&!(gripEnt->flags&FL_NO_KNOCKBACK))) )
				{//carry
					if ( self->s.number < MAX_CLIENTS && s_gripCarryDist[self->s.number] > 0.0f )
					{//JKEnhanced: the mouse wheel set how far out (WP_GripScroll)
						dist = s_gripCarryDist[self->s.number];
					}
					//cap dist
					else if ( dist > FORCE_GRIP_3_MAX_DIST )
					{
						dist = FORCE_GRIP_3_MAX_DIST;
					}
					else if ( dist < FORCE_GRIP_3_MIN_DIST )
					{
						dist = FORCE_GRIP_3_MIN_DIST;
					}
					VectorMA( self->client->renderInfo.handLPoint, dist, dir, gripOrg );
				}
				else if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
				{//just lift
					VectorCopy( self->client->ps.forceGripOrg, gripOrg );
				}
				else
				{
					VectorCopy( gripEnt->currentOrigin, gripOrg );
				}
				if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
				{//if holding him, make sure there's a clear LOS between my hand and him
					trace_t gripTrace;
					vec3_t losOrg;
					VectorCopy( gripEntOrg, losOrg );
					if ( !gripEnt->client )
					{//JKEnhanced: an object's origin can sit in the floor; check its middle
						VectorAdd( gripEnt->absmin, gripEnt->absmax, losOrg );
						VectorScale( losOrg, 0.5f, losOrg );
					}
					gi.trace( &gripTrace, self->client->renderInfo.handLPoint, NULL, NULL, losOrg, gripEnt->client ? ENTITYNUM_NONE : gripEnt->s.number, MASK_FORCE_PUSH, (EG2_Collision)0, 0 );
					if ( gripTrace.startsolid
						|| gripTrace.allsolid
						|| gripTrace.fraction < 1.0f )
					{//no clear trace, drop them
						WP_ForcePowerStop( self, FP_GRIP );
						return;
					}
				}
				//now move them
				if ( gripEnt->client )
				{
					if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
					{//level 1 just holds them
 						VectorSubtract( gripOrg, gripEntOrg, gripEnt->client->ps.velocity );
						if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2
							&& (!gripEnt->client || (!gripEnt->message&&!(gripEnt->flags&FL_NO_KNOCKBACK)) ) )
						{//level 2 just lifts them
							float gripDist = VectorNormalize( gripEnt->client->ps.velocity )/3.0f;
							if ( gripDist < 20.0f )
							{
								if (gripDist<2.0f)
								{
									VectorClear(gripEnt->client->ps.velocity);
								}
								else
								{
									VectorScale( gripEnt->client->ps.velocity, (gripDist*gripDist), gripEnt->client->ps.velocity );
								}
							}
							else
							{
								VectorScale( gripEnt->client->ps.velocity, (gripDist*gripDist), gripEnt->client->ps.velocity );
							}
						}
					}
					//stop them from thinking
					gripEnt->client->ps.pm_time = 2000;
					gripEnt->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
					if ( gripEnt->NPC )
					{
						if ( !(gripEnt->NPC->aiFlags&NPCAI_DIE_ON_IMPACT) )
						{//not falling to their death
							gripEnt->NPC->nextBStateThink = level.time + 2000;
						}
						if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 && !s_gripHeldObject[self->s.number] )
						{//level 1 just holds them (JKEnhanced: a body doesn't turn to face us)
							vectoangles( dir, angles );
							gripEnt->NPC->desiredYaw = AngleNormalize180(angles[YAW]+180);
							gripEnt->NPC->desiredPitch = -angles[PITCH];
							SaveNPCGlobals();
							SetNPCGlobals( gripEnt );
							NPC_UpdateAngles( qtrue, qtrue );
							gripEnt->NPC->last_ucmd.angles[0] = ucmd.angles[0];
							gripEnt->NPC->last_ucmd.angles[1] = ucmd.angles[1];
							gripEnt->NPC->last_ucmd.angles[2] = ucmd.angles[2];
							RestoreNPCGlobals();
							//FIXME: why does he turn back to his original angles once he dies or is let go?
						}
					}
					else if ( !gripEnt->s.number )
					{
						//vectoangles( dir, angles );
						//gripEnt->client->ps.viewangles[0] = -angles[0];
						//gripEnt->client->ps.viewangles[1] = AngleNormalize180(angles[YAW]+180);
						gripEnt->enemy = self;
						NPC_SetLookTarget( gripEnt, self->s.number, level.time+1000 );
					}

					gripEnt->client->ps.eFlags |= EF_FORCE_GRIPPED;
					//dammit!  Make sure that saber stays off!
					WP_DeactivateSaber( gripEnt );
				}
				else
				{//move
					if ( s_gripHeldObject[self->s.number] )
					{//JKEnhanced 4B.5: a body part / item / prop - moved smoothly here
						if ( gripEnt->s.eType == ET_ITEM && self->s.number < MAX_CLIENTS
							&& ( DistanceSquared( gripEnt->currentOrigin, self->currentOrigin ) < 64.0f*64.0f
								|| ( s_gripCarryDist[self->s.number] > 0.0f && s_gripCarryDist[self->s.number] <= GRIP_SCROLL_MIN_DIST ) ) )
						{//pulled an item all the way in: pick it up
							extern void Touch_Item( gentity_t *ent, gentity_t *other, trace_t *trace );
							trace_t pickTr;
							memset( &pickTr, 0, sizeof( pickTr ) );
							pickTr.entityNum = self->s.number;
							Touch_Item( gripEnt, self, &pickTr );
							if ( !gripEnt->inuse || (gripEnt->s.eFlags&EF_NODRAW) || (gripEnt->svFlags&SVF_NOCLIENT) )
							{//took it (gone, or hidden to respawn): nothing to drop
								s_gripPickupTime[self->s.number] = level.time;//(WP_GripScroll eats the wheel for a moment)
								self->client->ps.forceGripEntityNum = ENTITYNUM_NONE;
								WP_ForcePowerStop( self, FP_GRIP );
								return;
							}
						}
						WP_GripMoveObject( self, gripEnt, gripOrg, (qboolean)( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2 ) );
					}
					else if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
					{//level 1 just holds them
						VectorCopy( gripEnt->currentOrigin, gripEnt->s.pos.trBase );
						VectorSubtract( gripOrg, gripEntOrg, gripEnt->s.pos.trDelta );
						if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2
							&& (!gripEnt->client || (!gripEnt->message&&!(gripEnt->flags&FL_NO_KNOCKBACK))) )
						{//level 2 just lifts them
							VectorScale( gripEnt->s.pos.trDelta, 10, gripEnt->s.pos.trDelta );
						}
						gripEnt->s.pos.trType = TR_LINEAR;
						gripEnt->s.pos.trTime = level.time;
					}

					gripEnt->s.eFlags |= EF_FORCE_GRIPPED;
				}

				//Shouldn't this be discovered?
				//AddSightEvent( self, gripOrg, 128, AEL_DANGER, 20 );
				AddSightEvent( self, gripOrg, 128, AEL_DISCOVERED, 20 );

				if ( s_gripHeldObject[self->s.number] )
				{//JKEnhanced 4B.5: a body / object - no choke damage, a slow trickle to hold it
					if ( self->client->ps.forcePowerDebounce[FP_GRIP] < level.time )
					{
						WP_ForcePowerDrain( self, FP_GRIP, 1 );
						self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + 400;
					}
				}
				else if ( self->client->ps.forcePowerDebounce[FP_GRIP] < level.time )
				{
					//GEntity_PainFunc( gripEnt, self, self, gripOrg, 0, MOD_CRUSH );
					if ( !gripEnt->client
						|| gripEnt->client->NPC_class != CLASS_VEHICLE
						|| (gripEnt->m_pVehicle
							&& gripEnt->m_pVehicle->m_pVehicleInfo
							&& gripEnt->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL) )
					{//we don't damage the empty vehicle
						gripEnt->painDebounceTime = 0;
						int gripDmg = forceGripDamage[self->client->ps.forcePowerLevel[FP_GRIP]];
						if ( gripLevel != -1 )
						{
							if ( gripLevel == 1 )
							{
								gripDmg = floor((float)gripDmg/3.0f);
							}
							else //if ( gripLevel == 2 )
							{
								gripDmg = floor((float)gripDmg/1.5f);
							}
						}
						G_Damage( gripEnt, self, self, dir, gripOrg, gripDmg, DAMAGE_NO_ARMOR, MOD_CRUSH );//MOD_???
					}
					if ( gripEnt->s.number )
					{
						if ( self->client->ps.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_2 )
						{//do damage faster at level 3
							self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + Q_irand( 150, 750 );
						}
						else
						{
							self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + Q_irand( 250, 1000 );
						}
					}
					else
					{//player takes damage faster
						self->client->ps.forcePowerDebounce[FP_GRIP] = level.time + Q_irand( 100, 600 );
					}
					if ( forceGripDamage[self->client->ps.forcePowerLevel[FP_GRIP]] > 0 )
					{//no damage at level 1
						WP_ForcePowerDrain( self, FP_GRIP, 3 );
					}
					if ( self->client->NPC_class == CLASS_KYLE
						&& (self->spawnflags&1) )
					{//"Boss" Kyle
						if ( gripEnt->client )
						{
							if ( !Q_irand( 0, 2 ) )
							{//toss him aside!
								vec3_t vRt;
								AngleVectors( self->currentAngles, NULL, vRt, NULL );
								//stop gripping
								TIMER_Set( self, "gripping", -level.time );
								WP_ForcePowerStop( self, FP_GRIP );
								//now toss him
								if ( Q_irand( 0, 1 ) )
								{//throw him to my left
									NPC_SetAnim( self, SETANIM_BOTH, BOTH_TOSS1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
									VectorScale( vRt, -1500.0f, gripEnt->client->ps.velocity );
									G_Knockdown( gripEnt, self, vRt, 500, qfalse );
								}
								else
								{//throw him to my right
									NPC_SetAnim( self, SETANIM_BOTH, BOTH_TOSS2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
									VectorScale( vRt, 1500.0f, gripEnt->client->ps.velocity );
									G_Knockdown( gripEnt, self, vRt, 500, qfalse );
								}
								//don't do anything for a couple seconds
								self->client->ps.weaponTime = self->client->ps.torsoAnimTimer + 2000;
								self->painDebounceTime = level.time + self->client->ps.weaponTime;
								//stop moving
								VectorClear( self->client->ps.velocity );
								VectorClear( self->client->ps.moveDir );
								return;
							}
						}
					}
				}
				else
				{
					//WP_ForcePowerDrain( self, FP_GRIP, 0 );
					if ( !gripEnt->enemy )
					{
						if ( gripEnt->client
							&& gripEnt->client->playerTeam == TEAM_PLAYER
							&& self->s.number < MAX_CLIENTS
							&& self->client
							&& self->client->playerTeam == TEAM_PLAYER )
						{//this shouldn't make allies instantly turn on you, let the damage->pain routine determine how allies should react to this
						}
						else
						{
							G_SetEnemy( gripEnt, self );
						}
					}
				}
				if ( gripEnt->client && gripEnt->health > 0 && !s_gripHeldObject[self->s.number] )
				{//JKEnhanced: the knocked out stay limp
					int anim = BOTH_CHOKE3; //left-handed choke
					if ( gripEnt->client->ps.weapon == WP_NONE || gripEnt->client->ps.weapon == WP_MELEE )
					{
						anim = BOTH_CHOKE1; //two-handed choke
					}
					if ( self->client->ps.forcePowerLevel[FP_GRIP] < FORCE_LEVEL_2 )
					{//still on ground, only set anim on torso
						NPC_SetAnim( gripEnt, SETANIM_TORSO, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					}
					else
					{//in air, set on whole body
						NPC_SetAnim( gripEnt, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					}
					gripEnt->painDebounceTime = level.time + 2000;
				}
			}
		}
		break;
	case FP_LIGHTNING:
		if ( self->client->ps.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_1 )
		{//higher than level 1
			if ( cmd->buttons & BUTTON_FORCE_LIGHTNING )
			{//holding it keeps it going
				self->client->ps.forcePowerDuration[FP_LIGHTNING] = level.time + 500;
				ForceLightningAnim( self );
			}
		}
		if ( !WP_ForcePowerAvailable( self, forcePower, 0 ) )
		{
			WP_ForcePowerStop( self, forcePower );
		}
		else
		{
			//JKEnhanced (3G/4A bug fix): this ran on every player command, so the player's Lightning hit
			//(and cost Force) once per rendered frame - several ticks per 50 ms server frame at high frame
			//rates (sandbox log2: 6-8 hits per timestamp). One tick per 50 ms now, the rate NPCs (server
			//frames) always had. Not saved: a stale value after a load only delays one tick.
			static int s_lightningNextTick[MAX_GENTITIES];
			if ( level.time < s_lightningNextTick[self->s.number] && s_lightningNextTick[self->s.number] - level.time <= 50 )
			{
				break;
			}
			s_lightningNextTick[self->s.number] = level.time + 50;
			ForceShootLightning( self );
			if ( self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
				|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
				|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
				|| self->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE )
			{//jackin' 'em up, Palpatine-style
				//extra cost
				WP_ForcePowerDrain( self, forcePower, 0 );
			}
			WP_ForcePowerDrain( self, forcePower, 0 );
		}
		break;
	//new Jedi Academy force powers
	case FP_RAGE:
		if (self->health < 1)
		{
			WP_ForcePowerStop(self, forcePower);
			break;
		}
		if (self->client->ps.forceRageDrainTime < level.time)
		{
			int addTime = 400;

			if ( self->NPC )
			{//JKEnhanced: an NPC's Rage costs health only down to a quarter of its max, and never past a boss
			 //stage line (it subtracts health directly, so it used to skip stages: Tavion raged herself to 1
			 //and died to the first Lightning tick). The player's Rage is unchanged (down to 1).
				extern int G_BossStageFloor( const gentity_t *ent );
				const int minHealth = Q_max( G_BossStageFloor( self ) + 1, self->max_health / 4 );
				if ( self->health - 2 >= minHealth )
				{
					self->health -= 2;
				}
			}
			else
			{
				self->health -= 2;
			}

			if (self->client->ps.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_1)
			{
				addTime = 100;
			}
			else if (self->client->ps.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_2)
			{
				addTime = 250;
			}
			else if (self->client->ps.forcePowerLevel[FP_RAGE] == FORCE_LEVEL_3)
			{
				addTime = 500;
			}
			self->client->ps.forceRageDrainTime = level.time + addTime;
		}

		if ( self->health < 1 )
		{
			self->health = 1;
			//WP_ForcePowerStop( self, forcePower );
		}
		else
		{
			self->client->ps.stats[STAT_HEALTH] = self->health;

			speed = forceSpeedValue[self->client->ps.forcePowerLevel[FP_RAGE]-1];
			if ( !self->s.number )
			{//player using force rage
				if ( !(self->client->ps.forcePowersActive&(1<<FP_SPEED))
					|| self->client->ps.forcePowerLevel[FP_RAGE] > self->client->ps.forcePowerLevel[FP_SPEED]+1 )
				{//either not using speed or speed is at a lower level than rage
					gi.cvar_set("timescale", va("%4.2f", speed));
					if ( g_timescale->value > speed )
					{
						newSpeed = g_timescale->value - 0.05;
						if ( newSpeed < speed )
						{
							newSpeed = speed;
						}
						gi.cvar_set("timescale", va("%4.2f", newSpeed));
					}
				}
			}
		}
		break;
	case FP_DRAIN:
		if ( cmd->buttons & BUTTON_FORCE_DRAIN )
		{//holding it keeps it going
			self->client->ps.forcePowerDuration[FP_DRAIN] = level.time + 500;
		}
		if ( !WP_ForcePowerAvailable( self, forcePower, 0 ) )
		{//no more force power, stop
			WP_ForcePowerStop( self, forcePower );
		}
		else if ( self->client->ps.forceDrainEntityNum >= 0 && self->client->ps.forceDrainEntityNum < ENTITYNUM_WORLD )
		{//holding someone
			if ( !WP_ForcePowerAvailable( self, FP_DRAIN, 0 )
				|| (self->client->ps.forcePowerLevel[FP_DRAIN]>FORCE_LEVEL_1
					&& !self->s.number
					&& !(cmd->buttons&BUTTON_FORCE_DRAIN)
					&& self->client->ps.forcePowerDuration[FP_DRAIN]<level.time) )
			{
				WP_ForcePowerStop( self, FP_DRAIN );
				return;
			}
			else
			{
				drainEnt = &g_entities[self->client->ps.forceDrainEntityNum];

				if ( !drainEnt )
				{//invalid ent
					WP_ForcePowerStop( self, FP_DRAIN );
					return;
				}
				else if ( (drainEnt->health <= 0&&drainEnt->takedamage) )//FIXME: what about things that never had health or lose takedamage when they die?
				{//dead ent
					WP_ForcePowerStop( self, FP_DRAIN );
					return;
				}
				else if ( drainEnt->client && drainEnt->client->moveType == MT_FLYSWIM && VectorLengthSquared( NPC->client->ps.velocity ) > (300*300) )
				{//flying creature broke free
					WP_ForcePowerStop( self, FP_DRAIN );
					return;
				}
				else if ( drainEnt->client
					&& drainEnt->health>0	//dead dudes don't fly
					&& (drainEnt->client->NPC_class == CLASS_BOBAFETT || drainEnt->client->NPC_class == CLASS_ROCKETTROOPER)
					&& self->client->ps.forcePowerDebounce[FP_DRAIN] < level.time
					&& !Q_irand( 0, 10 ) )
				{//boba fett - fly away!
					drainEnt->client->ps.forceJumpCharge = 0;//so we don't play the force flip anim
					drainEnt->client->ps.velocity[2] = 250;
					drainEnt->client->ps.forceJumpZStart = drainEnt->currentOrigin[2];//so we don't take damage if we land at same height
					drainEnt->client->ps.pm_flags |= PMF_JUMPING;
					G_AddEvent( drainEnt, EV_JUMP, 0 );
					JET_FlyStart( drainEnt );
					WP_ForcePowerStop( self, FP_DRAIN );
					return;
				}
				else if ( drainEnt->NPC
					&& drainEnt->client
					&& drainEnt->client->ps.forcePowersKnown
					&& (drainEnt->client->NPC_class==CLASS_REBORN||drainEnt->client->ps.weapon==WP_SABER)
					&& !Jedi_CultistDestroyer(drainEnt)
					&& level.time-(self->client->ps.forcePowerDebounce[FP_DRAIN]>self->client->ps.forcePowerLevel[FP_DRAIN]*500)//at level 1, I always get at least 500ms of drain, at level 3 I get 1500ms
					&& !Q_irand( 0, 100-(drainEnt->NPC->stats.evasion*8)-(g_spskill->integer*15) ) )
				{//a jedi who broke free FIXME: maybe have some minimum grip length- a reaction time?
					WP_ForceForceThrow( drainEnt );
					//FIXME: I need to go into some pushed back anim...
					WP_ForcePowerStop( self, FP_DRAIN );
					//can't drain again for 2 seconds
					self->client->ps.forcePowerDebounce[FP_DRAIN] = level.time + 4000;
					return;
				}
				else
				{
					/*
					int drainLevel = WP_AbsorbConversion( drainEnt, drainEnt->client->ps.forcePowerLevel[FP_ABSORB], self, FP_DRAIN, self->client->ps.forcePowerLevel[FP_DRAIN], forcePowerNeeded[FP_DRAIN] );//JKEnhanced: was indexed by LEVEL
					if ( !drainLevel )
					{
						WP_ForcePowerStop( self, forcePower );
						return;
					}
					*/

					//NPC_SetAnim( self, SETANIM_BOTH, BOTH_HUGGER1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					if ( self->client->ps.torsoAnim != BOTH_FORCE_DRAIN_GRAB_START
						|| !self->client->ps.torsoAnimTimer )
					{
						NPC_SetAnim( self, SETANIM_BOTH, BOTH_FORCE_DRAIN_GRAB_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					}
					if ( self->handLBolt != -1 )
					{
						G_PlayEffect( G_EffectIndex( "force/drain_hand" ), self->playerModel, self->handLBolt, self->s.number, self->currentOrigin, 200, qtrue );
					}
					if ( self->handRBolt != -1 )
					{
						G_PlayEffect( G_EffectIndex( "force/drain_hand" ), self->playerModel, self->handRBolt, self->s.number, self->currentOrigin, 200, qtrue );
					}

					//how far are they
					dist = Distance( self->client->renderInfo.eyePoint, drainEnt->currentOrigin );
					if ( DistanceSquared( drainEnt->currentOrigin, self->currentOrigin ) > FORCE_DRAIN_DIST_SQUARED )
					{//must be close, got away somehow!
						WP_ForcePowerStop( self, FP_DRAIN );
						return;
					}

					//keep my saber off!
					WP_DeactivateSaber( self, qtrue );
					if ( drainEnt->client )
					{
						//now move them
						VectorCopy( self->client->ps.viewangles, angles );
						angles[0] = 0;
						AngleVectors( angles, dir, NULL, NULL );
						/*
						VectorMA( self->currentOrigin, self->maxs[0], dir, drainEnt->client->ps.forceDrainOrg );
						trace_t	trace;
						gi.trace( &trace, drainEnt->currentOrigin, drainEnt->mins, drainEnt->maxs, drainEnt->client->ps.forceDrainOrg, drainEnt->s.number, drainEnt->clipmask );
						if ( !trace.startsolid && !trace.allsolid )
						{
							G_SetOrigin( drainEnt, trace.endpos );
							gi.linkentity( drainEnt );
							VectorClear( drainEnt->client->ps.velocity );
						}
						VectorMA( self->currentOrigin, self->maxs[0]*0.5f, dir, drainEnt->client->ps.forceDrainOrg );
						*/
						//stop them from thinking
						drainEnt->client->ps.pm_time = 2000;
						drainEnt->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
						if ( drainEnt->NPC )
						{
							if ( !(drainEnt->NPC->aiFlags&NPCAI_DIE_ON_IMPACT) )
							{//not falling to their death
								drainEnt->NPC->nextBStateThink = level.time + 2000;
							}
							vectoangles( dir, angles );
							drainEnt->NPC->desiredYaw = AngleNormalize180(angles[YAW]+180);
							drainEnt->NPC->desiredPitch = -angles[PITCH];
							SaveNPCGlobals();
							SetNPCGlobals( drainEnt );
							NPC_UpdateAngles( qtrue, qtrue );
							drainEnt->NPC->last_ucmd.angles[0] = ucmd.angles[0];
							drainEnt->NPC->last_ucmd.angles[1] = ucmd.angles[1];
							drainEnt->NPC->last_ucmd.angles[2] = ucmd.angles[2];
							RestoreNPCGlobals();
							//FIXME: why does he turn back to his original angles once he dies or is let go?
						}
						else if ( !drainEnt->s.number )
						{
							drainEnt->enemy = self;
							NPC_SetLookTarget( drainEnt, self->s.number, level.time+1000 );
						}

						drainEnt->client->ps.eFlags |= EF_FORCE_DRAINED;
						//dammit!  Make sure that saber stays off!
						WP_DeactivateSaber( drainEnt, qtrue );
					}
					//Shouldn't this be discovered?
					AddSightEvent( self, drainEnt->currentOrigin, 128, AEL_DISCOVERED, 20 );

					if ( self->client->ps.forcePowerDebounce[FP_DRAIN] < level.time )
					{
						int drainLevel = WP_AbsorbConversion( drainEnt, drainEnt->client->ps.forcePowerLevel[FP_ABSORB], self, FP_DRAIN, self->client->ps.forcePowerLevel[FP_DRAIN], forcePowerNeeded[FP_DRAIN] );//JKEnhanced: was indexed by LEVEL
						if ( (drainLevel && drainLevel == -1)
							|| Q_irand( drainLevel, 3 ) < 3 )
						{//the drain is being absorbed
							ForceDrainEnt( self, drainEnt );
						}
						WP_ForcePowerDrain( self, FP_DRAIN, 3 );
					}
					else
					{
						if ( !Q_irand( 0, 4 ) )
						{
							WP_ForcePowerDrain( self, FP_DRAIN, 1 );
						}
						if ( !drainEnt->enemy )
						{
							G_SetEnemy( drainEnt, self );
						}
					}
					if ( drainEnt->health > 0 )
					{//still alive
						//NPC_SetAnim( drainEnt, SETANIM_BOTH, BOTH_HUGGEE1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						NPC_SetAnim( drainEnt, SETANIM_BOTH, BOTH_FORCE_DRAIN_GRABBED, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					}
				}
			}
		}
		else if ( self->client->ps.forcePowerLevel[forcePower] > FORCE_LEVEL_1 )
		{//regular distance-drain
			if ( cmd->buttons & BUTTON_FORCE_DRAIN )
			{//holding it keeps it going
				self->client->ps.forcePowerDuration[FP_DRAIN] = level.time + 500;
				if ( self->client->ps.torsoAnim == BOTH_FORCE_DRAIN_START )
				{
					if ( !self->client->ps.torsoAnimTimer )
					{
						NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCE_DRAIN_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					}
					else
					{
						NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCE_DRAIN_START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					}
				}
				else
				{
					NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCE_DRAIN_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				}
			}
			if ( !WP_ForcePowerAvailable( self, forcePower, 0 ) )
			{
				WP_ForcePowerStop( self, forcePower );
			}
			else
			{
				ForceShootDrain( self );
			}
		}
		break;
	case FP_PROTECT:
		break;
	case FP_ABSORB:
		break;
	case FP_SEE:
		break;
	case FP_INSANITY:
		break;
	case FP_STASIS:
		break;
	case FP_REPULSE://JKEnhanced 4B (restored from 45210704): charging - let go to throw
		if ( PM_InKnockDown( &self->client->ps ) )
		{//knocked off our feet mid-charge: it's lost (what was spent so far stays spent)
			self->client->ps.repulseChargeStart = 0;
			self->s.loopSound = 0;
			WP_ForcePowerStop( self, FP_REPULSE );
			return;
		}
		if ( !self->client->ps.repulseChargeStart )
		{
			WP_ForcePowerStop( self, FP_REPULSE );
			return;
		}
		{
			const int chargeTime = WP_RepulseChargeTime( self );
			const qboolean full = (qboolean)( chargeTime >= WP_RepulseFullCharge( self ) );
			//the player pays as they charge; out of Force = it goes off now with what it has
			const qboolean haveForce = ( self->s.number < MAX_CLIENTS ) ? WP_RepulseChargeDrain( self, chargeTime )
				: WP_ForcePowerAvailable( self, FP_REPULSE, WP_RepulseCost( self ) );
			if ( ( !self->s.number && !(cmd->buttons&BUTTON_REPULSE) )
				|| WP_RepulseHeld( self )//gripped / drained / locked mid-charge: the throw won't go (4B.4)
				|| full || !haveForce )
			{//let go, fully charged (holding it isn't a way to sit out the fight) or out of Force: it goes off now
				if ( g_debugForce && g_debugForce->integer && self->s.number < MAX_CLIENTS )
				{
					gi.Printf( "  REPULSE fires after %d ms (%s), paid %d, Force %d\n", level.time - self->client->ps.repulseChargeStart,
						WP_RepulseHeld( self ) ? "held - cancelled" : full ? "full charge" : !haveForce ? "out of Force" : "let go",
						s_repulseSpent[self->s.number], self->client->ps.forcePower );
				}
				WP_ForcePowerStop( self, FP_REPULSE );
				return;
			}
		}
		NPC_SetAnim( self, SETANIM_BOTH, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		if ( self->client->ps.torsoAnimTimer < 100 )
		{//hold the pose at least another 100 ms
			self->client->ps.torsoAnimTimer = 100;
		}
		if ( self->client->ps.legsAnimTimer < 100 )
		{
			self->client->ps.legsAnimTimer = 100;
		}
		{//shots coming at us are caught and held, and go back out when it fires (fp_repulse.cpp)
			extern void G_RepulseCatchShots( gentity_t *self );
			G_RepulseCatchShots( self );
		}
		VectorClear( self->client->ps.velocity );
		cmd->forwardmove = 0;
		cmd->rightmove = 0;
		cmd->upmove = 0;
		VectorClear( self->client->ps.moveDir );
		break;
	default:
		break;
	}
}

void WP_CheckForcedPowers( gentity_t *self, usercmd_t *ucmd )
{
	for ( int forcePower = FP_FIRST; forcePower < NUM_FORCE_POWERS; forcePower++ )
	{
		if ( (self->client->ps.forcePowersForced&(1<<forcePower)) )
		{
			switch ( forcePower )
			{
			case FP_HEAL:
				ForceHeal( self );
				//do only once
				self->client->ps.forcePowersForced &= ~(1<<forcePower);
				break;
			case FP_LEVITATION:
				//nothing
				break;
			case FP_SPEED:
				ForceSpeed( self );
				//do only once
				self->client->ps.forcePowersForced &= ~(1<<forcePower);
				break;
			case FP_PUSH:
				ForceThrow( self, qfalse );
				//do only once
				self->client->ps.forcePowersForced &= ~(1<<forcePower);
				break;
			case FP_PULL:
				ForceThrow( self, qtrue );
				//do only once
				self->client->ps.forcePowersForced &= ~(1<<forcePower);
				break;
			case FP_TELEPATHY:
				//FIXME: target at enemy?
				ForceTelepathy( self );
				//do only once
				self->client->ps.forcePowersForced &= ~(1<<forcePower);
				break;
			case FP_GRIP:
				ucmd->buttons &= ~(BUTTON_ATTACK|BUTTON_ALT_ATTACK|BUTTON_FORCE_FOCUS|BUTTON_FORCE_DRAIN|BUTTON_FORCE_LIGHTNING|BUTTON_SABERTHROW);
				ucmd->buttons |= BUTTON_FORCEGRIP;
				//holds until cleared
				break;
			case FP_LIGHTNING:
				ucmd->buttons &= ~(BUTTON_ATTACK|BUTTON_ALT_ATTACK|BUTTON_FORCE_FOCUS|BUTTON_FORCEGRIP|BUTTON_FORCE_DRAIN|BUTTON_SABERTHROW);
				ucmd->buttons |= BUTTON_FORCE_LIGHTNING;
				//holds until cleared
				break;
			case FP_SABERTHROW:
				ucmd->buttons &= ~(BUTTON_ATTACK|BUTTON_ALT_ATTACK|BUTTON_FORCE_FOCUS|BUTTON_FORCEGRIP|BUTTON_FORCE_DRAIN|BUTTON_FORCE_LIGHTNING);
				ucmd->buttons |= BUTTON_SABERTHROW;
				//holds until cleared?
				break;
			case FP_SABER_DEFENSE:
				//nothing
				break;
			case FP_SABER_OFFENSE:
				//nothing
				break;
			case FP_RAGE:
				ForceRage( self );
				//do only once
				self->client->ps.forcePowersForced &= ~(1<<forcePower);
				break;
			case FP_PROTECT:
				ForceProtect( self );
				//do only once
				self->client->ps.forcePowersForced &= ~(1<<forcePower);
				break;
			case FP_ABSORB:
				ForceAbsorb( self );
				//do only once
				self->client->ps.forcePowersForced &= ~(1<<forcePower);
				break;
			case FP_DRAIN:
				ucmd->buttons &= ~(BUTTON_ATTACK|BUTTON_ALT_ATTACK|BUTTON_FORCE_FOCUS|BUTTON_FORCEGRIP|BUTTON_FORCE_LIGHTNING|BUTTON_SABERTHROW);
				ucmd->buttons |= BUTTON_FORCE_DRAIN;
				//holds until cleared
				break;
			case FP_SEE:
				//nothing
				break;
			case FP_INSANITY:
				ForceInsanity( self );
				//do only once
				self->client->ps.forcePowersForced &= ~(1<<forcePower);
				break;
			case FP_STASIS:
				ForceStasis( self );
				//do only once
				self->client->ps.forcePowersForced &= ~(1<<forcePower);
				break;
			case FP_REPULSE://JKEnhanced 4B: scripted - charge while forced
				ucmd->buttons &= ~(BUTTON_ATTACK|BUTTON_ALT_ATTACK|BUTTON_FORCE_FOCUS|BUTTON_FORCEGRIP|BUTTON_FORCE_LIGHTNING|BUTTON_SABERTHROW|BUTTON_FORCE_DRAIN);
				ucmd->buttons |= BUTTON_REPULSE;
				break;
			}
		}
	}
}

void WP_ForcePowersUpdate( gentity_t *self, usercmd_t *ucmd )
{
	qboolean	usingForce = qfalse;
	int			i;
	//see if any force powers are running
	if ( !self )
	{
		return;
	}
	if ( !self->client )
	{
		return;
	}

	if ( self->health <= 0 )
	{//if dead, deactivate any active force powers
		for ( i = 0; i < NUM_FORCE_POWERS; i++ )
		{
			if ( self->client->ps.forcePowerDuration[i] || (self->client->ps.forcePowersActive&( 1 << i )) )
			{
				WP_ForcePowerStop( self, (forcePowers_t)i );
				self->client->ps.forcePowerDuration[i] = 0;
			}
		}
		return;
	}

	WP_CheckForcedPowers( self, ucmd );

	{//JKEnhanced: a two-handed Force pose (Repulse / two-handed Lightning) doesn't outlast its power. These
	 //anims loop, and with the saber lit nothing replaced the pose until the next swing (hands out, blade out)
		playerState_t *ps = &self->client->ps;
		const int torso = ps->torsoAnim;
		if ( ( torso == BOTH_FORCE_2HANDEDLIGHTNING || torso == BOTH_FORCE_2HANDEDLIGHTNING_START
				|| torso == BOTH_FORCE_2HANDEDLIGHTNING_HOLD || torso == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE
				|| ( torso == BOTH_SWIM_IDLE1 && self->waterlevel < 2 && (ps->forcePowersKnown&(1<<FP_REPULSE)) ) )//(Repulse's charge pose)
			&& ps->torsoAnimTimer <= 0 && ps->weaponTime <= 0
			&& !(ps->forcePowersActive&((1<<FP_LIGHTNING)|(1<<FP_REPULSE))) && !ps->repulseChargeStart )
		{
			NPC_SetAnim( self, SETANIM_TORSO, ps->legsAnim, SETANIM_FLAG_OVERRIDE );
			if ( ps->weapon == WP_SABER )
			{
				ps->saberMove = ps->saberBounceMove = LS_READY;
				ps->saberBlocked = BLOCKED_NONE;
			}
		}
	}

	if ( !self->s.number )
	{//player uses different kind of force-jump
	}
	else
	{
		/*
		if ( ucmd->buttons & BUTTON_FORCEJUMP )
		{//just charging up
			ForceJumpCharge( self, ucmd );
		}
		else */
		if ( self->client->ps.forceJumpCharge )
		{//let go of charge button, have charge
			//if leave the ground by some other means, cancel the force jump so we don't suddenly jump when we land.
			if ( self->client->ps.groundEntityNum == ENTITYNUM_NONE
				&& !PM_SwimmingAnim( self->client->ps.legsAnim ) )
			{//FIXME: stop sound?
				//self->client->ps.forceJumpCharge = 0;
				//FIXME: actually, we want this to still be cleared... don't clear it if the button isn't being pressed, but clear it if not holding button and not on ground.
			}
			else
			{//still on ground, so jump
				ForceJump( self, ucmd );
				return;
			}
		}
	}
	
	if ( !self->s.number
		&& self->client->NPC_class == CLASS_BOBAFETT )
	{//Boba Fett
		if ( ucmd->buttons & BUTTON_FORCEGRIP )
		{//start wrist laser
			Boba_FireWristMissile( self, BOBA_MISSILE_LASER );
			return;
		}
		else if ( self->client->ps.forcePowerDuration[FP_GRIP] )
		{
			Boba_EndWristMissile( self, BOBA_MISSILE_LASER );
			return;
		}
	}
	else if ( ucmd->buttons & BUTTON_FORCEGRIP )
	{
		ForceGrip( self );
	}

	if ( !self->s.number
		&& self->client->NPC_class == CLASS_BOBAFETT )
	{//Boba Fett
		if ( ucmd->buttons & BUTTON_FORCE_LIGHTNING )
		{//start flamethrower
			Boba_DoFlameThrower( self );
			return;
		}
		else if ( self->client->ps.forcePowerDuration[FP_LIGHTNING] )
		{
			self->client->ps.forcePowerDuration[FP_LIGHTNING] = 0;
			Boba_StopFlameThrower( self );
			return;
		}
	}
	else if ( ucmd->buttons & BUTTON_FORCE_LIGHTNING )
	{
		ForceLightning( self );
	}

	if ( !self->s.number
		&& self->client->NPC_class == CLASS_BOBAFETT )
	{//Boba Fett
		if ( ucmd->buttons & BUTTON_FORCE_DRAIN )
		{//start wrist rocket
			Boba_FireWristMissile( self, BOBA_MISSILE_VIBROBLADE );
			//Boba_VibrobladePunch( self );
			return;
		}
		else if ( self->client->ps.forcePowerDuration[FP_DRAIN] )
		{
			Boba_EndWristMissile( self, BOBA_MISSILE_VIBROBLADE );
			//self->client->ps.forcePowerDuration[FP_DRAIN] = 0;
			//self->client->ps.torsoAnimTimer  =	0;
			
			return;
		}
	}
	else if ( ucmd->buttons & BUTTON_FORCE_DRAIN )
	{
		if ( !ForceDrain2( self ) )
		{//can't drain-grip someone right in front
			if ( self->client->ps.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_1 )
			{//try ranged
				ForceDrain( self, qtrue );
			}
		}
	}

	if ( ucmd->buttons & BUTTON_REPULSE )
	{//JKEnhanced 4B: start charging (+force_repulse, or F with Repulse selected)
		ForceRepulse( self );
	}
	WP_GripThrownUpdate( self );	//JKEnhanced 4B.5: an object we threw hits whoever's in its path

	for ( i = 0; i < NUM_FORCE_POWERS; i++ )
	{
		if ( self->client->ps.forcePowerDuration[i] )
		{
			if ( self->client->ps.forcePowerDuration[i] < level.time )
			{
				if ( (self->client->ps.forcePowersActive&( 1 << i )) )
				{//turn it off
					WP_ForcePowerStop( self, (forcePowers_t)i );
				}
				self->client->ps.forcePowerDuration[i] = 0;
			}
		}
		if ( (self->client->ps.forcePowersActive&( 1 << i )) )
		{
			if ( i != FP_PROTECT )
			{//JKEnhanced 4B.3: Protect no longer stops regen (L1-2 paid up front, L3 pays per block); Absorb still does
				usingForce = qtrue;
			}
			WP_ForcePowerRun( self, (forcePowers_t)i, ucmd );
		}
	}
	if ( self->client->ps.saberInFlight )
	{//don't regen force power while throwing saber
		if ( self->client->ps.saberEntityNum < ENTITYNUM_NONE && self->client->ps.saberEntityNum > 0 )//player is 0
		{//
			if ( &g_entities[self->client->ps.saberEntityNum] != NULL && g_entities[self->client->ps.saberEntityNum].s.pos.trType == TR_LINEAR )
			{//fell to the ground and we're trying to pull it back
				usingForce = qtrue;
			}
		}
	}
	if ( PM_ForceUsingSaberAnim( self->client->ps.torsoAnim ) )
	{
		usingForce = qtrue;
	}
	if ( !usingForce )
	{//when not using the force, regenerate at 10 points per second
		if ( self->client->ps.forcePowerRegenDebounceTime < level.time )
		{
			int regenTime;
			int defLevel = self->client->ps.forcePowerLevel[FP_SABER_DEFENSE];
			int regenMult = ( defLevel >= FORCE_LEVEL_3 ) ? 2 : 1;
			if (!self->s.number && self->client->NPC_class == CLASS_PLAYER)
			{
				regenTime = Q_max(g_forceRegenTime->integer, 1);
			}
			else
			{
				regenTime = Q_max( self->client->ps.forcePowerRegenRate, 1 );
			}
			if ( self->client->ps.forceRageRecoveryTime >= level.time )
			{//regen half as fast
				regenTime *= 2;
			}
			//JKEnhanced 3G: count every tick that came due since the last frame. The debounce used to
			//restart from "now", dropping the overshoot, so at 30 fps (33 ms frames) a 50 ms tick came
			//every ~66 ms - Force regen depended on frame rate. Only catches up a few ticks (not
			//after a long pause, e.g. a power-use debounce or a full bar).
			const int late = level.time - self->client->ps.forcePowerRegenDebounceTime;
			const qboolean onSchedule = (qboolean)( late < regenTime * 4 );
			const int ticks = onSchedule ? 1 + late / regenTime : 1;
			const int perTick = ( self->client->ps.forcePowerRegenAmount ? self->client->ps.forcePowerRegenAmount : 1 ) * regenMult;//(0 meant +1; Saber Defense 3 doubles it - the player is always 0)
			WP_ForcePowerRegenerate( self, perTick * ticks );
			self->client->ps.forcePowerRegenDebounceTime = onSchedule
				? self->client->ps.forcePowerRegenDebounceTime + ticks * regenTime
				: level.time + regenTime;
		}
	}
}

void WP_InitForcePowers( gentity_t *ent )
{
	if ( !ent || !ent->client )
	{
		return;
	}

	if ( !ent->client->ps.forcePowerMax )
	{
		ent->client->ps.forcePowerMax = FORCE_POWER_MAX;
	}
	if ( !ent->client->ps.forcePowerRegenRate )
	{
		ent->client->ps.forcePowerRegenRate = 100;
	}
	ent->client->ps.forcePower = ent->client->ps.forcePowerMax;
	ent->client->ps.forcePowerRegenDebounceTime = level.time;

	ent->client->ps.forceGripEntityNum = ent->client->ps.forceDrainEntityNum = ent->client->ps.pullAttackEntNum = ENTITYNUM_NONE;
	ent->client->ps.forceRageRecoveryTime = 0;
	ent->client->ps.forceDrainTime = 0;
	ent->client->ps.pullAttackTime = 0;

	if ( ent->s.number < MAX_CLIENTS )
	{//player
		if ( !g_cheats->integer )//devmaps give you all the FP
		{
			ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_1;
		}
		else
		{
			ent->client->ps.forcePowersKnown = ( 1 << FP_HEAL )|( 1 << FP_LEVITATION )|( 1 << FP_SPEED )|( 1 << FP_PUSH )|( 1 << FP_PULL )|( 1 << FP_TELEPATHY )|( 1 << FP_GRIP )|( 1 << FP_LIGHTNING)|( 1 << FP_SABERTHROW)|( 1 << FP_SABER_DEFENSE )|( 1 << FP_SABER_OFFENSE )|( 1<< FP_RAGE )|( 1<< FP_DRAIN )|( 1<< FP_PROTECT )|( 1<< FP_ABSORB )|( 1<< FP_SEE )
				|( 1 << FP_INSANITY )|( 1 << FP_STASIS )|( 1 << FP_REPULSE );//JKEnhanced 4B: Repulse known, level 0 until the story raises it
			ent->client->ps.forcePowerLevel[FP_HEAL] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_LEVITATION] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_PUSH] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_PULL] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_SABERTHROW] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_SPEED] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_LIGHTNING] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_TELEPATHY] = FORCE_LEVEL_2;

			ent->client->ps.forcePowerLevel[FP_RAGE] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_PROTECT] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_ABSORB] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_DRAIN] = FORCE_LEVEL_1;
			ent->client->ps.forcePowerLevel[FP_SEE] = FORCE_LEVEL_1;

			ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_SABER_OFFENSE] = FORCE_LEVEL_3;
			ent->client->ps.forcePowerLevel[FP_GRIP] = FORCE_LEVEL_2;
			
			ent->client->ps.forcePowerLevel[FP_INSANITY] = FORCE_LEVEL_2;
			ent->client->ps.forcePowerLevel[FP_STASIS] = FORCE_LEVEL_2;
		}
		int defLevel = ent->client->ps.forcePowerLevel[FP_SABER_DEFENSE];
		ent->client->ps.forcePowerMax = FORCE_POWER_MAX + ( defLevel > FORCE_LEVEL_1 ? ( defLevel - FORCE_LEVEL_1 ) * 50 : 0 );
		ent->client->ps.forcePower = ent->client->ps.forcePowerMax;
	}
}

bool WP_DoingMoronicForcedAnimationForForcePowers(gentity_t *ent)
{
	// :P --eez
	if( !ent->client ) return false;
	if( ent->client->ps.legsAnim == BOTH_FORCE_ABSORB_START ||
		ent->client->ps.legsAnim == BOTH_FORCE_ABSORB_END ||
		ent->client->ps.legsAnim == BOTH_FORCE_ABSORB ||
		ent->client->ps.torsoAnim == BOTH_FORCE_RAGE ||
		ent->client->ps.legsAnim == BOTH_FORCE_PROTECT )
		return true;
	return false;
}
