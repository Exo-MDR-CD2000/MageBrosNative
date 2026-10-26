# Changelog

What changed in each version of Mage Bros. Native. Every version is published as a
[GitHub release](https://github.com/Exo-MDR-CD2000/MageBrosNative/releases) with its Windows zip.
You still need your own copy of Mage Bros. 2.0 (the freeware build) to play: put
`MageBrosNative.exe` next to `MageBros.exe`.

Where we are and what comes next: [docs/ROADMAP.md](docs/ROADMAP.md).

## [0.4.14] - 2026-10-26

- Fixed: at the start of level 3-9 a black strip showed on the wall to the left, just above the
  water, because the level opens on a row of pits and the original game never built the wall
  below floor level there (a slip in the original game too). The wall now carries on down to the
  water in the same brick as every other pit edge. No other level is affected.

## [0.4.13] - 2026-10-26

- **HUD settings**: Options > Video > HUD position, HUD max width and HUD size now work.
  - **HUD position** (widescreen): Centered keeps the HUD where the original has it, inside the
    middle 4:3 area (the default); Edges moves the level information to the left edge, the
    players' panels to the right edge and the FPS counter to the bottom-right corner.
  - **HUD max width**: with Edges, how far the HUD may spread on very wide screens (21:9 by
    default, so a 32:9 screen keeps it off the far corners).
  - **HUD size**: 50% to 200%, each part growing from its corner (the FPS counter too). When the
    level information and the panels would meet (two players on a boss level, or a narrow
    screen), the HUD stops at the largest size that fits.
- HUD position now starts as Centered. Settings files from earlier versions are set back to
  Centered once (the setting did nothing before).
- Fixed: in a two-player game, when both mages lost gold at once, player 1's "you have lost ...
  gold" message covered the top of player 2's, clipping its mage icon (a slip in the original
  game too). Player 2's message is now a little lower, so both show in full.
- The original game's known slips and quirks, and what this port does about each, are now listed
  at the end of `docs/REVERSE_ENGINEERING.md`.

## [0.4.12] - 2026-10-26

- **Test shortcut to the ending**: press F3 during a level to jump straight to the ending, or
  start the game with `--ending` to open on it. Like the other test keys (N / B, F2), nothing
  is saved.

## [0.4.11] - 2026-10-26

- **The ending in widescreen**: the black film-strip bands at the top and bottom of the ending now
  run the whole width of a wide screen, where before the corridor showed through above and below
  them at the sides. The credits now slide in from the edge of the wide screen and out past the
  other edge, instead of waiting in view at the right and vanishing at the left. In 4:3 nothing
  changes.

## [0.4.10] - 2026-10-25

- **The Saito Games logo fills widescreen**: the white behind the logo at start-up now reaches
  the edges of a wide screen (16:9, 21:9 and wider) instead of stopping with black bars at the
  sides. The logo itself is unchanged, and in 4:3 nothing changes.

## [0.4.9] - 2026-10-25

- **The shop takes your usual keys**: buying an item or choosing EXIT now works with any key
  that fires or jumps for you in the level, and with Enter. Before, only Right Shift and Right
  Ctrl worked on the keyboard (player 2: Left Shift and Left Ctrl), as in the original, so the
  `[` and `]` keys, the one-player Left Shift / Left Ctrl and Enter did nothing in the shop.
  Keys you changed in Options > Controls count too.

## [0.4.8] - 2026-10-25

- **The shop in widescreen**: the shop's board and items now slide in from the edges of a wide
  screen (and back out to them) instead of popping up at the sides, and the level behind the shop
  is dimmed across the whole screen while it is open, so the menu stands out. The life, gold and
  magic display stays bright. In 4:3 the shop is exactly the original, without the dimming.

## [0.4.7] - 2026-10-25

- **The green background fills widescreen**: the scrolling green tiles behind the title, the save
  slots and level select now cover the whole screen in widescreen (16:9, 21:9 and wider), where
  before the left edge was black and the right edge partly so. The middle is exactly the
  original's, and in 4:3 nothing changes.
- **Fades cover the whole screen**: the striped fade between screens (and when a level starts or
  ends) now reaches the edges of a wide screen instead of only the middle.

## [0.4.6] - 2026-10-25

- **Widescreen sound**: sounds are panned left and right by where they happen, as in the
  original. In widescreen, things at the edges of the wider screen sounded much quieter than at
  the edges of the original screen, and on 21:9 some objects you could see made no sound at all.
  The panning now follows the shape of the window (any size, even while you resize it), so a
  sound at the edge of a wide screen is as loud and as far to that side as one at the edge of the
  original. In 4:3 nothing changes.

## [0.4.5] - 2026-10-25

- **No more pop-in at the edges in widescreen**: tall pillars, hammers and other objects near the
  back of a level or down in its pits could appear and vanish at the left and right edges of a
  wide screen. The original only draws objects close to the middle of its 4:3 view; the game now
  draws as far as the wider view reaches (at 4:3 it is exactly the original). What objects do,
  and when enemies wake up, is unchanged.
- Ultrawide (21:9) now plays properly: every object, enemy and boss shows and behaves normally
  right to the edges of the screen.

## [0.4.4] - 2026-10-25

- **Adaptive VSync**: Options > Video > VSync is now Off, On or Adaptive. Adaptive waits for the
  screen like On, but a picture that comes too late shows at once (a little tearing instead of
  a stutter). It is offered when your graphics driver supports it.
- **VSync tells you when it isn't in charge**: your graphics driver's own setting (NVIDIA
  Control Panel, AMD Software) can override the game's. When the frame rate shows that it does,
  the VSync row's help line says so. The log records what took effect.
- Fixed: the frame limit ran slightly slow (60 gave 58 frames a second); it is exact now.
- The help lines for VSync, Smooth motion and Frame limit explain how the three work together.

## [0.4.3] - 2026-10-25

- **Monitors are remembered by name**: if you plug monitors in or out, or Windows renumbers
  them, the game still opens on the monitor you chose. If that monitor isn't connected, the game
  uses your main monitor and goes back to the chosen one once it is there again.
- **Clearer Monitor row**: Options > Video > Monitor shows the monitor's number and resolution,
  and the line below it the full name and refresh rate. The log (`magebros_native.log`) lists
  every monitor at each start.
- Fixed: going back to a window could put it on another monitor than the one chosen (where it
  had been before), and F11 then went fullscreen there. A remembered position now only counts on
  the chosen monitor, and F11 goes fullscreen on the monitor the window has just been moved to.
- Fixed: in borderless fullscreen the picture was laid out one row taller than the screen
  (an extra row kept off the screen so graphics drivers don't switch to exclusive mode). It now
  fits the screen exactly.

## [0.4.2] - 2026-10-25

- **Refresh rate for exclusive fullscreen**: Options > Video has a new Refresh rate row. In
  exclusive fullscreen you can pick any rate your monitor offers at the chosen resolution (60,
  120, 144, 165 Hz...), or Desktop, your desktop's own rate (the default, as before). It is
  applied with Enter and the usual keep / revert question. The game itself still runs at the
  same speed; this only sets how often the screen refreshes.
- **A shorter resolution list** for exclusive fullscreen: modes smaller than the game's
  640 x 480 are gone, Desktop comes last, and the list stops at its ends instead of jumping from
  Desktop to the smallest mode.
- While a display change waits for Enter, the `*` now marks only the rows that will change.

## [0.4.1] - 2026-10-25

- **The window remembers itself**: its size (resizing it by dragging the edges too), where it
  was, whether it was maximized and which monitor it was on are saved and restored on the next
  start. If that spot is no longer on a monitor (one was unplugged, say) it opens centred.
- **F11 / Alt+Enter** now goes from the window to the fullscreen mode you used last, borderless
  or exclusive, and the choice is saved.
- **Window sizes**: Options > Video > Resolution lists the game's own 4:3 sizes first, from
  640 x 480 up, with whole multiples marked "2x", "3x" and so on (the 2D art scales evenly at
  those), then wider sizes. Only sizes that fit your monitor with the title bar are offered.
- **The first start** opens the largest 4:3 window that fits your monitor, instead of always
  1280 x 960 (2560 x 1920 on a 4K screen). "Reset all settings" does the same.
- A window on a monitor with a different Windows display scale no longer grows a little every
  time the game starts.

## [0.4.0] - 2026-10-25

Stage 7 begins: the options menu (F10 or gamepad Start), brought over from Hopmon Native, is
now made for Mage Bros.

- **Pause**: P pauses and resumes the game, as in the original (the manual's "P: Pause").
- **FPS counter**: Options > Video > Show FPS now shows the original's own frames-per-second
  counter in the bottom right of every screen.
- **Music pauses** while the game window is in the background, as in the original.
- **Replacement music works**: MP3 or FLAC files named `Music01` to `Music06` in a `music`
  folder next to `MageBros.exe` now play instead of the MIDI tracks (Options > Audio > Music
  source).
- **Controls**: the keyboard page now has player 1's keys, player 2's keys and the general keys
  (select, pause, back), each of which can be rebound. The gamepad page sets the fire and jump
  buttons. In the menu, Ctrl now selects as Enter does (Z and X, Hopmon's keys, no longer do).
- **The menu** plays the title menu's sounds. It offers **Main menu**, back to the title, and
  in a level also **Level select**, which leaves the level for the world map with your gold,
  life, magic and wand as they were when the level started (neither saves the level you leave). It opens only
  while you can play: not during the logo, the ending, the fades or a mage's death.
- Settings that aren't ready for Mage Bros. yet are greyed out: the HUD options, the field of
  view, cheats and mods. Hopmon's two cheats are gone. The ini's notes on keys, buttons, the HUD
  and cheats describe Mage Bros. now; an older ini is rewritten with them, keeping your settings.
- The whole game has been played from start to finish on 0.3.6: stages 5 and 6 are complete.

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
