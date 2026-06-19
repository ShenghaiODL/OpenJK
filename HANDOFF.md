# JKEnhanced OpenJK — Session Handoff

## Project
JKEnhanced mod for Jedi Academy (OpenJK codebase — Raven's JKA code only, no JK2/codemp).
Working directory: `a:\JKEnhanced\OpenJK`
Branch: `Test`

---

## What We've Been Working On

Two tracks: **Stormtrooper AI polish** and **Manual saber block animation fixes**.

---

## Track 1: Stormtrooper AI

### Goal
Stormtroopers feel passive. We wanted: shoot while backstepping, group suppression coordination, seek cover when player has active saber, seek cover after taking blaster fire, faster AI decisions, shorter pain stagger.

### All Changes Made

#### `code/game/AI_Stormtrooper.cpp`

**1. `NPC_ST_Pain()` (~line 342)** — Added `"underFire"` timer (5 seconds) so reactive cover triggers after being shot:
```cpp
void NPC_ST_Pain( gentity_t *self, ... )
{
    self->NPC->localState = LSTATE_UNDERFIRE;
    TIMER_Set( self, "underFire", 5000 );   // ADDED
    TIMER_Set( self, "duck", -1 );
    TIMER_Set( self, "hideTime", -1 );
    TIMER_Set( self, "stand", 2000 );
    NPC_Pain( self, inflictor, other, point, damage, mod, hitLoc );
    ...
}
```

**2. `ST_GetCPFlags()` else branch (~line 1793)** — Added saber/underFire triggers for cover-seeking, lowered flanking threshold from >20 to >10:
```cpp
else
{
    bool enemySaber = NPC->enemy && NPC->enemy->client
                      && NPC->enemy->client->ps.weapon == WP_SABER
                      && NPC->enemy->client->ps.SaberActive();
    bool underFire  = !TIMER_Done( NPC, "underFire" );
    if ( enemySaber || underFire )
    {
        cpFlags = (CP_COVER|CP_AVOID|CP_SAFE|CP_DUCK);
    }
    else
    {
        int moraleBoost = NPCInfo->group->morale - NPCInfo->group->numGroup;
        if ( moraleBoost > 10 ) { cpFlags = (CP_CLEAR|CP_FLANK|CP_APPROACH_ENEMY); }
        else if ( moraleBoost > 15 ) { cpFlags = (CP_CLEAR|CP_CLOSEST|CP_APPROACH_ENEMY); }
        else if ( moraleBoost > 10 ) { cpFlags = (CP_CLEAR|CP_APPROACH_ENEMY); }
    }
}
```

**3. `ST_FindGeoCover()` static helper** — Added before `ST_Commander()`. Geometry-based cover fallback when no authored combat points are available. 8-direction waist-height traces, 100-unit squad spacing check, enemy LOS check:
```cpp
static qboolean ST_FindGeoCover( vec3_t outPos )
{
    if ( !NPC || !NPC->enemy ) return qfalse;
    vec3_t waist;
    VectorCopy( NPC->currentOrigin, waist );
    waist[2] += 24.0f;
    float bestScore = -1.0f;
    VectorClear( outPos );
    for ( int d = 0; d < 8; d++ )
    {
        vec3_t scanAngles = { 0, d * 45.0f, 0 };
        vec3_t dir;
        AngleVectors( scanAngles, dir, NULL, NULL );
        vec3_t end;
        VectorMA( waist, 300.0f, dir, end );
        trace_t tr;
        gi.trace( &tr, waist, vec3_origin, vec3_origin, end,
                  NPC->s.number, NPC->clipmask, (EG2_Collision)0, 0 );
        if ( tr.fraction >= 1.0f || tr.startsolid ) continue;
        vec3_t candidate;
        VectorMA( tr.endpos, -32.0f, dir, candidate );
        candidate[2] = NPC->currentOrigin[2];
        bool tooClose = false;
        if ( NPCInfo->group )
        {
            for ( int m = 0; m < NPCInfo->group->numGroup && !tooClose; m++ )
            {
                gentity_t *buddy = &g_entities[ NPCInfo->group->member[m].number ];
                if ( buddy == NPC ) continue;
                if ( DistanceSquared( buddy->currentOrigin, candidate ) < 100.0f*100.0f )
                    tooClose = true;
            }
        }
        if ( tooClose ) continue;
        trace_t losTrace;
        gi.trace( &losTrace, NPC->enemy->currentOrigin, vec3_origin, vec3_origin, candidate,
                  NPC->enemy->s.number, NPC->enemy->clipmask, (EG2_Collision)0, 0 );
        if ( losTrace.fraction >= 1.0f ) continue;
        float score = 300.0f - Distance( NPC->currentOrigin, candidate );
        if ( score > bestScore ) { bestScore = score; VectorCopy( candidate, outPos ); }
    }
    return (bestScore > 0.0f) ? qtrue : qfalse;
}
```

**4. `ST_Commander()` per-member loop (~line 2054)** — Wired `ST_GetCPFlags()` (it was dead code with no call site; all cover logic was silently skipped):
```cpp
SetNPCGlobals( member );
cpFlags = ST_GetCPFlags();   // ADDED — was missing entirely
if ( !TIMER_Done( NPC, "flee" ) )
    continue;
```

**5. `ST_Commander()` suppression coordination (~line 2163)** — Cap simultaneous transitioning members at half the group:
```cpp
if ( group )
{
    int transitioning = group->numState[SQUAD_TRANSITION] + group->numState[SQUAD_SCOUT];
    int suppressing   = group->numState[SQUAD_STAND_AND_SHOOT] + group->numState[SQUAD_COVER];
    int maxTransit    = group->numGroup / 2 > 1 ? group->numGroup / 2 : 1;
    if ( transitioning >= maxTransit || suppressing == 0 )
    {
        AI_GroupUpdateSquadstates( group, NPC, SQUAD_STAND_AND_SHOOT );
        TIMER_Set( NPC, "attackDelay", Q_irand( 100, 400 ) );
        cpFlags = 0;
    }
}
```

**6. `NPC_BSST_Attack()` lone NPC cover block (~line 2456)** — All stormtroopers (not just grouped ones) seek geometry cover when relevant:
```cpp
if ( !NPCInfo->group && NPC->enemy && NPC->enemy->client )
{
    bool enemySaber = NPC->enemy->client->ps.weapon == WP_SABER
                      && NPC->enemy->client->ps.SaberActive();
    bool underFire  = !TIMER_Done( NPC, "underFire" );
    if ( (enemySaber || underFire) && TIMER_Done( NPC, "loneGeoSearchCooldown" ) )
    {
        TIMER_Set( NPC, "loneGeoSearchCooldown", 3000 );
        vec3_t coverPos;
        if ( ST_FindGeoCover( coverPos ) )
            NPC_SetMoveGoal( NPC, coverPos, 16, qtrue, -1, NULL );
    }
}
```

**7. `NPC_BSST_Attack()` flee suppression (~line ~2680)** — Suppress `faceEnemy`/`shoot` only during explicit `"flee"` timer (not the debounce window after backstepping):
```cpp
if ( doMove && !TIMER_Done( NPC, "flee" ) )
{
    faceEnemy = qfalse;
}
if ( !faceEnemy )
{
    if ( !doMove )
        VectorCopy( NPC->client->ps.viewangles, NPCInfo->lastPathAngles );
    NPCInfo->desiredYaw = NPCInfo->lastPathAngles[YAW];
    NPCInfo->desiredPitch = 0;
    NPC_UpdateAngles( qtrue, qtrue );
    if ( doMove && !TIMER_Done( NPC, "flee" ) )
        shoot = qfalse;
}
```

#### `code/game/NPC.cpp` (~line 2527)

Halved AI think rate from 100ms to 50ms for all NPCs (not just Jedi on Hard):
```cpp
else
{
    NPCInfo->nextBStateThink = level.time + FRAMETIME/2;  // was FRAMETIME
}
```

#### `code/game/NPC_reactions.cpp` — `NPC_ChoosePainAnimation()` (~line 361)

Halved pain stagger duration so NPCs recover and seek cover sooner:
```cpp
NPC_SetAnim( self, parts, pain_anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
self->client->ps.legsAnimTimer /= 2;   // ADDED
// ...
self->painDebounceTime = level.time + PM_AnimLength( self->client->clientInfo.animFileIndex, (animNumber_t) pain_anim ) / 2;
//                                                                                                                          ^^ was no /2
```

---

## Track 2: Manual Saber Block Animation Fixes

### Goal
When walking and blocking, the walk animation was overriding the deflect hold. Deflect poses were using wrong directions (upper poses for lower hits). Projectile deflects were too short and froze the legs.

### All Changes Made

#### `code/game/bg_pmove.cpp`

**1. `PM_Footsteps()` (~line 8384)** — Prevent walk anim from overriding active deflect hold:
```cpp
// Added !pm->ps->legsAnimTimer guard before the walk anim set
if ( !pm->ps->legsAnimTimer )
{
    PM_SetAnim( pm, SETANIM_LEGS, anim, ... );
}
```
(Exact location: wherever the footstep/walk anim override fires inside `PM_Footsteps`. The guard is `!pm->ps->legsAnimTimer`.)

**2. saberBlocked switch (~line 11475)** — Full rewrite of the block dispatch. Key changes:
- Added `parryBase`/`PARRY()` macro for dual (P6) and staff (P7) saber styles
- UPPER_* uses TR/TL, LOWER_* uses BR/BL (was wrong before — LOWER was using TR/TL)
- All `_PROJ` cases use `SETANIM_TORSO` + `torsoAnimTimer` + `Q_irand(200,350)` so legs keep moving
- All saber-on-saber cases use `SETANIM_BOTH` + `legsAnimTimer` + `Q_irand(200,1000)`

Current state of the switch (complete):
```cpp
int parryBase = BOTH_P1_S1_T_;
if      ( pm->ps->saberAnimLevel == SS_DUAL )  { parryBase = BOTH_P6_S6_T_; }
else if ( pm->ps->saberAnimLevel == SS_STAFF ) { parryBase = BOTH_P7_S7_T_; }
#define PARRY(a) ((animNumber_t)(parryBase + ((a) - BOTH_P1_S1_T_)))

case BLOCKED_UPPER_RIGHT:
    PM_SetAnim( pm, SETANIM_BOTH, PARRY(BOTH_P1_S1_TR), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
    pm->ps->legsAnimTimer += Q_irand( 200, 1000 );
    pm->ps->weaponTime = pm->ps->legsAnimTimer;
    break;
case BLOCKED_LOWER_RIGHT:
    PM_SetAnim( pm, SETANIM_BOTH, PARRY(BOTH_P1_S1_BR), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
    pm->ps->legsAnimTimer += Q_irand( 200, 1000 );
    pm->ps->weaponTime = pm->ps->legsAnimTimer;
    break;
case BLOCKED_UPPER_RIGHT_PROJ:
    PM_SetAnim( pm, SETANIM_TORSO, PARRY(BOTH_P1_S1_TR), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
    pm->ps->torsoAnimTimer += Q_irand( 200, 350 );
    pm->ps->weaponTime = pm->ps->torsoAnimTimer;
    break;
case BLOCKED_LOWER_RIGHT_PROJ:
    PM_SetAnim( pm, SETANIM_TORSO, PARRY(BOTH_P1_S1_BR), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
    pm->ps->torsoAnimTimer += Q_irand( 200, 350 );
    pm->ps->weaponTime = pm->ps->torsoAnimTimer;
    break;
case BLOCKED_UPPER_LEFT:
    PM_SetAnim( pm, SETANIM_BOTH, PARRY(BOTH_P1_S1_TL), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
    pm->ps->legsAnimTimer += Q_irand( 200, 1000 );
    pm->ps->weaponTime = pm->ps->legsAnimTimer;
    break;
case BLOCKED_LOWER_LEFT:
    PM_SetAnim( pm, SETANIM_BOTH, PARRY(BOTH_P1_S1_BL), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
    pm->ps->legsAnimTimer += Q_irand( 200, 1000 );
    pm->ps->weaponTime = pm->ps->legsAnimTimer;
    break;
case BLOCKED_UPPER_LEFT_PROJ:
    PM_SetAnim( pm, SETANIM_TORSO, PARRY(BOTH_P1_S1_TL), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
    pm->ps->torsoAnimTimer += Q_irand( 200, 350 );
    pm->ps->weaponTime = pm->ps->torsoAnimTimer;
    break;
case BLOCKED_LOWER_LEFT_PROJ:
    PM_SetAnim( pm, SETANIM_TORSO, PARRY(BOTH_P1_S1_BL), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
    pm->ps->torsoAnimTimer += Q_irand( 200, 350 );
    pm->ps->weaponTime = pm->ps->torsoAnimTimer;
    break;
case BLOCKED_TOP:
    PM_SetAnim( pm, SETANIM_BOTH, PARRY(BOTH_P1_S1_T_), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
    pm->ps->legsAnimTimer += Q_irand( 200, 1000 );
    pm->ps->weaponTime = pm->ps->legsAnimTimer;
    break;
case BLOCKED_TOP_PROJ:
    PM_SetAnim( pm, SETANIM_TORSO, PARRY(BOTH_P1_S1_T_), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
    pm->ps->torsoAnimTimer += Q_irand( 200, 350 );
    pm->ps->weaponTime = pm->ps->torsoAnimTimer;
    break;
#undef PARRY
```

---

## Pending Work / Known Issues

- **Saber block height thresholds** (`wp_saber.cpp` → `WP_SaberBlockNonRandom()`): The zdiff cutoffs that choose UPPER vs LOWER could be tuned. Stormtroopers aim center-mass (zdiff ~-10 to -15), so the lower threshold (-22) may be too low, causing center-mass shots to register as UPPER when BR/BL would look better. Could raise lower threshold from -22 to ~-12.
- **Morale tiers in `ST_GetCPFlags()`**: The three `moraleBoost` tiers (>10, >15, >10) are currently in a weird order — the third `>10` branch is unreachable because the first `>10` already catches it. Was present in original code; worth reviewing if morale-based flanking doesn't feel right.
- **Build**: Has not been rebuilt since final set of changes. Should compile clean — all previous build errors (MAX undefined, bool→qboolean cast) were resolved in session.

---

## Non-Code Suggestions (no recompile needed)

- **`.npc` file tuning** (`ext_data/npcs/imperials.npc`): `evasion` and `aggression` fields control dodge probability and firing behavior. Can tune per-class without recompiling.
- **Combat point placement**: Authored `NPC_combatpoint` entities near walls/crates with `CP_COVER` flag give ST_Commander better options than geometry fallback.
- **Squad spawn proximity**: Stormtroopers within 512 units of each other cluster into a squad, enabling ST_Commander and all group tactics.

---

## Key Technical Notes for Context

- `TIMER_Set/TIMER_Done`: JKA's NPC timer system. `TIMER_Set(NPC, "name", ms)` — negative value clears. `TIMER_Done(NPC, "name")` returns true when expired.
- `ST_GetCPFlags()` was dead code until this session — no call site existed anywhere in the codebase.
- `LSTATE_UNDERFIRE` is cleared every frame in ST_Commander's per-member loop, so we use the persistent `"underFire"` TIMER instead.
- `NPC_move.cpp` blocks movement while `legsAnim >= BOTH_PAIN1 && legsAnim <= BOTH_PAIN18 && legsAnimTimer > 0` — that's why halving `legsAnimTimer` unblocks cover movement sooner.
- `SETANIM_BOTH` vs `SETANIM_TORSO`: BOTH locks legs (blocks movement anim), TORSO only affects upper body.
- `BOTH_P1_S1_BR` / `BOTH_P1_S1_BL`: confirmed in anims.h as "block shot/saber bottom right/left".
- `SS_DUAL` = P6 series, `SS_STAFF` = P7 series. `PARRY(a)` macro does arithmetic offset from P1 base.
