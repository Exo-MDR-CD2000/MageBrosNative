#include "d3dapp.h"
#include "interp.h"
#include "d3dtextr.h"
#include "../core/config.h"
#include "../core/log.h"
#include "../gfx/device.h"
#include "../input/input.h"
#include "../audio/midi.h"
#include "../ui/options_menu.h"
#include <SDL3/SDL.h>
#include <cstdlib>
#include <sstream>

static float g_tickAlpha = 0.f;
static uint64_t g_timeBase = 0;

DWORD AppTimeMs() { return (DWORD)(SDL_GetTicks() - g_timeBase + 1000); }
float AppTickAlpha() { return g_tickAlpha; }

HRESULT BackBuffer::Blt(const RECT* dst, Surface* src, const RECT* srcRect, bool keySrc) {
    return GetDevice().BltToBackBuffer(dst, src, srcRect, keySrc);
}
HRESULT BackBuffer::ColorFill(const RECT* dst, uint32_t rgb) { return GetDevice().ColorFill(dst, rgb); }

CD3DApplication::CD3DApplication() : menu_(std::make_unique<OptionsMenu>(this)) {}
CD3DApplication::~CD3DApplication() {
    menu_.reset();  // owns a GL texture: free it while the context still exists
    D3DTextr_FreeAll();
    GetDevice().Shutdown();
    if (glContext_) SDL_GL_DestroyContext((SDL_GLContext)glContext_);
    if (window_) SDL_DestroyWindow(window_);
}

Device* CD3DApplication::GetD3DDevice() { return &GetDevice(); }
int CD3DApplication::DisplayFPS() const { return GetConfig().showFps ? 1 : 0; }

bool CD3DApplication::OptionsOpen() const { return menu_ && menu_->IsOpen(); }

void CD3DApplication::OpenOptions() {
    if (menu_) menu_->Open();
}

void CD3DApplication::ApplyVSync() { SDL_GL_SetSwapInterval(GetConfig().vsync ? 1 : 0); }

// A monitor by its [video] display number (0 = primary, 1..n in SDL's order; the primary if
// that one isn't connected).
static SDL_DisplayID DisplayFor(int idx) {
    SDL_DisplayID d = SDL_GetPrimaryDisplay();
    if (idx > 0) {
        int n = 0;
        SDL_DisplayID* ids = SDL_GetDisplays(&n);
        if (ids && idx <= n) d = ids[idx - 1];
        SDL_free(ids);
    }
    return d;
}

// Monitor selected in the config (0 = primary).
static SDL_DisplayID ConfigDisplay() { return DisplayFor(GetConfig().displayIndex); }

// A window whose inside starts at (x, y) and is w wide can still be grabbed: the top of its
// inside lies on some monitor with at least 40 pixels of it across.
static bool WindowPosVisible(int x, int y, int w) {
    int n = 0;
    SDL_DisplayID* ids = SDL_GetDisplays(&n);
    bool ok = false;
    for (int i = 0; ids && i < n && !ok; i++) {
        SDL_Rect b;
        if (!SDL_GetDisplayBounds(ids[i], &b)) continue;
        ok = y >= b.y && y < b.y + b.h - 40 && x + w > b.x + 40 && x < b.x + b.w - 40;
    }
    SDL_free(ids);
    return ok;
}

void CD3DApplication::WindowFitArea(int display, int* w, int* h) const {
    SDL_DisplayID d = DisplayFor(display);
    SDL_Rect u{0, 0, 1920, 1080};
    SDL_GetDisplayUsableBounds(d, &u);
    int top = 0, left = 0, bottom = 0, right = 0;
    bool known = window_ && !(SDL_GetWindowFlags(window_) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS)) &&
                 SDL_GetWindowBordersSize(window_, &top, &left, &bottom, &right) && (top | left | bottom | right);
    if (!known) {
        // no bordered window to measure: a Windows 10/11 title bar and frame at that monitor's scale
        float s = SDL_GetDisplayContentScale(d);
        if (s <= 0.f) s = 1.f;
        top = (int)(40 * s + 0.5f);
        left = right = bottom = (int)(8 * s + 0.5f);
    }
    *w = std::max(320, u.w - left - right);
    *h = std::max(240, u.h - top - bottom);
}

void CD3DApplication::DefaultWindowSize(int display, int* w, int* h) const {
    int fw = 0, fh = 0;
    WindowFitArea(display, &fw, &fh);
    *w = 640;
    *h = 480;
    for (float s : kWindowScales) {
        int sw = (int)(640 * s + 0.5f), sh = (int)(480 * s + 0.5f);
        if (sw <= fw && sh <= fh) *w = sw, *h = sh;
    }
}

// The window at its saved size, where it was last if that is still on a monitor, otherwise
// centred on the chosen one. Twice: moving to a monitor with another display scale makes Windows
// resize the window for that scale (keeping the old scale's frame), so the size is set again
// once it is there, and a centred window centred again at that size.
void CD3DApplication::PlaceWindow() {
    const Config& cfg = GetConfig();
    SDL_DisplayID target = ConfigDisplay();
    bool saved = cfg.windowPosSet && WindowPosVisible(cfg.windowX, cfg.windowY, cfg.windowWidth);
    for (int pass = 0; pass < 2; pass++) {
        SDL_SetWindowSize(window_, cfg.windowWidth, cfg.windowHeight);
        if (saved) SDL_SetWindowPosition(window_, cfg.windowX, cfg.windowY);
        else SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED_DISPLAY(target), SDL_WINDOWPOS_CENTERED_DISPLAY(target));
        SDL_SyncWindow(window_);
    }
}

void CD3DApplication::SaveWindowState() {
    windowChangedAt_ = 0;
    Config& cfg = GetConfig();
    if (!window_ || cfg.fullscreen != FullscreenMode::Windowed) return;
    SDL_WindowFlags f = SDL_GetWindowFlags(window_);
    if (f & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_MINIMIZED | SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN)) return;
    bool changed = false;
    bool max = (f & SDL_WINDOW_MAXIMIZED) != 0;
    if (max != cfg.maximized) cfg.maximized = max, changed = true;
    if (!max) {
        // the size and place to come back to (a maximized window keeps the ones from before)
        int w = 0, h = 0, x = 0, y = 0;
        SDL_GetWindowSize(window_, &w, &h);
        SDL_GetWindowPosition(window_, &x, &y);
        if (w >= 320 && h >= 240 && (w != cfg.windowWidth || h != cfg.windowHeight))
            cfg.windowWidth = w, cfg.windowHeight = h, changed = true;
        if (!cfg.windowPosSet || x != cfg.windowX || y != cfg.windowY)
            cfg.windowPosSet = true, cfg.windowX = x, cfg.windowY = y, changed = true;
    }
    // the monitor it is on now (kept as 0 while that is the primary)
    SDL_DisplayID d = SDL_GetDisplayForWindow(window_);
    if (d && d != DisplayFor(cfg.displayIndex)) {
        int idx = 0;
        if (d != SDL_GetPrimaryDisplay()) {
            int n = 0;
            SDL_DisplayID* ids = SDL_GetDisplays(&n);
            for (int i = 0; ids && i < n; i++)
                if (ids[i] == d) idx = i + 1;
            SDL_free(ids);
        }
        cfg.displayIndex = idx, changed = true;
    }
    if (!changed) return;
    if (!cfg.Save()) Log("Config: could not save %s", cfg.path.c_str());
    Log("Display: window %dx%d at (%d, %d)%s, monitor %d", cfg.windowWidth, cfg.windowHeight, cfg.windowX, cfg.windowY,
        cfg.maximized ? ", maximized" : "", cfg.displayIndex);
}

// Borderless mode on Windows is a plain borderless window rather than SDL's "desktop fullscreen":
// GPU drivers promote an OpenGL window that exactly covers a monitor to exclusive fullscreen,
// which makes Alt+Tab flicker, black out or minimize the game. One extra pixel of height keeps
// the window from matching the monitor exactly (that row is off-screen). Elsewhere SDL's
// desktop fullscreen already behaves as borderless.
static bool UseBorderlessWindow() {
#if defined(_WIN32)
    return true;
#else
    return SDL_getenv("MBN_BORDERLESS_WINDOW") != nullptr;  // for testing the Windows path
#endif
}

void CD3DApplication::ApplyDisplaySettings() {
    const Config& cfg = GetConfig();
    SDL_DisplayID target = ConfigDisplay();
    // leave fullscreen (and a maximized window) first so size and monitor changes take effect cleanly
    SDL_SetWindowFullscreen(window_, false);
    SDL_RestoreWindow(window_);
    SDL_SyncWindow(window_);
    if (cfg.fullscreen != FullscreenMode::Windowed) GetConfig().lastFullscreen = cfg.fullscreen;  // for F11

    if (cfg.fullscreen == FullscreenMode::Windowed) {
        SDL_SetWindowBordered(window_, true);
        SDL_SetWindowResizable(window_, true);
        PlaceWindow();
        if (cfg.maximized) {
            SDL_MaximizeWindow(window_);
            SDL_SyncWindow(window_);
        }
        Log("Display: window %dx%d%s, monitor %d", cfg.windowWidth, cfg.windowHeight, cfg.maximized ? " (maximized)" : "",
            cfg.displayIndex);
    } else if (cfg.fullscreen == FullscreenMode::Borderless) {
        SDL_Rect b{0, 0, cfg.windowWidth, cfg.windowHeight};
        SDL_GetDisplayBounds(target, &b);
        if (UseBorderlessWindow()) {
            SDL_SetWindowResizable(window_, false);
            SDL_SetWindowBordered(window_, false);
            SDL_SetWindowPosition(window_, b.x, b.y);
            SDL_SetWindowSize(window_, b.w, b.h + 1);
            SDL_SyncWindow(window_);
            SDL_RaiseWindow(window_);
        } else {
            SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED_DISPLAY(target), SDL_WINDOWPOS_CENTERED_DISPLAY(target));
            SDL_SyncWindow(window_);
            SDL_SetWindowFullscreenMode(window_, nullptr);
            SDL_SetWindowFullscreen(window_, true);
            SDL_SyncWindow(window_);
        }
        Log("Display: borderless %dx%d, monitor %d", b.w, b.h, cfg.displayIndex);
    } else {
        // exclusive: desktop resolution unless a specific mode was chosen
        SDL_SetWindowBordered(window_, true);
        SDL_SetWindowResizable(window_, true);
        SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED_DISPLAY(target), SDL_WINDOWPOS_CENTERED_DISPLAY(target));
        SDL_SyncWindow(window_);
        SDL_DisplayMode mode{};
        bool have = false;
        if (cfg.fullscreenWidth > 0 && cfg.fullscreenHeight > 0)
            have = SDL_GetClosestFullscreenDisplayMode(target, cfg.fullscreenWidth, cfg.fullscreenHeight, 0.f, true, &mode);
        if (!have) {
            if (const SDL_DisplayMode* desk = SDL_GetDesktopDisplayMode(target)) {
                mode = *desk;
                have = true;
            }
        }
        SDL_SetWindowFullscreenMode(window_, have ? &mode : nullptr);
        SDL_SetWindowFullscreen(window_, true);
        SDL_SyncWindow(window_);
        Log("Display: exclusive %dx%d @ %.0f Hz, monitor %d", mode.w, mode.h, mode.refresh_rate, cfg.displayIndex);
    }
}

// Alt+Enter / F11: from a window to the fullscreen mode used last (borderless, or exclusive at its
// chosen resolution), from either fullscreen back to the window; saved like a menu change.
void CD3DApplication::ToggleFullscreen() {
    Config& cfg = GetConfig();
    cfg.fullscreen = cfg.fullscreen == FullscreenMode::Windowed ? cfg.lastFullscreen : FullscreenMode::Windowed;
    ApplyDisplaySettings();
    if (!cfg.Save()) Log("Config: could not save %s", cfg.path.c_str());
}

HRESULT CD3DApplication::Create(const std::vector<std::string>& args) {
    Config& cfg = GetConfig();
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == "--auto" && i + 1 < args.size()) {
            std::stringstream ss(args[++i]);
            std::string item;
            while (std::getline(ss, item, ';')) {
                size_t c = item.find(':');
                if (c == std::string::npos) continue;
                AutoStep s;
                s.t = (DWORD)atoi(item.substr(0, c).c_str());
                std::string act = item.substr(c + 1);
                size_t eq = act.find('=');
                s.action = act.substr(0, eq);
                s.arg = eq == std::string::npos ? "" : act.substr(eq + 1);
                s.done = false;
                auto_.push_back(s);
            }
        }
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    window_ = SDL_CreateWindow(m_strWindowTitle, cfg.windowWidth, cfg.windowHeight,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN);
    if (!window_) {
        Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return E_FAIL;
    }
    SDL_SetWindowMinimumSize(window_, 320, 240);
    if (cfg.displayIndex > 0) {
        int n = 0;
        SDL_DisplayID* ids = SDL_GetDisplays(&n);
        if (!ids || cfg.displayIndex > n)
            Log("Config: display %d not found (%d connected), using the primary monitor", cfg.displayIndex, n);
        SDL_free(ids);
    }
    if (cfg.firstRun) {
        // a fresh ini: the largest 4:3 window that fits the monitor, kept from now on
        DefaultWindowSize(cfg.displayIndex, &cfg.windowWidth, &cfg.windowHeight);
        SDL_SetWindowSize(window_, cfg.windowWidth, cfg.windowHeight);
        cfg.Save();
        Log("Display: first start, window %dx%d", cfg.windowWidth, cfg.windowHeight);
    }
    PlaceWindow();
    glContext_ = SDL_GL_CreateContext(window_);
    if (!glContext_) {
        Log("OpenGL 3.3 context creation failed: %s", SDL_GetError());
        return E_FAIL;
    }
    SDL_GL_MakeCurrent(window_, (SDL_GLContext)glContext_);
    SDL_GL_SetSwapInterval(cfg.vsync ? 1 : 0);
    if (!GetDevice().Init(window_)) return E_FAIL;
    if (cfg.fullscreen != FullscreenMode::Windowed) ApplyDisplaySettings();
    SDL_ShowWindow(window_);
    if (cfg.fullscreen == FullscreenMode::Windowed && cfg.maximized) SDL_MaximizeWindow(window_);
    m_bActive = true;

    g_timeBase = SDL_GetTicks();
    HRESULT hr = OneTimeSceneInit();
    if (FAILED(hr)) return hr;
    hr = InitDeviceObjects();
    if (FAILED(hr)) return hr;
    m_bReady = true;
    m_dwLastFrameTime = AppTimeMs();
    autoStart_ = AppTimeMs();
    return S_OK;
}

void CD3DApplication::Pause(bool pause) {
    (void)pause;  // time base handling is implicit: Render3DEnvironment clamps catch-up to 20 ticks
}

void CD3DApplication::UpdateStats() {
    float t = AppTimeMs() * 0.001f;
    statsFrames_++;
    if (t - statsLastTime_ > 1.0f) {
        m_fFPS = statsFrames_ / (t - statsLastTime_);
        statsFrames_ = 0;
        statsLastTime_ = t;
    }
}

HRESULT CD3DApplication::Render3DEnvironment(DWORD now, bool forceRender) {
    if (now < m_dwLastFrameTime) m_dwLastFrameTime = now;
    menu_->Update(now);
    if (menu_->IsOpen()) {
        // game paused underneath: keep showing the last frame, don't accumulate catch-up time
        m_dwLastFrameTime = now;
        g_tickAlpha = 1.f;
        GetDevice().BeginFrame();
        HRESULT hr = Render();
        if (FAILED(hr)) return hr;
        menu_->Render(&GetDevice());
        UpdateStats();
        GetDevice().Present();
        SDL_GL_SwapWindow(window_);
        return S_OK;
    }
    DWORD elapsed = now - m_dwLastFrameTime;
    DWORD ticks = elapsed / 10;
    if (ticks == 0 && !forceRender) return S_OK;
    if (ticks > 0) {
        m_dwLastFrameTime = now - elapsed % 10;
        if (m_bFrameMoving || m_bSingleStep) {
            Interp::BeginFrameMove();
            HRESULT hr = FrameMove(ticks);
            Interp::EndFrameMove();
            if (FAILED(hr)) return hr;
            m_bSingleStep = false;
        }
    }
    g_tickAlpha = (float)(now - m_dwLastFrameTime) / 10.f;
    if (g_tickAlpha > 1.f) g_tickAlpha = 1.f;
    if (!m_bFrameMoving) g_tickAlpha = 1.f;
    GetDevice().BeginFrame();
    HRESULT hr = Render();
    if (FAILED(hr)) return hr;
    UpdateStats();
    GetDevice().Present();
    SDL_GL_SwapWindow(window_);
    return S_OK;
}

void CD3DApplication::HandleAutomation(DWORD now) {
    DWORD t = now - autoStart_;
    for (auto& s : auto_) {
        if (s.done || t < s.t) continue;
        s.done = true;
        if (s.action == "key" || s.action == "down" || s.action == "up") {
            SDL_Scancode sc = SDL_GetScancodeFromName(s.arg.c_str());
            int dik = Input::DikFromScancode(sc);
            if (s.action != "up") {
                Input::InjectKey(dik, true);
                OnKeyDown(dik);
            }
            if (s.action != "down") {
                // a "key" tap stays down for 50 ms so the 100 Hz polling sees it
                AutoStep rel{s.t + 50, "up", s.arg, false};
                if (s.action == "up") Input::InjectKey(dik, false);
                else auto_.push_back(rel);
            }
            return;  // vector may have grown
        } else if (s.action == "sdlkey") {
            // a real key press through the SDL event queue (reaches the options menu)
            SDL_Event ev{};
            ev.type = SDL_EVENT_KEY_DOWN;
            ev.key.scancode = SDL_GetScancodeFromName(s.arg.c_str());
            ev.key.down = true;
            ev.key.windowID = SDL_GetWindowID(window_);
            SDL_PushEvent(&ev);
            ev.type = SDL_EVENT_KEY_UP;
            ev.key.down = false;
            SDL_PushEvent(&ev);
        } else if (s.action == "focus") {
            // test-only: simulate the window losing (focus=0) or regaining (focus=1) focus
            SDL_Event ev{};
            ev.type = s.arg == "0" ? SDL_EVENT_WINDOW_FOCUS_LOST : SDL_EVENT_WINDOW_FOCUS_GAINED;
            ev.window.windowID = SDL_GetWindowID(window_);
            SDL_PushEvent(&ev);
        } else if (s.action == "vpad") {
            // test-only virtual controller: "vpad", "padaxis=<axis>:<value>", "padbtn=<button>:<0|1>"
            SDL_VirtualJoystickDesc desc;
            SDL_INIT_INTERFACE(&desc);
            desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
            desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
            desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
            desc.name = "Automation pad";
            SDL_JoystickID id = SDL_AttachVirtualJoystick(&desc);
            autoPad_ = SDL_OpenJoystick(id);
            Log("auto: virtual pad %u %s", (unsigned)id, autoPad_ ? "attached" : SDL_GetError());
        } else if ((s.action == "padaxis" || s.action == "padbtn") && autoPad_) {
            size_t c = s.arg.find(':');
            int idx = atoi(s.arg.substr(0, c).c_str()), val = atoi(s.arg.substr(c + 1).c_str());
            if (s.action == "padaxis") SDL_SetJoystickVirtualAxis((SDL_Joystick*)autoPad_, idx, (Sint16)val);
            else SDL_SetJoystickVirtualButton((SDL_Joystick*)autoPad_, idx, val != 0);
        } else if (s.action == "shot") {
            GetDevice().SaveScreenshot(s.arg.c_str());
            Log("auto: screenshot %s", s.arg.c_str());
        } else if (s.action == "quit") {
            quit_ = true;
        } else {
            OnAutomation(s.action, s.arg);
        }
    }
}

int CD3DApplication::Run() {
    const Config& cfg = GetConfig();
    uint64_t perfFreq = SDL_GetPerformanceFrequency();
    uint64_t lastPresent = SDL_GetPerformanceCounter();
    while (!quit_) {
        SDL_Event e;
        bool gotEvent = m_bActive ? SDL_PollEvent(&e) : SDL_WaitEventTimeout(&e, 100);
        while (gotEvent) {
            Input::HandleEvent(e);
            switch (e.type) {
                case SDL_EVENT_QUIT: quit_ = true; break;
                case SDL_EVENT_WINDOW_FOCUS_GAINED: Input::SetFocus(true); OnActivateApp(true); break;
                case SDL_EVENT_WINDOW_FOCUS_LOST: Input::SetFocus(false); OnActivateApp(false); break;
                case SDL_EVENT_WINDOW_MINIMIZED: m_bActive = false; break;
                case SDL_EVENT_WINDOW_RESTORED:
                case SDL_EVENT_WINDOW_MAXIMIZED:
                    WindowChanged(AppTimeMs());
                    [[fallthrough]];
                case SDL_EVENT_WINDOW_SHOWN:
                    if (!m_bActive) m_dwLastFrameTime = AppTimeMs();
                    m_bActive = true;
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                case SDL_EVENT_WINDOW_MOVED:
                case SDL_EVENT_WINDOW_DISPLAY_CHANGED: WindowChanged(AppTimeMs()); break;
                case SDL_EVENT_KEY_DOWN:
                    if (!e.key.repeat && ((e.key.scancode == SDL_SCANCODE_RETURN && (e.key.mod & SDL_KMOD_ALT)) ||
                                          e.key.scancode == SDL_SCANCODE_F11 || e.key.scancode == SDL_SCANCODE_F12)) {
                    } else if (menu_->HandleEvent(e)) {
                        break;
                    }
                    if (e.key.repeat) break;
                    if ((e.key.scancode == SDL_SCANCODE_RETURN && (e.key.mod & SDL_KMOD_ALT)) ||
                        e.key.scancode == SDL_SCANCODE_F11) {
                        ToggleFullscreen();
                        break;
                    }
                    if (e.key.scancode == SDL_SCANCODE_F12) {
                        char name[1024];
                        const char* base = SDL_GetBasePath();
                        snprintf(name, sizeof name, "%smagebros_%u.bmp", base ? base : "", (unsigned)AppTimeMs());
                        GetDevice().SaveScreenshot(name);
                        Log("Saved screenshot %s", name);
                        break;
                    }
                    {
                        std::vector<int> diks;
                        Input::DiksForScancode(e.key.scancode, &diks);
                        for (int d : diks) OnKeyDown(d);
                    }
                    break;
                default:
                    menu_->HandleEvent(e);  // gamepad: Start opens the menu; navigation while open
                    break;
            }
            gotEvent = SDL_PollEvent(&e);
        }
        if (quit_) break;
        if (Midi::PollFinished()) OnMidiNotifySuccessful();
        DWORD now = AppTimeMs();
        if (windowChangedAt_ && now - windowChangedAt_ >= kWindowSettleMs) SaveWindowState();
        if (!auto_.empty()) HandleAutomation(now);
        if (!m_bActive || !m_bReady) continue;

        // frame pacing for the interpolated (decoupled) path; the options menu always redraws (max 60 fps)
        bool menuOpen = menu_->IsOpen();
        bool force = cfg.interpolation || menuOpen;
        int cap = menuOpen ? 60 : cfg.maxFps;
        if (force && cap > 0) {
            double minDt = 1.0 / cap;
            double dt = (double)(SDL_GetPerformanceCounter() - lastPresent) / perfFreq;
            if (dt < minDt) {
                SDL_DelayPrecise((uint64_t)((minDt - dt) * 1e9 * 0.9));
                continue;
            }
        }
        DWORD before = m_dwLastFrameTime;
        if (FAILED(Render3DEnvironment(now, force))) quit_ = true;
        if (!force && m_dwLastFrameTime == before) SDL_Delay(1);  // original only renders on tick boundaries
        else lastPresent = SDL_GetPerformanceCounter();
    }
    if (windowChangedAt_) SaveWindowState();  // a resize or move just before quitting
    DeleteDeviceObjects();
    FinalCleanup();
    return 0;
}
