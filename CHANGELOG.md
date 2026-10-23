# Changelog

What changed in each version of Mage Bros. Native. Every version is published as a
[GitHub release](https://github.com/Exo-MDR-CD2000/MageBrosNative/releases) with its Windows zip.
You still need your own copy of Mage Bros. 2.0 (the freeware build) to play: put
`MageBrosNative.exe` next to `MageBros.exe`.

Where we are and what comes next: [docs/ROADMAP.md](docs/ROADMAP.md).

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
