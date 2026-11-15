# JKEnhanced

A single-player overhaul of *Star Wars Jedi Knight: Jedi Academy*, built on [OpenJK](https://github.com/JACoders/OpenJK).
The goal is **lethal but fair**: weapons and sabers are deadly, enemies are smarter, and every danger can be seen
coming.

> Savegames from older versions of the mod will not load.

## Saber combat

The rule is simple: **a blade that reaches a body cuts, and the only thing that stops a blade is another blade.**
Duels are about getting the other fighter's blade out of the way. Everything that decides a fight shows on screen.

**Blocking**
- Hold the block button (`+saberblock`) to raise your guard. It stops any swing from the front.
- Each block costs Force. How much depends on the attacker's style: Strong and heavy attacks cost the most.
- Holding block never stops your Force from coming back, so against blasters you can stand and deflect.
- When your Force runs out, your guard collapses and you are left open.
- Your blade stops a swing that physically runs into it, even when you aren't blocking.

**Openings**
- **Perfect parry:** hold block and tap attack as the hit lands. It's free and staggers the attacker.
- **Pressure:** keep hitting a guard until their Force runs out.
- **Saber locks:** tap attack to push. You can look around during a lock. The loser is left open.
- **Commitment:** someone who is swinging isn't guarding. Every enemy string of attacks ends in a
  follow-through, a held pose with the blade out wide, and that is your opening.
- **Kicks and Force:** a kick or push that catches someone mid-swing staggers them. Against a raised guard it only
  costs them Force.

**Cuts**
- How much a cut does depends on how deep the blade goes and where it lands.
- A graze or a limb cut is a wound. Three wounds finish anyone.
- A deep limb cut has an effect: an arm slows their guard, a leg slows them down, a hand makes them drop the saber.
- A solid cut to the torso or head kills.
- Bosses lose a stage of health to a killing cut instead of dying.
- Cuts on you are softened by difficulty, and you are never maimed.
- Ordinary troopers die to any real swing. Cutting off a trooper's hand is non-lethal: they surrender or run.

**Saber enemies**
- They block with a reaction time based on skill, not dice. A fast enough swing beats a slow guard.
- Their attacks come in fixed strings per style, so you can learn them.
- They keep a steady fighting distance instead of hopping and rolling around.
- They spend Force carefully: they keep half for guarding, space out their ranged powers and back off to recover.
- They can telegraph a heavy attack with a glow and a sound.

**Styles** each have their own block cost, guard width, wind-up, recovery and lock strength, set in
`ext_data/saberanims.cfg`. Katas have their own bind (`+kata`), and every style can kick.

## The Force

- Enemies have a real Force bar. Their powers cost Force like yours do, so no endless Push or saber-throw spam.
- **Repulse** is back as a core power: hold to charge, release to throw everyone around you back. Bosses use it
  when they lose a stage.
- **Stasis** freezes the enemy nearest your aim.
- **Insanity** breaks an enemy's nerve. At level 3 they shoot at a phantom beside you.
- **Speed** has three levels: quicker movement, keeping control when shoved, and at level 3 sending blaster bolts
  straight back at the shooter.
- **Grip** level 3 carries what it holds, and the mouse wheel moves it closer or farther.
- Lightning ticks at a steady rate, so frame rate no longer changes its strength.
- Force costs and enemy Force pools are in `ext_data/force.cfg`.

## Weapons

Every blaster has its own job, and NPCs fire exactly the way you do: the same rates, bursts and bolts.

- **Bryar pistol:** precise sidearm with a charged alt-fire.
- **DL-44 pistol:** slow, heavy hand-cannon.
- **E-11:** the all-rounder.
- **Repeater:** three-round bursts, and an alt-fire that throws farther the longer you charge it.
- **Z-6 rotary cannon:** spins up, builds heat that spreads its fire, made for suppression. Model and
  animations from MBII.
- **Bowcaster:** single heavy bolts, and a charged explosive alt-fire that knocks people down.
- **Cycler rifle:** replaces the disruptor. Reloads between shots, fires a travel-time bolt, and headshots do
  double damage.
- **DEMP2:** slower, and its charged alt-fire really scales with the charge.

**Loadouts:** you carry one weapon each of pistol, medium, heavy and throwable.
- Keys 1-5 select the saber or a slot.
- Hold use on a weapon on the ground to swap for it, and press G to drop yours.
- New weapons can be added through `weapons.dat` and `items.dat` without code changes.

**Shields** go up to 200 (100 on Jedi Master).
- A full shield absorbs every hit, and lower shields absorb less.
- Shields slowly recharge up to 75 after a few seconds without taking damage.

**Hits on enemies:** headshots on helmeted enemies count, and chest shots are no longer read as arm hits.

## Enemies

- **Nerve:** each enemy holds their nerve or loses it, based on what they see happen: allies falling, near
  misses, explosions, the Force. A broken enemy drops their gun and runs or surrenders.
- **Squads:** enemies fight in squads of up to 8, led by the highest rank.
  - They choose between holding cover, moving, flanking, suppressing and throwing grenades.
  - Half the squad covers while the other half moves.
- **Roles:** snipers, grenadiers and Tusken each fight in their own way. Snipers show a laser while charging a shot.
- **Saboteurs** fade in and out of cloak, shown as a refraction shimmer, and strike while hidden.
- **Disarmed enemies** look for another weapon, surrender or fight with their fists.
- **Companions** stay close to you and follow when you move on.

## Taking enemies alive

- Kicks, fists, the stun baton, a fully charged DEMP2 alt-fire and slamming someone with push or pull knock them
  out instead of killing them.
- Knocked-out, surrendered and fleeing enemies count as dealt with for the mission.
- Teammates can wake a knocked-out ally, and then they come looking for you.
- Bosses, Force users and droids can't be taken alive. `g_nonLethal 0` turns this off.

## Stealth

- Enemies notice you in stages: unaware, suspicious (`?`), searching (`!`), then alert.
- Light, distance, movement and crouching all matter. A meter shows how lit you are while crouched.
- Saber and melee kills are silent. Bodies get found, and you can drag them out of sight with use.
- Hit use behind an unaware enemy to knock them out.
- Mind trick makes enemies forget what they saw, or draws them to a spot.

## Bosses and difficulty

- Bosses fight in health stages and get a boss bar. Each time they lose a stage they blast you back and recover
  for a moment.
- `ext_data/difficulty.cfg` sets per difficulty how hard enemies hit, how well they shoot and how long they hold
  their nerve. Higher difficulties make enemies sharper, not tougher.

## Camera, HUD and feel

- The third-person camera changes with what you hold: a loose view with the saber off, a duelling view with it
  lit, over the shoulder with guns. Shots land where the crosshair points.
- Hit indicators point at whoever just hit you, and live grenades and mines near you are marked.
- Small hit-stop and camera kick on saber hits.
- Your movement and Force regen no longer change with frame rate.
- Ledge assist: hold jump and forward at a ledge just out of reach.
- Menu animations run at the same speed at any frame rate.

## Graphics

- Uses SomaZ's latest rend2: bloom (`r_dynamicGlowBloom`), SMAA (`r_smaa`), volumetric fog (`r_volumetricFog`),
  faster loads.
- New blaster bolts, muzzle flashes and impacts.
- Gibs and severed limbs collide properly and can be grabbed with push and pull.

## Debug tools

- `g_debugBlock 1` prints why a hit got through a block.
- `g_debugDamage 1` prints where each hit landed and its damage.
- `g_debugForce 1` prints every Force power use.
- `forcesandbox` gives every Force power plus a ring of test targets.
- `forcetally` shows how often each enemy used each power.

## Building and installing

You need Jedi Academy installed ([Steam](https://store.steampowered.com/app/6020/),
[GOG](https://www.gog.com/game/star_wars_jedi_knight_jedi_academy)). To build, follow OpenJK's
[compilation guide](https://github.com/JACoders/OpenJK/wiki/Compilation-guide). For installing a client, see
[this guide](https://jkhub.org/tutorials/errors-problems/installing-openjk-or-other-clients-on-windows-mac-or-linux-r83/).

## Credits and license

Built on [OpenJK](https://github.com/JACoders/OpenJK) by the
[JACoders](https://github.com/orgs/JACoders/people) and its
[contributors](https://github.com/JACoders/OpenJK/graphs/contributors). Renderer from
[SomaZ's rend2](https://github.com/SomaZ/OpenJK). Z-6 model and animations from MBII.

Licensed under GPLv2, like OpenJK. See [LICENSE.txt](LICENSE.txt).
