// User settings for the enhanced features, stored in magebros_native.ini.
// None of these change gameplay logic; the simulation always runs the original 10 ms tick.
#pragma once
#include <string>
#include <map>
#include <vector>

enum class AspectMode { Original4x3 = 0, Stretch = 1, Widescreen = 2 };
enum class FullscreenMode { Windowed = 0, Borderless = 1, Exclusive = 2 };

struct Config {
    // video
    int windowWidth = 1280;        // a fresh ini gets the largest 4:3 size that fits the monitor
    int windowHeight = 960;
    bool windowPosSet = false;     // window_x / window_y given (otherwise the window is centred)
    int windowX = 0, windowY = 0;  // the window's last position (its client area's top left)
    bool maximized = false;        // the window was maximized (width / height: its size before)
    FullscreenMode fullscreen = FullscreenMode::Windowed;
    FullscreenMode lastFullscreen = FullscreenMode::Borderless;  // where F11 / Alt+Enter goes from a window
    int fullscreenWidth = 0;       // exclusive fullscreen screen mode; 0 = desktop resolution
    int fullscreenHeight = 0;
    float fullscreenRefresh = 0.f; // exclusive fullscreen refresh rate (Hz); 0 = the desktop's
    int displayIndex = 0;          // 0 = primary monitor, 1..n = monitors in system order
    std::string displayName;       // that monitor's name; when connected it wins over the number
    int vsync = 1;                 // 0 off, 1 on, 2 adaptive (on where the driver can't)
    int maxFps = 0;                // 0 = uncapped (or vsync limited)
    bool interpolation = true;     // smooth motion between 100 Hz logic ticks
    int msaa = 4;                  // 0, 2, 4, 8
    int anisotropy = 8;            // 1 = off
    bool smoothTextures = false;   // force linear filtering on 3D textures that used point sampling
    bool smoothSprites = false;    // bilinear scaling of 2D sprites
    bool perPixelLighting = false; // evaluate the original lighting per pixel instead of per vertex
    bool enhancedShadows = false;  // softer blob shadows
    int brightness = 100;          // 50..150 %, applied as a gamma curve to the final picture
    float renderScale = 1.0f;      // supersampling factor for the 3D scene
    AspectMode aspect = AspectMode::Original4x3;
    float fovScale = 1.0f;         // multiplies the original vertical FOV (widescreen keeps vertical FOV)
    bool showFps = false;
    int hudLayout = 0;             // widescreen only: 0 = original centred 4:3 HUD (default), 1 = HUD at screen edges
    float hudMaxAspect = 2.4f;     // edges layout spreads at most this wide (0 = no limit)
    float hudScale = 1.0f;         // in-game HUD size (0.5..2.0); each part grows from its screen corner

    // audio
    int masterVolume = 50;   // 0..100 (default 50)
    int sfxVolume = 50;
    int musicVolume = 50;
    std::string soundFont;
    int musicSource = 0;            // 0 = MP3/FLAC from the music folder when present, else MIDI; 1 = MIDI only
    std::string musicFiles[6];      // optional file name in the music folder for each track (Music01..06)

    // input
    int gamepadDeadzone = 25;  // percent
    // Remapping: original DirectInput key code -> list of SDL scancode names that trigger it
    std::map<int, std::vector<std::string>> keyBinds;
    std::map<int, std::vector<std::string>> padBinds;  // joystick button index -> gamepad button names

    std::string gamePath;      // optional explicit path to MageBros.exe
    std::string savePath;      // [game] save_path: empty = the per-user data folder (see core/saves.h)
    // [mods]: installed add-ons in load order (first wins) with on/off, and the level pack to play
    std::vector<std::pair<std::string, bool>> mods;
    std::string modPack = "original";
    // [cheats] (off by default; not used yet: Mage Bros.' own cheats come in stage 7)
    bool unlockAllLevels = false;     // every level selectable; saved progress is left untouched

    std::string path;
    int version = 0;                    // config_version found in the file (0 = older file)
    static constexpr int kCurrentVersion = 20;
    bool firstRun = false;              // no ini existed at start (not saved): size the window to the monitor
    bool Load(const std::string& file);
    bool Save() const;
};

Config& GetConfig();
