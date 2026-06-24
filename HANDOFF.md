# JKEnhanced OpenJK — Session Handoff

## Project
JKEnhanced mod for Jedi Academy (OpenJK codebase — Raven's JKA code only, no JK2/codemp).
Working directory: `a:\JKEnhanced\OpenJK`
Branch: `Test`

---

## What We've Been Working On

Three tracks: **Stormtrooper AI polish**, **Manual saber block animation fixes**, and **AI modernization + NPC variety**.

---

## Track 1: Stormtrooper AI

### All Changes Made

#### `code/game/AI_Stormtrooper.cpp`

**`NPC_ST_Pain()` (~line 342)** — Added `"underFire"` timer (5 seconds) so reactive cover triggers after being shot.

**`ST_GetCPFlags()` else branch (~line 1793)** — Added saber/underFire triggers for cover-seeking, lowered flanking threshold. `ST_GetCPFlags()` was dead code with no call site — wired it into `ST_Commander()` per-member loop.

**`ST_FindGeoCover()` static helper** — Geometry-based cover fallback. 8-direction waist-height traces, squad spacing check (100u), eye-height enemy LOS check (`CONTENTS_SOLID`), dot-product direction rejection (won't pick cover in the enemy's direction). Scores by distance-from-enemy minus half-distance-to-NPC.

**`ST_Commander()` suppression coordination** — Cap simultaneous transitioning members at half the group; rest forced to `SQUAD_STAND_AND_SHOOT`.

**`NPC_BSST_Attack()` lone NPC cover block** — Solo stormtroopers also seek geo-cover when facing saber or under fire.

**Cornered strafing** — When `ST_FindGeoCover` fails (no cover anywhere), NPC erratically strafes away from `closestBuddy` rather than standing frozen.

**Buddy-pair suppression** — When a member moves to geo-cover in `ST_Commander`, their `closestBuddy` is pushed to `SQUAD_STAND_AND_SHOOT` immediately.

**`ST_ResolveBlockedShot()` angle finding** — When duck/stand coordination fails, laterally traces ±64 units to find a clear shot; briefly strafes that direction instead of resetting all timers.

#### `code/game/NPC.cpp` (~line 2527)

Reactions-stat-driven think rate: `220 - reactions*30` ms (reactions 1→190ms, 3→130ms, 5→70ms). Applies to all NPC types.

#### `code/game/AI_Utils.cpp`

Casualty morale penalty: −5 `moraleAdjust` per dead member removed from group, capped at −20. Decays naturally at 1/sec.

#### `code/game/NPC_reactions.cpp` — `NPC_ChoosePainAnimation()`

Halved pain stagger duration so NPCs recover and seek cover sooner (`legsAnimTimer /= 2`, same for `painDebounceTime`).

---

## Track 2: Manual Saber Block Animation Fixes

#### `code/game/bg_pmove.cpp`

**`PM_Footsteps()`** — Added `!pm->ps->legsAnimTimer` guard to prevent walk anim from overriding an active deflect hold.

**`saberBlocked` switch (~line 11475)** — Full rewrite:
- `PARRY()` macro handles dual (P6) and staff (P7) style offsets from P1 base
- UPPER_* uses TR/TL, LOWER_* uses BR/BL (was wrong before)
- All `_PROJ` cases: `SETANIM_TORSO` + `torsoAnimTimer` + `Q_irand(200,350)` — legs keep moving
- All saber-on-saber cases: `SETANIM_BOTH` + `legsAnimTimer` + `Q_irand(200,1000)`

---

## Track 3: AI Modernization + NPC Variety

### `.npc` File Tuning (`Mod Files/ext_data/npcs/`)

`imperials.npc`:
- `StormPilot`: `reactions 3 → 2` (~160ms think rate, vehicle operator not infantry)

`misc.npc`:
- `Rebel`, `Rebel2`: `reactions 3 → 2` (~160ms, militia not trained soldiers)

### NPC_StormtrooperRandom System

**New classname `NPC_StormtrooperRandom`** — weighted random pool of stormtrooper variants. Existing `NPC_Stormtrooper` spawners in all maps are untouched.

**`Mod Files/ext_data/npcs/stormtrooper_random.npc`** (NEW FILE) — 4 variant blocks:
- `stormtrooper_rifle` — standard blaster, baseline stats
- `stormtrooper_heavy` — WP_REPEATER, health 55, aim 2, aggression 5
- `stormtrooper_officer` — `customSkin officer`, health 50, reactions 4, aim 3, intelligence 3, rank ensign, stofficer1 sounds
- `stormtrooper_grenadier` — WP_THERMAL + WP_BLASTER, aggression 5

**`Mod Files/ext_data/npcs/stormtrooper_random.cfg`** (NEW FILE) — Weight table:
```
stormtrooper_rifle      5
stormtrooper_heavy      2
stormtrooper_officer    2
stormtrooper_grenadier  1
```
Adding a new variant = add a block to the `.npc` + a line to the `.cfg`. No C++ required.

**`code/game/NPC_spawn.cpp`** (~line 2800) — `ST_LoadRandomVariants()` reads the cfg via `gi.FS_ReadFile`, parses name+weight pairs, hardcoded fallback if file missing. `SP_NPC_StormtrooperRandom()` does a weighted `Q_irand` pick.

**`code/game/g_spawn.cpp`** — Forward declaration + `{"NPC_StormtrooperRandom", SP_NPC_StormtrooperRandom}` in `spawns[]` table.

---

## Ready for Next Session

**Data-driven weapon damage (Part 2)** — see plan file. Goal: move NPC difficulty damage constants (`BLASTER_NPC_DAMAGE_EASY` etc.) from `weapons.h` hardcodes into `weapons.dat` fields, so weapon stat variants require no C++ changes.

---

## Key Technical Notes

- `TIMER_Set/TIMER_Done`: NPC timer system. `TIMER_Set(NPC, "name", ms)` — negative clears. `TIMER_Done` returns true when expired.
- `ST_GetCPFlags()` was dead code (no call site) until wired in — all morale-based cover/flank behavior was silently skipped before.
- `LSTATE_UNDERFIRE` is cleared every frame in ST_Commander loop, so the persistent `"underFire"` TIMER is used for cross-frame state.
- `COM_ParseString` / `COM_ParseInt` in `q_shared.h` — return `qboolean` (qtrue = error/EOF). File reading uses `gi.FS_ReadFile` + `gi.FS_FreeFile`.
- `weaponData[]` array is indexed by `weapon_t` enum. `gi.FS_ReadFile("ext_data/weapons.dat", ...)` is the load entry point in `g_weaponLoad.cpp`.
- `SETANIM_BOTH` locks legs (blocks movement anim); `SETANIM_TORSO` only affects upper body.
- `SS_DUAL` = P6 series, `SS_STAFF` = P7 series. `PARRY(a)` macro does arithmetic offset from P1 base.
- Reactions stat formula: `220 - reactions*30` ms; reactions 1→190ms, 3→130ms, 5→70ms.
- `moraleAdjust` is the event lever (decays ±1/sec); `morale` is the per-frame recalculated result feeding `ST_GetCPFlags()`.
