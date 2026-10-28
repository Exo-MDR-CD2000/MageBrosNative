#include "config.h"
#include "log.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <sstream>

static Config g_config;
Config& GetConfig() { return g_config; }

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}
static std::vector<std::string> splitList(const std::string& s) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item = trim(item);
        if (!item.empty()) out.push_back(item);
    }
    return out;
}
static bool toBool(const std::string& v) {
    std::string l = v;
    std::transform(l.begin(), l.end(), l.begin(), ::tolower);
    return l == "1" || l == "true" || l == "yes" || l == "on";
}

bool Config::Load(const std::string& file) {
    path = file;
    size_t seen = 0;
    FILE* f = OpenFileUtf8(file.c_str(), "r");
    if (!f) return false;
    char line[1024];
    std::string section;
    while (fgets(line, sizeof line, f)) {
        std::string l = trim(line);
        if (l.empty() || l[0] == ';' || l[0] == '#') continue;
        if (l[0] == '[') {
            section = trim(l.substr(1, l.find(']') - 1));
            continue;
        }
        size_t eq = l.find('=');
        if (eq == std::string::npos) continue;
        std::string k = trim(l.substr(0, eq)), v = trim(l.substr(eq + 1));
        int iv = atoi(v.c_str());
        size_t before = seen;
        seen++;
        if (section == "" && k == "config_version") version = iv;
        else if (section == "video") {
            if (k == "width") windowWidth = std::max(320, iv);
            else if (k == "height") windowHeight = std::max(240, iv);
            else if (k == "fullscreen_width") fullscreenWidth = iv <= 0 ? 0 : std::max(320, iv);
            else if (k == "fullscreen_refresh") fullscreenRefresh = std::clamp((float)atof(v.c_str()), 0.f, 1000.f);
            else if (k == "fullscreen_height") fullscreenHeight = iv <= 0 ? 0 : std::max(240, iv);
            else if (k == "fullscreen") fullscreen = (FullscreenMode)std::clamp(iv, 0, 2);
            else if (k == "last_fullscreen") lastFullscreen = iv == 2 ? FullscreenMode::Exclusive : FullscreenMode::Borderless;
            else if (k == "maximized") maximized = toBool(v);
            else if (k == "window_x" || k == "window_y") {
                // both must be numbers; an empty value means "centred"
                if (v.empty()) windowPosSet = false;
                else {
                    (k == "window_x" ? windowX : windowY) = iv;
                    windowPosSet = true;
                }
            }
            else if (k == "display") displayIndex = std::max(0, iv);
            else if (k == "display_name") displayName = v;
            else if (k == "vsync") vsync = iv == 2 || v == "adaptive" ? 2 : toBool(v) ? 1 : 0;
            else if (k == "max_fps") maxFps = std::max(0, iv);
            else if (k == "interpolation") interpolation = toBool(v);
            else if (k == "msaa") msaa = std::clamp(iv, 0, 16);
            else if (k == "anisotropy") anisotropy = std::clamp(iv, 1, 16);
            else if (k == "smooth_textures") smoothTextures = toBool(v);
            else if (k == "smooth_sprites") smoothSprites = toBool(v);
            else if (k == "per_pixel_lighting") perPixelLighting = toBool(v);
            else if (k == "enhanced_shadows") enhancedShadows = toBool(v);
            else if (k == "brightness") brightness = std::clamp(iv, 50, 150);
            else if (k == "render_scale") renderScale = std::clamp((float)atof(v.c_str()), 0.25f, 4.0f);
            else if (k == "aspect") aspect = (AspectMode)std::clamp(iv, 0, 2);
            else if (k == "fov_scale") fovScale = std::clamp((float)atof(v.c_str()), 0.5f, 2.0f);
            else if (k == "show_fps") showFps = toBool(v);
            else if (k == "hud_layout") hudLayout = std::clamp(iv, 0, 1);
            else if (k == "hud_scale") hudScale = std::clamp((float)atof(v.c_str()), 0.5f, 2.0f);
            else if (k == "hud_max_aspect") {
                float a = (float)atof(v.c_str());
                hudMaxAspect = a <= 0.f ? 0.f : std::clamp(a, 4.f / 3.f, 10.f);
            }
            else seen = before;
        } else if (section == "audio") {
            if (k == "master_volume") masterVolume = std::clamp(iv, 0, 100);
            else if (k == "sfx_volume") sfxVolume = std::clamp(iv, 0, 100);
            else if (k == "music_volume") musicVolume = std::clamp(iv, 0, 100);
            else if (k == "soundfont") soundFont = v;
            else if (k == "music_source") {
                std::string l = v;
                for (auto& c : l) c = (char)tolower((unsigned char)c);
                musicSource = (l == "midi" || l == "1") ? 1 : 0;
            }
            else if (k.size() == 7 && k.rfind("music_", 0) == 0 && k[6] >= '1' && k[6] <= '6') musicFiles[k[6] - '1'] = v;
            else if (k == "force_soft_synth") {}  // removed in config_version 2 (a SoundFont is now always preferred)
            else seen = before;
        } else if (section == "input") {
            if (k == "gamepad_deadzone") gamepadDeadzone = std::clamp(iv, 0, 90);
            else seen = before;
        } else if (section == "keys") {
            // dik_<hex or dec> = Scancode name list
            char* end = nullptr;
            int code = k.rfind("dik_", 0) == 0 ? (int)strtol(k.c_str() + 4, &end, 0) : -1;
            if (code > 0 && code < 256 && end && *end == 0) keyBinds[code] = splitList(v);
            else seen = before;
        } else if (section == "gamepad") {
            char* end = nullptr;
            int code = k.rfind("button_", 0) == 0 ? (int)strtol(k.c_str() + 7, &end, 10) : -1;
            if (code >= 0 && code < 32 && end && end != k.c_str() + 7 && *end == 0) padBinds[code] = splitList(v);
            else seen = before;
        } else if (section == "cheats") {
            if (k == "unlock_all_levels") unlockAllLevels = toBool(v);
            else seen = before;
        } else if (section == "mods") {
            if (k == "pack") modPack = v.empty() ? "original" : v;
            else {
                bool found = false;
                for (auto& m : mods)
                    if (m.first == k) {
                        m.second = toBool(v);
                        found = true;
                    }
                if (!found) mods.push_back({k, toBool(v)});
            }
        } else if (section == "game") {
            if (k == "path") gamePath = v;
            else if (k == "save_path") savePath = v;
            else seen = before;
        } else {
            seen = before;
        }
        if (seen == before && !(section == "" && k == "config_version"))
            Log("Config: ignoring unknown setting '%s' in [%s]", k.c_str(), section.c_str());
    }
    fclose(f);
    // config_version 21: [video] interpolation (Smooth motion) defaults to 0, as the original
    // draws; files written before it had 1 (from Hopmon Native, when it only repeated pictures),
    // so it is reset once
    if (version > 0 && version < 21) interpolation = false;
    // config_version 20: [video] hud_layout defaults to 0 (the original's place); files written
    // before it had 1 (Hopmon Native's default) while the setting did nothing, so it is reset
    if (version > 0 && version < 20) hudLayout = 0;
    // config_version 19: [video] vsync 2 (adaptive)
    // config_version 18: [video] display_name
    // config_version 17: [video] fullscreen_refresh
    // config_version 16: [video] window_x / window_y, maximized and last_fullscreen; the window's
    // size, position and monitor and Alt+Enter / F11 are saved
    // config_version 15: the [keys] and [gamepad] notes describe Mage Bros.' keys, and Hopmon's
    // two cheats are gone
    // config_version 14: [mods] lists add-ons with the winner first (it was last before)
    if (version > 0 && version < 14) std::reverse(mods.begin(), mods.end());
    return true;
}

bool Config::Save() const {
    if (path.empty()) return false;
    FILE* f = OpenFileUtf8(path.c_str(), "w");
    if (!f) return false;
    auto P = [&](const char* text) { fputs(text, f); };
    P("; ===========================================================================\n"
      "; Mage Bros. Native settings\n"
      ";\n"
      "; Easiest: press F10 (or Start on a gamepad) in the game to open the options\n"
      "; menu. Changes made there apply immediately and are saved to this file.\n"
      ";\n"
      "; To edit by hand: close the game, edit a value, save, and start the game.\n"
      "; Lines starting with ; are comments.\n"
      "; On/off settings accept 1/0, true/false, yes/no or on/off.\n"
      "; Out-of-range numbers are clamped to the nearest allowed value; unknown\n"
      "; settings are ignored and reported in magebros_native.log.\n"
      "; Delete this file to restore all defaults.\n"
      ";\n"
      "; None of these settings change gameplay: the game always runs its original\n"
      "; rules and 100-steps-per-second timing.\n"
      "; ===========================================================================\n");
    fprintf(f, "config_version = %d\n\n", kCurrentVersion);

    P("[game]\n"
      "; Where your original MageBros.exe is. Leave empty to look next to this program\n"
      "; (and in the current folder). Example: path = D:\\Games\\Mage Bros\\MageBros.exe\n");
    fprintf(f, "path = %s\n\n", gamePath.c_str());
    P("; save_path: where saved progress is kept. Leave empty for the default:\n"
      ";   %APPDATA%\\MageBrosNative\\saves on Windows, ~/.local/share/MageBrosNative/saves on Linux.\n"
      ";   A relative path is relative to this program's folder (save_path = save keeps\n"
      ";   saves in a 'save' folder next to it); %NAME% environment variables work.\n"
      ";   Example: save_path = D:\\Games\\Mage Bros saves\n"
      ";   When you change it, existing saves are copied to the new folder on the next\n"
      ";   start (the old copies are left in place).\n");
    fprintf(f, "save_path = %s\n\n", savePath.c_str());

    P("[video]\n"
      "; width / height: window size when fullscreen = 0, in screen pixels (with\n"
      ";   Windows display scaling above 100% a window looks smaller than these\n"
      ";   numbers suggest). Resizing the window by dragging its edges is saved here.\n"
      ";   Minimum 320 x 240. The first start picks the largest 4:3 size that fits\n"
      ";   the monitor (the game is 640 x 480: 1280 x 960 is 2x, 1920 x 1440 3x).\n");
    fprintf(f, "width = %d\nheight = %d\n\n", windowWidth, windowHeight);
    P("; window_x / window_y: where the window was last (its inside's top-left corner,\n"
      ";   in desktop coordinates). Empty = centred on the monitor. Kept up to date\n"
      ";   when you move the window; ignored if that spot is no longer on a monitor.\n");
    if (windowPosSet) fprintf(f, "window_x = %d\nwindow_y = %d\n\n", windowX, windowY);
    else fprintf(f, "window_x =\nwindow_y =\n\n");
    P("; maximized: 1 = the window was maximized and opens maximized again; width and\n"
      ";   height keep its size from before. 0 = not maximized (default).\n");
    fprintf(f, "maximized = %d\n\n", maximized ? 1 : 0);
    P("; fullscreen: 0 = window (default)\n"
      ";             1 = borderless: a window without borders covering the whole\n"
      ";                 monitor at its desktop resolution. Alt+Tab works instantly.\n"
      ";                 (recommended)\n"
      ";             2 = exclusive fullscreen at fullscreen_width x fullscreen_height.\n"
      ";                 Alt+Tab minimizes the game.\n"
      "; Alt+Enter or F11 switches between the window and the fullscreen mode used\n"
      "; last (last_fullscreen), and is saved.\n");
    fprintf(f, "fullscreen = %d\n\n", (int)fullscreen);
    P("; last_fullscreen: the fullscreen mode Alt+Enter / F11 goes to from a window:\n"
      ";   1 = borderless (default), 2 = exclusive. Set by itself whenever you use\n"
      ";   one of them.\n");
    fprintf(f, "last_fullscreen = %d\n\n", lastFullscreen == FullscreenMode::Exclusive ? 2 : 1);
    P("; fullscreen_width / fullscreen_height: the screen resolution used by\n"
      ";   exclusive fullscreen (fullscreen = 2). 0 = your desktop resolution\n"
      ";   (default). Other values pick the closest mode the monitor supports.\n");
    fprintf(f, "fullscreen_width = %d\nfullscreen_height = %d\n\n", fullscreenWidth, fullscreenHeight);
    P("; fullscreen_refresh: the refresh rate (Hz) used by exclusive fullscreen.\n"
      ";   0 = your desktop's refresh rate (default). Other values, e.g. 60, 144 or\n"
      ";   165, pick the closest rate the monitor has at that resolution. Window and\n"
      ";   borderless always run at the desktop's rate. The game itself always runs\n"
      ";   100 steps a second; this only sets how often the screen is refreshed.\n");
    fprintf(f, "fullscreen_refresh = %g\n\n", fullscreenRefresh);
    P("; display: which monitor to use. 0 = primary monitor (default),\n"
      ";   1 = first monitor, 2 = second, ... in the order Windows lists them (the\n"
      ";   log, magebros_native.log, lists them at every start).\n"
      ";   Moving the window to another monitor updates it.\n");
    fprintf(f, "display = %d\n\n", displayIndex);
    P("; display_name: that monitor's name, set by itself. While a monitor of that name\n"
      ";   is connected it is used, whatever its number has become (after plugging\n"
      ";   monitors in or out, say); while it isn't, the primary monitor is used.\n"
      ";   Empty = go by the number alone.\n");
    fprintf(f, "display_name = %s\n\n", displayName.c_str());
    P("; vsync: 1 = wait for the monitor's refresh before showing each picture: no\n"
      ";            tearing (default)\n"
      ";        2 = adaptive: as 1, but a picture that is late shows at once (a\n"
      ";            little tearing instead of a stutter); where the graphics driver\n"
      ";            can't do it, the game uses 1 and says so in the log\n"
      ";        0 = off: show each picture as soon as it is drawn (tearing). With\n"
      ";            interpolation = 1 and max_fps = 0 that is as fast as the graphics\n"
      ";            card can go; set max_fps to keep it down.\n"
      "; Your graphics driver's own setting (NVIDIA Control Panel, AMD Software) can\n"
      "; override this one; the options menu tells you when it seems to.\n");
    fprintf(f, "vsync = %d\n\n", vsync);
    P("; max_fps: frame-rate cap. 0 = no cap (default). Any positive number, e.g. 60,\n"
      ";   144. Only applies when interpolation = 1; without interpolation the game\n"
      ";   draws at most 100 frames per second, like the original.\n");
    fprintf(f, "max_fps = %d\n\n", maxFps);
    P("; interpolation: Smooth motion. 1 = draw at the monitor's rate, with the level\n"
      ";   between the game's 100 Hz steps, so it moves smoothly at any refresh\n"
      ";   rate; the picture trails the game by up to 10 ms. The game itself\n"
      ";   runs exactly the same. 0 = draw only on the game's steps, like the\n"
      ";   original (default).\n");
    fprintf(f, "interpolation = %d\n\n", interpolation ? 1 : 0);
    P("; msaa: antialiasing (smooth polygon edges). 0 = off, 2, 4 (default), 8 or 16.\n"
      ";   Values above what your graphics card supports are lowered automatically.\n");
    fprintf(f, "msaa = %d\n\n", msaa);
    P("; anisotropy: texture sharpness on floors seen at an angle. 1 = off,\n"
      ";   2, 4, 8 (default) or 16.\n");
    fprintf(f, "anisotropy = %d\n\n", anisotropy);
    P("; render_scale: draws the 3D scene at this multiple of the screen size, then\n"
      ";   scales it to fit. 1.0 = native (default). 1.5 or 2.0 = supersampling\n"
      ";   (sharper, slower). Below 1.0 = faster, blurrier. Range 0.25 to 4.0.\n");
    fprintf(f, "render_scale = %.2f\n\n", renderScale);
    P("; aspect: 0 = original 4:3 picture with black bars on wide screens (default)\n"
      ";         1 = stretch 4:3 to fill the window (distorted)\n"
      ";         2 = widescreen: the 3D view is widened to fill the window; menus and\n"
      ";             the on-screen display stay 4:3 in the middle. On a screen\n"
      ";             narrower than 4:3 this behaves like 0.\n");
    fprintf(f, "aspect = %d\n\n", (int)aspect);
    P("; fov_scale: EXPERIMENTAL, best left at 1.0. Camera field of view in levels.\n"
      ";   1.0 = original (default). Above 1.0 shows more (zoomed out), below 1.0\n"
      ";   shows less (zoomed in). Range 0.5 to 2.0. The original's levels are\n"
      ";   built only as far as its own view, so above 1.0 their unbuilt edges show\n"
      ";   (black below the near edge, above the back wall and past the ends). The\n"
      ";   title, level select, shop panel and ending always use the original view.\n");
    fprintf(f, "fov_scale = %.2f\n\n", fovScale);
    P("; smooth_textures: 1 = bilinear filtering on 3D textures the original drew\n"
      ";   pixelated. 0 = original look (default).\n");
    fprintf(f, "smooth_textures = %d\n\n", smoothTextures ? 1 : 0);
    P("; smooth_sprites: 1 = smooth scaling of 2D images (logo, menus, HUD).\n"
      ";   0 = sharp pixels (default).\n");
    fprintf(f, "smooth_sprites = %d\n\n", smoothSprites ? 1 : 0);
    P("; per_pixel_lighting: 1 = the original lights calculated per pixel (smoother\n"
      ";   shading on round objects). 0 = per corner, like the original (default).\n");
    fprintf(f, "per_pixel_lighting = %d\n\n", perPixelLighting ? 1 : 0);
    P("; enhanced_shadows: 1 = soft-edged shadows under characters and objects.\n"
      ";   0 = original shadows (default).\n");
    fprintf(f, "enhanced_shadows = %d\n\n", enhancedShadows ? 1 : 0);
    P("; brightness: picture brightness in percent, 50 to 150. 100 = original\n"
      ";   (default). Higher lifts dark and middle tones (whites stay white), lower\n"
      ";   darkens them.\n");
    fprintf(f, "brightness = %d\n\n", brightness);
    P("; show_fps: 1 = show the original frames-per-second counter, at the bottom\n"
      ";   right of every screen. 0 = hide (default).\n");
    fprintf(f, "show_fps = %d\n\n", showFps ? 1 : 0);
    P("; hud_layout: where the in-game HUD (status display, FPS counter) goes\n"
      ";   when aspect = 2. 0 = the original's place inside the centred 4:3 area\n"
      ";   (default). 1 = at the screen edges. No effect with aspect 0 or 1.\n");
    fprintf(f, "hud_layout = %d\n\n", hudLayout);
    P("; hud_max_aspect: with hud_layout = 1, how far the HUD may spread on very\n"
      ";   wide screens, as an aspect ratio. 2.4 (default) = about 21:9, so on a\n"
      ";   32:9 monitor the HUD stays within a 21:9 area in the middle instead of\n"
      ";   the far corners. 1.78 = 16:9. 0 = always use the full screen width.\n");
    fprintf(f, "hud_max_aspect = %.2f\n\n", hudMaxAspect);
    P("; hud_scale: size of the in-game HUD, 0.5 to 2.0. 1.0 = original size\n"
      ";   (default). Each part grows or shrinks from its own screen corner.\n"
      ";   Works with every aspect setting.\n");
    fprintf(f, "hud_scale = %.2f\n\n", hudScale);

    P("[audio]\n"
      "; Volumes from 0 (silent) to 100 (full); the default is 50 for each.\n"
      "; master_volume scales both sound effects and music.\n");
    fprintf(f, "master_volume = %d\nsfx_volume = %d\nmusic_volume = %d\n\n", masterVolume, sfxVolume, musicVolume);
    P("; soundfont: a General MIDI SoundFont (.sf2) to play the music with. Leave\n"
      ";   empty to use soundfont.sf2 or magebros.sf2 if one is in the program or game\n"
      ";   folder, otherwise the Windows built-in MIDI synth (like the original).\n"
      ";   system = always use the Windows MIDI synth.\n"
      ";   A relative path is relative to this program's folder.\n"
      ";   Example: soundfont = C:\\SoundFonts\\GeneralUser GS.sf2\n");
    fprintf(f, "soundfont = %s\n\n", soundFont.c_str());
    P("; music_source: where the music comes from.\n"
      ";   auto = MP3 or FLAC files from a folder named \"music\" next to MageBros.exe\n"
      ";          (or next to this program) replace the matching MIDI track; any\n"
      ";          track without one plays its MIDI as usual (default)\n"
      ";   midi = always play the original MIDI files\n"
      "; Files are found by the MIDI track's name: Music01.mp3 or Music01.flac\n"
      "; replaces Music01.mid, and so on (FLAC is used if both exist).\n"
      ";   (Music01 .. Music06, the six tracks next to MageBros.exe)\n"
      "; The original MIDI files stay where they are (next to MageBros.exe).\n"
      "; Replacement music plays, loops, stops and pauses exactly like the MIDI\n"
      "; music. Files with LOOPSTART and LOOPLENGTH (or LOOPEND) tags, in samples,\n"
      "; loop seamlessly between those points; others loop from end to start.\n");
    fprintf(f, "music_source = %s\n\n", musicSource == 1 ? "midi" : "auto");
    P("; music_1 .. music_6: optional file name (inside the music folder) to use for\n"
      ";   that track instead of Music01..Music06 with .mp3/.flac. Leave empty to use\n"
      ";   the default names. Example: music_2 = world 1 theme.flac\n");
    for (int i = 0; i < 6; i++) fprintf(f, "music_%d = %s\n", i + 1, musicFiles[i].c_str());
    fprintf(f, "\n");

    P("[input]\n"
      "; gamepad_deadzone: how far (percent) an analog stick must move before it\n"
      ";   counts. 0 to 90, default 25. Raise it if your character drifts.\n");
    fprintf(f, "gamepad_deadzone = %d\n\n", gamepadDeadzone);

    P("[keys]\n"
      "; Key remapping. Format:  dik_<code> = <key>[, <key>, ...]\n"
      "; <code> is the game's original key code (below). Listing keys REPLACES the\n"
      "; default key for that action, so include the original key if you want to\n"
      "; keep it. Key names are SDL names: A..Z, 0..9, Space, Return, Escape, Tab,\n"
      "; Backspace, Up, Down, Left, Right, Left Shift, Right Shift, Left Ctrl,\n"
      "; Right Ctrl, Left Alt, Right Alt, Keypad 0..Keypad 9, Keypad Enter, F1..F9.\n"
      "; (F10, F11 and F12 are reserved for the options menu, fullscreen and screenshots.)\n"
      ";\n"
      ";   code  original key   used for\n"
      ";   0xC8  Up arrow       player 1 up / menu up\n"
      ";   0xD0  Down arrow     player 1 down / menu down\n"
      ";   0xCB  Left arrow     player 1 left / menu left\n"
      ";   0xCD  Right arrow    player 1 right / menu right\n"
      ";   0x9D  Right Ctrl     player 1 fire / menu select\n"
      ";   0x1A  [              player 1 fire\n"
      ";   0x36  Right Shift    player 1 jump / menu select\n"
      ";   0x1B  ]              player 1 jump\n"
      ";   0x11  W              player 2 up / menu up\n"
      ";   0x1F  S              player 2 down / menu down\n"
      ";   0x1E  A              player 2 left / menu left\n"
      ";   0x20  D              player 2 right / menu right\n"
      ";   0x1D  Left Ctrl      player 2 fire / menu select (player 1's too, alone)\n"
      ";   0x02  1              player 2 fire\n"
      ";   0x2A  Left Shift     player 2 jump / menu select (player 1's too, alone)\n"
      ";   0x03  2              player 2 jump\n"
      ";   0x1C  Return         menu select\n"
      ";   0x19  P              pause\n"
      ";   0x01  Escape         menu back / give up the current level\n"
      ";\n"
      "; Examples (remove the leading ; to use):\n"
      ";dik_0x9D = Right Ctrl, Space\n"
      ";dik_0x36 = Right Shift, Up\n");
    for (auto& kv : keyBinds) {
        fprintf(f, "dik_0x%02X = ", kv.first);
        for (size_t i = 0; i < kv.second.size(); i++) fprintf(f, "%s%s", i ? ", " : "", kv.second[i].c_str());
        fprintf(f, "\n");
    }
    P("\n[gamepad]\n"
      "; Gamepad remapping. Format:  button_<n> = <button>[, <button>, ...]\n"
      "; <n> is the original joystick button number. The game uses only two:\n"
      ";   button_0  fire / menu select   default: a (bottom face button)\n"
      ";   button_1  jump / menu select   default: b (right face button)\n"
      "; Gamepad 1 plays player 1 and gamepad 2 player 2; both use these.\n"
      "; Movement is always the left stick and the d-pad.\n"
      "; Button names: a, b, x, y (positions as on an Xbox pad, same buttons on\n"
      "; PlayStation/Switch pads), back, start, leftshoulder, rightshoulder,\n"
      "; leftstick, rightstick, dpup, dpdown, dpleft, dpright, guide.\n"
      "; Listing buttons replaces the default for that number.\n"
      "; Example:\n"
      ";button_0 = a, rightshoulder\n"
      ";button_1 = b, leftshoulder\n");
    for (auto& kv : padBinds) {
        fprintf(f, "button_%d = ", kv.first);
        for (size_t i = 0; i < kv.second.size(); i++) fprintf(f, "%s%s", i ? ", " : "", kv.second[i].c_str());
        fprintf(f, "\n");
    }
    P("\n[cheats]\n"
      "; Game modifiers. All off by default; with them off the game plays exactly\n"
      "; like the original. (Not used yet: the cheats come in a later version.)\n"
      ";\n"
      "; unlock_all_levels: 1 = every level can be chosen on the level-select\n"
      ";   screen, in the original game and in level packs. Your real progress (the\n"
      ";   highest level you have reached) is not changed, so turning this off again\n"
      ";   brings back exactly the levels you had unlocked. Default 0.\n");
    fprintf(f, "unlock_all_levels = %d\n", unlockAllLevels ? 1 : 0);
    P("\n[mods]\n"
      "; Level packs go in mods/levelpacks/, add-ons in mods/addons/ (next to this program);\n"
      ";   (not used yet: mods come in a later version).\n"
      "; pack: the level pack to play. original = the original 40 levels (default);\n"
      ";   otherwise the id of an installed level pack.\n");
    fprintf(f, "pack = %s\n", modPack.c_str());
    P("; One line per installed add-on, in load order: <add-on id> = on / off. When two\n"
      ";   add-ons replace the same thing, the one higher up wins (as in the menu). New\n"
      ";   add-ons are added at the end, switched on.\n");
    for (auto& m : mods) fprintf(f, "%s = %s\n", m.first.c_str(), m.second ? "on" : "off");
    fclose(f);
    return true;
}
