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
The perfect-parry window now scales with the attack: slower, heavier styles (Strong, Desann) give you a wider window, Fast and dual/staff flurries a tighter one, faster-playing swings tighten it, and telegraphed heavy attacks and katas widen it (x1.5, g_perfectParryHeavyScale). g_debugParry 1 prints how early your tap was against the window for each hit
A perfect parry now pays off properly: the attacker loses more guard (40, g_perfectParryGuardDamage) and is knocked off balance for a moment (can't parry or attack, g_perfectParryStagger), you gain a little Force (g_perfectParryForceGain, double against heavy attacks), and you get a riposte -- attack within 0.6s and the swing comes out instantly and goes through their parry/dodge (g_perfectRiposteTime; no damage bonus). Perfectly parrying a kata stops it dead and staggers them
Enemies that land a perfect parry on you get the same stagger on you, but no riposte damage bonus
No more dice in saber clashes (g_saberDeterministic, on by default; 0 brings back the stock rolls): a held block against a normal swing always holds -- what wears you down is Force, and harder swings (Strong/Desann, heavy attacks) cost more to block; at 0 Force your block breaks. Only a telegraphed heavy attack can disarm or knock you down, and only a perfect parry stops one cleanly. Only masters and bosses can knock your swing away, and only when they READ you -- they're hanging back defending and you hit from the same direction twice or swing a long string of attacks -- so mix up your angles. Regular saber enemies just block.
Block always answers: pressing block cancels your own swing, its recovery or a bounce straight into guard, so holding block never "does nothing". Cancelling the live part of a swing costs a little Force (5, g_blockCancelCost) and only works early in the swing (first 200ms, g_blockCancelWindow) and not twice in a row (500ms, g_blockCancelCooldown) -- after that you're committed; wind-ups, bounces and recoveries can always be cancelled.
Feints: pull back your wind-up or the start of a swing (within 0.3s) with block while an enemy is squaring up to parry it, and they bite -- for a moment they can't parry or attack, and your next swing comes out instantly. Costs 5 Force (a wind-up feint only costs it if someone bites). Each enemy is wary of feints for a few seconds after biting, and masters see through about half
Pressing block too late no longer does nothing: the block is queued, and the moment your swing ends you drop straight into guard
Fixed: Force Drain's close-range grab went straight through Absorb, and Absorb gave no Force back from Drain -- an Absorbing target can't be grabbed now and gets Force back from every Drain tick
Fixed: the stealth light meter showed while you were knocked down or rolling
Fixed: Insanity cost twice its Force when it worked, and full price when it missed or hit someone immune
Fixed: enemies lying on the ground or getting up could still parry your saber (a stab-down on a downed Reborn did no damage)
Fixed: bosses using Force Rage drained their own health straight through their guard stages; an enemy's Rage now never takes it below a quarter of its health or past a stage
Fixed: your Force Lightning hit (and cost Force) once per rendered frame, so it was far stronger at high frame rates; it now ticks at a steady rate like everyone else's
Fixed: Absorb sometimes gave back huge amounts of Force (up to 48 from a single Grip or Push) because it read the wrong cost
Fixed: a boss's stage-break Repulse was cut off by the hit that caused it; it now fires right after, full body, with a shockwave and screen shake when the stage breaks
Fixed: Stasis did nothing unless your crosshair was exactly on the target; it now takes the enemy nearest your aim
Fixed: Tavion (scepter) could freeze in place for the rest of the fight after a scepter ground slam
Fixed: Tavion's scepter beam hit every 50 ms with no limit and could take 700+ health in a second; it now hits a few times a second for much less
Fixed: Tavion (scepter) and dual-saber Alora never got their boss Repulse or boss-sized guard
Fixed: an enemy stabbing down at you while you were knocked down did uncapped damage on every frame (thousands per hit); it is now one normal hit
Enemies now have a real Force bar: their powers cost Force like yours and they back off to recover when they run low, so no more endless Push and saber-throw spam. Blocking and jumping stay free for them; bosses get a bigger bar that refills faster
Force Push and Pull hitting several targets cost a little more per extra target (15 / 20 / 25 / 30) instead of up to 50
Force costs and the enemy Force pool are tunable in ext_data/force.cfg
Force Repulse is back as a core power for you: hold to charge, release to blast everyone around you away. Its rank sets the reach and how far the charge can build
Breaking a boss's guard while they're at the end of a stage now clears the stage right away, instead of needing another hit in a one-second window
Fixed: holding Force Lightning on a boss locked them in their resist pose until they died; after a moment of resisting they now Push you away
Force Lightning does less damage per tick (about 40% of before) now that it ticks at a steady rate
Breaking an enemy's guard no longer multiplies your saber damage (it was up to x3.75); a broken guard still can't parry, dodge or soak hits
Fixed: some enemies (Gran shooters, Rodians, Trandoshans) never got the Z-6 rotary cannon they were meant to carry; they do now
New testing tools: forcesandbox (cheat: all Force powers plus a ring of test targets), g_debugForce 1 (prints every Force cast, its cost and what it did), forcetally (how often each enemy used each power)
Swing meets swing: if you're mid-swing and an enemy's swing lands on you from the front, your swing blocks it (any direction) and theirs bounces off -- no more trading hits (g_saberClashBlock). Heavy attacks still need a perfect parry. The only times block can't save you are when you've been visibly punished: your swing got knocked away (broken parry), you're knocked down, or staggered. Saber locks only happen when both swings are heavy, and not over and over
Block release grace: your block stays up for a moment after you let go of it (350ms, g_blockReleaseGrace). If a hit lands in that time while you're only starting an attack (wind-up/transition, not the actual strike), your attack is cancelled into a parry instead of eating the hit -- since you can't hold block and attack at the same time, letting go to swing no longer leaves you wide open. No grace if your swing already got knocked away or you're mid-strike
g_debugBlock 1: whenever a saber hit gets through while you're blocking, the console says why (block not active, out of Force, mid-animation, their swing broke your parry and with what numbers, hit from behind/the side, etc.)
NPCs with a guard pool show a thin orange composure bar under their health bar; flashes white while broken
Boss guard stages: bosses (Desann, Tavion, Alora, Kyle and other boss characters) fight in stages -- their health is split into thirds (g_bossGuardStages), and you can't cut through a stage while their guard is up (hits that would go past it wear their guard down instead). Break their guard and you can take that stage -- but only that one. Each time they lose a stage they blast you back with Force Repulse, are untouchable for a moment (1.5s, g_bossStageRecover) and come back with a stronger guard. No more bosses dropping to two lucky hits
Enemies use Force Repulse too: bosses get it automatically, and any NPC can be given it in its .npc file (FP_REPULSE). They use it to clear space when two or more enemies crowd them, or to get you off them when they are trying to recover -- watch for the two-handed charge pose. Scripts can grant it with setForceRepulse / SET_FORCE_REPULSE
New .npc key "guardStages N": set how many guard stages an NPC fights in (bosses default to g_bossGuardStages, everyone else 1 = none; any NPC with a guard pool can opt in)
Boss stages stick: Drain and Heal can no longer heal a boss back into a stage you already took off them
One saber swing now hits you (or an enemy saberist) once -- the blade staying in contact used to land 2-5 full hits from a single swing
Saber-lock wins and stab-downs only do their uncapped finishing damage to someone who actually lost the lock or is on the ground -- no more 1000+ damage hits from a lock-win animation brushing past you
Fixed: after an enemy's heavy attack got interrupted, their next normal swings were still treated as heavy for a few seconds (your swing wouldn't clash-block them and normal blocks failed)
Master duelists read a flurry after about 5 swings in a row, not 2-3
Enemy saber hits on you now depend on who is swinging: a basic saberist takes about 4-5 hits to kill you, trained Jedi/Reborn about 3, bosses 2. Telegraphed heavy attacks hit 1.5x harder, Easy/Hard scale it x0.75/x1.25, and nothing but a lost saber lock or a stab while you are down one-shots you. Realistic saber combat now only makes YOUR hits lethal; hits on you follow the same numbers (g_saberPlayerDamageScale to tune)
Getting your swing knocked away or staggered is punished once: the hit that lands ends it and you can block again
Hit indicators: red marks around the crosshair point at whoever just hit you, and turn with you (cg_damageIndicator)
Distinct sounds when you break an enemy's guard and when a hit gets through your block (g_guardBreakSound / g_blockFailSound)
Snipers show a red laser while they charge a shot, and live enemy grenades and det packs near you are marked on screen (or at the edge of the crosshair ring when behind you) (cg_threatCues)
Enemy katas have a cooldown (they used to be rolled every moment you were in range) and play the same warning sound as a heavy attack
Fixed: a thrown saber hit you (or an enemy) every frame it was touching you; it now hits once on the way out and once on the way back, at the same damage as that enemy's swing. Enemies also throw their saber less often (every 8-14s at most)
New difficulty file ext_data/difficulty.cfg: enemy bolt damage, weapon skill, saber damage to you, your parry window, enemy nerve, enemy guard and how often they throw heavy attacks, per difficulty (replaces g_npcDamageEasy/Normal/Hard). Easy now also gives a wider parry window, weaker guards and fewer heavies; Hard the opposite
Frame rate no longer changes how you move: at low frame rates your movement now runs in small steps (g_playerMaxStep), so jumps and landings match high frame rates. Force regen no longer slows down at low frame rates
Ledge assist: hold jump and forward into a ledge just out of reach and you get lifted onto it (g_ledgeAssist)
Note: savegames from before this update won't load


HEAVY ATTACKS
--------------------------------------
Saber-capable enemies can now telegraph a heavy attack: a visibly slowed windup into a normal-speed strike, so you get a real tell instead of every swing looking the same -- one enemy can only do this once every 7 seconds (g_heavyAttackCooldown)
Only a perfect parry stops a heavy attack cleanly -- perfectly parrying one breaks the enemy's guard outright. Blocking it with bad timing breaks your block: a long stagger where you can't block or attack (0.9s, g_heavyBlockStagger), most of the damage still gets through (x0.75, g_heavyBlockDamage) and it costs Force (g_heavyBlockForceCost). Not blocking at all takes the full hit
Heavy attacks now have a sound on the windup as well as the glow (g_heavyAttackSound) so you can time the parry by ear
Enemies pick their moment for a heavy attack instead of rolling dice: when you're recovering or off balance, low on Force, mid-swing or backing off -- and they avoid throwing one into a ready guard. Better fighters (higher blockSkill) read it better (g_heavyAttackThreshold)
A Force push/pull/repulse/grip that actually staggers someone now locks out the caster's own follow-up attack for a share of the victim's recovery time (g_forceStaggerLockoutScale, 60%) so you can't just push someone down and get a free hit every time
Katas now count as heavy attacks too -- mistiming a block against one gets the same broken-block treatment


SABER DAMAGE
--------------------------------------
A real saber swing is lethal to regular enemies wherever it lands -- troopers, officers and the like take at least 80% of their health from any proper hit. Just brushing someone with the blade while it's transitioning or blocking no longer counts as a near-kill (it used to do a minimum of 25)
Cutting off a regular enemy's hand is non-lethal: the hand comes off, they scream, drop their gun, stagger for a moment, then either throw their hands up or run from you (and give up once cornered). They count as killed so missions keep progressing, but you can still finish them off //needs testing; a fuller non-lethal system will come later
Against saber fighters, bosses and you, damage depends on where the blade lands: head x1.5, torso x1, arms/legs x0.8, hands/feet x0.5 -- a swing through the neck now actually counts as a head hit
Fixed saber hit locations being applied twice against saber users (an arm hit could end up at a quarter damage), and the same chest/helmet detection fixes the blasters got
Removed the pile of per-difficulty saber damage caps (bosses could only ever take 10-25 per hit, you could only take 7-19 from a boss, etc). Saber fight difficulty now comes from the guard/parry system; the one cap left is that a single saber hit never takes more than half your health (g_saberMaxPlayerHitPct)
g_debugDamage 1 prints each saber hit's location, raw contact damage and final damage


SABER STYLES
--------------------------------------
Each style now plays differently mechanically, not just visually:
  Every style has its own parry recovery time
  Fast and Dual: quicker on your feet
  Strong: bonus guard damage -- breaks an enemy's guard noticeably faster
  Staff: blocking blaster fire costs less and covers a wider angle
  Dual: exempt from the "blocking the same direction over and over" penalty
New dedicated Kata keybind (+kata, in the controls menu) -- the old attack+alt-attack combo stopped working once block took over the right mouse button, so katas were basically unreachable before
Only boss-tier enemies get their guard instantly refilled after a break now; regular saber users have to recover it
While an enemy's guard is still intact they shrug off part of your saber damage, so breaking guard first actually matters
Saber clashes/bounces kick in with partial Saber Offense investment now, instead of needing it maxed


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
Running dry no longer leaves you stuck spinning with an empty gun -- it spins down and auto-switches like everything else
NPCs with the Z-6 now lay down long suppression streams (longer and more frequent on higher difficulties) instead of short bursts
// Fixed this round: muzzle flash/shot trail effects were firing slower than the gun itself, so most shots showed no visual at all; missing shader definitions were throwing console warnings; dropped Z-6 pickups showed the repeater's world model instead of their own; max-heat spread was a little much and got trimmed down


REPEATER REWORK
----------------------------------------
Primary fire is a 3-round burst rather than straight full-auto -- holding the trigger keeps firing burst after burst automatically, no need to release and re-press
Alt fire ("blob") charges the longer you hold it, throwing farther with a longer charge
A burst always finishes once started -- tapping the trigger can't cut it short or turn it back into full-auto
Alt fire now has a charge-up hum and a glow on the barrel while charging
NPCs with the repeater fire the same 3-round bursts
// Fixed this round: an instant tap on alt fire was getting the same power/range as a "properly" charged shot -- there was no actual weak/uncharged state at all. Quick taps are now noticeably weaker and shorter-range, full charge unchanged


CYCLER RIFLE / AMBAN SNIPER (Replaces Disruptor for player and will eventually for NPCs)
------------------------------------------------------------
New weapon using the Amban model and MB2 skeleton animations
Cycler rifle that needs to be reloaded in between shots.
Keeps disruptor mechanic, but needs to charge for longer.
Shots are real travel-time bolts now instead of instant hitscan beams, and hit harder; headshots do double damage. Disintegration is gone from this weapon -- a full charge is one heavy bolt rather than a beam through several enemies


KICK SYSTEM
-----------
Kick (+kick) is now available to all saber styles for the player
Previously restricted to double bladed saber
//TODO: Be able to throw Staff Saber


BLASTER BALANCING
-----------------
Blaster bolt velocity: 2300 -> 3000
Every blaster now has its own role instead of being a reskin of the others:
  Bryar pistol: precise sidearm, 18 dmg, one shot every 0.6s, charged alt
  Blaster pistol (DL-44): slow heavy hand-cannon, 26 dmg, one shot every 0.8s -- also fixed it secretly using the Bryar's damage
  E-11: the all-rounder, 20 dmg, one shot every 0.45s, alt still sprays
  Repeater: 3-round bursts (rounds 0.1s apart, 0.75s between bursts), 12 dmg per bolt
  Z-6: suppression -- 12 dmg at a very high rate, heat ruins accuracy on long strings
  Bowcaster: heavy single shots, 55 dmg, slow; charged explosive alt
  Disruptor / Cycler: snipers -- very fast bolts, double damage headshots
NPCs now shoot exactly like you do: same fire rates, same bursts, same bolts. No more NPCs firing faster than the gun can, or a different set of hidden "NPC damage" numbers per weapon //the old tables were all over the place, some NPCs did a third of your damage with a gun and others did full damage with another
NPC damage = your damage x a difficulty multiplier (g_npcDamageEasy/Normal/Hard, 0.4/0.6/0.8), and NPC bolts fly a bit slower (g_npcBoltSpeed, 0.75) so you can still dodge and deflect them
New NPC weapon skill (1-5): low-skill troops spray more, fire shorter strings, pause longer between shots and take longer to open fire on you; elite troops are tight, sustained and quick on the trigger. Set per NPC with "weaponSkill" in the .npc file, everyone else uses g_npcWeaponSkill (3). Difficulty shifts it by one either way
Weapon stats (velocity, spread, bursts, NPC pacing) all live in weapons.dat now, and the custom weapon slots can actually fire -- a new blaster is just a weapons.dat/items.dat entry //baseline for adding more guns later
DEMP2, flechette, concussion rifle and rocket launcher are on the same system too: NPCs fire them at your rate with the same skill rules. Heavy weapons have a per-weapon NPC damage scale (npcDamageScale) so a trooper with a concussion rifle or rocket launcher doesn't one-shot you, and Boba Fett's rockets use it too
DEMP2 fires about 3x slower (1.35s), and its charged alt now actually scales with charge: the shockwave hits for 1x/2x/3x damage (it used to read the repeater's numbers and the charge did nothing)
Homing rockets keep the speed they were fired at instead of snapping back to a fixed speed
Bowcaster charged alt-fire now has a big red blast and shock ring that shows its actual damage radius (primary impacts unchanged)
NPCs no longer get bonus health on higher difficulties (+25%/+50% before) -- difficulty now comes from how hard they hit and how well they shoot. Bosses (Tavion, Kyle, Alora) still scale
Better hit detection for guns: headshots on helmets now count as headshots (they used to register as no location and lost the 2x), chest shots no longer get downgraded to arm hits just for being a little off-centre, and an NPC's arm raised across its chest while aiming no longer soaks your chest shots. g_debugDamage 1 also prints which part of the model each bolt hit
Note: savegames from before this update won't load


BLASTER VISUALS
-----------------
New bolt look for the blaster, both pistols, repeater and Z-6: thin hot core with a proper red glow that actually blooms with dynamic glow on //going for a Battlefront II feel
Repeater and Z-6 bolts are red now (repeater was blue, Z-6 read orange)
Muzzle flashes glow, are about half the size they were, and sit on the barrel properly instead of slightly above it
Bolts hitting walls get an extra red flash and a few glowing sparks on top of the usual impact
Z-6 barrel glow at high heat is red to match


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
Nerve replaces squad morale: every NPC has its own nerve (steady / shaken / breaking / broken) that drops when they see allies killed, maimed or taken out, get hit, near-missed, caught in explosions or hit with the Force, and rises when they land hits, get reinforced or their side gets a kill. Officers, numbers and wounds all count. Broken NPCs drop their gun and run or put their hands up (g_npcSurrender)
Squads are explicit now: NPCs placed together (or sharing a squadname key) and anyone nearby when a fight starts form a squad of up to 8 -- no more every-enemy-on-the-map-is-one-group. The highest rank leads and calls out lines as the leader's nerve shifts
Gunners pick what to do from a scored list (hold cover, relocate, advance, flank, suppress, grenade, stand and fire, retreat, flee, track you down) instead of a pile of flags -- at most half a squad moves at once while the rest covers them, and two flank from different sides
Snipers, grenadiers and Tusken now run on the same AI as stormtroopers with their own profiles (snipers hold and relocate, grenadiers lob from range, staff Tusken close in and lunge); .npc files can pick a profile with aiProfile
Companions fight on a leash and come back to you when you run off, instead of standing their ground no matter what
Enemies spread their fire across you and your allies instead of only ever shooting at the player
Stormtroopers fall back on geometry-based cover searches when no pre-placed cover point is nearby, holding a duck/pop-out firing pattern instead of standing in the open; squads coordinate so at least half the group keeps firing while the rest repositions, rather than everyone exposing at once
NPC think rate now scales with their Reactions stat, so elite troops react noticeably faster than grunts instead of everything sharing one tick rate
Stormtroopers reposition more often, fire while moving/retreating more, and hold a line-of-sight "grace window" before breaking off a shot; Reborn/Sith without chase-enemies flags now slowly advance instead of standing idle, and their rocket/saber missile-reflect chances are now probabilistic instead of guaranteed
New NPC_StormtrooperRandom spawner: drops a weighted-random stormtrooper variant (rifle/heavy/officer/grenadier) from a config file instead of needing a distinct classname per spawn -- add new variants by editing the .npc/.cfg, no recompile
Various stormtrooper fixes: grenadier now switches back to its blaster properly after throwing, saber no longer flickers off/on during door-cam cutscenes, fixed stormtrooper_random always falling back to the hardcoded variant list
Toned down how much NPCs strafe and jump around during saber duels -- was getting a bit chaotic to actually read what they were doing
Fixed stormtroopers refusing to shoot at you while you backpedal or stand side-on to them (with a gun or a saber) -- they were treating the third-person camera's position as where your body was, so every shot looked blocked
Disarmed NPCs now react instead of standing there: they run for cover and look for a weapon on the way, surrender (cower, hands up) if their squad's morale is broken or you corner them with a lit saber, and otherwise go down swinging in a desperate melee
NPCs can re-arm from weapon racks placed in the level, as well as picking up dropped weapons like before


NON-LETHAL
--------------------------------------
Enemies can be taken alive. Blunt finishing blows knock them out instead of killing them: kicks, fists, the stun baton, a full-charge DEMP2 alt shot, or slamming them into a wall with push/pull. Lethal weapons are still lethal
Taken-out enemies (knocked out, surrendered, fled, hand severed) count as dealt with for the mission -- doors, triggers and scripts fire like a kill would -- they drop their weapon and keys, and show up as "Enemies Subdued" on the mission stats
Disarming a rattled enemy makes them give up; mind trick 3 knocks someone out for 75 Force
A teammate who finds someone knocked out will kneel down, wake them up, and the whole squad comes looking for you
Bosses, Force users, droids and scripted characters can't be taken alive; g_nonLethal 0 turns it all off


STEALTH
--------------------------------------
Enemies notice you in stages instead of instantly: unaware, suspicious ("?" over their head -- they look your way), searching ("!" -- they come and check), then alert. Searches end after a while if they don't find you
Light, distance, movement and crouching all matter; the light check counts ambient and dynamic light (your own lit saber included), and a meter shows how lit you are while crouched (cg_drawLightMeter)
Noise is consistent: crouch-walking is nearly silent, walking is quiet, sprinting and landings carry. A lit saber is a visible glow, not an instant giveaway
Saber and melee kills are silent -- only enemies who see it know
Bodies get found: a first body makes them suspicious, a second one gets the squad searching. Hit use on a body (dead or knocked out) to drag it out of sight
Sneak up behind someone who hasn't noticed you and hit use to knock them out in one punch (placeholder animation for now). Force users can't be snuck up on
Mind trick 1-2 makes them forget what they saw; the level 2 distraction draws them to the spot
g_stealth 0 restores the old instant spotting if anything goes wrong


SABER DUELS (AI)
--------------------------------------
Saber enemies fight with a plan now: they press, defend and wait for you to over-commit, circle at the edge of reach, back off to recover Force, open with a push or pull, throw their saber, or taunt -- instead of flailing constantly
They read your actual swing: fast attacks get blocked, heavy ones get side-stepped by the better fighters, and grunts sometimes misread a swing completely. How good they are comes from the .npc blockSkill (or their rank)
NPC blocks cost Force just like yours do (g_npcBlockCost); run them dry and their blocks fail. Skilled ones can land perfect parries on you too
In group fights one presses at a time while the others circle and swap in
Your saber target's health, guard and Force bars show without Force Sight
Riposte: attack right after a block and the swing comes out immediately
A kick against a saber user whose guard is still up gets blocked -- break their guard first


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
rend2 updated to SomaZ's latest (Aug 2026). New options: real bloom on glowing effects (r_dynamicGlowBloom 1-2), SMAA anti-aliasing (r_smaa 1, or 2 for temporal), volumetric fog lit by the map (r_volumetricFog 1), chromatic aberration on refraction (r_refractionChromaticAberration)
Faster map loads and vid_restart (compiled shaders are cached), fixed missing projectile splash effects, plus a long list of renderer bug fixes
Animated md3 models now animate smoothly instead of snapping between frames
cg_fovViewModel now works correctly with cg_fovAspectAdjust and FOV changes


CAMERA
--------
Third-person camera now shifts between three profiles depending on what you're holding -- a looser "explore" view with the saber holstered, a tighter dueling view with the blade lit, and a closer over-the-shoulder "shooter" view for guns -- smoothly blending between them and re-tracing to avoid clipping into walls
Shooter mode aims to fix the old mismatch between the 3D crosshair and where shots actually land //inspired by Jedi Survivor's camera, still tuning the exact offsets
Point-blank shots now land where the crosshair is (even with the gun pressed against an enemy), and Force push/pull grab whatever is under the crosshair in third person


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
- Change the JK_VERSION define in codemp/qcommon/game_version.h from "OpenJK" to your project name
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
