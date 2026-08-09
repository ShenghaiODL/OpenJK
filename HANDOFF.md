# JKEnhanced OpenJK — Session Handoff

## Project
JKEnhanced mod for Jedi Academy (OpenJK codebase — Raven's JKA code only, no JK2/codemp).
Branch: `working-build-between-2-pcs` (the old `Test` branch is retired and deleted —
it was a duplicate of this branch's history with nothing unique on it; everything
now happens directly on `working-build-between-2-pcs`).

**Two machines share this repo via GitHub** — either can have the more recent commits
at any given time, so always check `git log`/`git status` fresh rather than assuming.

| | Desktop (home PC) | Laptop |
|---|---|---|
| Windows user | `Sheng` | `toitl` |
| Monitor | 1440p | 1080p |
| Working directory | `a:\JKEnhanced\OpenJK` | `C:\Users\toitl\OneDrive\Documents\GitHub\OpenJK` |

**Machine this session is on: Laptop.** Update this line at the start of each
session to say which physical machine you're on — the Windows username (`whoami`) or
home directory path is the reliable tell if it's unclear.

**Standing workflow, after every plan's changes are made (not just this one — every
future plan too):**
1. Copy every edited *asset* file (data/`.efx`/`.shader`/etc. — not code, code stays in
   git) into `Drive Mirror\Sheng's JK Enhanced\Staged and changed files\`, mirroring its
   relative path under whichever content root it came from (`Sheng's Ultimate Weapons
   Redesign\...` or `Mod Assets (Including Base Files)\...`). Copies, not moves — the
   real files stay in place and keep working; this folder exists purely so the *specific
   changed files* can be picked out and moved to the other PC individually, without
   syncing/duplicating the whole multi-gigabyte asset tree and without risk of dragging
   along third-party/non-permissioned content that happens to sit nearby.
2. Update this file (`HANDOFF.md`) with what changed.
3. Update `README.md` with player-facing patch notes for anything gameplay-visible.

The "Staged and changed files" folder is a running staging area, not cleared between
plans — check what's already in it before assuming it's empty, and don't assume
everything in it is from the most recent plan.

**Asset workflow is moving to a Google-Drive-mirrored folder** — old external
OneDrive workspace paths below are being superseded by
`C:\Users\toitl\OneDrive\Desktop\Drive Mirror\Sheng's JK Enhanced\` (synced via Google
Drive to both machines; local junction/copy paths may differ slightly per machine).
Once the migration is confirmed complete, `Mod Files/`/`Game Assets/` in this repo
(already `.gitignore`d, never actually tracked) get deleted and this section gets
trimmed down to just the Drive Mirror folder. Mapping so far:
- `combined base with jkenhanced` → `Mod Assets (Including Base Files)` (confirmed 1:1)
- `sheng ultimate weapons` → `Sheng's Ultimate Weapons Redesign` (confirmed 1:1)
- `big update` → **stale, no mapping** — its contents (`default.cfg`, `ext_data/
  weapons.dat`, `ui/controls.menu`, `ui/ingamecontrols.menu`, all dated Jul 1, plus an
  unrelated 2018-dated `st_deathtrooper` asset bundle) predate Track 10's later
  weapons.dat edits and don't match anything current. Safe to ignore/delete.

Old paths (kept here only until every reference elsewhere in this doc is migrated —
see the per-Track notes below for what still points at these):
- `C:\Users\toitl\OneDrive\Desktop\JKA Workspace\combined base with jkenhanced` (also
  seen as `C:\Users\Sheng\OneDrive\Desktop\JKA Workspace\...` on the desktop — same
  OneDrive-synced content, different username in the path)
- `C:\Users\toitl\OneDrive\Desktop\JKA Workspace\sheng ultimate weapons`
- `C:\Users\toitl\OneDrive\Desktop\JKA Workspace\big update` (stale, see above)

A separate phased implementation plan (saber styles, disarm/surrender AI, companion AI,
weapon reworks) lives outside the repo at
`C:\Users\Sheng\.claude\plans\alright-time-to-actually-nifty-rossum.md` — check it for
per-phase status before starting new work in those areas; Tracks 8-10 correspond to
Phases 1/2/4 of that plan.

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

**Known issue (found this session) — RESOLVED by Track 8.** User reported the block
"stagger" sometimes removed the ability to block again for a beat, which read as
frustrating. The proper "parry" system floated here as a future redesign is exactly
what Track 8's "Saber combat" rework became (perfect parry, HUD cooldown bar, split
saber/blaster cooldowns) — confirmed fixed by the user's own in-game testing.

---

## Track 3: AI Modernization + NPC Variety

### `.npc` File Tuning (`Mod Assets (Including Base Files)/ext_data/npcs/`)

`imperials.npc`:
- `StormPilot`: `reactions 3 → 2` (~160ms think rate, vehicle operator not infantry)

`misc.npc`:
- `Rebel`, `Rebel2`: `reactions 3 → 2` (~160ms, militia not trained soldiers)

### NPC_StormtrooperRandom System

**New classname `NPC_StormtrooperRandom`** — weighted random pool of stormtrooper variants. Existing `NPC_Stormtrooper` spawners in all maps are untouched.

**`Mod Assets (Including Base Files)/ext_data/npcs/stormtrooper_random.npc`** (NEW FILE) — 4 variant blocks:
- `stormtrooper_rifle` — standard blaster, baseline stats
- `stormtrooper_heavy` — WP_REPEATER, health 55, aim 2, aggression 5
- `stormtrooper_officer` — `customSkin officer`, health 50, reactions 4, aim 3, intelligence 3, rank ensign, stofficer1 sounds
- `stormtrooper_grenadier` — WP_THERMAL + WP_BLASTER, aggression 5

**`Mod Assets (Including Base Files)/ext_data/npcs/stormtrooper_random.cfg`** (NEW FILE) — Weight table:
```
stormtrooper_rifle      5
stormtrooper_heavy      2
stormtrooper_officer    2
stormtrooper_grenadier  1
```
Adding a new variant = add a block to the `.npc` + a line to the `.cfg`. No C++ required.

**`code/game/NPC_spawn.cpp`** (~line 2800) — `ST_LoadRandomVariants()` reads the cfg via `gi.FS_ReadFile`, parses name+weight pairs, hardcoded fallback if file missing. `SP_NPC_StormtrooperRandom()` does a weighted `Q_irand` pick. **This session**: removed two unconditional `Com_Printf` debug dumps (variant count + per-variant list) that were spamming console on every spawn — the yellow fallback-warning print stays (only fires when the cfg file is missing, a genuine diagnostic).

**`code/game/g_spawn.cpp`** — Forward declaration + `{"NPC_StormtrooperRandom", SP_NPC_StormtrooperRandom}` in `spawns[]` table.

**Note (historical, worth re-checking against the Drive Mirror folder)**: `combined
base with jkenhanced` did NOT have this NPC system as of the session that added it —
`stormtrooper_random.npc`/`.cfg` only existed in the repo's (gitignored, local-only)
`Mod Files/`. Also confirmed `imperials.npc`/`misc.npc` in `combined base` had
*additional* undocumented tuning beyond what's listed here (health cuts, model/skin
swaps to `stormie`/`chimaera_rgb`) that never made it into this repo. This is exactly
the kind of divergence the Drive-Mirror-as-single-source-of-truth move is meant to
prevent going forward — worth confirming both pieces (the stormtrooper_random files,
and whatever the undocumented `.npc` tuning was) actually made it into `Mod Assets
(Including Base Files)` before `Mod Files/` gets deleted.

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

**RESOLVED.** Confirmed fixed by the user in a later session — repeater blocking works
now. (The follow-up bolt-prediction fix in the "bunch of bug fixes" commit and the
Track 8 saber-combat rework's cooldown retuning likely closed the remaining gap noted
here previously.)

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
  default (`Mod Assets (Including Base Files)/default.cfg`). Intentionally cannot drop the saber (`WP_SABER` is
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
- `ui/controls.menu` and `ingamecontrols.menu` (both under `Mod Assets (Including Base
  Files)\ui\` in the Drive Mirror folder, not the repo) — WEAPON BINDING section consolidated from 13
  individual per-weapon rows down to 8 slot-based rows (saber/pistol/medium/heavy/throwable/
  next/prev/drop). New "Medium Weapon"/"Heavy Weapon"/"Drop Weapon" labels use plain
  literal text (existing pattern in this menu format, e.g. `text "Saber Block and Alt
  Fire"`) rather than string-table keys, and their descriptions deliberately avoid naming
  specific vanilla weapons (future-proofing for when more weapons land in each class).
- `code/ui/ui_shared.cpp`, `g_bindCommands[]` — the controls-menu bind-display/rebind
  system resolves commands through this hardcoded array (`BindingIDFromName`); added
  `dropweapon` and `weaponslot 1`-`5` so those rows show a real key and are rebindable
  (previously showed `???` and couldn't be changed).
- `Mod Assets (Including Base Files)/default.cfg` — keys 1-5 now bind
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
- **DONE.** Model swap (Item 5b) is complete — marking this track fully closed. Cloak
  visibility may get another art pass at some point, but that would be independent
  work, not a continuation of anything documented here.

---

## Track 7: Ultimate Weapons Mod Effects (external workspace only)

**Scope**: this work happens entirely in `Sheng's Ultimate Weapons Redesign` (the
Drive Mirror folder — formerly the OneDrive `sheng ultimate weapons` workspace,
confirmed 1:1 mapped) — never in the repo or `Mod Files/`. Confirmed early in the
original session that the repo's own `Mod Files/effects/blaster/*` and
`thermal/explosion.efx` are NOT the real Ultimate Weapons Mod content (simplified
rewrites, missing the master `shaders/UltimateWeapons.shader` and ~194 dependent
files) — irrelevant now since this track abandoned the repo entirely per the user's
direction and works from the real extracted pk3 content instead.

**Overall status: pretty much done.** Thermal detonator tuning below confirmed
tested and working. Blaster VFX (see below) is the one piece still outstanding.

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

**TESTED — confirmed good.**

### Blaster VFX — not started
Item 3 from the original checklist (blaster muzzle/bolt/impact effects reading as "awful").
Not touched this session; same working-directory scope applies whenever it's picked up.

---

## Track 8: Saber Style Mechanical Identity + Kata Keybind — DONE

Implements Phase 1 of the external plan (see path above). Gave each saber style (Fast/
Medium/Strong/Dual/Staff/Desann/Tavion) a distinct mechanical identity beyond animation:
per-style parry recovery time, a Strong guard-break punishment bonus, Fast/Dual mobility
bumps, cheaper/wider Staff missile-block, Dual exemption from the same-direction guard-
damage penalty, and a real dedicated Kata keybind (new `BUTTON_KATA` bit, `+kata`/`-kata`
console commands, bind-menu rows) replacing the old dead left-click+right-click combo that
the Block bind had made unreachable. Also picked up, from in-game testing, a guard FP-drain
bug fix, a perfect-parry-refund change, folding katas into the heavy-attack mistimed-block
treatment, boss-tier-only instant guard refill, a guard-intact damage-resistance scale, and
loosening the saber clash/bounce gate for partial Saber Offense investment. All changes are
in `code/game/wp_saber.cpp`, `code/game/bg_pmove.cpp`, `code/game/g_active.cpp`,
`code/game/AI_Jedi.cpp`, `code/qcommon/q_shared.h`, `code/client/cl_input.cpp`,
`code/ui/ui_shared.cpp`, plus `Mod Assets (Including Base Files)/ui/controls.menu` /
`ingamecontrols.menu` / `MENUS.str`. Iterated through several rounds of in-game testing
already — considered stable.

---

## Track 9: Disarmed NPC Behavior + Weapon Re-arm — DONE (not yet tested in-game)

Implements Phase 2 of the external plan. Gave disarmed NPCs a full reaction chain: flee
toward cover on disarm, search for a weapon while fleeing, surrender if morale-routed or
cornered by an active saber, or fall back to a faster/occasionally-grab-move desperate melee
if nothing else works. Several real bugs found and fixed getting the `bState` dispatch to
actually reach these branches (flee/combat dispatch are mutually exclusive `bState`s; melee-
flagged NPCs could still be flagged surrender-eligible; disarmed NPCs would charge an active
lit saber barehanded) — see the plan file's Phase 2 section for the blow-by-blow.

**Reverted mid-session**: the original plan's item 6, a terminal "surrender → capture"
state (seated pose, glowing wrist binders, `MOD_CAPTURED`), was implemented but never held
up in testing — pose kept reverting, state kept getting preempted. Fully walked back per
user direction: `NPC_Capture()`, the `firstSurrenderTime` field, and the
`binders/wrist_glow.efx` asset are all gone. Surrendered NPCs now just cower/arms-up
indefinitely with no terminal state — a stealth system is the suggested place to revisit
this later, not scheduled.

**Weapon re-arm reworked**: rather than an NPC preferring its own dropped weapon (that
whole mechanic, including a `droppedWeapon` field, was removed), NPCs now only auto-rearm
from a deliberate level-designer-placed source — a new `FL_NPC_REARM_SOURCE` entity flag
(`code/game/g_local.h`) set on weapon items spawned by `misc_model_ammo_rack`
(`GunRackAddItem`, `code/game/g_misc_model.cpp`) and checked in a new branch of
`CheckItemCanBePickedUpByNPC` (`code/game/g_items.cpp`) that skips the normal "dropped
item" ownership/age checks. This is additive — picking up an ordinary dropped weapon (a
dead comrade's, etc.) still works exactly as before.

Other files touched: `code/game/b_public.h`, `NPC_spawn.cpp`, `wp_saber.cpp`,
`NPC_combat.cpp`, `NPC_behavior.cpp`, `NPC.cpp`, `AI_Civilian.cpp`.

---

## Track 10: Weapon Reworks — Z-6 / Repeater / Disruptor — DONE, two rounds of in-game fixes applied

Implements Phase 4 of the external plan (done ahead of Phase 3 per user request — Phase 3,
Companion AI, is still queued/not started).

**Z-6 Rotary** — split off `WP_FireZ6Rotary` (`code/game/wp_repeater.cpp`) instead of
sharing `WP_FireRepeater`; real heat (`playerState_t::z6Heat`/`z6HeatDecayTime`,
`code/qcommon/q_shared.h`) that degrades accuracy as it climbs, Force Push interruption,
own missile think/effects (fixed a real pre-existing bug where Z-6/DC-15S/DC-15A shared one
cgame effect handle and silently overwrote each other's shot trail). Damage tuned to 14 —
above the repeater's, per the user's original "Z-6 > repeater > blaster" ordering (not the
plan text's "lower damage" framing).

**Repeater** — primary fires a 3-round burst (reuses the existing `weaponShotCount`
counter); alt-fire ("blob") now charges like the disruptor/bryar, throwing farther the
longer it's held; alt-fire VFX recolored red/orange (later reverted, see below).

**Disruptor** — both fire modes converted from instant hitscan to real travel-time
`CreateMissile()` bolts (`code/game/wp_disruptor.cpp`, fully rewritten), higher damage,
slower rate, reusing the blaster's visual style. Disintegration (`MOD_SNIPER`) fully
removed from the disruptor (confirmed `g_combat.cpp`'s disintegration code is still needed
as-is for the Tusken rifle and a `g_mover.cpp` trap — left untouched). Dropped, as a
disclosed scope reduction: the old alt-fire multi-target penetration trace has no clean
equivalent once it's a single projectile — a full charge is now one much harder-hitting
bolt instead of a beam through several enemies.

**Side quest, still unresolved**: investigated a pre-existing (unrelated) bug where the
bowcaster's alt-fire charge visual doesn't show. Shader/texture/registration all look
structurally correct, matching Bryar's working pattern — could not root-cause via static
reading alone, needs in-game debugging.

### First in-game test pass — user feedback + fixes applied
- **Repeater still fired full-auto.** Root cause: the burst counter set a fast 60ms gap for
  the first two shots of a burst but fell through to the weapon's normal 50ms rate on the
  third instead of a real cooldown — never actually paused. Fixed: third shot now forces a
  450ms gap (`code/game/bg_pmove.cpp`, `case WP_REPEATER` in the fire-rate switch).
- **Repeater alt-fire charge had no sound, charged too fast, went too far.** No
  `altchargesound` key existed for `WP_REPEATER` in `weapons.dat` at all — the generic
  charge-sound wiring in `PM_DoChargedWeapons` silently no-ops if the key is missing.
  Added one (reusing the disruptor's charge-whine sound). Slowed `REPEATER_ALT_CHARGE_UNIT`
  150ms → 250ms and reduced max charge scale 2.0x → 1.48x (`code/game/wp_repeater.cpp`).
- **Z-6 heat "wasn't doing much."** Spread scale was a flat linear curve off heat (max
  3.5x). Replaced with a cubic curve (gentler at low/mid heat, up to 8x at full heat) and
  slowed per-shot heat gain 4 → 3 so the ramp is felt over a couple seconds instead of
  saturating almost instantly (`code/game/wp_repeater.cpp`, `WP_FireZ6Rotary`).
- **Disruptor primary wanted more damage + a headshot multiplier.** Bumped
  `DISRUPTOR_MAIN_DAMAGE` 30 → 40 (`code/game/weapons.h`). Headshot multiplier already
  exists generically — `damageModifier[HL_HEAD] = 2.0f` (`code/game/g_combat.cpp` ~5320,
  applied unless the MOD is listed in `G_NonLocationSpecificDamage`) — and `MOD_DISRUPTOR`
  isn't excluded, so headshots already do 2x. No code added; flagged to user in case they
  want more than the generic 2x specifically for this weapon.
- **All Phase 4 `.efx` visual recolors reverted** per user request — they're going to hand-
  author the visuals themselves instead. Exactly reverted (had originals on record):
  deleted the new `effects/z6/barrel_glow.efx`, restored `effects/z6/shot.efx` and
  `effects/repeater/alt_projectile.efx` to their exact pre-session colors. **Resolved**:
  `effects/repeater/altmuzzle_flash.efx` and `effects/repeater/concussion.efx` were
  reconstructed via a best-effort R/B channel swap at the time, but the user has real
  backup `.efx` files for these that should be workable to revert to properly — no
  longer a static-reconstruction guess. **Still dangling, but accepted**:
  `WP_FireZ6Rotary` still calls `G_PlayEffect("z6/barrel_glow", ...)` at high heat, and
  since that file is deleted, the call still silently no-ops. User's own call: probably
  fine to just re-implement the asset on their side rather than strip the code call.
- **Gotcha found this round**: there are three separate `weapons.dat` copies in play —
  the local (gitignored, not actually git-tracked) `Mod Files/ext_data/` and
  `Game Assets/ext_data/` (both stale/untouched by this session's changes) and
  `ext_data/weapons.dat` inside `Mod Assets (Including Base Files)` in the Drive Mirror
  folder (the live one, holding all of this session's `firetime`/`missileFuncName`/
  `damage`/`altchargesound` changes). Make sure it's the Drive Mirror copy that gets
  used for any test/deployment target — see the migration plan in "Ready for Next
  Session" for retiring the other two copies entirely.

**Not yet retested in-game** as of that round of fixes.

### Second in-game test pass — user feedback + fixes applied

- **Repeater primary fire "still fully auto"** — investigated the burst logic directly
  (`bg_pmove.cpp:14492-14506`/`13757`+), confirmed it's structurally correct and should
  produce a 60/60/450ms rhythm. Turned out to be a stale build — hadn't been rebuilt
  since that code was written. **Confirmed working correctly once rebuilt, no code
  change needed.**
- **Repeater alt-fire overshoots, charged and uncharged.** Root cause:
  `chargeCount` (`wp_repeater.cpp:112-121`) was floored to a minimum of `1`, so even an
  instant tap got the same `1.0x` scale as a "properly" minimally-charged shot — no
  weak/uncharged state existed at all. Fixed: floor lowered to `0`, `chargeScale`
  formula changed to `0.55f + chargeCount * 0.186f` (0.55x instant tap → 1.48x full
  charge, same top end as before).
- **Z-6 fires faster than its own effects can render.** `muzzle_flash.efx`/`shot.efx`
  had `repeatDelay` (122ms/300ms) far exceeding the 50ms `firetime`, so the engine's own
  repeat-suppression silently skipped most triggers. Fixed: both dropped to
  `repeatDelay 40` in **both** content copies (Drive Mirror `Sheng's Ultimate Weapons
  Redesign` and `Mod Assets (Including Base Files)` — confirmed byte-identical bug in
  both, fixed both rather than gambling on pk3 load order).
- **Z-6 effect renders through the player.** `CalcMuzzlePoint` (`g_weapon.cpp:533-593`)
  had no case for `WP_Z6_ROTARY` at all, falling back to the raw entity origin (near the
  feet) — this fed both the missile spawn point and the `barrel_glow` effect trigger
  (`wp_repeater.cpp:312-316`), since both read the same `muzzle` global. Fixed: added a
  real `WP_Z6_ROTARY` case (eye height + forward offset, two-handed-weapon pattern).
- **Z-6 shader warnings** (`Couldn't find image for shader models/weapons2/
  saber_plasmaGE/...` — no, unrelated; the real ones were `z6/blastersideflash`/
  `blasterfrontflash`). Root cause: `.efx` files reference `gfx/effects/z6/
  blastersideflash`/`blasterfrontflash`, but no `.shader` block exists at that exact
  path in either content location — only under `clone/` (`cloneblasters.shader`), a
  copy-paste-without-rename leftover from when Z-6 was cloned from the clone-blaster
  assets. Checked the base mod files too per a user hint that they might have it
  independently — they don't (`Mod Assets/shaders/z6.shader` only has the weapon model's
  own skin materials; `effects.shader` has the un-prefixed vanilla generic version).
  Fixed: added `z6/blastersideflash`/`blasterfrontflash` shader blocks (mirroring the
  `clone/` ones exactly) to `cloneblasters.shader` in **both** content locations.
- **Z-6 dropped weapon shows the repeater's model.** `items.dat`'s `weapon_z6` block had
  `worldmodel models/weapons2/heavy_repeater/heavy_repeater_w.glm` — a copy-paste
  leftover from the repeater's own entry. Fixed to `models/weapons2/z6_rotary/model.glm`,
  matching `weapons.dat`'s own (correct) key.
- **Z-6 heat spread felt a bit too much at max heat.** `wp_repeater.cpp:279-280`'s cubic
  curve coefficient (`7.0f`, giving up to 8x baseline spread at full heat) trimmed to
  `5.0f` (6x max), same gentle-then-steep shape, lower ceiling.
- **`z6/barrel_glow.efx`** — user found a backup after all (previous note above was
  wrong, no asset existed at all as of that writing). Restored to `Sheng's Ultimate
  Weapons Redesign`; copied into `Mod Assets (Including Base Files)` too since it only
  existed in one location. Re-diagnosed the "renders through the player" complaint
  properly once the asset existed to actually check: `barrel_glow` triggers via
  `G_PlayEffect`, a server-broadcast world-space effect (same category as explosions),
  **not** the viewmodel-only sprite mechanism bowcaster's charge flash uses — no
  first/third-person split needed for this one, the `CalcMuzzlePoint` position fix
  above should be sufficient on its own. Verified its shader reference
  (`gfx/effects/whiteGlow`) is real and already defined — no orphaned-path issue here.
- **Bowcaster charge effect invisible in third person** (yours or anyone's). Turned out
  simpler than first diagnosed: there's a working third-person equivalent of the
  charge-flash code in `code/cgame/cg_players.cpp:9813-9865` (gated on
  `cent->currentState.number == 0 && cg.renderingThirdPerson` — "render my own weapon
  while I'm looking at myself in third person," exactly the reported scenario), and it
  already had a complete, correct bowcaster branch — but the condition gating it in
  (`:9819`) checked `ps->weaponstate == WEAPON_CHARGING` instead of
  `WEAPON_CHARGING_ALT` (every other weapon in the same list correctly used `_ALT`).
  One-word fix. (The original diagnosis assumed the fix would need mirroring
  `CG_DoMuzzleFlash`'s view-mode-aware pattern — turned out unnecessary; that function's
  `cg.renderingThirdPerson` branch is actually dead code, since its only caller,
  `CG_AddViewWeapon`, already returns early whenever that flag is true.)

All of the above staged in `Drive Mirror\Sheng's JK Enhanced\Staged and changed files\`
per the new standing workflow (see `## Project` header). **Not yet retested in-game**
as of this round.

---

## Ready for Next Session

**Resolved since the above tracks were written** (confirmed directly by the user in
a later session, recorded here so nothing gets re-investigated from scratch):
- ~~Saber block stagger / possible parry system~~ — **done**. Landed as the "Saber
  combat" work (perfect-parry retrigger changed to tap-attack-while-holding-block,
  HUD cooldown bar, saber/blaster cooldowns split 1000ms/150ms, disarm-on-perfect-
  parry-of-thrown-saber, telegraphed heavy attacks with a slowed windup, Force
  stagger follow-up lockout, Force economy retune, Force Lightning block fix, gib/
  corpse physics v2). Two commits, both titled "Saber combat".
- ~~Test Track 5's swap/drop fixes and Track 6's cloak fixes in-game~~ — user
  confirmed these are fine/working.
- ~~Saboteur model swap~~ (Item 5b) — **done**.
- ~~Data-driven weapon damage (Part 2)~~ — user confirmed this **no longer has a
  purpose** now that the class-based loadout system exists; fully dead, don't revisit.
- ~~Ultimate Weapons Mod effects~~ (Track 7) — thermal detonator tuning **tested,
  confirmed good**. **Blaster VFX tuning (Item 3) is still explicitly outstanding** —
  user confirmed it's separate and not included in "pretty much done."
- **Morale system** — user says it's "ironed out as well as we can" for now; not
  going to get more tuning passes. Open idea for later (not started, no design work):
  the Stormtrooper AI decision logic (morale tiers, geo-cover fallback, suppression
  coordination, buddy-pair logic, etc.) has grown into a lot of individually bolted-on
  flags/timers checked in priority order, with no single place that explains *why* a
  decision was made. A utility-scoring layer (score a few candidate actions each
  think using the inputs that already exist, pick the highest, log the scores under a
  debug cvar) was suggested as a future direction — would also double as
  instrumentation for the backpedal-fire bug below. Purely a suggestion, not scheduled.

**Fixed this session** (separate from the above — a shorter side session focused on
one specific bug, not part of Tracks 1-10):
- **Weapon-select menu wrongly required two bonus weapons.** With the class-based
  loadout system, the two bonus-weapon slots can never hold the same class, and Heavy
  is locked out early in the story — so at low `tier_storyinfo`, only one class is
  ever selectable, and `UI_WeaponsSelectionsComplete` (`code/ui/ui_main.cpp:6858`)
  requiring *both* slots meant "Begin Mission" could never turn on. Fixed to require
  *either* slot (plus the throwable) — commit `9ecdc5cd`. **User confirmed working.**

**Still genuinely open:**
- **Backpedaling repeater troopers won't fire — user wants this one finally solved,
  bumping priority.** Re-confirmed by the user as being
  about the *player* backpedaling away from an NPC (not the NPC's own movement), and
  that it happens **immediately at any range** — this rules out `NPC_EnemyTooFar()`'s
  weapon-max-range cutoff as the explanation. Two independent static-analysis passes
  (the original one below, and a second one specifically re-checking for any fire-
  gating on the *enemy's* velocity/movement) both came up empty in
  `AI_Stormtrooper.cpp`/`NPC_combat.cpp` — no code path found that reads enemy
  movement in the fire-decision, aim-cone, or target-leading logic (there is no
  target-leading logic at all; `NPC_ShotEntity()` only uses the enemy's current
  position). User declined debug logging for now — needs either that instrumentation
  or a broader static sweep (other NPC classes' combat files, player-side
  `hitAlly`/aim-miss code) before resorting to live tracing.
- **README is now out of date against Track 8/9/10.** It documents the original
  Saber Guard/Perfect Parry system, class-based loadouts, and the first round of
  Saber combat retuning, but has nothing yet on: saber style mechanical identity +
  Kata keybind (Track 8), disarmed-NPC flee/surrender/desperate-melee behavior +
  weapon re-arm via `FL_NPC_REARM_SOURCE` (Track 9), or the Z-6/repeater/disruptor
  reworks (Track 10). Needs a pass.
- **Test Track 9 in-game** — disarmed-NPC flee/surrender/desperate-melee behavior and
  weapon re-arm via `FL_NPC_REARM_SOURCE`. Confirmed still not tested since it was written.
- ~~Retest Track 10's *first* round of follow-up fixes~~ — superseded by the second
  round (see Track 10's "Second in-game test pass").
- **Retested the second round — three new issues found, not yet fixed:**
  - **Z-6 blaster bolts still render through the player.** The `CalcMuzzlePoint` fix
    (adding a `WP_Z6_ROTARY` case) wasn't sufficient — the bolts themselves (not just
    the `barrel_glow` effect) still clip through the player model. Needs another look;
    possibly the projectile's own client-side rendering (`FX_Z6ProjectileThink`) has a
    separate positioning issue from the server-side spawn point, or the muzzle offset
    added isn't large/correct enough to clear the model on the first visible frame.
  - **Z-6 running out of ammo mid-fire gets the player stuck trying to auto-switch
    weapons.** User's probable fix: just stop auto-switching weapons on empty for the
    Z-6 (or in general). Workaround in the meantime: dropping the Z-6 un-sticks it.
  - **Repeater burst can be short-circuited by tapping.** The 3-round burst should
    always fire all 3 no matter how briefly the trigger is held, but right now a quick
    tap can release after 1-2 shots, and the `weaponShotCount` counter keeps its
    position across separate trigger pulls — so the *next* tap can immediately land on
    the 450ms "burst gate" delay instead of starting a fresh burst. Needs either a real
    burst-lock (commit to firing 3 once started, ignore button release until the burst
    completes) or resetting the counter properly between separate presses.
- **NPCs need to fire the repeater and Z-6 like the reworked player versions do.**
  Stormtroopers etc. still fire the repeater the old way (not the new 3-round burst
  pattern) — the burst logic added to `bg_pmove.cpp`'s `case WP_REPEATER` is presumably
  player-path-only, or NPC fire-decision code (`AI_Stormtrooper.cpp`/`NPC_combat.cpp`)
  has its own separate repeater handling that never got updated to match. For the Z-6,
  explicitly **not** meant to mirror the player's pattern at all — user wants NPC Z-6
  use to be all-or-nothing sustained suppression fire, not short bursts. Needs its own
  NPC-side fire-decision logic, not just reusing whatever the player path ends up doing.
- `repeater/altmuzzle_flash.efx` / `repeater/concussion.efx` — still the one open item
  from the *first* round of Track 10 fixes (user has real backup `.efx` files to revert
  to properly, just not done yet).
- **Need an actual manual `.efx` tuning pass using EffectsEd** for the visual issues
  above (and others) rather than pure text/data edits — user flagged EffectsEd itself
  as painful to use and floated either improving it or finding/building an alternative
  as a possible side-project. Not scoped or started.
- ~~Bowcaster charge VFX bug~~ — **fixed**, see Track 10's second pass (one-word typo,
  `WEAPON_CHARGING` → `WEAPON_CHARGING_ALT`, `cg_players.cpp:9819`).
- **Phase 3 (Companion AI overhaul)** — queued in the external plan, not started. Was
  deliberately deferred behind Phase 4 (Track 10) per user request; pick up once Track 10
  is confirmed working.
- **Drive Mirror asset migration** (see `## Project` header) — three steps, in order:
  1. Reconcile: confirm everything of value in the repo's local (gitignored) `Mod
     Files/` actually made it into `Mod Assets (Including Base Files)` — see the
     Track 3 note about `stormtrooper_random.npc`/`.cfg` and undocumented `.npc`
     tuning that previously only existed in one place or the other.
  2. Once confirmed, delete `Mod Files/`/`Game Assets/` locally and trim their two
     lines out of `.gitignore` (they're not git-tracked, so no history/repo surgery —
     just local cleanup).
  3. Set up directory junctions (`mklink /J`, no admin needed) from the relevant
     `GameData` subfolders into the Drive Mirror folder on each machine, so edits are
     live in-game immediately with no manual copy step. Local junction target paths
     may differ per machine.
  `big update` (the old ad-hoc transfer staging folder) is confirmed stale/dead — no
  mapping into the new structure, safe to ignore or delete whenever.

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
- **Three `weapons.dat` copies exist right now, being reduced to one**: the local
  (gitignored, not actually in the repo) `Mod Files/ext_data/` and `Game Assets/ext_data/`
  are both stale as of Track 10 and slated for deletion, vs. `ext_data/weapons.dat` in
  `Mod Assets (Including Base Files)` (the Drive Mirror folder — the live/deployed one).
  Until the migration (see "Ready for Next Session") is done, always confirm which copy
  you're editing and which one actually ships.
- **Drive Mirror is the asset source of truth going forward** — `Mod Files/`/`Game
  Assets/` are being retired once reconciled (see "Ready for Next Session"). Once
  directory junctions from `GameData` into the mirror folder are set up on both
  machines, editing a data/UI/effects file anywhere becomes live in-game immediately on
  both PCs with no manual copy step — update this note once that's actually done.
- `PM_DoChargedWeapons` (`code/game/bg_pmove.cpp` ~13420-13470) auto-plays
  `weaponData[weapon].chargeSnd`/`altChargeSnd` when a weapon enters `WEAPON_CHARGING`/
  `WEAPON_CHARGING_ALT` — but only if the corresponding `chargesound`/`altchargesound` key
  actually exists in that weapon's `weapons.dat` block. Adding charge behavior in code to a
  weapon that never had a charge mechanic before (e.g. the repeater in Track 10) does
  *not* get a sound for free — the data key has to be added too, and it fails silently
  (no warning) if forgotten.
- `defaultDamage[]` (`code/game/g_weaponLoad.cpp` ~268) is a **fallback only** — a weapon's
  real damage comes from its `weapons.dat` `damage`/`altdamage` key if present, and only
  falls back to the `weapons.h` constant (e.g. `DISRUPTOR_MAIN_DAMAGE`) if that key is
  absent from the block. Check the actual `.dat` block before assuming a `weapons.h`
  damage constant is dead or live.
- Generic per-hitlocation damage multiplier: `damageModifier[HL_MAX]`
  (`code/game/g_combat.cpp` ~5320, `HL_HEAD` = 2.0x) applies automatically to **any**
  weapon whose MOD isn't listed in `G_NonLocationSpecificDamage()` (~line 5434) — most
  hitscan/projectile weapons already get a free 2x headshot bonus with no per-weapon code
  needed. Check this list before adding a bespoke headshot multiplier for a "new" weapon.
