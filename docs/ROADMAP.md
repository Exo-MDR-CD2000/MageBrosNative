# Roadmap

Each stage ends with a Windows zip you can test with your own `MageBros.exe`.

| Stage | What it delivers | Status |
|---|---|---|
| 1 | Asset loading from `MageBros.exe`, ColDet collision library, Windows build | Done |
| 2 | Engine: window, OpenGL renderer, sound, MIDI music, keyboard/gamepad, options menu | Done |
| 3 | Level viewer: all 40 levels built by the original map code | Done |
| 4 | First playable: World 1 with player 1 | Done |
| 5 | The whole game: all 4 worlds, every enemy, platform and boss, two-player co-op | Built: chunks 1–18 done; chunk 19's play-through waits for the owner's full test (below) |
| **6** | **Front end: title, save slots, level select, shop, ending, saving** | **In progress**: chunks 1–5 of 9 done (below) |
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
| 5e | The dragon, World 1's boss (`XBossBase`, `XBoss_Dragon`, its 16 body segments, `JSpline`, `XBullet_Flame`) | level 1-10: the dragon swims, crosses the floor breathing fire, shows its hearts on the status display; defeating it stops the music and opens the goal | Done |
| 6 | Checks and the playable World 1 zip | the remaining effects and the objects World 1 doesn't use (the turning cannon, the straight cannonball, platforms `B` and `F`) checked by screenshot; World 1 played start to finish by the owner | Done |

## Stage 5, chunk by chunk

Small chunks, in level order: each one makes the next levels playable. Each chunk is analysis,
code, the RE notes, checks by screenshot and a Windows zip. "Levels" lists where the new
objects first appear; "Playable" lists the levels that have nothing left unbuilt once the chunk
is done.

| Chunk | What it covers | Levels | Playable | Status |
|---|---|---|---|---|
| 1 | World 2 scenery and the mushroom: tree `T`, plant `U`, mushroom `h` / king mushroom `2` | 2-1 to 2-10 | | Done |
| 2 | Needle balls `x` `y` | 2-1, 2-2, 2-4, 2-5 | 2-1 | Done |
| 3 | Squid `c` / king squid `4` | 2-2 to 2-9 | 2-5 | Done |
| 4 | Hammer `k` and camera shake (`XCamera::StartShaking`) | 2-2, 2-4, 2-6, 2-9 | 2-2 | Done |
| 5 | Fans `q` `r` `s` | 2-3, 2-4, 2-6 to 2-9 | 2-3, 2-4 | Done |
| 6 | Turtle `e` | 2-6 to 2-9 | 2-6 to 2-9 | Done |
| 7a | The genie, World 2's boss (`7`), and its lamp (`XStaticObj_GenieLamp`, the lamp steam): hiding, steaming, appearing, moving, opening its mouth, its hearts, taking damage and defeat; its attack state does not fire yet | 2-10 | | Done |
| 7b | The genie's attack: needle-ball bullets (`XBullet_NeedleBall`) fired from its mouth | 2-10 | all of World 2 | Done |
| 8 | Pots `M` `N` and the kiwi `f` | 3-1 to 3-10 | | Done |
| 9 | Jump needle `z` | 3-1 onward | 3-1 | Done |
| 10 | Stalactite `u` (shakes the camera) | 3-2 onward | 3-2, 3-6, 3-9 | Done |
| 11 | Rocket launcher `R` and its rocket | 3-3 onward | 3-3 | Done |
| 12 | Floor fan `t` | 3-4, 3-5, 3-7 | 3-4 | Done |
| 13 | Ghost `g` | 3-5, 3-7, 3-8 | 3-5, 3-7, 3-8 | Done |
| 14 | The nest and its birds, World 3's boss (`8`) | 3-10 | all of World 3 | Done |
| 15a | The wizard, World 4's boss (`9`): hiding, appearing, gliding, circling, its hearts and defeat, and its blade, which blows the mage along; its wand turns go straight back in for now | 4-10 | | Done |
| 15b | The wizard's wand and its fireballs (`XBoss_WizardWand`, `XBullet_WizardFire`, `XParticle_WizardFireFlash`; `XCollideSphere` is the bullets' radius); every object code in the 40 levels built; World 4 played level by level by the owner | 4-10 | all 40 levels | Done |
| 16 | Player 2 (`XHeroB`): its models, keys and gamepad, its own status, the camera midpoint, the two-player start and level end; every enemy, boss, bullet, coin, torch, platform and the shop check both heroes; each player's shot colour; player 2's HUD panel; F2 / `--two-player` | all | | Done |
| 17 | Hero-on-hero collision and carrying: each hero's box blocks the other, standing on the other (`heroAbove`, not while a floor fan lifts), the lower hero carrying the upper, platforms lifting and carrying a stacked pair | all | | Done |
| 18 | Player 2's platform shadow (`XShadowOnPlatform`'s second shadow); player 2's HUD panel checked against `XStatusDisplay::Render` (identical to player 1's) | all | | Done |
| 19 | Checks and the whole-game zip, played start to finish by the owner | | | Deferred: the owner will play the whole game once the front end is in |

Chunk 7 was split in two (7a, 7b) because the genie is the largest piece of World 2. Chunk 14 was
planned as two halves too, but the bird's five states form one loop that shares its steering, so
it was built in one go. Chunk 15 was split in two (15a, 15b): the wizard waits for each
attack to finish, so its wand, which brings a new bullet, particle and collision sphere, could
come separately. World 4
reuses the earlier bosses (4-2 dragon, 4-5 genie, 4-8 nest) and only needs the wizard
on top of what worlds 1–3 add. The split comes from counting each object code in the 40 shipped
levels.

## Stage 6, chunk by chunk

The front end is `XApplication`'s state machine (`FrameMove`, `SetState`): each state builds one
`XProcessObject` screen, which fades in, runs, fades out and sets the next state.

| State | Screen | Goes to |
|---|---|---|
| 3 | `XLogoObject` | title |
| 4 | `XTitleObject` | save slots (one or two players), or quit (state 1) |
| 5 | `XSelectSlot` | level select, or back to the title |
| 6 | `XSelectLevelObject` | the game, the register screen, or back to the title |
| 7 | `XGameObject` | level select, or the ending after the last world |
| 8 | `XRegisterInfoObject` | level select (never reached here: `RegInfo_Registered` returns 1 in the freeware build) |
| 9 | `XEndingObject` | logo |

| Chunk | What it covers | What you can test | Status |
|---|---|---|---|
| 1 | The screen framework: `XApplication`'s states, `XProcessObject`'s fade in and out (the game's own fades included), and the logo (`XLogoObject`) | the game starts on the logo, then goes into the game (the title's stand-in); levels fade in and out | Done |
| 2 | The title (`XTitleObject`, `XCamera::ResetForTitle`): one player, two players, quit; it sets the one-player flag (F2 and `--two-player` become test hooks) | the title screen and its menu | Done |
| 3 | Saving: `XSaveData`, `XLevelStatus` (`Reset`, `Valid`, `IncreaseLevel` / `World`, `IsLastLevel` / `World`), both heroes' status, `XApplication::DoSaveData`; the port's own save files in the per-user folder (`%APPDATA%\MageBrosNative\saves`, or the ini's `save_path`), through a new `core/saves` that does what `save_path` already documents (modelled on Hopmon Native's). The original's registry saves are not imported | progress and gold kept between runs | Done |
| 4 | The save slots (`XSelectSlot`): one-player and two-player slots, loading, a new game, deleting a slot | choosing, starting and deleting slots | Done |
| 5 | Level select (`XSelectLevelObject`) and the game's real level ends (`XGameObject`'s process types 0–4): clearing or dying returns to level select, the last level of a world moves on, the last world goes to the ending; the register screen noted as unreachable | picking any unlocked level; the stand-ins (restart on death, next world) are gone | Done |
| 6 | The shop screen (`XShop`): opened by the shop building, life, magic level and wands for gold (`_CheckEnabledItems`), each player's own purchases | buying in the shop of each world | **Next** |
| 7 | The other wands' shots (`XHeroFireList::_WandNormal2` / `3`, `_WandChase` / `2` / `3`, `XHeroFire_Chase`) | every wand bought in the shop fires its own pattern | |
| 8 | The ending (`XEndingObject`, `XCamera::ResetForEnding`), then back to the logo | finishing 4-10 | |
| 9 | Checks and the stage 6 zip: the whole flow from logo to ending by screenshot | the complete game, ready for the owner's full play-through (stage 5 chunk 19 and stage 6 together) | |

Chunks 6 and 7 may swap: the shop can sell wands before they fire their own patterns, but the
wands are easier to check once they can be bought.

## Open items

| Item | Status |
|---|---|
| Which levels are boss levels | Settled: any level with a boss code (`6`–`9`); level 10 of each world and World 4's levels 2, 5, 8 |
| Which way the mage faces at the start | Settled: yaw 0, along the level |
| Two-player camera midpoint | Done (stage 5 chunk 16): the midpoint, at most 120 behind the hero further along |
| Camera shake | Done (stage 5 chunk 4): `XCamera::StartShaking`, used by the hammer and the stalactite |
| Hero-on-hero collision and carrying | Done (stage 5 chunk 17) |
| Static objects' collision with the hero | Done (5a): `XStaticObjList` and `XCollideBox` as the original |
| Colour under the pit water | Settled (0.2.4): black, the original's clear colour; nothing is drawn under the water sheets |
| Sound panning | Done (4c): pan, fade and per-sound copies as `JSound::PlayStereo` |
| Death and the last level of a world | Done (stage 6 chunk 5): back to level select, as the original |

## Verification

- Format, collision and map tests that need no game files (`ctest`), plus `mbn_map_tests` and
  `mbn_hero_tests` on all 40 levels of your copy (configure with `-DMBN_GAME_EXE=...` to run them
  in `ctest`). The hero test lands the mage on every start floor, checks the jump height (74.82,
  the original's per-tick result) and runs 800,000 ticks of random play without the box ever
  ending up inside the level.
- `MageBrosAssets --selftest` against your copy.
- For game modules: side-by-side comparison with the original on recorded inputs once the game
  loop runs.
