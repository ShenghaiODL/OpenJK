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


Z-6 ROTARY BLASTER CANNON (from JKEnhanced)
----------------------------------------
Spin-up mechanic: must hold fire to spin up the barrel (500ms minimum) before it fires
Loop sound plays while spinning; spin-down sound plays on button release
Model and animations courtesy of MBII
Stormtroopers, officers, and select NPCs can now spawn with the Z-6 as a weapon pool option
Currently shoots red blaster EFX.
// Not currently super happy with the projectiles and damage. Will probably adjust.


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


NPC AI IMPROVEMENTS
-----------------------------------
Ranged NPCs now hold their ground and continue shooting when the enemy is in FOV,
rather than chasing whenever they lack a perfect shot line
Ranged NPCs stop advancing when the enemy closes within ~150 units; will step back
rather than rush into melee
NPCs seek cover when below 50% health
When an NPC discovers a dead teammate, it alerts nearby allies
New AI Class_Mando; same as a stormtrooper, but has a flamethrower. //Might add more to this later, this was just to see if I could.


DEATH ANIMATION FIX
--------------------
During manual block ideation, ran into an issue where if the player died, and tried to reload the game, they could not move or use their saber. This is now fixed.
New console command g_saberloaddebug to inspect animation and movement state for debugging


RENDERER
--------
Vertex limit per surface raised (from my brief testing, negligible impact on fps/performance. Opens up a couple of cool possibilties with rend2 I think.)


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
