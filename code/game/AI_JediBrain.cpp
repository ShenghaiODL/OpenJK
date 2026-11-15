/*
===========================================================================
Jedi tactical layer. See AI_JediBrain.h.
===========================================================================
*/

#include "b_local.h"
#include "anims.h"
#include "wp_saber.h"
#include "AI_JediBrain.h"
#include "g_force.h"
#include "g_difficulty.h"

extern qboolean	PM_SaberInAttack( int move );
extern qboolean	PM_SaberInStart( int move );
extern qboolean	PM_SaberInReturn( int move );
extern qboolean	PM_SaberInBounce( int move );
extern qboolean	PM_SaberInBrokenParry( int move );
extern qboolean	PM_SaberInSpecialAttack( int anim );
extern qboolean	PM_InKnockDown( playerState_t *ps );
extern qboolean	Jedi_CultistDestroyer( gentity_t *self );
extern cvar_t	*d_aiLog;

static const char *tacticNames[JT_NUM] =
{
	"none", "press", "defend", "keep distance", "recover", "force opener", "saber throw", "taunt"
};

const char *AI_JediTacticName( int tactic )
{
	return ( tactic >= 0 && tactic < JT_NUM ) ? tacticNames[tactic] : "?";
}

// minimum time a tactic runs before another may replace it (one-shots end themselves)
static const int tacticHoldMs[JT_NUM] = { 0, 2500, 1500, 1500, 2500, 400, 400, 1200 };

// who has the lead on each enemy: one saber fighter presses at a time, the rest circle
static int s_pressLead[MAX_GENTITIES];
static int s_pressLeadTime[MAX_GENTITIES];

int AI_JediBlockSkill( const gentity_t *npc )
{
	if ( !npc || !npc->NPC )
	{
		return 3;
	}
	if ( npc->NPC->stats.blockSkill >= 1 && npc->NPC->stats.blockSkill <= 5 )
	{
		return npc->NPC->stats.blockSkill;
	}
	if ( (npc->NPC->aiFlags&NPCAI_BOSS_CHARACTER) )
	{
		return 5;
	}
	switch ( npc->client->NPC_class )
	{
	case CLASS_DESANN:
	case CLASS_LUKE:
	case CLASS_TAVION:
	case CLASS_ALORA:
	case CLASS_KYLE:
		return 5;
	case CLASS_SHADOWTROOPER:
		return 4;
	default:
		break;
	}
	switch ( npc->NPC->rank )
	{
	case RANK_CIVILIAN:
	case RANK_CREWMAN:		return 1;
	case RANK_ENSIGN:		return 2;
	case RANK_LT_JG:
	case RANK_LT:			return 3;
	case RANK_LT_COMM:
	case RANK_COMMANDER:	return 4;
	default:				return 5;
	}
}

qboolean AI_JediEnemyOpen( const gentity_t *npc )
{
	const gentity_t *enemy = npc->enemy;
	if ( !enemy || !enemy->client )
	{
		return qfalse;
	}
	playerState_t *ps = &enemy->client->ps;
	if ( PM_InKnockDown( ps ) )
	{
		return qtrue;
	}
	if ( enemy->NPC && enemy->NPC->guardBreakTime > level.time )
	{
		return qtrue;
	}
	if ( ps->saberBlocked == BLOCKED_PARRY_BROKEN || PM_SaberInBrokenParry( ps->saberMove ) || PM_SaberInBounce( ps->saberMove ) )
	{
		return qtrue;
	}
	if ( PM_SaberInReturn( ps->saberMove ) && ps->saberAnimLevel >= SS_STRONG )
	{//slow recovery off a heavy swing
		return qtrue;
	}
	if ( ps->weapon == WP_SABER && !ps->SaberActive() )
	{
		return qtrue;
	}
	return qfalse;
}

static float ForceFrac( const gentity_t *ent )
{
	if ( !ent || !ent->client || ent->client->ps.forcePowerMax <= 0 )
	{
		return 1.0f;
	}
	return (float)ent->client->ps.forcePower / (float)ent->client->ps.forcePowerMax;
}

/*
AI_JediBrain_HeavyAttackScore (JKEnhanced)

How good a moment this is for a telegraphed heavy attack (Jedi_CheckHeavyAttack throws one above
g_heavyAttackThreshold). Up: we're pressing, the enemy is open (recovering, bounced, broken,
knocked down) or committed to their own swing, low on Force (blocks cost Force), backing off.
Down: the enemy stands ready with block up (a likely perfect parry), our own guard is low. Lower
blockSkill fighters judge it noisily.
*/
float AI_JediBrain_HeavyAttackScore( gentity_t *npc )
{
	if ( !npc || !npc->NPC || !npc->client || !npc->enemy || !npc->enemy->client )
	{
		return 0.0f;
	}
	const gentity_t		*enemy = npc->enemy;
	const playerState_t	*eps = &enemy->client->ps;
	const qboolean		open = AI_JediEnemyOpen( npc );
	const int			skill = AI_JediBlockSkill( npc );
	float s = 0.15f;

	if ( npc->NPC->duelRole == JT_PRESS )
	{
		s += 0.25f;
	}
	if ( open )
	{
		s += 0.45f;
	}
	else if ( PM_SaberInAttack( eps->saberMove ) || PM_SaberInSpecialAttack( eps->torsoAnim ) )
	{//committed to their own swing
		s += 0.25f;
	}
	if ( ForceFrac( enemy ) < 0.3f )
	{
		s += 0.25f;
	}
	vec3_t toEnemy;
	VectorSubtract( enemy->currentOrigin, npc->currentOrigin, toEnemy );
	VectorNormalize( toEnemy );
	if ( DotProduct( eps->velocity, toEnemy ) > 100.0f )
	{//backing away from us
		s += 0.1f;
	}
	if ( !open && eps->weapon == WP_SABER && eps->saberBlockingTime > level.time )
	{//standing ready with block up: a heavy into that is asking to be perfect-parried
		s -= 0.35f;
	}
	if ( WP_SaberRules() ? ForceFrac( npc ) < 0.3f//saber rules: Force is the guard
		: ( npc->NPC->guardMax > 0 && npc->NPC->guard < npc->NPC->guardMax * 3 / 10 ) )
	{
		s -= 0.2f;
	}
	s += Q_flrand( -0.25f, 0.25f ) * ( 5 - skill ) / 4.0f;
	return s;
}

// how many other saber fighters are already in this enemy's face
static int EngagedOthers( const gentity_t *npc )
{
	gentity_t	*list[64];
	vec3_t		mins, maxs;
	int			count = 0;
	for ( int k = 0; k < 3; k++ )
	{
		mins[k] = npc->enemy->currentOrigin[k] - 160;
		maxs[k] = npc->enemy->currentOrigin[k] + 160;
	}
	const int n = gi.EntitiesInBox( mins, maxs, list, 64 );
	for ( int k = 0; k < n; k++ )
	{
		const gentity_t *e = list[k];
		if ( e == npc || !e->NPC || !e->client || e->health <= 0 || e->enemy != npc->enemy || e->client->ps.weapon != WP_SABER )
		{
			continue;
		}
		if ( e->NPC->duelRole == JT_PRESS || e->NPC->duelRole == JT_DEFEND || e->NPC->duelRole == JT_FORCE_OPENER )
		{
			count++;
		}
	}
	return count;
}

static qboolean LeadIsFree( const gentity_t *npc )
{
	const int e = npc->enemy->s.number;
	const int lead = s_pressLead[e];
	if ( lead == 0 || lead == npc->s.number || level.time - s_pressLeadTime[e] > 1500 )
	{
		return qtrue;
	}
	const gentity_t *l = &g_entities[lead];
	if ( !l->inuse || !l->NPC || l->health <= 0 || l->enemy != npc->enemy || l->NPC->duelRole != JT_PRESS )
	{
		return qtrue;
	}
	return qfalse;
}

static qboolean Usable( const gentity_t *npc, int power )
{
	return (qboolean)( (npc->client->ps.forcePowersKnown&(1<<power)) && npc->client->ps.forcePowerLevel[power] > 0 );
}

//can it fire right now? Under the saber rules that includes the Force a saber fighter keeps back
//for guarding (WP_ForcePowerAvailable): a tactic built on a power they can't pay for does nothing.
extern qboolean WP_ForcePowerUsable( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
static qboolean CanUse( gentity_t *npc, int power )
{
	return (qboolean)( Usable( npc, power ) && ( !WP_SaberRules() || WP_ForcePowerUsable( npc, (forcePowers_t)power, 0 ) ) );
}

int AI_JediTactic( gentity_t *npc, float enemyDist )
{
	if ( !npc || !npc->NPC || !npc->client || !npc->enemy || !npc->enemy->client )
	{
		return JT_NONE;
	}
	gNPC_t *ni = npc->NPC;
	if ( npc->client->ps.weapon != WP_SABER || Jedi_CultistDestroyer( npc )
		|| npc->client->NPC_class == CLASS_BOBAFETT || npc->client->NPC_class == CLASS_ROCKETTROOPER )
	{
		ni->duelRole = JT_NONE;
		return JT_NONE;
	}

	const int cur = ni->duelRole;
	if ( cur == JT_PRESS )
	{//keep the lead fresh while pressing
		s_pressLead[npc->enemy->s.number] = npc->s.number;
		s_pressLeadTime[npc->enemy->s.number] = level.time;
	}
	if ( cur != JT_NONE && !TIMER_Done( npc, "jediTactic" ) )
	{
		return cur;
	}
	TIMER_Set( npc, "jediTactic", Q_irand( 600, 1000 ) );

	const qboolean	enemySaber	= (qboolean)( npc->enemy->client->ps.weapon == WP_SABER && npc->enemy->client->ps.SaberActive() );
	const qboolean	enemyOpen	= AI_JediEnemyOpen( npc );
	const qboolean	enemySwing	= (qboolean)( PM_SaberInAttack( npc->enemy->client->ps.saberMove ) || PM_SaberInStart( npc->enemy->client->ps.saberMove )
										|| PM_SaberInSpecialAttack( npc->enemy->client->ps.torsoAnim ) );
	const float		myForce		= ForceFrac( npc );
	const float		enemyForce	= ForceFrac( npc->enemy );
	const float		myHealth	= npc->max_health > 0 ? (float)npc->health / npc->max_health : 1.0f;
	const float		enemyHealth	= npc->enemy->max_health > 0 ? (float)npc->enemy->health / npc->enemy->max_health : 1.0f;
	const float		guard		= WP_SaberRules() ? myForce//saber rules: Force is the guard
									: ( ni->guardMax > 0 ? (float)ni->guard / ni->guardMax : 1.0f );
	const int		tier		= AI_NerveTier( npc );
	const int		skill		= AI_JediBlockSkill( npc );
	const qboolean	leadFree	= LeadIsFree( npc );
	const int		engaged		= EngagedOthers( npc );
	const qboolean	raging		= (qboolean)( (npc->client->ps.forcePowersActive&(1<<FP_RAGE)) != 0 );
	const float		drive		= tier == NT_STEADY ? 1.0f : ( tier == NT_SHAKEN ? 0.6f : 0.25f );

	float s[JT_NUM] = { 0 };

	// PRESS: when they're open, low on Force, or hurting -- and only if nobody else has the lead
	if ( leadFree && engaged < 2 )
	{
		s[JT_PRESS] = 0.45f + ( enemyOpen ? 0.4f : 0.0f ) + ( enemyForce < 0.3f ? 0.2f : 0.0f )
			+ Com_Clamp( -0.2f, 0.2f, ( myHealth - enemyHealth ) * 0.5f );
		s[JT_PRESS] *= drive;
		if ( myForce < 0.15f )
		{
			s[JT_PRESS] *= 0.4f;
		}
		if ( raging )
		{
			s[JT_PRESS] += 0.5f;
		}
	}
	// DEFEND: they're swinging, or we'd rather they over-commit first; better fighters like this more
	//(only in reach, and with limited patience: waiting at range, or for someone who never swings,
	//was a standoff that looked like the AI had frozen)
	static int s_lastEnemySwing[MAX_GENTITIES];
	if ( enemySwing || level.time < s_lastEnemySwing[npc->s.number]
		|| cur == JT_NONE )//a new duel: their patience starts now (it started at 0, so DEFEND was cut from the first rescore)
	{
		s_lastEnemySwing[npc->s.number] = level.time;
	}
	if ( enemySaber && enemyDist > 160 )
	{//out of reach: nothing to wait for
		s[JT_DEFEND] = 0.1f;
	}
	else if ( enemySaber )
	{
		s[JT_DEFEND] = 0.35f + ( enemySwing ? 0.3f : 0.0f ) + ( myForce < 0.5f ? 0.15f : 0.0f ) + 0.04f * skill;
		if ( level.time - s_lastEnemySwing[npc->s.number] > 3000 )
		{//they're not biting: stop waiting
			s[JT_DEFEND] *= 0.4f;
		}
	}
	else
	{//guns: nothing to wait for
		s[JT_DEFEND] = 0.1f;
	}
	// KEEP_DISTANCE: someone else has them, or we're losing our nerve
	s[JT_KEEP_DISTANCE] = 0.15f + ( !leadFree || engaged >= 2 ? 0.6f : 0.0f ) + ( tier >= NT_BREAKING ? 0.35f : 0.0f );
	// RECOVER: out of Force or guard
	//(saber rules: Force is the guard. Once backing off they stay off until there is enough for a few
	//stops - going in and out around the line every second or two read as dithering)
	if ( WP_SaberRules() ? ( myForce < 0.25f || ( cur == JT_RECOVER && myForce < 0.5f ) )
		: ( myForce < 0.25f || guard < 0.3f ) )
	{
		s[JT_RECOVER] = WP_SaberRules() ? 1.2f//(beats everything, so being swung at does not pull them back into DEFEND half-recovered)
			: 0.6f + ( 1.0f - myForce ) * 0.3f + ( guard < 0.3f ? 0.2f : 0.0f );
		if ( raging )
		{
			s[JT_RECOVER] = 0;
		}
	}
	// FORCE_OPENER: shove a staggered enemy, or drag one in from just out of reach
	if ( TIMER_Done( npc, "jediOpenerDebounce" ) && myForce > 0.35f )
	{
		//(saber rules: not when they are already in blade reach - cut them, don't shove them away)
		if ( CanUse( npc, FP_PUSH ) && enemyOpen && enemyDist < 200 && ( !WP_SaberRules() || enemyDist > 48 ) )
		{
			s[JT_FORCE_OPENER] = 0.7f;
		}
		else if ( CanUse( npc, FP_PULL ) && enemyDist > 160 && enemyDist < 450 && !enemySwing )
		{
			s[JT_FORCE_OPENER] = 0.45f;
		}
	}
	// SABER_THROW: at someone standing off who isn't swinging
	if ( TIMER_Done( npc, "saberThrowDebounce" ) && CanUse( npc, FP_SABERTHROW ) && myForce > 0.3f
		&& enemyDist > 200 && enemyDist < 500 && !enemySwing && !npc->client->ps.saberInFlight )
	{
		s[JT_SABER_THROW] = 0.4f + ( ni->rank >= RANK_LT_JG ? 0.1f : 0.0f );
	}
	// TAUNT: confident, at a distance, not pressed
	if ( TIMER_Done( npc, "jediTauntDebounce" ) && enemyDist > 250 && tier == NT_STEADY && myHealth > 0.5f && !enemySwing )
	{
		s[JT_TAUNT] = 0.3f;
	}

	// pick, with a little noise and commitment to what we're doing
	const float	noise = ( 5 - skill ) * 0.03f;
	int			best = JT_DEFEND;
	float		bestScore = -1.0f;
	for ( int t = 1; t < JT_NUM; t++ )
	{
		if ( s[t] <= 0 )
		{
			continue;
		}
		float v = s[t] + Q_flrand( -noise, noise );
		if ( t == cur )
		{
			v += 0.1f;
		}
		if ( v > bestScore )
		{
			bestScore = v;
			best = t;
		}
	}
	if ( cur != JT_NONE && best != cur && level.time < TIMER_Get( npc, "jediTacticHold" ) && best != JT_RECOVER )
	{//committed a while longer (but running out of Force always wins)
		return cur;
	}
	if ( best != cur )
	{
		TIMER_Set( npc, "jediTacticHold", tacticHoldMs[best] );
		if ( d_aiLog && d_aiLog->integer && ( d_aiLog->integer < 0 || d_aiLog->integer == npc->s.number ) )
		{
			gi.Printf( "JEDI %s (%d): %s -> %s | F %.2f G %.2f eF %.2f open %d swing %d lead %d eng %d\n", npc->NPC_type, npc->s.number,
				AI_JediTacticName( cur ), AI_JediTacticName( best ), myForce, guard, enemyForce, enemyOpen, enemySwing, leadFree, engaged );
		}
	}
	ni->duelRole = best;
	if ( best == JT_PRESS )
	{
		s_pressLead[npc->enemy->s.number] = npc->s.number;
		s_pressLeadTime[npc->enemy->s.number] = level.time;
	}
	return best;
}

void AI_JediTacticDone( gentity_t *npc )
{
	if ( !npc || !npc->NPC )
	{
		return;
	}
	TIMER_Set( npc, "jediTactic", 0 );
	TIMER_Set( npc, "jediTacticHold", 0 );
}

/*
-------------------------
WP_NPCPerfectParry
-------------------------
*/
static int s_ppDecidedTime[MAX_GENTITIES];
static qboolean s_ppDecided[MAX_GENTITIES];

qboolean WP_NPCPerfectParry( gentity_t *self )
{
	if ( !self || !self->NPC || !self->client || self->s.number < MAX_CLIENTS || self->client->ps.weapon != WP_SABER )
	{
		return qfalse;
	}
	const int n = self->s.number;
	if ( s_ppDecidedTime[n] == level.time )
	{//already decided for this frame's collision
		return s_ppDecided[n];
	}
	s_ppDecidedTime[n] = level.time;
	s_ppDecided[n] = qfalse;
	if ( self->NPC->perfectParryDebounce > level.time || self->NPC->guardBreakTime > level.time )
	{
		return qfalse;
	}
	extern qboolean WP_NPCForceFree( const gentity_t *self, int forcePower );
	if ( self->client->ps.forcePowerMax > 0 && self->client->ps.forcePower <= 0 && !WP_NPCForceFree( self, FP_SABER_DEFENSE ) )//4B: blocks are Force-free for NPCs
	{
		return qfalse;
	}
	const int skill = AI_JediBlockSkill( self );
	extern cvar_t *g_saberDeterministic;
	if ( g_saberDeterministic && g_saberDeterministic->integer )
	{//JKEnhanced: no dice - only masters and bosses (blockSkill 5: boss characters, Desann/Luke/Tavion/
	 //Alora/Kyle, captain rank, or a .npc blockSkill 5) knock your swing away, and only when they READ it:
	 //they're in their defensive stance (the DEFEND tactic) and you're predictable - the same direction
	 //twice in a row, or a flurry of 5+ swings. Everyone else just blocks.
		const gentity_t *enemy = self->enemy;
		const int readSkill = ( G_SpeedLevel( self ) >= FORCE_LEVEL_3 ) ? 4 : 5;//4B.6: Speed 3 (Focus) - a skill-4 duelist reads you too
		if ( skill < readSkill || !enemy || !enemy->client )
		{
			return qfalse;
		}
		if ( self->NPC->duelRole != JT_DEFEND )
		{
			return qfalse;
		}
		//saberAttackChainCount goes up about twice per chained swing (PM_SetSaberMove and the chain
		//code both count it), so 10 is a real flurry of ~5 swings; 5 was firing after 2-3
		if ( self->NPC->sameBlockDirCount >= 2
			|| enemy->client->ps.saberAttackChainCount >= 10 )
		{
			s_ppDecided[n] = qtrue;
			self->NPC->perfectParryDebounce = level.time + 2500 - skill * 200;
		}
		return s_ppDecided[n];
	}
	//skill 1: never, 2: 6 %, 3: 12 %, 4: 18 %, 5: 24 % of blocks; harder on higher difficulty
	int chance = ( skill - 1 ) * 6 + G_DifficultyLevel() * 2;//(Jedi Master counts as 3)
	if ( G_SpeedLevel( self ) >= FORCE_LEVEL_3 )
	{//JKEnhanced 4B.6: Focus widens it (force.cfg speed.parryPct)
		chance = chance * G_ForceTuning()->speedParryPct / 100;
	}
	if ( skill > 1 && Q_irand( 0, 99 ) < chance )
	{
		s_ppDecided[n] = qtrue;
		self->NPC->perfectParryDebounce = level.time + Q_irand( 3000, 6000 ) - skill * 400;
	}
	return s_ppDecided[n];
}
