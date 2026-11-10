# Roadmap

Each stage ends with a Windows zip you can test with your own `MageBros.exe`.

| Stage | What it delivers | Status |
|---|---|---|
| 1 | Asset loading from `MageBros.exe`, ColDet collision library, Windows build | Done |
| 2 | Engine: window, OpenGL renderer, sound, MIDI music, keyboard/gamepad, options menu | Done |
| 3 | Level viewer: all 40 levels built by the original map code | Done |
| 4 | First playable: World 1 with player 1 | Done |
| 5 | The whole game: all 4 worlds, every enemy, platform and boss, two-player co-op | Done |
| 6 | Front end: title, save slots, level select, shop, ending, saving | Done |
| **7** | **Modern extras: the options menu, video, smooth motion, cheats, mods** | **In progress**: chunks 1 and 2a to 2d-ii done; 2d-iii done but the options menu, left as it is for now; 2d-iv done (below); smooth motion done (0.4.16 to 0.4.20, [SMOOTH_MOTION.md](SMOOTH_MOTION.md)); the graphics options reviewed (0.4.21); cheats done (C1 to C4, 0.4.23 to 0.4.26); frame timing reviewed and reworked (0.4.27 to 0.4.32, merged into `main` on 2026-11-10); enemies in view wake on the widest screens and the version shows in the options menu (0.4.33); the Statistics Overlay (its panel 0.4.34, its graphs 0.4.35); mods and the config tooling still to come |

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
| 19 | Checks and the whole-game zip, played start to finish by the owner | | | Done (with stage 6 chunk 9): the owner played the whole game, 0.3.6, and it plays fine (2026-10-25) |

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
| 6 | The shop screen (`XShop`): opened by the shop building, life, magic level and wands for gold (`_CheckEnabledItems`), each player's own purchases | buying in the shop of each world | Done |
| 7 | The other wands' shots (`XHeroFireList::_WandNormal2` / `3`, `_WandChase` / `2` / `3`, `XHeroFire_Chase`) | every wand bought in the shop fires its own pattern | Done |
| 8 | The ending (`XEndingObject`, `XCamera::ResetForEnding`), then back to the logo | finishing 4-10 | Done |
| 9 | Checks and the stage 6 zip: the whole flow from logo to ending | the complete game | Done: the owner played 0.3.6 from start to finish (2026-10-25) |

Chunks 6 and 7 were released together, in 0.3.5, so the wands the shop sells fire their own
patterns from the start.

## Stage 7, chunk by chunk

Every extra is opt-in: with every option at its default the game plays as the original (the one
exception, the owner's choice of 2026-10-29: Antialiasing 4x and Texture filtering 8x are on by
default; they only smooth edges and distant textures, and Off in each gives the original's
picture). The options menu, the config and the renderer's extras came from Hopmon Native; stage 7
makes them Mage Bros.' own and finishes the ones that need the game's help.

| Chunk | What it covers | What you can test | Status |
|---|---|---|---|
| 1 | The options menu made for Mage Bros.: the original's P pause and focus-loss music pause, the original's FPS counter (`XFPSDisplay`), the menu's sounds (the title's 0xec / 0xed), Main menu, and in a level Level select (back to the save as the level began), F10 only while a player has control, player 1 / player 2 / general key pages, gamepad fire and jump, MP3/FLAC music replacement wired in, Hopmon's text and cheats gone, unbuilt settings greyed out | every menu page; P; the FPS counter; Main menu; replacement music | Done |
| 2a | Windowed mode: the window's size (dragged too), position, maximized state and monitor are saved and restored (a position no longer on any monitor falls back to centred); F11 / Alt+Enter goes to the fullscreen mode used last and is saved; the window size list has the game's 4:3 sizes (multiples of 640 x 480, "2x", "3x") first, then wider ones, all fitting the monitor with the title bar; the first start picks the largest 4:3 window that fits | resizing, maximizing, moving and closing the window, then reopening; F11; the Resolution list | Done (0.4.1, tested by the owner) |
| 2b | Exclusive fullscreen: a Refresh rate row (the rates the monitor has at the chosen size, or Desktop: `fullscreen_refresh`); the resolution list drops modes below 640 x 480 and the copy of Desktop, and stops at its ends (Desktop last) instead of wrapping round; `*` only on the rows that will change | choosing a mode and a refresh rate, keep and revert | Done (0.4.2, tested by the owner) |
| 2c | Monitors: remembered by name (`display_name`; when that monitor isn't connected the primary is used and the name kept); the Monitor row shows number and resolution, its help the full name and rate; F11 goes fullscreen on the monitor the window has just moved to; a saved window position counts only on the chosen monitor; borderless lays the picture out on the visible rows (not its hidden extra one); the monitors listed in the log at start; Window and Borderless checked on the owner's three monitors (each at its own refresh rate) | moving between monitors in each mode | Done (0.4.3, tested by the owner) |
| 2d-i | VSync: Off / On / Adaptive (adaptive only where the driver can do it, else On, and said so); what took effect read back and logged; a driver overriding it (NVIDIA Control Panel, AMD Software) noticed from the frame rate and shown in the help line; the frame limit made exact (60 gave 58); help text on how VSync, Smooth motion and Frame limit work together; every combination checked in Window and Borderless on the 144 Hz and 60 Hz monitors | VSync, Smooth motion and Frame limit together, with Show FPS | Done (0.4.4) |
| 2d-ii | Widescreen culling: objects at the edges of a wide screen (tall pillars, hammers, any object at the back of a level or in its pits) popped in and out, because the original only draws objects within 400 along the level and its 4:3 view never reaches that; the draw range now grows with the view's width (533 at 16:9), the update range (game logic) is unchanged. Checked by comparing screenshots against a draw-everything build at 16:10, 16:9 and 21:9 in 1-1, 1-6, 2-4 and 3-5: identical | the edges of the screen in widescreen, 16:9 above all | Done (0.4.5; the owner played 21:9 too, with every object, enemy and boss normal to the edges) |
| 2d-ii (sound) | Widescreen sound: positioned sounds are panned and faded by their distance from the screen's centre over 240, made for 4:3, so at a wide screen's edges they were much quieter than at the original's (and at 21:9 some visible objects silent); the 240 now grows with the view's width, taken from the window's shape each time a sound plays (any window size, resized live); 4:3 and Stretch unchanged. Checked by logging pans at 1280 x 960, 1920 x 1080 and 2560 x 1080 and through a live resize | sounds at the left and right edges in widescreen | Done (0.4.6, tested by the owner) |
| 2d-iii (backgrounds) | The title's scrolling tile background (the title, save slots and level select) filled out to the view's width: the original's 7 x 5 grid of the 120 x 120 tile, plus as many columns either side as the view needs, in step with it; the venetian-blind fade (`XProcessObject`) spanning the whole view (it covered only the middle 640, so a wide screen's edges stayed in view while fading, levels included). Checked by screenshots at 4:3 (byte for byte the same as before), 16:9, 21:9 and 32:9, fades included | the title, save slots and level select in widescreen, and their fades | Done (0.4.7) |
| 2d-iii (shop) | The shop in widescreen: the board and the items slide in from just past the view's edges and back out to them in the same 40 ticks (they started and ended at the 4:3 screen's edges, x −280 and 640, in plain view at the sides of a wide screen, so they popped in and out); the level dimmed (black at 0.4, fading with the slide) across the whole view while the shop is open, under the status display, only when the view is wider than 4:3 (the owner's choice). The layout stays centred as the original. Checked by screenshots at 4:3 (byte for byte the same as before, sliding out included), 16:9 and 21:9 | opening and closing the shop in widescreen | Done (0.4.8) |
| Shop keys | Not widescreen, found by the owner while testing the shop: the shop's buy / select button takes every key that fires or jumps for the shopper in the level (player 1's `[` `]`, the one-player Left Ctrl / Left Shift, player 2's `1` `2`, remaps) and Enter; the original took only Right Shift / Right Ctrl (player 2: Left Shift / Left Ctrl) | buying and leaving with each key | Done (0.4.9) |
| 2d-iii (logo) | The Saito Games logo screen: its white backdrop (a 600 x 600 rectangle with texture 0x13c, a plain white 64 x 64 bitmap) widened by the view's width over 640, so it reaches the sides instead of leaving black bars; the logo bitmap (0x2d1) unchanged. Checked by screenshots: 4:3 and Stretch byte for byte the same as before, 16:9, 21:9 and 32:9 white to both edges | the logo at start-up in widescreen | Done (0.4.10) |
| 2d-iii (ending) | The ending: the film strips (black bands with a sprocket row, 80-wide tiles) take in as many tiles as the view's width needs, so they cover the corridor to the edges (it showed above and below them at the sides); each credit waits and leaves as far past the view's edges as the original's x 820 and −180 are past the 4:3 screen's, on the original's ticks (it sat in view at the right for its 2-second wait and vanished on screen at the left). The corridor, mages and fade needed nothing. Checked: at 4:3 and Stretch a log over about 6,600 frames showed the original's 9 tiles and every credit at the original's x (screenshots can't match byte for byte here: the corridor scrolls once a frame); screenshots at 16:9, 21:9 and 32:9 | the ending in widescreen | Done (0.4.11, tested by the owner with F3) |
| Ending test hook | F3 in a level and `--ending` on the command line go straight to the ending, for the owner to test 0.4.11 (beside `--auto ending`) | reaching the ending | Done (0.4.12) |
| 2d-iii (HUD) | The HUD settings hooked into `XStatusDisplay` and `XFPSDisplay`: each group drawn as a unit through `Device::BeginHudScale(scale, anchor, shift)` (a shift added for this): the level info from the top-left corner, both players' panels as one block from the top-right corner, the lost-gold messages from the middle, the FPS counter from the bottom-right corner. HUD position Edges moves the level info and the panels by `HudEdgeShift` (capped by HUD max width), the FPS counter too; HUD size scales them, limited to the largest size where the level info (the boss's full row of hearts on a boss level) and the panels stay 8 apart, never below 100%. In levels only: level select keeps the original's layout; the FPS counter follows on every screen. HUD position defaults to Centered (the owner: the original's place by default); config_version 20 resets older files once. Also fixed, in every mode (found by the owner, a slip in the original too, listed in "Bugs and quirks in the original" in `REVERSE_ENGINEERING.md`): player 2's lost-gold message, 27 below player 1's while both are 35 high, was partly covered by it; it is now 12 lower. Checked: at 100%, 4:3, Stretch, 16:9 Centered and 4:3 with Edges byte for byte the same as before; screenshots of Edges at 16:9, 21:9 and 32:9 (HUD max width 21:9 and Unlimited), sizes 50% to 200%, one and two players, normal and boss levels | the HUD rows of Options > Video in levels | Done (0.4.13) |
| 3-9's start wall | Not widescreen, found by the owner: a legacy slip (in the original too). The level opens on a row of pits and the original builds end walls only from the floor's height up, so a black strip showed on the start wall above the water; such end walls now get a cliff face below them, in every mode. Listed in "Bugs and quirks in the original" in `REVERSE_ENGINEERING.md` | the start of 3-9 | Done (0.4.14) |
| 2d-iii (options menu) | The options menu in widescreen | the options menu in widescreen | Left as it is for now (the owner) |
| 2d-iv | Field of view: `XGameScene::Render` turns `Device::SetGameplayView` on for the level's 3D and off before the shop and the HUD, so the renderer's lens scaling (both parts of the lens divided by the setting) applies to levels only; the title, level select, the shop's panel (sized to its 2D board), the HUD and the ending keep the original lens. Zoomed out, the draw range and the sound panning grow by the setting (`ViewReachFactor` = widescreen factor × the setting above 100%); zoomed in they keep the original's. The update range (game logic) is unchanged. The menu row offers 50% to 200% (the owner's choice) and shows the angle; the setting is marked experimental, with players advised to leave it at 100% (the owner: help line, ini comment, README and CHANGELOG); the help line warns that above 100% the level's unbuilt edges show (black below the near edge from about 105%, above the back wall and past the ends further out), as the original's levels are built only as far as its own view. Checked: at 100% levels at 4:3 and 16:9 and the open shop byte for byte the same as 0.4.14; at 200%, 16:9 and 21:9, in 1-1, 1-6, 2-4 and 3-5, the same objects as a draw-everything build (the only differences, a few pixels, were a slug caught a moment apart); the shop's panel lined up at 50% and 150% | Field of view in levels | Done (0.4.15) |
| 3:2 check | Screens at 3:2 (Surface-style laptops, 2256 x 1504 and 3000 x 2000) checked in Widescreen: the layout comes from the window's shape (480 high, 720 wide at 3:2), so nothing needed changing. Screenshots at 1500 x 1000: the logo, title, 1-1 with the HUD Centered and at Edges, the shop and the ending all fill the view; two-player and fades mid-way not captured | a 3:2 window or screen in Widescreen | Checked (0.4.32, 2026-11-10); no code changed |
| Wake range | On the widest views, enemies stood frozen at the screen's edges: nine enemy types (slime, bee, wallaby, mushroom, squid, kiwi, turtle, ghost, fish) do nothing beyond 480 along Z (`IsUpdatable`), which the original's 4:3 view never reaches but 32:9 does where enemies walk (about 600). The update range is now the larger of 480 and the view's reach plus 40 (321 times `ViewReachFactor` plus 40), set once a tick: exactly 480 up to about 1.83:1 at 100% (4:3 and 16:9 screenshots byte for byte as 0.4.32), 610 at 21:9, 896 at 32:9. Built in, no setting (the owner's choice: a deliberate change on views the original could never show). Details: `REVERSE_ENGINEERING.md`, "Widescreen and the draw range" | a 32:9 window in 1-1: the bee on the barrels at the right edge flies (it stood still in 0.4.32) | Done (0.4.33) |
| Version in the menu | The build's version (`MBN_VERSION_STRING`, as the window's title bar and the exe's Properties) shown small and grey at the right of the options menu's title row, on every page, so the owner and players can tell which build is running | opening the options menu (F10) | Done (0.4.33) |
| Statistics Overlay, O1 | A see-through panel (F9 cycles Off / Compact / Full, saved as `stats_overlay`; a row in Options > Video) of how the game runs and on what: Compact the frame rate, frame times, processor time, the swap's and the card's waits and the game's ticks; Full adds the display settings in effect and the graphics card. Its shape follows the view's width: one column up to about 16:10, two from about 16:9. Reads the frame loop only; drawn in one batch (two draws), so it costs about nothing (2045 frames a second with Full against about 2060 without). Plan, decisions and measurements: [STATS_OVERLAY.md](STATS_OVERLAY.md) | F9 in a level, at 4:3 and widescreen | Done (0.4.34) |
| Statistics Overlay, O2 | The graphs, one column a frame from the last 512 kept: frame time (the expected time as a gold line halfway up, stutters red), smooth motion's blend, ticks a frame. Under the figures in Compact and in Full at 16:9 and wider; beside Full in a small panel of its own at 4:3 to 16:10, where there is no room under it (no extra F9 state needed). About 0.01 ms a frame in Compact, 0.03 in Full | F9 in a level, at a frame limit, at 4:3 and widescreen | Done (0.4.35) |
| Smooth motion | Drawing the game between its ticks at any refresh rate, the game itself unchanged: A the clock, the camera and the mages (0.4.16); B1 every model in a level (0.4.17); B2 sprites: particles, shots, the flame, the water (0.4.18); C the shop's slide and the ending's credits (0.4.19); D help text, docs, final checks (0.4.20). Off by default. Built on the `interpolation-test` branch, merged into `main` on 2026-10-28 and released as v0.4.20. How it works, each chunk's plan and checks: [SMOOTH_MOTION.md](SMOOTH_MOTION.md) | Options > Video > Smooth motion at 60 and 144 Hz | Done (0.4.16 to 0.4.20) |
| Graphics review | Options > Graphics checked setting by setting on Mage Bros.' scenes (1-6 with each setting alone; pixel counts and close-ups), 2026-10-29. Fixed: **Texture filtering** never reached the game's own (point-filtered) textures, so 2x to 16x looked the same and only switched on point-sampled mipmaps; now, on, textures shrunk on screen (far or at an angle) get trilinear and anisotropic filtering while magnified ones keep the original's point sampling (`Device` texture state); **Render scale** above 200% was shrunk to the window by plain bilinear, skipping most pixels; now through mipmaps made each frame (`resolveMips_`); **screenshots** now take the Brightness curve. Checked: Texture filtering off and Smooth textures on byte for byte the same as 0.4.20 (24 shots); Brightness 100% screenshots unchanged; the steep walls of 2-4 clean at 8x where they were noisy; render scale 2x to 4x run without errors. Working as they were: Antialiasing, Smooth textures, Smooth 2D images, Per-pixel lighting, Soft shadows. **Decided (the owner, 2026-10-29): Antialiasing (4x) and Texture filtering (8x) stay on by default**, inherited from Hopmon Native, an exception to the rule of the original's look by default (both only smooth edges and distant textures; Off in each gives the original's picture) | Options > Graphics, Texture filtering on steep walls, Render scale 300% and 400%, F12 with Brightness changed | Done (0.4.21) |
| Cheats C1 | Options > Cheats opened; **Unlock all levels** wired to level select (every level of the four worlds offered; the saved level status never changed by it; a current level past those reached moves back once the cheat is off); `Config::AnyCheat` and a CHEATS mark at the bottom-left on level select and in levels. Checked: with the cheat off, level select (two save states) and 1-1 byte for byte as 0.4.21; with it on, world 4 picked and played from a save with only 1-3 reached, the save's progress unchanged afterwards | Options > Cheats from the title; level select with the cheat on and off | Done (0.4.23) |
| Cheats C2 | Infinite lives (a hit never takes the last life), Invincible (`XHero::Damage` and `KnockBack` do nothing), Infinite magic (firing costs nothing), No falling deaths (a fall below -200, or into the water after its splash, puts the hero back where it last stood on the map's own floor, flashing 100 ticks). In Mage Bros. `life` is health and a fall kills outright, so only No falling deaths covers falls. The heroes get the cheats as `XHero::cheats`, set by `XGameScene::Tick` (the hero tests leave them off). Checked: with them off, 1-1 still and moving and firing byte for byte as 0.4.23; with a temporary hook hitting the hero every 1.5 s: off, dead after five hits; Infinite lives, held at 1; Invincible, no change; ten shots at full magic; 4-10's pit and 1-10's water, back on the floor | Any level: hits, firing, pits and water | Done (0.4.24) |
| Cheats C3 | Max magic level (7), Choose your wand (Off or wand type 0 to 5), Max gold (9999), set in each playing hero's status by `XGameScene::InitLevel` before `Reset` (which reads the magic level and the wand); saved like anything bought. Checked: with them off, 1-1 and 1-3 started from a save byte for byte as 0.4.24; with them on, from a save with 50 gold and the single wand: 9999 gold, three homing shots a press, and the save after Escape at magic level 7, wand 5, 9000 gold (the original's tenth lost) | Level start, the shop, firing | Done (0.4.25) |
| Cheats C4 | Moon jump (the jump starts at `kJumpSpeed` times the square root of 1.5 or 2; falling unchanged), Run speed (`kWalkSpeed` times 1.25 to 2), One-hit kills (`XEnemyBase::Damage` and `XBossBase::Damage` bring the health to one hit first; turtles and ghosts, never hurt by shots in the original, stay so), Rapid fire (holding fire fires again every 12 ticks). Checked: with them off, 1-1 still and moving, firing and jumping byte for byte as 0.4.25, hero tests unchanged; jumps 74.80 / 112.68 / 150.64 high; 60 against 120 walked in 50 ticks; 13 shots in 1.5 s held (one without); 4-2's fence door (health 5) in five shots off, one on; 1-10's dragon defeated by one shot | Jumping, walking, firing, fights | Done (0.4.26) |
| Frame timing review | Refresh rates, VSync, the frame limit and smooth motion reviewed (2026-11-09) and reworked on the `smooth-motion-rework` branch, a version a fix: 0.4.27 ticks no longer up to 1 ms late after the menu or a pause; 0.4.28 frame limits that fit the monitor (its refresh rate and a half, third and quarter of it, paced by the driver with VSync); 0.4.29 Low latency (off by default: no finished frames queued in the driver with VSync); 0.4.30 no false "VSync forced on" warning for a driver cap just under the refresh rate; 0.4.31 with Smooth motion off, each frame drawn when its tick is due; 0.4.32 the frame limiter starts each frame within 0.1 ms of when it is due (was up to 1.3 ms late). Each measured before and after; then every rate from 30 to 360 frames a second measured: 100 ticks a second and every picture within 0.035 ms of where it should be. The details: [SMOOTH_MOTION.md](SMOOTH_MOTION.md), "Review of 2026-11-09". Set aside: the renderer's work per draw ([RENDERER_OPTIMIZATION.md](RENDERER_OPTIMIZATION.md)) | Options > Video: Frame limit's refresh-rate entries, Low latency; a frame limit on a G-Sync / FreeSync monitor | Done (0.4.27 to 0.4.32; the owner tried 0.4.28, 0.4.29, 0.4.31 and 0.4.32); merged into `main` on 2026-11-10 (pull request #3), released as v0.4.32 |

**Done**: chunk 1 (the options menu), chunk 2 (the Video page; the options menu in widescreen is
left as it is, the owner's choice), smooth motion (0.4.16 to 0.4.20), the graphics review
(0.4.21) and the cheats (C1 to C4, 0.4.23 to 0.4.26). The per-frame items in the "frame-rate dependence" table below are settled: each is
handled or kept by the owner's choice.

**Set aside (the owner, 2026-10-29): the present path.** Presenting through a DXGI flip swapchain
of the port's own, so a window and borderless are as smooth as exclusive fullscreen on NVIDIA and
AMD without driver settings. Two chunks of five were built on the `dxgi-present` branch (0.4.22
there) and pushed to GitHub, kept apart from `main`; its `docs/PRESENT_PATH.md` has the reasons,
the measurements and how to resume. NVIDIA's "Vulkan/OpenGL present method: Prefer layered on DXGI
swapchain" first seemed to cure the choppiness, then no longer did (the owner, 2026-11-02), so the
README no longer recommends it; window and borderless smoothness is still open (`CLAUDE.md`,
"Windowed / borderless smoothness"). `main` skipped 0.4.22, which the branch uses.

**Reviewed (2026-11-09, 0.4.26): refresh rates, VSync, the frame limit and smooth motion.** The
design holds at any refresh rate; one small bug (ticks up to 1 ms late after the menu or a pause)
and seven improvements are listed in [SMOOTH_MOTION.md](SMOOTH_MOTION.md) ("Review of
2026-11-09"), being built one at a time on the `smooth-motion-rework` branch, a version each
(0.4.27 to 0.4.32 so far): the bug is fixed
there (measured before and after), and so are the frame limits that fit the monitor (the
refresh rate and its half, third and quarter, paced by the driver with VSync; liked by
the owner) and a Low latency setting (off by default: no finished frames queued in the driver
with VSync; kept by the owner, who noticed little on this machine, for players it may help), the false "VSync forced on" warning is fixed, and with Smooth motion off each frame
is drawn when its tick is due, the frame limiter starts each frame within 0.1 ms of when it is
due (it was up to 1.3 ms late) and the stale comments are corrected. The other refresh
rates were then measured (2026-11-10): from 30 to 360 frames a second the game runs its 100
ticks a second and every picture is drawn where it should be, to within 0.035 ms of movement.
The renderer's work per draw (6) is not planned (written up for later in
[RENDERER_OPTIMIZATION.md](RENDERER_OPTIMIZATION.md)). The list is closed; the branch was merged into `main` on 2026-11-10 (pull request #3) and released as v0.4.32.

**The Statistics Overlay** (planned 2026-11-10; O1 built as 0.4.34, O2 (the graphs) as 0.4.35, O3 optional): F9, off by default, reading values
only: frame rate and frame times, the game's ticks, smooth motion's blend, the display settings
in effect, the graphics card and driver; chunks O1 (the panel), O2 (the graphs) and O3 (recording
to a file, optional), in [STATS_OVERLAY.md](STATS_OVERLAY.md). It was set aside as a debug overlay
on 2026-10-27.

**Still open, to be planned with the owner**: mods (add-ons and level packs, with `MODDING.md`; the Mods pages are greyed out and
`core/mods.cpp` is a stub); the config tooling (`--set`, `CONFIG.md`) and the README sections
Hopmon Native has.

**Set aside for later** (the owner's notes; maybe more than a game of this size needs):

- the renderer's work per draw (every draw sets all its state and uploads its mesh again): an
  optimization for weak graphics hardware only, changing no game logic and no picture. Written
  up with a plan in chunks in [RENDERER_OPTIMIZATION.md](RENDERER_OPTIMIZATION.md) (2026-11-10);
  to be considered if players on low-end hardware report low frame rates;
- for the field of view, possibly with mods: scenery beyond the original's view (deeper cliffs
  below the near edge, taller walls, something past the ends) so zooming out holds up;
- perhaps a "widescreen max width" setting (the picture limited to a chosen shape, with bars
  beyond, as HUD max width does for the HUD). The frozen enemies at 32:9 it was first thought
  of for are answered by the update range (below, 0.4.33);
- a frame-limit choice for G-Sync / FreeSync (the monitor's refresh rate less 3): planned in
  smooth motion's chunk D and then dropped (2026-10-28: a few milliseconds at most, and the
  graphics drivers already offer the cap); why, and how it would be built, are in
  [SMOOTH_MOTION.md](SMOOTH_MOTION.md) ("D1").

## Stage 7: frame-rate dependence

The game logic runs in fixed 10 ms ticks (`XApplication::FrameMove(n)` with up to 10 ticks a
frame), so its speed doesn't depend on the frame rate. The original drew at most 100 frames a
second and normally ran 640 x 480 exclusive fullscreen with VSync, about 60 frames a second (see
"The frame loop and the frame rate" in `REVERSE_ENGINEERING.md`). Nothing the game simulates is
tied to the frame rate, so no speed-ups at high refresh rates. High refresh rates must stay that way:
more frames drawn, with the game's speed and behaviour unchanged.

A few things in the original advance once per frame instead of once per tick. The port keeps them
per frame for now, to stay one-to-one. On a high-refresh display they run faster than the
original intended. Stage 7 should move each one to ticks (or scale it by the elapsed time) as
part of high-refresh support. Add each new one to this table as it's found.

| Where | What advances per frame | Found in |
|---|---|---|
| The ending (`XEndingObject::ProcessFrame`) | `textureOffset += 0.008` each call: the corridor's floor and walls scroll faster at higher frame rates (the credits and the mages' walk use ticks). `ProcessFrame` runs only on frames with at least one tick, so at most 100 times a second. **Kept as the original with Smooth motion off** (the owner's decision, 2026-10-25: a minor visual effect, hardly noticeable in play). **With Smooth motion on, once a tick at the speed the frame rate gave it (0.008 x the frame rate up to 100, spread over 100 ticks), drawn between ticks** (the owner, 2026-10-28: at 144 Hz the once-a-frame scroll stood still on one frame in three; the speed of the faster frame rates kept, as it conveys the mages leaving at speed); the details are under the table | stage 6 chunk 8 (0.3.6); smooth motion 0.4.19 |
| The heroes' fire and jump presses (`XHeroA` / `XHeroB::_UpdateInput` read the once-a-frame `JDInput`) | A press counts on every tick of its frame: one at 100 frames a second, sometimes two at 60 (a 10 ms jump buffer on landing). **Already handled**: the port reads the heroes' keys every tick, as the original at 100 frames a second, whatever the frame rate (the owner's decision) | the frame-loop review, stage 7 chunk 2d-i (0.4.4) |
| The menus and the shop (`JDInput::UpdateInputState`, `XShop::GetState` in `XGameObject::ProcessFrame`) | The menus' keys are read once a frame ("pressed" = held now and not at the last read), and the shop's state once a frame. This changes no speed; drawing between ticks must keep it on frames that tick | stage 7 chunk 1 (0.4.0) |

**The ending's corridor, for reference.** The original updates the corridor once a frame, and
with VSync its frames followed the monitor's refresh, so the scroll ran at refresh / 60 of its
60 Hz speed: 1x at 60 Hz (LCDs, most setups), 1.25x at 75 Hz and about 1.4x at 85 Hz (common
CRT settings in 2003), and 1.67x at 100 Hz or more, where the 100-a-second cap of the frame loop
stops it. The port behaves the same: on the owner's 60 Hz receiver 1x, on the 144 Hz monitors
1.67x, with Smooth motion on or off (extra frames between ticks repeat the last position).
Only the textures move faster: the mages' walk, the credits, the music and the ending's length
run on ticks. Had it been changed, the scroll would have been `0.008 x 60 / 100 = 0.0048` per
tick (`textureOffset += 0.0048f * n`), the 60 Hz speed at any frame rate.

**With Smooth motion on (0.4.19)** the corridor moves once a tick, drawn between ticks, at the
speed the frame rate gives it: `0.008 x min(frame rate, 100) / 100` a tick (`CorridorPerTick`,
from the measured frame rate, `AppFrameRate`). Smooth motion draws 144 frames a second at
144 Hz, but the once-a-frame scroll only moved on the frames that ran a tick, about 100 of them,
so the corridor stood still on about one frame in three, a stutter behind the gliding credits
that the owner noticed. The owner first had it moved to the 60 Hz speed, then chose to keep the
faster frame rates' speed (it conveys the mages leaving at speed). Measured: at 144 Hz 0.800 a
second, even on every frame (Smooth motion off: 0.800 a second, 0.69 to 1.00 frame to frame); at a
frame limit of 60, 0.480. With Smooth motion off the scroll is the original's.

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
