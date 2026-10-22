# Roadmap

Each stage ends with a Windows zip you can test with your own `MageBros.exe`.

| Stage | What it delivers | Status |
|---|---|---|
| 1 | Asset loading from `MageBros.exe`, ColDet collision library, Windows build | Done |
| 2 | Engine: window, OpenGL renderer, sound, MIDI music, keyboard/gamepad, options menu | Done |
| 3 | Level viewer: all 40 levels built by the original map code | Done |
| **4** | **First playable: World 1 with player 1** | **In progress** |
| 5 | The whole game: all 4 worlds, every enemy, platform and boss, two-player co-op | |
| 6 | Front end: title, save slots, level select, shop, ending, saving | |
| 7 | Modern extras: widescreen, high refresh, MSAA, rebinding, music replacement, mods | |

## Stage 4, chunk by chunk

| Chunk | What it covers | What you can test | Status |
|---|---|---|---|
| 1 | Level start, camera and music decoded | viewer camera fixed | Done |
| 2 | Model library, original camera, game scene | the mage standing in World 1-1 | Done |
| 3a | Movement analysis: hero update, input, map collision | (notes) | Done |
| 3b | Movement code (`XHero`, `XMapCollision`), headless test on all 40 levels | drop in, run, jump, fall, blocked by walls and gaps | Done |
| 4a | Level loop analysis: game flow, goal, torches, shots, coins, magic, sound panning | (notes) | Done |
| 4b | Level loop code (`XLevelLoop`): shots, magic, torches, coins (ColDet), goal, warp, damage | fire shots, light the torches, collect coins, warp to the next level | Done |
| 4c | Status display (`XStatusDisplay`, `JSprite`, `JNumber`), damage flash, stereo sound, the death sequence | the original HUD; panned sounds; dying costs gold | Done |
| 4d | Shadow (`XShadow`) and particle effects (`XParticleList`, `JObject3D`) | the hero's shadow, shot trails and flashes, torch flames, coin sparkles, warp rings and lights, landing dust, explosions | Done |
| 5a | Enemy framework (`XEnemyList`, ColDet hits, damage, defeat, coin drops), static objects (`XStaticObjList`: pillars, barrels, fences), slimes and spikes | shoot slimes, collect their coins, avoid spikes | Done |
| 5b | Bees, wallabies, barrels, fence doors, needle fans | the rest of World 1's enemies (fish aside) | Done |
| 5c | Cannons and their shots (`XBulletList`), platforms (all seven kinds, A to G) | levels 1-4 to 1-9: cannons firing rolling balls, elevators, shuttles and fixed platforms that carry the mage | Done |
| 5d | Water (`XWater`) and fish (`XEnemy_Fish`) | the water in the pits of every World 1 level, falling in, waves; fish in levels 1-3, 1-4, 1-7 and 1-9 | Done |
| **5e** | **The dragon (World 1's boss)** | **level 1-10** | **Next** |
| 6 | Polish and the playable zip | World 1 start to finish | |

Detailed next steps for 5e onward (which original classes to read, where each piece plugs in)
are in [CLAUDE.md](../CLAUDE.md), "Where we are".

## Open items

| Item | Status |
|---|---|
| Which levels are boss levels | Settled: any level with a boss code (`6`–`9`); level 10 of each world and World 4's levels 2, 5, 8 |
| Which way the mage faces at the start | Settled: yaw 0, along the level |
| Two-player camera midpoint, camera shake | Stage 5 (player 2); camera shake with the boss (5e) |
| Hero-on-hero collision and carrying | Stage 5 (player 2) |
| Static objects' collision with the hero | Done (5a): `XStaticObjList` and `XCollideBox` as the original |
| Sound panning | Done (4c): pan, fade and per-sound copies as `JSound::PlayStereo` |
| Death and the last level of a world | Restart / next world as stand-ins until level select exists (stage 6) |

## Verification

- Format, collision and map tests that need no game files (`ctest`), plus `mbn_map_tests` and
  `mbn_hero_tests` on all 40 levels of your copy (configure with `-DMBN_GAME_EXE=...` to run them
  in `ctest`). The hero test lands the mage on every start floor, checks the jump height (74.82,
  the original's per-tick result) and runs 800,000 ticks of random play without the box ever
  ending up inside the level.
- `MageBrosAssets --selftest` against your copy.
- For game modules: side-by-side comparison with the original on recorded inputs once the game
  loop runs.
