/*
===========================================================================
Class fire modules for the trooper loop.

What used to be whole copies of the trooper AI for snipers, grenadiers and Tusken
(AI_Sniper.cpp, AI_Grenadier.cpp, AI_Tusken.cpp) is now just the part that was actually
different: how they aim and when they strike. Movement and decisions come from the brain
with a per-class profile (AI_BrainProfiles.cpp). Everything here runs on the NPC globals
from inside NPC_BSST_Attack.
===========================================================================
*/

#include "b_local.h"
#include "g_navigator.h"
#include "anims.h"
#include "g_functions.h"
#include "../cgame/cg_local.h"
#include "AI_Brain.h"

extern void		Saboteur_Cloak( gentity_t *self );
extern void		Saboteur_Decloak( gentity_t *self, int uncloakTime );
extern void		G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
extern void		CalcMuzzlePoint( gentity_t *const ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint, float lead_in );
extern void		NPC_ChangeWeapon( int newWeapon );

#define	SPF_NO_HIDE			2

/*
================================================================================
SNIPER: misses the first few shots at long range, then leads the target; hides after a shot
================================================================================
*/
static qboolean Sniper_EvaluateShot( int hit )
{
	if ( !NPC->enemy )
	{
		return qfalse;
	}

	gentity_t *hitEnt = &g_entities[hit];
	if ( hit == NPC->enemy->s.number
		|| ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == NPC->client->enemyTeam )
		|| ( hitEnt && hitEnt->takedamage && ((hitEnt->svFlags&SVF_GLASS_BRUSH)||hitEnt->health < 40||NPC->s.weapon == WP_EMPLACED_GUN) )
		|| ( hitEnt && (hitEnt->svFlags&SVF_GLASS_BRUSH)) )
	{//can hit enemy or will hit glass, so shoot anyway
		return qtrue;
	}
	return qfalse;
}

static void Sniper_UpdateEnemyPos( void )
{
	int index;
	for ( int i = MAX_ENEMY_POS_LAG-ENEMY_POS_LAG_INTERVAL; i >= 0; i -= ENEMY_POS_LAG_INTERVAL )
	{
		index = i/ENEMY_POS_LAG_INTERVAL;
		if ( !index )
		{
			CalcEntitySpot( NPC->enemy, SPOT_HEAD_LEAN, NPCInfo->enemyLaggedPos[index] );
			NPCInfo->enemyLaggedPos[index][2] -= Q_flrand( 2, 16 );
		}
		else
		{
			VectorCopy( NPCInfo->enemyLaggedPos[index-1], NPCInfo->enemyLaggedPos[index] );
		}
	}
}

// a 0..1 aim offset that only changes when a shot goes off, so the barrel doesn't bob up and
// down every frame while they line up (it used to be a fresh random each think)
static float Sniper_AimJitter( void )
{
	unsigned int h = (unsigned int)NPC->s.number * 2654435761u
		^ (unsigned int)NPCInfo->shotTime * 40503u
		^ (unsigned int)NPC->count * 97u;
	h ^= h >> 13;
	h *= 0x5bd1e995u;
	h ^= h >> 15;
	return ( h % 1000 ) / 1000.0f;
}

// the sniper's aim: deliberate misses first at long range, then a lagged lead
static void Sniper_FaceEnemy( qboolean enemyLOS, qboolean shoot, float enemyDistSq )
{
	if ( NPC->enemy )
	{
		vec3_t	muzzle, eye, target, angles, forward, right, up;
		AngleVectors( NPC->client->ps.viewangles, forward, right, up );
		CalcMuzzlePoint( NPC, forward, right, up, muzzle, 0 );
		// aim from the head like NPC_FacePosition does: the muzzle sits off to the side of the
		// view, so angles taken from it shift the view, which moves the muzzle... up close that
		// loop never settles and the rifle wags side to side
		CalcEntitySpot( NPC, SPOT_HEAD_LEAN, eye );
		CalcEntitySpot( NPC->enemy, SPOT_ORIGIN, target );

		if ( enemyDistSq > 65536 && NPCInfo->stats.aim < 5 )//past 256
		{
			if ( NPC->count < (5-NPCInfo->stats.aim) )
			{//miss a few times first
				if ( shoot && TIMER_Done( NPC, "attackDelay" ) && level.time >= NPCInfo->shotTime )
				{//ready to fire again
					qboolean	aimError = qfalse;
					qboolean	hit = qtrue;
					int			tryMissCount = 0;
					trace_t		trace;

					GetAnglesForDirection( eye, target, angles );
					AngleVectors( angles, forward, right, up );

					while ( hit && tryMissCount < 10 )
					{
						tryMissCount++;
						if ( !Q_irand( 0, 1 ) )
						{
							aimError = qtrue;
							if ( !Q_irand( 0, 1 ) )
							{
								VectorMA( target, NPC->enemy->maxs[2]*Q_flrand(1.5, 4), right, target );
							}
							else
							{
								VectorMA( target, NPC->enemy->mins[2]*Q_flrand(1.5, 4), right, target );
							}
						}
						if ( !aimError || !Q_irand( 0, 1 ) )
						{
							if ( !Q_irand( 0, 1 ) )
							{
								VectorMA( target, NPC->enemy->maxs[2]*Q_flrand(1.5, 4), up, target );
							}
							else
							{
								VectorMA( target, NPC->enemy->mins[2]*Q_flrand(1.5, 4), up, target );
							}
						}
						gi.trace( &trace, muzzle, vec3_origin, vec3_origin, target, NPC->s.number, MASK_SHOT, (EG2_Collision)0, 0 );
						hit = Sniper_EvaluateShot( trace.entityNum );
					}
					NPC->count++;
				}
				else
				{
					if ( !enemyLOS )
					{
						NPC_UpdateAngles( qtrue, qtrue );
						return;
					}
				}
			}
			else
			{//based on distance, aim value, difficulty and enemy movement, miss
				int missFactor = 8-(NPCInfo->stats.aim+g_spskill->integer) * 3;
				if ( missFactor > ENEMY_POS_LAG_STEPS )
				{
					missFactor = ENEMY_POS_LAG_STEPS;
				}
				else if ( missFactor < 0 )
				{
					missFactor = 0 ;
				}
				VectorCopy( NPCInfo->enemyLaggedPos[missFactor], target );
			}
			GetAnglesForDirection( eye, target, angles );
		}
		else
		{
			target[2] += Sniper_AimJitter() * NPC->enemy->maxs[2];
			GetAnglesForDirection( eye, target, angles );
		}

		NPCInfo->desiredYaw		= AngleNormalize360( angles[YAW] );
		NPCInfo->desiredPitch	= AngleNormalize360( angles[PITCH] );

		// the misses above are the sniper's inaccuracy; skip the generic per-NPC aim wobble
		// (NPC_UpdateFiringAngles re-rolls a few degrees of yaw/pitch error every 0.25-2 s,
		// which reads as the rifle twitching sideways on a sniper who's otherwise still)
		NPCInfo->lastAimErrorYaw = NPCInfo->lastAimErrorPitch = 0;
		NPCInfo->aimErrorDebounceTime = level.time + 60000;
	}
	NPC_UpdateAngles( qtrue, qtrue );
}

static void Sniper_StartHide( void )
{
	int duckTime = Q_irand( 1000, 2500 );

	TIMER_Set( NPC, "duck", duckTime );
	if ( NPC->client->NPC_class == CLASS_SABOTEUR )
	{
		Saboteur_Cloak( NPC );
	}
	TIMER_Set( NPC, "attackDelay", duckTime + Q_irand( 500, 2000 ) );
}

static void Sniper_Think( float enemyDistSq )
{
	Sniper_UpdateEnemyPos();

	if ( level.time - NPCInfo->enemyLastSeenTime > 10000 )
	{//next time we see him, we'll miss a few times first
		NPC->count = 0;
	}

	// a disruptor up close is just a rifle (the long-range switch back to alt is in NPC_BSST_Attack)
	if ( enemyDistSq < 16384 && NPC->client->ps.weapon == WP_DISRUPTOR && (NPCInfo->scriptFlags & SCF_ALT_FIRE) )
	{
		trace_t	trace;
		gi.trace ( &trace, NPC->enemy->currentOrigin, NPC->enemy->mins, NPC->enemy->maxs, NPC->currentOrigin, NPC->enemy->s.number, NPC->enemy->clipmask, (EG2_Collision)0, 0 );
		if ( !trace.allsolid && !trace.startsolid && (trace.fraction == 1.0 || trace.entityNum == NPC->s.number ) )
		{//he can get right to me
			NPCInfo->scriptFlags &= ~SCF_ALT_FIRE;
			NPC_ChangeWeapon( NPC->client->ps.weapon );
		}
	}
}

// The Tusken rifle is backwards: ALT fire is the shot, primary is a bash with the stock.
// So the flag stays on (shoot) unless the enemy is actually within striking distance.
static void TuskenRifle_Think( float enemyDistSq )
{
	const float strikeRange = NPC->maxs[0] + NPC->enemy->maxs[0] + 40.0f;
	const qboolean inStrike = (qboolean)( enemyDistSq < strikeRange * strikeRange );

	if ( inStrike && (NPCInfo->scriptFlags & SCF_ALT_FIRE) )
	{
		NPCInfo->scriptFlags &= ~SCF_ALT_FIRE;
		NPC_ChangeWeapon( WP_TUSKEN_RIFLE );
	}
	else if ( !inStrike && !(NPCInfo->scriptFlags & SCF_ALT_FIRE) )
	{
		NPCInfo->scriptFlags |= SCF_ALT_FIRE;
		NPC_ChangeWeapon( WP_TUSKEN_RIFLE );
	}
}

/*
================================================================================
GRENADIER: anyone carrying thermals. A grenadier with a gun lobs through the brain's
AA_GRENADE (swap, throw, swap back in NPC_BSST_Attack) and otherwise fights like a
trooper; only a thermal-only thrower (Gran) goes to fists when you get in close.
================================================================================
*/
extern int ST_PrimaryGunFor( gentity_t *ent );
static void Grenadier_Think( float enemyDistSq )
{
	const qboolean enemySaber = (qboolean)( NPC->enemy->client && NPC->enemy->client->ps.weapon == WP_SABER && NPC->enemy->client->ps.SaberActive() );

	if ( ST_PrimaryGunFor( NPC ) != WP_NONE )
	{//has a gun: the sidearm pipeline handles the thermal, nothing to do here
		return;
	}
	if ( enemyDistSq < 16384 && !enemySaber )//128
	{//enemy is close and not using saber
		if ( NPC->client->ps.weapon == WP_THERMAL )
		{
			trace_t	trace;
			gi.trace ( &trace, NPC->currentOrigin, NPC->enemy->mins, NPC->enemy->maxs, NPC->enemy->currentOrigin, NPC->s.number, NPC->enemy->clipmask, (EG2_Collision)0, 0 );
			if ( !trace.allsolid && !trace.startsolid && (trace.fraction == 1.0 || trace.entityNum == NPC->enemy->s.number ) )
			{//I can get right to him
				NPC_ChangeWeapon( WP_MELEE );
				NPCInfo->scriptFlags |= SCF_CHASE_ENEMIES;
				AI_Brain_Interrupt( NPC );
			}
		}
	}
	else if ( enemyDistSq > 65536 || enemySaber )//256
	{//enemy is far or using saber
		if ( NPC->client->ps.weapon == WP_MELEE && NPC->client->ps.weapons[WP_THERMAL] )
		{//fisticuffs, make switch to thermal if have it
			NPC_ChangeWeapon( WP_THERMAL );
			AI_Brain_Interrupt( NPC );
		}
	}
}

/*
================================================================================
TUSKEN / MELEE: close to lunge range, strike, taunt now and then
================================================================================
*/
void NPC_Tusken_Precache( void )
{
	int i;
	for ( i = 1; i < 5; i ++ )
	{
		G_SoundIndex( va( "sound/weapons/tusken_staff/stickhit%d.wav", i ) );
	}
}

static void NPC_Tusken_Taunt( void )
{
	NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_TUSKENTAUNT1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	TIMER_Set( NPC, "taunting", NPC->client->ps.torsoAnimTimer );
	TIMER_Set( NPC, "duck", -1 );
}

void Tusken_StaffTrace( gentity_t *self )
{
	if ( !self->ghoul2.size()
		|| self->weaponModel[0] <= 0 )
	{
		return;
	}

	int			boltIndex = gi.G2API_AddBolt(&self->ghoul2[self->weaponModel[0]], "*weapon", qfalse);
	if ( boltIndex != -1 )
	{
		int curTime = (cg.time?cg.time:level.time);
		qboolean hit = qfalse;
		int	lastHit = ENTITYNUM_NONE;
		for ( int time = curTime-25; time <= curTime+25&&!hit; time += 25 )
		{
			mdxaBone_t	boltMatrix;
			vec3_t		tip, dir, base, angles={0,self->currentAngles[YAW],0};
			vec3_t		mins={-2,-2,-2},maxs={2,2,2};
			trace_t		trace;

			gi.G2API_GetBoltMatrix(self->ghoul2, self->weaponModel[0],
						boltIndex,
						&boltMatrix, angles, self->currentOrigin, time,
						NULL, self->s.modelScale );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, base );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, dir );
			VectorMA( base, -20, dir, base );
			VectorMA( base, 78, dir, tip );
	#ifndef FINAL_BUILD
			if ( d_saberCombat->integer > 1 )
			{
				G_DebugLine(base, tip, 1000, 0x000000ff, qtrue);
			}
	#endif
			gi.trace( &trace, base, mins, maxs, tip, self->s.number, MASK_SHOT, G2_RETURNONHIT, 10 );
			if ( trace.fraction < 1.0f && trace.entityNum != lastHit )
			{//hit something
				gentity_t *traceEnt = &g_entities[trace.entityNum];
				if ( traceEnt->takedamage
					&& (!traceEnt->client || traceEnt == self->enemy || traceEnt->client->NPC_class != self->client->NPC_class) )
				{//smack
					int dmg = Q_irand( 5, 10 ) * (g_spskill->integer+1);

					G_Sound( traceEnt, G_SoundIndex( va( "sound/weapons/tusken_staff/stickhit%d.wav", Q_irand( 1, 4 ) ) ) );
					G_Damage( traceEnt, self, self, vec3_origin, trace.endpos, dmg, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
					if ( traceEnt->health > 0
						&& ( (traceEnt->client&&traceEnt->client->NPC_class==CLASS_JAWA&&!Q_irand(0,1))
							|| dmg > 19 ) )//FIXME: base on skill!
					{//do pain on enemy
						G_Knockdown( traceEnt, self, dir, 300, qtrue );
					}
					lastHit = trace.entityNum;
					hit = qtrue;
				}
			}
		}
	}
}

qboolean G_TuskenAttackAnimDamage( gentity_t *self )
{
 	if (self->client->ps.torsoAnim==BOTH_TUSKENATTACK1 ||
		self->client->ps.torsoAnim==BOTH_TUSKENATTACK2 ||
		self->client->ps.torsoAnim==BOTH_TUSKENATTACK3 ||
		self->client->ps.torsoAnim==BOTH_TUSKENLUNGE1)
	{
		float		current	  = 0.0f;
		int			end		  = 0;
		int			start	  = 0;
		if (!!gi.G2API_GetBoneAnimIndex(&
					self->ghoul2[self->playerModel],
					self->lowerLumbarBone,
					level.time,
					&current,
					&start,
					&end,
					NULL,
					NULL,
					NULL))
		{
 			float percentComplete = (current-start)/(end-start);
			switch (self->client->ps.torsoAnim)
			{
			case BOTH_TUSKENATTACK1: return (qboolean)(percentComplete>0.3 && percentComplete<0.7);
			case BOTH_TUSKENATTACK2: return (qboolean)(percentComplete>0.3 && percentComplete<0.7);
			case BOTH_TUSKENATTACK3: return (qboolean)(percentComplete>0.1 && percentComplete<0.5);
			case BOTH_TUSKENLUNGE1:  return (qboolean)(percentComplete>0.3 && percentComplete<0.5);
			}
		}
	}
	return qfalse;
}

// a melee fighter's "clear shot" is being in reach; the staff lunges from just outside it
static void Melee_Think( qboolean enemyLOS, qboolean *enemyCS, qboolean *shoot, float enemyDistSq )
{
	const float	enemyDist	= sqrtf( enemyDistSq );
	const float	boundsMin	= (NPC->maxs[0]+NPC->enemy->maxs[0]);
	const float	lungeRange	= (boundsMin + 65.0f);
	const float	strikeRange = (boundsMin + 40.0f);
	const qboolean staff	= (qboolean)( NPC->client->ps.weapon == WP_TUSKEN_STAFF );
	const qboolean canSee	= (qboolean)( (level.time - NPCInfo->enemyLastSeenTime) < 3000 );
	const qboolean inReach	= (qboolean)( enemyDist < ( staff ? lungeRange : boundsMin + 16.0f ) );

	if ( staff && canSee && !inReach && TIMER_Done( NPC, "tuskenTauntCheck" ) )
	{
		TIMER_Set( NPC, "tuskenTauntCheck", Q_irand( 2000, 6000 ) );
		if ( !Q_irand( 0, 3 ) )
		{
			NPC_Tusken_Taunt();
		}
	}

	if ( !TIMER_Done( NPC, "taunting" ) )
	{
		*enemyCS = qfalse;
		*shoot = qfalse;
		return;
	}

	*enemyCS = (qboolean)( inReach && canSee && InFOV( NPC->enemy, NPC, 90, 45 ) );
	*shoot = *enemyCS;
	if ( *shoot )
	{
		ucmd.buttons &= ~BUTTON_ALT_ATTACK;
		if ( staff && enemyDist > strikeRange )
		{//not in strike range: lunge
			ucmd.buttons |= BUTTON_ALT_ATTACK;
		}
	}
}

/*
================================================================================
The hooks NPC_BSST_Attack calls
================================================================================
*/
qboolean ST_IsMeleeWeapon( int weapon )
{
	return (qboolean)( weapon == WP_MELEE || weapon == WP_TUSKEN_STAFF || weapon == WP_STUN_BATON );
}

// after perception, before the brain: class-specific weapon swaps and strike decisions
void ST_ClassFireModule( qboolean enemyLOS, qboolean enemyInFOV, qboolean *enemyCS, qboolean *shoot, float enemyDistSq )
{
	if ( !NPC->enemy )
	{
		return;
	}
	switch ( NPCInfo->aiProfile )
	{
	case AIPROF_SNIPER:
		Sniper_Think( enemyDistSq );
		break;
	case AIPROF_GRENADIER:
		Grenadier_Think( enemyDistSq );
		break;
	default:
		break;
	}
	if ( NPC->client->ps.weapon == WP_TUSKEN_RIFLE )
	{
		TuskenRifle_Think( enemyDistSq );
	}
	if ( ST_IsMeleeWeapon( NPC->client->ps.weapon )
		|| ( NPC->client->ps.weapon == WP_TUSKEN_RIFLE && !(NPCInfo->scriptFlags & SCF_ALT_FIRE) ) )
	{//fists, staff, or a rifle butt: strike by reach
		Melee_Think( enemyLOS, enemyCS, shoot, enemyDistSq );
	}
}

// qtrue if the class handled facing/aiming itself
qboolean ST_ClassFaceEnemy( qboolean enemyLOS, qboolean shoot, float enemyDistSq )
{
	if ( NPCInfo->aiProfile == AIPROF_SNIPER && NPC->enemy )
	{
		Sniper_FaceEnemy( enemyLOS, shoot, enemyDistSq );
		return qtrue;
	}
	return qfalse;
}

// right after the trigger is pulled
void ST_ClassAfterShot( void )
{
	if ( NPCInfo->aiProfile == AIPROF_SNIPER )
	{
		if ( !( ucmd.buttons & (BUTTON_ATTACK|BUTTON_ALT_ATTACK) ) )
		{//trigger wasn't pulled this think (fire timing): wait for it, don't hide for nothing
			TIMER_Set( NPC, "attackDelay", NPCInfo->shotTime-level.time );
			return;
		}
		G_SoundOnEnt( NPC, CHAN_WEAPON, "sound/null.wav" );
		//took a shot, now hide
		if ( !(NPC->spawnflags&SPF_NO_HIDE) && !Q_irand( 0, 1 ) )
		{
			Sniper_StartHide();
		}
		else
		{
			TIMER_Set( NPC, "attackDelay", NPCInfo->shotTime-level.time );
		}
	}
	else if ( ST_IsMeleeWeapon( NPC->client->ps.weapon ) )
	{
		TIMER_Set( NPC, "attackDelay", NPCInfo->shotTime-level.time );
	}
}
