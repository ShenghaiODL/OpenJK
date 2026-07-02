# JKEnhanced OpenJK — Session Handoff

## Project
JKEnhanced mod for Jedi Academy (OpenJK codebase — Raven's JKA code only, no JK2/codemp).
Working directory: `a:\JKEnhanced\OpenJK`
Branch: `Test`

External workspace directories used this session (outside the git repo):
- `C:\Users\toitl\OneDrive\Desktop\JKA Workspace\combined base with jkenhanced` — the
  user's actively-deployed/tested game folder. Treated as more current than `Mod Files/`
  in this repo for some content (see Track 6) — always check both when a data file seems
  stale.
- `C:\Users\toitl\OneDrive\Desktop\JKA Workspace\sheng ultimate weapons` — extracted copy
  of the real "Ultimate Weapons Mod" pk3, used as the sole workspace for VFX/effects work
  (Track 7). Deliberately kept separate from the repo and from `combined base`.
- `C:\Users\toitl\OneDrive\Desktop\JKA Workspace\big update` — a staging folder holding
  copies of this session's new/modified *data* files only (not code), mirroring their
  relative paths (`ext_data\weapons.dat`, `default.cfg`, `ui\controls.menu`,
  `ui\ingamecontrols.menu`) so they can be dropped into a deployment target later.

---

## What We've Been Working On (prior session)

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

**Known issue (found this session, not yet fixed)**: user reports the block "stagger" sometimes removes the ability to block again for a beat, which reads as frustrating. User is considering a proper "parry" system as a future redesign — explicitly deferred, not scheduled yet.

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

**`code/game/NPC_spawn.cpp`** (~line 2800) — `ST_LoadRandomVariants()` reads the cfg via `gi.FS_ReadFile`, parses name+weight pairs, hardcoded fallback if file missing. `SP_NPC_StormtrooperRandom()` does a weighted `Q_irand` pick. **This session**: removed two unconditional `Com_Printf` debug dumps (variant count + per-variant list) that were spamming console on every spawn — the yellow fallback-warning print stays (only fires when the cfg file is missing, a genuine diagnostic).

**`code/game/g_spawn.cpp`** — Forward declaration + `{"NPC_StormtrooperRandom", SP_NPC_StormtrooperRandom}` in `spawns[]` table.

**Note**: `combined base with jkenhanced` did NOT have this NPC system as of this session's start — `stormtrooper_random.npc`/`.cfg` only existed in the repo's `Mod Files/`. Also confirmed `imperials.npc`/`misc.npc` in `combined base` have *additional* undocumented tuning beyond what's listed here (health cuts, model/skin swaps to `stormie`/`chimaera_rgb`) that never made it into this repo — worth reconciling in a future session if that divergence matters.

---

## Track 4: Repeater Blocking Fix

**Problem**: saber block couldn't reliably deflect Imperial Repeater fire. Root cause:
repeater `firetime` is 50ms (`Mod Files/ext_data/weapons.dat`), but the post-block
recovery window shared with saber-vs-saber parries (`parryDebounce[]`,
`code/game/wp_saber.cpp:346-352`) is 100ms at low Force Saber Defense — so `WP_SaberStartMissileBlockCheck`
(`wp_saber.cpp:7899`) was skipping most bolts in a burst entirely.

**Fix**: added `EVASION_MISSILE_PARRY` (`wp_saber.h`), a separate
`missileParryDebounce[] = {100, 50, 25, 15}` (`wp_saber.cpp:346` area), branched on the
existing `missileBlock` bool in `WP_SaberBlockNonRandom` (`wp_saber.cpp:7746`) and
`Jedi_ReCalcParryTime` (`AI_Jedi.cpp:2651`, player-only branch — NPCs don't consume a
Force-power pool to block so no NPC-side change needed). Saber-vs-saber parry timing is
completely untouched.

**Status per this session's testing**: "SEEMS to be better, it's still not 100%." No new
specifics given. Known residual gap: Defense level 0 still shares the 100ms window (2x the
repeater's 50ms cadence) — that's the likely next lever if the user wants to chase this
further, but not scheduled.

---

## Track 5: Data-Driven Weapons + Class-Based Loadout

### Reserved custom weapon slots
`code/game/weapons.h` — added `WP_CUSTOM_1` through `WP_CUSTOM_16` to the `weapon_t` enum
(indices 35-50), purely additive. Seeded all 6 default-stat arrays and
`playerUsableWeapons[]` in `g_weaponLoad.cpp` (51 entries each). **All 16 slots now have
real `weapons.dat` blocks** — full clones of `WP_BLASTER` (same model/icon/fire-behavior),
differing only in `weaponclass weapon_customN` (unique classname) and `weaponslot NONE`
(unassigned until someone turns one into a real distinct weapon by editing its block).

### Item registration for the reserved slots
`code/game/g_itemLoad.cpp` — `G_SynthesizeCustomWeaponItems()` runs at level init (called
from `InitGame`, `g_main.cpp`, right after `IT_LoadItemParms()`), unconditionally
ensuring every `weapon_t` from `WP_SABER` to `WP_CUSTOM_16` has a `bg_itemlist` entry,
synthesizing a minimal placeholder into one of 16 reserved `ITM_CUSTOM_N_PICKUP` slots
(`g_items.h`) for any that don't. **Do not gate this on whether the weapon has real
`weapons.dat` content** — that was tried and caused a crash (see "Crashes fixed" below).

### Class-based loadout
- `code/game/weapons.h` — `weaponClass_t` enum (`WPCLASS_NONE/PISTOL/MEDIUM/HEAVY/THROWABLE`),
  `loadoutClass` field on `weaponData_t`. Parsed via new `weaponslot` key
  (`g_weaponLoad.cpp`, `WPN_WeaponSlot`) — deliberately a different key name from the
  pre-existing unrelated `weaponclass` key (which sets `classname`, not loadout class).
  All 34 real weapons tagged in `weapons.dat` (PISTOL: blaster pistol, bryar pistol;
  MEDIUM: blaster, cycler rifle, bowcaster, repeater, DEMP2; HEAVY: rocket launcher,
  Z6 rotary, flechette, concussion rifle; THROWABLE: thermal, tripmine, detpack; saber is
  never tagged — excluded from this system entirely, see below).
- `code/game/g_items.cpp`, `Pickup_Weapon` — walking over a weapon of a class already held
  does nothing (item stays in world) unless `+use` is pressed, in which case it swaps
  (drops the old one via new `G_DropClassWeapon`, grants the new one, switches to it
  immediately). **This session's testing found and fixed**: the swap only fires on a
  *press*, not while `+use` is held — uses the existing generic `gentity_t::useDebounceTime`
  field (`g_shared.h:1128`, same idiom as `g_turret.cpp:2396-2398`) rather than checking
  raw button state every frame, which was previously causing a drop/re-pickup cascade.
  Also generalized `Touch_Item`'s anti-instant-repickup check (`ent->delay`) — it was
  hardcoded to only protect `WP_SABER`/`WP_EMPLACED_GUN`, so any other dropped weapon
  (including `dropweapon`'s output) could be instantly re-picked-up by the same player;
  now applies to any `IT_WEAPON` item, and `G_DropClassWeapon` sets `dropped->delay =
  level.time + 500` to use it.
- `code/game/g_cmds.cpp` — new `dropweapon` command (`Cmd_WeaponDrop_f`), bound to `G` by
  default (`Mod Files/default.cfg`). Intentionally cannot drop the saber (`WP_SABER` is
  excluded from the loadout-class system, matching "we do not collect sabers" — the
  player's saber is only ever lost/reclaimed via the existing, separately-gated
  saber-throw mechanic, `g_saberPickuppableDroppedSabers` cvar, default off).
- `code/cgame/cg_weapons.cpp` — number keys 1-5 rebound to per-slot selection
  (`CG_WeaponSlot_f`/`CG_ResolveWeaponSlot`, new), not per-weapon: 1=saber, 2=pistol,
  3=medium, 4=heavy, 5=throwable. Resolves to whichever owned weapon occupies that class,
  preferring the currently-active one if already in that class (so existing per-weapon
  cycle groups, e.g. thermal/tripmine/detpack and blaster_pistol/bryar_pistol, still work
  through the shared `CG_SelectWeaponNum` core, refactored out of the old `CG_Weapon_f`).
- **Known accepted gap**: several non-pickup grant paths (the `give` cheat, ICARUS scripted
  `SET WEAPON`, emplaced-gun mounting, vehicle mounting) bypass the class-conflict check
  entirely, since they write `ps.weapons[]` directly. Deliberately left alone — these are
  debug/scripted paths where enforcing the restriction risks breaking intended level
  design, and the drop/re-pickup fix above means even a bypassed double-grant can't corrupt
  state, just leaves both weapons owned until manually dropped.

### UI changes
- `code/ui/ui_main.cpp`, `UI_AddWeaponSelection` — the pre-mission "Weapon Select" screen
  (`ingamewpnselect.menu`) had its own independent two-slot bonus-weapon system
  (`uiInfo.selectedWeapon1/2`) that didn't know about loadout classes; now checks a new
  local `UI_GetWeaponLoadoutClass()` (deliberately NOT reusing `weaponData[]` — that global
  is only defined in `code/ui/gameinfo.cpp`, which isn't compiled into this executable
  target, so referencing it directly fails to link) and bumps a same-class slot before
  accepting a new pick, mirroring the in-mission swap behavior.
- `Mod Files/ui/controls.menu` and `ingamecontrols.menu` (both under `combined base with
  jkenhanced\ui\`, not the repo) — WEAPON BINDING section consolidated from 13
  individual per-weapon rows down to 8 slot-based rows (saber/pistol/medium/heavy/throwable/
  next/prev/drop). New "Medium Weapon"/"Heavy Weapon"/"Drop Weapon" labels use plain
  literal text (existing pattern in this menu format, e.g. `text "Saber Block and Alt
  Fire"`) rather than string-table keys, and their descriptions deliberately avoid naming
  specific vanilla weapons (future-proofing for when more weapons land in each class).
- `code/ui/ui_shared.cpp`, `g_bindCommands[]` — the controls-menu bind-display/rebind
  system resolves commands through this hardcoded array (`BindingIDFromName`); added
  `dropweapon` and `weaponslot 1`-`5` so those rows show a real key and are rebindable
  (previously showed `???` and couldn't be changed).
- `Mod Files/default.cfg` (and `combined base`'s copy) — keys 1-5 now bind
  `weaponslot 1`-`5` instead of `weapon 1`-`5`; new `bind g dropweapon`.

### Crashes fixed getting here (chronological, for context if something regresses)
1. **"Couldn't find item for weapon 35"** (`bg_misc.cpp:262`, `FindItemForWeapon`) — no
   `bg_itemlist` entry for the new `WP_CUSTOM` slots. Root-caused to `NPC_WeaponsForTeam()`
   returning a 32-bit `int` bitmask, consumed by two loops
   (`NPC_stats.cpp:1383`/`NPC_PrecacheWeapons`, `NPC_spawn.cpp:945`/`NPC_SetWeapons`) that
   iterated `curWeap` up to `WP_NUM_WEAPONS` (now 51) doing `1 << curWeap` — shifting a
   32-bit int by ≥32 is UB and aliases on x86 (`1<<35` behaves as `1<<3`/`WP_BLASTER`), so
   any NPC with the blaster bit set (i.e. almost any stormtrooper) spuriously looked like
   it also owned weapon 35. **Fixed**: both loops now cap at `curWeap < 32`. Also added
   `G_SynthesizeCustomWeaponItems()` (see above) so an empty reserved slot never lacks an
   item regardless.
2. **Read access violation in `G_Alloc`, `g_debugalloc` was nullptr** — the synthesis call
   was originally placed in `GetGameAPI()` (`g_main.cpp:944`), which runs once at DLL load
   before cvars are registered. Moved to `InitGame()` (`g_main.cpp:782`), right after
   `IT_LoadItemParms()`, which runs per-level after cvars exist.
3. A first attempt gated the synthesis on `weaponData[wp].classname[0]` being set (i.e.
   only synthesize for weapons with a real `weapons.dat` block) — this reintroduced the
   original crash for any *still-empty* reserved slot, since none had real blocks at that
   point. Removed the gate; now unconditional per slot (see above). This became moot once
   all 16 slots got real blaster-clone blocks, but the unconditional check stays as a
   safety net for any future 17th+ slot or genuinely-still-empty state.

---

## Track 6: Saboteur Cloak Rework

**Direction**: cloak less often, but stronger while active — harder to see, and able to
fire while cloaked (previously could do neither).

- `code/game/ai.h` — `Saboteur_Decloak`'s default `uncloakTime` raised `2000 → 8000` (the
  cooldown before a Saboteur is allowed to cloak again after any decloak trigger).
- `code/cgame/cg_players.cpp` (~line 4923) — the fallback (non-refractive-renderer) cloak
  visual dimmed from full 255 alpha to ~35% (`shaderRGBA` scaled to 90), via the same
  `RF_RGB_TINT` mechanism already used for the uncloak fade transition just above it.
- **Regression found + fixed this session**: removing the forced-decloak-before-fire calls
  (originally in `AI_Stormtrooper.cpp`'s attack-think) to let Saboteurs fire while cloaked
  exposed a separate, pre-existing generic block — `NPC_combat.cpp:1289-1294`, inside
  `WeaponThink()`, unconditionally returns if `PW_CLOAKED` is active, for any NPC. Fixed by
  exempting `CLASS_SABOTEUR` specifically (everything else using this powerup, e.g. Shadow
  Trooper, keeps the block). Separately, removing those same two call sites also meant
  nothing ever re-polled the `"decloakwait"` timer set in `NPC_combat.cpp:497-501` on
  engaging an enemy — Saboteurs would cloak once and never decloak again. Fixed by adding
  an unconditional per-tick `Saboteur_Decloak(NPC)` call back into the attack-think loop
  (`AI_Stormtrooper.cpp`) — safe to call every tick since the function already no-ops via
  its own internal `TIMER_Done` check until the wait timer actually expires.
- User note: cloak visibility "might need more adjusting later... will wait until I swap
  in the model to confirm one way or the other" — model swap is the user's own task
  (5b, not started).

---

## Track 7: Ultimate Weapons Mod Effects (external workspace only)

**Scope**: this work happens entirely in
`C:\Users\toitl\OneDrive\Desktop\JKA Workspace\sheng ultimate weapons\` — never in the
repo or `Mod Files/`. Confirmed early this session that the repo's own
`Mod Files/effects/blaster/*` and `thermal/explosion.efx` are NOT the real Ultimate
Weapons Mod content (simplified rewrites, missing the master
`shaders/UltimateWeapons.shader` and ~194 dependent files) — irrelevant now since this
track abandoned the repo entirely per the user's direction and works from the real
extracted pk3 content instead.

### Thermal detonator (`effects/thermal/explosion.efx`) — tuned this session
User-reported issues from an in-game screenshot: scorch decal visible through terrain,
effect "over the top" (too much smoke), screen shake too strong. Applied:
- `Decal "WallDamage"` size `200 230 → 100 130` (engine hardcodes a 20-unit decal
  projection depth, `code/cgame/cg_marks.cpp:171`, not adjustable via `.efx` — reducing
  radius is the available content-level lever to reduce bleed-through, doesn't fix decal
  clipping in general).
- `SmokeThick` (the dominant smoke group — by far the longest-lived and largest of the
  three cloud/smoke particle blocks): `count 10 12 → 5 6`, `life 1e4-2.2e4 → 4000 7000`,
  size end `430 570 → 260 340`.
- `ExplosionCloud` `count 12 16 → 8 10` (secondary trim, applied alongside rather than
  held back — no live iterative testing loop this session to validate incrementally).
- `CameraShake "Shake"`: `bounce 5 9 → 2 4`, `radius 3000 → 1200`. `life` left alone
  (800-1000ms) — intensity/range were the complaints, not duration.

**Not yet tested in-game** — user said they probably wouldn't get to it the same night
these changes were made. No deployment path from this working directory into a loadable
pk3/GameData folder has been set up yet; that's still an open step whenever testing happens.

### Blaster VFX — not started
Item 3 from the original checklist (blaster muzzle/bolt/impact effects reading as "awful").
Not touched this session; same working-directory scope applies whenever it's picked up.

---

## Ready for Next Session

- **Test Track 5's swap/drop fixes and Track 6's cloak fixes in-game** — none of this
  session's final bugfix-pass changes (Items A/C/D from the latest plan) have been
  rebuilt/tested yet as of this writing.
- **Backpedaling repeater troopers won't fire** — user flagged as a recurring/known issue,
  not a regression from this session. Investigated `AI_Stormtrooper.cpp`'s fire-decision
  logic, `NPC_combat.cpp`'s `WeaponThink()`/LOS checks, and the HANDOFF Track 1 additions
  (`"underFire"` timer, `ST_GetCPFlags`) — no code found that explicitly gates fire on
  enemy movement direction. Needs live NPC debug tracing (log `shoot`/`enemyLOS`/`enemyCS`
  frame-by-frame against a backpedaling player), not further static reading.
- **Saber block stagger / possible parry system** — user's own words: "for later." No
  design work done yet.
- **Blaster VFX tuning** (Item 3) — not started, same `sheng ultimate weapons` workspace
  scope as Track 7's thermal work.
- **Saboteur model swap** (Item 5b) — user's own task, not started as of this session.
- **Deployment path** for `sheng ultimate weapons` and `big update` folder content into an
  actual loadable pk3/GameData location — never set up, needed before any of Track 7's
  changes or the `big update` data files can actually be tested.
- The previously-flagged **"Data-driven weapon damage (Part 2)"** idea (moving
  `*_NPC_DAMAGE_EASY/NORMAL/HARD` macros from `weapons.h` into data) is **deprioritized** —
  user reconsidered given the class-based loadout system now exists and may want any
  future data-format work to fold into that instead of being done standalone.

---

## Key Technical Notes

- `TIMER_Set/TIMER_Done`: NPC timer system. `TIMER_Set(NPC, "name", ms)` — negative clears. `TIMER_Done` returns true when expired.
- `ST_GetCPFlags()` was dead code (no call site) until wired in — all morale-based cover/flank behavior was silently skipped before.
- `LSTATE_UNDERFIRE` is cleared every frame in ST_Commander loop, so the persistent `"underFire"` TIMER is used for cross-frame state.
- `COM_ParseString` / `COM_ParseInt` in `q_shared.h` — return `qboolean` (qtrue = error/EOF). File reading uses `gi.FS_ReadFile` + `gi.FS_FreeFile`.
- `weaponData[]` array is indexed by `weapon_t` enum. `gi.FS_ReadFile("ext_data/weapons.dat", ...)` is the load entry point in `g_weaponLoad.cpp`. **This global is separately defined per-module** (`code/ui/gameinfo.cpp`, compiled into game/cgame but NOT into the executable that hosts `code/ui/ui_main.cpp`) — don't reference it from `ui_main.cpp` directly, it won't link.
- `SETANIM_BOTH` locks legs (blocks movement anim); `SETANIM_TORSO` only affects upper body.
- `SS_DUAL` = P6 series, `SS_STAFF` = P7 series. `PARRY(a)` macro does arithmetic offset from P1 base.
- Reactions stat formula: `220 - reactions*30` ms; reactions 1→190ms, 3→130ms, 5→70ms.
- `moraleAdjust` is the event lever (decays ±1/sec); `morale` is the per-frame recalculated result feeding `ST_GetCPFlags()`.
- `gentity_t` (not `gclient_t`) carries the generic per-entity debounce/timing fields —
  `delay`, `count`, `useDebounceTime`, `attackDebounceTime`, etc. (`g_shared.h:1115-1129`).
  `useDebounceTime` in particular is the established "press, not hold" idiom elsewhere in
  this codebase (`g_turret.cpp:2396-2398`) — check it before acting, set a short cooldown
  after.
- `bg_itemlist[]`/`items.dat` loading is **game-module-only** (`IT_LoadItemParms`, called
  from `InitGame` in `g_main.cpp`, never from the shared `gameinfo.cpp`/`GI_Init` that
  cgame/ui also use) — don't assume cgame or ui have their own independently-populated copy.
- `NPC_WeaponsForTeam()` returns a **32-bit `int` bitmask** — any loop consuming it with
  `1 << weaponIndex` must stay under 32 or hit undefined-behavior shift aliasing on x86.
- Console command `uimenu <name>` force-opens a named menu directly (e.g.
  `uimenu ingameWpnSelect` for the pre-mission Weapon Select screen) without needing to
  trigger the real mission-transition flow — useful for testing menus in isolation.
