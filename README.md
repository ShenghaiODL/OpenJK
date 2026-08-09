## Patch Notes -- JKEnhanced / OpenJK
## =====================================

SABER REWORK:
---------------------

MANUAL SABER BLOCKING
---------------------
Replaced auto-blocking with a dedicated manual block button (+saberblock) //Saberlocks are very infrequent as a side effect, might have had one total in my testing since.
Blocking costs force points, scaling with your Saber Defense level; fails with a "no force" sound when depleted //Still don't think this is 100% perfect, probably a fair few edge cases. 
Block button can share a bind with alt-attack: bind mouse2 "+saberblock; +altattack"


MISSILE DEFLECTION
--------------------------
Reflection accuracy is now tied to FP Saber Defense level:
  Level 3: 75% chance, aimed along the player's crosshair
  Level 2: 50% chance, aimed back at the shooter with medium spread
  Level 1: 10% chance, aimed back at the shooter with large spread
Bowcaster bolts are now blockable at all difficulty levels; they dissipate on saber contact instead of reflecting //Made balancing around NPCs with the bowcaster very difficult. Might revisit this and just reduce the number of bowcasters, but hey.


SABER GUARD & PERFECT PARRY
--------------------------------------
Boss-tier and other saber-capable NPCs now have a hidden "guard" (composure) pool alongside health, sized off their Saber Defense level (70/100/130) and boosted 50% for bosses (Desann, Tavion, Alora, Kyle, Shadowtrooper, or anything flagged as a boss character)
Every parry, kick, resisted Force push, and lightning tick chips away at guard; repeatedly blocking from the same direction loses effectiveness (down to 40% after a few reps) so turtling in one spot stops working
At zero guard the NPC breaks -- an extended stagger (1.6-2.4s, 0.9-1.3s for bosses) where they can't parry or evade and take 2.5x saber damage -- then guard fully refills once the break window ends, so bosses effectively fight in phases
New player mechanic: hold block and tap attack right as a hit lands (default 200ms window, g_perfectParryWindow) for a "perfect parry" -- guaranteed deflect/parry regardless of Force Defense level, costs no Force, and knocks a huge chunk off an NPC's guard. Distinct crosshair flash + sound on success, HUD cooldown bar so you can see when the next window is up //basically a timing-based alternative to the FP-gated block, should reward actually paying attention instead of just holding the button
Perfect parry retrigger changed from re-pressing block to tapping attack while block is already held -- no more releasing block just to get a fresh timing window
Cooldown between perfect-parry windows is now split by threat: 1000ms after a saber hit, only 150ms if a blaster bolt is incoming (g_perfectParryCooldownSaber / g_perfectParryCooldownMissile) -- blasters fire way faster than sabers swing, so the old single cooldown made blocking a burst basically impossible
Perfectly parrying a thrown saber now disarms the thrower entirely; NPCs throw their sabers less often and can't spam it back-to-back anymore
Removed the old requirement to strafe toward side attacks while manually blocking -- holding block now parries from any direction, timing is the skill test instead
NPCs with a guard pool show a thin orange composure bar under their health bar; flashes white while broken


HEAVY ATTACKS
--------------------------------------
Saber-capable enemies can now telegraph a heavy attack: a visibly slowed windup into a normal-speed strike, so you get a real tell instead of every swing looking the same -- one enemy can only do this once every 7 seconds (g_heavyAttackCooldown)
Blocking a heavy attack without perfect timing still staggers you, but damage is halved compared to a normal mistimed block -- rewards at least trying to block a heavy swing even if the timing's off
A Force push/pull/repulse/grip that actually staggers someone now locks out the caster's own follow-up attack for a share of the victim's recovery time (g_forceStaggerLockoutScale, 60%) so you can't just push someone down and get a free hit every time


FORCE ECONOMY
--------------------------------------
Passive Force regen ticks faster (every 60ms instead of 100ms)
Non-perfect blocks cost a lot more now -- 5x the base cost blocking a saber hit, 1.5x blocking a blaster bolt -- so eating the timing window matters more //perfect parries are still completely free, this is just the "you didn't time it" tax
Longer pause before regen resumes after blocking
Force Lightning can now only be blocked by actually holding block, not just standing in a ready pose, and blocking it drains Force power like everything else


Weapon Rework:
---------------------

WOOKIEE BOWCASTER REWORK
-------------------------
Main fire fires a single green bolt (no longer a multi-bolt spread)
Alt fire fires a charged explosive bolt -- hold to charge, release to fire; damage scales 70-100 based on charge time
Player damage increased: 45 -> 55
NPC hard difficulty damage: 36 -> 50
Explosive bolt knocks down anyone within half the splash radius
Alt fire nerf: significantly slower fire rate when charged
// Fixed this round: the charge-up glow only rendered in first person -- third-person viewers (including yourself, if you switched your own camera to third person) never saw it at all, even though the sound always played. One condition was checking the wrong internal state; now shows correctly in both view modes


Z-6 ROTARY BLASTER CANNON (from JKEnhanced)
----------------------------------------
Spin-up mechanic: must hold fire to spin up the barrel (500ms minimum) before it fires
Loop sound plays while spinning; spin-down sound plays on button release
Model and animations courtesy of MBII
Stormtroopers, officers, and select NPCs can now spawn with the Z-6 as a weapon pool option
Real heat mechanic: sustained fire builds heat, which widens the spread the longer you hold the trigger (gentle at first, falls off a cliff near max heat) -- trades accuracy for suppression instead of just being a bigger repeater
// Fixed this round: muzzle flash/shot trail effects were firing slower than the gun itself, so most shots showed no visual at all; missing shader definitions were throwing console warnings; dropped Z-6 pickups showed the repeater's world model instead of their own; max-heat spread was a little much and got trimmed down


REPEATER REWORK
----------------------------------------
Primary fire is a 3-round burst rather than straight full-auto -- holding the trigger keeps firing burst after burst automatically, no need to release and re-press
Alt fire ("blob") charges the longer you hold it, throwing farther with a longer charge
// Fixed this round: an instant tap on alt fire was getting the same power/range as a "properly" charged shot -- there was no actual weak/uncharged state at all. Quick taps are now noticeably weaker and shorter-range, full charge unchanged


CYCLER RIFLE / AMBAN SNIPER (Replaces Disruptor for player and will eventually for NPCs)
------------------------------------------------------------
New weapon using the Amban model and MB2 skeleton animations
Cycler rifle that needs to be reloaded in between shots.
Keeps disruptor mechanic, but needs to charge for longer.


KICK SYSTEM
-----------
Kick (+kick) is now available to all saber styles for the player
Previously restricted to double bladed saber
//TODO: Be able to throw Staff Saber


BLASTER BALANCING
-----------------
Blaster bolt velocity: 2300 -> 3000


CLASS-BASED WEAPON LOADOUTS
--------------------------------------
Weapons are now grouped into loadout classes -- Pistol, Medium, Heavy, Throwable (the saber is its own thing, excluded entirely) -- and you can only hold one weapon per class at a time
Walking over a weapon whose class you already hold leaves it on the ground; hold +use to swap it in (drops your current one, picks up the new one, switches to it immediately) -- weapon pickups can now be use-grabbed from further away (128 units) instead of requiring you to stand right on top of them, with an on-screen "Swap X for Y" / "Take X" hint while looking at one
New dropweapon command (bound to G by default) to voluntarily drop your current weapon; can't drop the saber this way
Number keys 1-5 now select loadout slots (1=saber, 2=pistol, 3=medium, 4=heavy, 5=throwable) instead of specific weapons, resolving to whatever you're currently holding in that slot
16 reserved "custom weapon" slots added under the hood so future weapons can be added via weapons.dat/items.dat alone, no code changes needed //groundwork for whenever I add more guns


SHIELD SYSTEM REDESIGN
--------------------------------------
Shield cap raised to 200 (100 on Jedi Master)
New Three-tier absorption:
  126-200 shields: 100% damage absorbed
   76-125 shields:  75% absorbed
     1-75 shields:  50% absorbed
  On Jedi Master (cap 100), the 100% absorption tier is never reachable
Passive shield regen: +5 every 250ms after 5 seconds without taking damage;
caps at 75 (50 on Jedi Master); requires a pickup to exceed the regen cap // Pickups currently bugged? Will also probably make this faster.
Armor now has its own pickup/charger cap (200 normal, 100 Jedi Master), decoupled from max health, instead of capping at whatever your max health happened to be


NPC AI IMPROVEMENTS
-----------------------------------
Ranged NPCs now hold their ground and continue shooting when the enemy is in FOV,
rather than chasing whenever they lack a perfect shot line
Ranged NPCs stop advancing when the enemy closes within ~150 units; will step back
rather than rush into melee
NPCs seek cover when below 50% health
When an NPC discovers a dead teammate, it alerts nearby allies
New AI Class_Mando; same as a stormtrooper, but has a flamethrower. //Might add more to this later, this was just to see if I could.
Squad morale & leadership: any rank can now lead a squad (previously locked to Imperial officers, so an all-trooper squad with no officer had no commander at all); morale rises/falls with combat outcomes and casualties and drives a 5-tier aggression scale, from routed/hiding at the bottom to charging/flanking at the top, with the commander calling out lines as the squad's mood shifts
Stormtroopers fall back on geometry-based cover searches when no pre-placed cover point is nearby, holding a duck/pop-out firing pattern instead of standing in the open; squads coordinate so at least half the group keeps firing while the rest repositions, rather than everyone exposing at once
NPC think rate now scales with their Reactions stat, so elite troops react noticeably faster than grunts instead of everything sharing one tick rate
Stormtroopers reposition more often, fire while moving/retreating more, and hold a line-of-sight "grace window" before breaking off a shot; Reborn/Sith without chase-enemies flags now slowly advance instead of standing idle, and their rocket/saber missile-reflect chances are now probabilistic instead of guaranteed
New NPC_StormtrooperRandom spawner: drops a weighted-random stormtrooper variant (rifle/heavy/officer/grenadier) from a config file instead of needing a distinct classname per spawn -- add new variants by editing the .npc/.cfg, no recompile
Various stormtrooper fixes: grenadier now switches back to its blaster properly after throwing, saber no longer flickers off/on during door-cam cutscenes, fixed stormtrooper_random always falling back to the hardcoded variant list
Toned down how much NPCs strafe and jump around during saber duels -- was getting a bit chaotic to actually read what they were doing


SABOTEUR CLOAK REWORK
--------------------------------------
Saboteurs now cloak far less often (cooldown 2s -> 8s) but are a real threat while cloaked instead of a non-issue: they can fire without decloaking now (previously had to decloak to shoot at all), and the cloak itself is dimmed to ~35% opacity so it's harder to spot on sight //still might need more tuning once the model gets swapped out


DEATH ANIMATION FIX
--------------------
During manual block ideation, ran into an issue where if the player died, and tried to reload the game, they could not move or use their saber. This is now fixed.
New console command g_saberloaddebug to inspect animation and movement state for debugging


RENDERER
--------
Vertex limit per surface raised (from my brief testing, negligible impact on fps/performance. Opens up a couple of cool possibilties with rend2 I think.)


CAMERA
--------
Third-person camera now shifts between three profiles depending on what you're holding -- a looser "explore" view with the saber holstered, a tighter dueling view with the blade lit, and a closer over-the-shoulder "shooter" view for guns -- smoothly blending between them and re-tracing to avoid clipping into walls
Shooter mode aims to fix the old mismatch between the 3D crosshair and where shots actually land //inspired by Jedi Survivor's camera, still tuning the exact offsets


PHYSICS
--------
Corpse despawn time is now a cvar (g_corpseRemovalTime) instead of hardcoded; set it to 0 to keep bodies around forever
Force push/pull can now grab severed limbs without needing a pixel-perfect crosshair on them, and gibs fly with less knockback so they don't ragdoll violently
Fixed gibs freezing mid-air or landing stuck pointing in strange directions after being Force pushed or pulled
Fixed pushed/pulled gibs clumping together instead of scattering
Gibs now actually collide with each other, with corpses, and with living characters instead of passing through everything


## Based on OpenJK

OpenJK is a community effort to maintain and improve the game and engine powering Jedi Academy and Jedi Outcast, while maintaining _full backwards compatibility_ with the existing games and mods.  
This project does not intend to add major features, rebalance, or otherwise modify core gameplay.

Our aims are to:

- Improve the stability of the engine by fixing bugs and improving performance.
- Support more hardware (x86_64, Arm, Apple Silicon) and software platforms (Linux, macOS)
- Provide a clean base from which new code modifications can be made.

[![discord](https://img.shields.io/badge/discord-join-7289DA.svg?logo=discord&longCache=true&style=flat)](https://discord.gg/dPNCfeQ)
[![forum](https://img.shields.io/badge/forum-JKHub.org%20OpenJK-brightgreen.svg)](https://jkhub.org/forums/forum/49-openjk/)

[![build](https://github.com/JACoders/OpenJK/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/JACoders/OpenJK/actions/workflows/build.yml?query=branch%3Amaster)
[![coverity](https://scan.coverity.com/projects/1153/badge.svg)](https://scan.coverity.com/projects/1153)

## Supported Games

| Game | Single Player | Multi Player |
| - | - | - |
| Jedi Academy | ✅ Stable | ✅ Stable |
| Jedi Outcast | 😧 Works, needs attention | 🙅 Not supported - consider [JK2MV](https://jk2mv.org) |

Please direct support queries, discussions and feature requests to the JKHub sub-forum or Discord linked above.

## License

OpenJK is licensed under GPLv2 as free software. You are free to use, modify and redistribute OpenJK following the terms in [LICENSE.txt](https://github.com/JACoders/OpenJK/blob/master/LICENSE.txt)

## For players

To install OpenJK, you will first need Jedi Academy installed. If you don't already own the game you can buy it from online stores such as [Steam](https://store.steampowered.com/app/6020/), [Amazon](https://www.amazon.com/Star-Wars-Jedi-Knight-Academy-Pc/dp/B0000A2MCN) or [GOG](https://www.gog.com/game/star_wars_jedi_knight_jedi_academy).

Download the [latest build](https://github.com/JACoders/OpenJK/releases/tag/latest) ([alt link](https://builds.openjk.org)) for your operating system.

Installing and running OpenJK:

[Follow this guide for installing JAEnhanced, OpenJK, TaystJK, EternalJK, or other similar clients](https://jkhub.org/tutorials/errors-problems/installing-openjk-or-other-clients-on-windows-mac-or-linux-r83/)

## For Developers

### Building OpenJK

- [Compilation guide](https://github.com/JACoders/OpenJK/wiki/Compilation-guide)
- [Debugging guide](https://github.com/JACoders/OpenJK/wiki/Debugging)

### Contributing to OpenJK

- [Fork](https://github.com/JACoders/OpenJK/fork) the project on GitHub
- Create a new branch and make your changes
- Send a [pull request](https://help.github.com/articles/creating-a-pull-request) to upstream (JACoders/OpenJK)

### Using OpenJK as a base for a new mod

- [Fork](https://github.com/JACoders/OpenJK/fork) the project on GitHub
- Change the `GAMEVERSION` define in [codemp/game/g_local.h](https://github.com/JACoders/OpenJK/blob/master/codemp/game/g_local.h) from "OpenJK" to your project name
- If you make a nice change, please consider back-porting to upstream via pull request as described above. This is so everyone benefits without having to reinvent the wheel for every project.

## Maintainers (full list: [@JACoders](https://github.com/orgs/JACoders/people))

Leads:

- [Ensiform](https://github.com/ensiform)
- [razor](https://github.com/Razish)
- [Xycaleth](https://github.com/xycaleth)

## Significant contributors ([full list](https://github.com/JACoders/OpenJK/graphs/contributors))

- [bibendovsky](https://github.com/bibendovsky) (save games, platform support)
- [BobaFett](https://github.com/Lrns123)
- [BSzili](https://github.com/BSzili) (JK2, platform support)
- [Cat](https://github.com/deepy) (infra)
- [Didz](https://github.com/dionrhys)
- [eezstreet](https://github.com/eezstreet)
- exidl (SDL2, platform support)
- [ImperatorPrime](https://github.com/ImperatorPrime) (JK2)
- [mrwonko](https://github.com/mrwonko)
- [redsaurus](https://github.com/redsaurus)
- [Scooper](https://github.com/xScooper)
- [Sil](https://github.com/TheSil)
- [smcv](https://github.com/smcv) (debian packaging)
- [Tristamus](https://tristamus.com>) (icon)
