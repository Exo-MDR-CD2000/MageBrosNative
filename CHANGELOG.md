# Changelog

What changed in each version of Mage Bros. Native. Every version is published as a
[GitHub release](https://github.com/Exo-MDR-CD2000/MageBrosNative/releases) with its Windows zip.
You still need your own copy of Mage Bros. 2.0 (the freeware build) to play: put
`MageBrosNative.exe` next to `MageBros.exe`.

Where we are and what comes next: [docs/ROADMAP.md](docs/ROADMAP.md).

## [0.3.6] - 2026-10-24

- **The ending**: clearing World 4's last level now plays the original's ending instead of
  jumping back to the logo. The two mages walk side by side down a long corridor between
  scrolling film strips, with the menu music, while four credits slide past: the creator, the
  thanks, "The End" and the Saito Games logo. Press a button on the last one to return to the
  logo.

## [0.3.5] - 2026-10-24

- **The shop**: walking up to a shop's door opens the original's MAGIC SHOP and pauses the
  level. Spend gold on a heal potion (one more heart), a magic potion (magic refills faster) and
  two wands that change with each world. Items you can't afford or already have are greyed out.
  The board shows the shopper's magic level and wand. Each player shops with their own keys or
  gamepad and their own gold. As in the original, Enter doesn't buy (use your fire or jump key),
  and Escape leaves the shop but also gives up the level.
- **The wands**: each wand now fires its own shots, as in the original. The double wand fires
  two shots side by side. The triple wand fires one shot and two smaller ones spread wider. The
  X wands fire the same patterns as homing shots: after a moment each turns towards the enemy it
  picked as it was fired, or the boss if no enemy was near.

## [0.3.4] - 2026-10-24

- **Level select**: after the save slots comes the original's world map, with the level dots
  of each world lit up to the level reached, a tick on each cleared world, and your gold and
  life at the top. START plays the marked level, SELECT LEVEL moves the marker to any level
  reached, BACK saves and returns to the title.
- **The real level endings**: losing a level now saves and returns to level select (it no longer
  just restarts), and clearing a world's last level saves and returns to level select at the next
  world, as in the original.

## [0.3.3] - 2026-10-24

- **The save slots**: after the title comes the original's slot screen, with 8 slots for
  one-player games and 8 for two-player games. Each saved slot shows when it was saved and the
  level reached. Choose a slot to continue it (an empty one starts a new game), or press Delete
  to delete a saved game, answering OKAY or CANCEL. Until level select arrives, a slot starts
  straight at the level reached.
- The menu music carries on from the title into the save slots, and a level replayed after a
  death keeps its music playing, as in the original.
- Jumping between levels with the test keys (N / B, F2) no longer saves progress.

## [0.3.2] - 2026-10-24

- **Saving**: progress is saved after every level, as in the original: the level you have
  reached, the worlds you have cleared, and each player's gold, life, magic and wand. Saves go to
  `%APPDATA%\MageBrosNative\saves` (or the folder set by `save_path` in `magebros_native.ini`),
  written safely with a backup of the previous save. Saves from the original game are not
  carried over.
- Until the save slots arrive, 1 PLAYER and 2 PLAYERS continue the first slot of their kind (a
  new game the first time).

## [0.3.1] - 2026-10-24

- **The title screen**: after the logo comes the original's title, with its scrolling
  background, the pulsing Mage Bros. logo, the two mages fading in and the menu music. Choose
  1 PLAYER, 2 PLAYERS or EXIT with the arrow keys, W / S or a gamepad, and Enter, Shift, Ctrl or
  a gamepad button; Escape picks EXIT. Until the save slots and level select arrive, both games
  start at World 1-1.

## [0.3.0] - 2026-10-24

The front end begins (stage 6).

- **The logo**: the game opens on the original's Saito Games logo with its jingle, as the original
  does, before going into the game (the title screen comes next).
- **Fades**: every level fades in and out with the original's venetian blind of black strips.
  A cleared level waits 2 seconds before fading out to the next, a lost one 3 seconds.
- Escape only gives up a level once it has finished fading in, as in the original.
- Leaving world 3 goes back to world 1 until worlds 1 to 3 are all cleared, which opens world 4,
  as in the original. After the last world the game goes back to the logo for now (the ending
  comes later in stage 6).

## [0.2.24] - 2026-10-24

- **Player 2's shadow on platforms**: player 2 now casts its shadow on the platform under it, as
  player 1 does.
- Player 2's status panel was checked against the original's drawing code: it is drawn exactly as
  player 1's.

## [0.2.23] - 2026-10-24

- **The mages bump into each other** and no longer pass through each other.
- **Standing on each other**: a mage can land on the other's head and stand there, jump off it,
  and bump its head on the mage above when jumping underneath. The mage underneath carries the
  one on top as it walks, and on a lift both go up and down together.
- A mage floated by a floor fan doesn't land on the other one.

## [0.2.22] - 2026-10-24

- **Two players**: press F2 in the game (it restarts the level) or start with `--two-player`.
  Player 2 is the second mage, with D, A, W, S to move, Left Ctrl or `1` to fire, Left Shift or
  `2` to jump, or the second gamepad. Each player has their own life, gold and magic and their
  own colour of shot, and the status display shows both. The camera follows the point between
  the two mages. Enemies chase whichever mage is closer, and coins go to whoever picks them up.
- In a two-player game the level is cleared once one mage has warped and the other has warped or
  died, and lost only when both have died.
- Not yet: the mages can't stand on or carry each other (they pass through each other), and
  player 2 casts no shadow on platforms. Both come next.

## [0.2.21] - 2026-10-24

- **The wizard's wand**: it holds out its wand, glowing at the tip, and fires fireballs that grow
  as they fly and home in on the mage, faster and more often as the attack goes on.
- **Every object in all 40 levels is now in the game**: all four worlds can be played with
  player 1. (Two-player co-op comes next.)

## [0.2.20] - 2026-10-24

- **The wizard**, World 4's boss (4-10), with 10 hearts: it rises out of the pit when the mage
  reaches the far edge of the arena, glides in, circles the middle faster and closer, then stops on
  the other side and glides out to attack.
- **The wizard's blade**: a great spinning blade whose wind blows the mage along the arena towards
  the pit. (Its wand attack comes in the next version; until then it skips those turns.)

## [0.2.19] - 2026-10-24

- **The nest**, World 3's boss (also in 4-8): a great nest floating in the water, turning slowly,
  with 10 hearts. Every hit makes it bob and splash and sends one of its 40 birds after the mage;
  each bird chases the mage for a while, circles the nest and flies back in. When the nest falls,
  every bird out of it bursts.
- **All of World 3 is playable**, 3-1 to 3-10.

## [0.2.18] - 2026-10-23

- **Ghosts** (World 3, also 4-9): they drift about through walls, bobbing in the air. Get close
  and one turns to face the mage, then swoops down at it with a wail. Shots can't kill a ghost,
  but they knock it back.
- **World 3 is playable up to 3-9**: only the boss level, 3-10, is left.

## [0.2.17] - 2026-10-23

- **Floor fans** (World 3, also 4-9): fans set in the floor whose spinning blades blow upward.
  Jump over one and the mage floats up on the air, bobbing about two body heights above it; its
  blades still hurt if you walk into them.
- Level 3-4 now has nothing left to build.

## [0.2.16] - 2026-10-23

- **Rocket launchers** (World 3, also 4-1, 4-4 and 4-7): every 10 seconds a rocket rises slowly
  out of the launcher, turns to face the mage, then launches and homes in on it, trailing smoke.
- Level 3-3 now has nothing left to build.
- This changelog, and each GitHub release now shows its section of it with a link to the whole
  file. (The release workflow also moved to `actions/checkout@v7`.)

## [0.2.15] - 2026-10-23

- **Stalactites** (World 3): they hang from the ceiling, drop spinning when the mage comes near,
  and land with a thud, a screen shake and a cloud of dust, staying stuck in the floor.
- Levels 3-2, 3-6 and 3-9 now have nothing left to build.

## [0.2.14] - 2026-10-23

- **Jump needles** (World 3): spikes sunk in the floor that spring up when the mage comes near,
  stay up for 2 seconds and sink back.
- Level 3-1 now has nothing left to build.

## [0.2.13] - 2026-10-23

- **Pots** (World 3): large vases and small pots that block the way.
- **Kiwis** (World 3): they wander, and run away when the mage comes close or shoots them.

## [0.2.12] - 2026-10-23

- **The genie's attack**: it opens its mouth and fires spiked balls that roll after the mage.
- **All of World 2 is playable**, 2-1 to 2-10.

## [0.2.11] - 2026-10-23

- **The genie**, World 2's boss (also in 4-5): it hides in its lamp until the mage comes near,
  the lamp rocks and steams, and the genie rises out of the spout. It floats about, turns to face
  the mage, glides between three spots and has 10 hearts. (Its attack came in 0.2.12.)

## [0.2.10] - 2026-10-23

- **Turtles** (World 2): they walk slowly and can't be killed; a shot sends them into their
  shells for a few seconds.
- Levels 2-1 to 2-9 now have nothing left to build.

## [0.2.9] - 2026-10-23

- **Fans** (World 2): fans on posts that blow the mage along the level or across it.

## [0.2.8] - 2026-10-23

- **Hammers** (World 2): a spiked roller that rises when the mage comes near, turns to face it
  and slams down, front or back.
- **Screen shake**, as in the original, when a hammer lands.

## [0.2.7] - 2026-10-23

Also contains 0.2.6, which was built for testing but not released on its own.

- **Needle balls** (World 2): spiked balls that roll back and forth.
- **Squids** and the king squid (World 2, also World 4): they crouch, jump in place to turn and
  walk.

## [0.2.5] - 2026-10-23

- World 2 begins: **trees**, **potted plants** and hopping **mushrooms** (with the king mushroom).

## [0.2.4] - 2026-10-23

- The game view clears to black, as the original does, so the pits show black under the water.

## [0.2.3] - 2026-10-22

- **Wall candles** on the back wall, with their flames.

## [0.2.2] - 2026-10-22

- The original's lighting and lens, and its handling of scaled models' lighting.

## [0.2.1] - 2026-10-22

- **Fence doors** are now the wooden gates of the original (they had been built as metal fence
  pieces, which made 1-2 impossible to finish).
- **The shop** building stands in 1-3, 1-6 and 1-9 (its screen comes later).

## [0.2.0] - 2026-10-22

- **World 1 is playable from start to finish**, 1-1 to 1-10, with player 1.

## [0.1.4] - 2026-10-22

- Pits fixed: the cliff faces are built only under floor edges, as in the original.

## [0.1.3] - 2026-10-22

- **The dragon**, World 1's boss: it swims round its pools, crosses the floor breathing fire and
  shows its hearts on the status display; defeating it opens the goal.

## [0.1.2] - 2026-10-22

- **Water** in the pits (falling in costs a life, with a splash) and **fish** that leap out of it.

## [0.1.1] - 2026-10-22

- **Cannons** and their cannonballs, and all seven kinds of **moving platform**.
- `make_release.bat` raises the version itself and runs the game tests.

## [0.1.0] - 2026-10-21

The first test build:

- Reads every asset from your own `MageBros.exe`; sound, MIDI music, keyboard and gamepad, the
  options menu.
- A level viewer showing all 40 levels, and a model viewer.
- World 1 with player 1: movement, shots, magic, torches, coins, the goal and warp, the status
  display, the death sequence, shadows and particle effects, slimes, bees, wallabies, spikes,
  needle fans, barrels and fence doors.
