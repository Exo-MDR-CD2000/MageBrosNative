// Replacement for the DX7 SDK CD3DApplication (d3dapp.cpp) on SDL3.
#pragma once
#include "../core/d3dtypes.h"
#include <memory>
#include <string>
#include <vector>

class Device;
class Surface;
class OptionsMenu;
struct SDL_Window;

// What the game sees as IDirectDrawSurface7* for the back buffer.
class BackBuffer {
public:
    // DDBLT_KEYSRC is the only flag that matters for the game
    HRESULT Blt(const RECT* dst, Surface* src, const RECT* srcRect, bool keySrc);
    HRESULT ColorFill(const RECT* dst, uint32_t rgb);
};

#define DDBLT_WAIT 0x01000000
#define DDBLT_KEYSRC 0x00008000

class CD3DApplication {
public:
    CD3DApplication();
    virtual ~CD3DApplication();

    virtual HRESULT Create(const std::vector<std::string>& args);
    virtual int Run();
    virtual void Pause(bool pause);

    Device* GetD3DDevice();
    BackBuffer* GetBackBuffer() { return &backBuffer_; }
    int GetScreenWidth() const { return 640; }
    int GetScreenHeight() const { return 480; }
    int GetFPS() const { return (int)(m_fFPS + 0.5f); }
    int DisplayFPS() const;
    SDL_Window* GetWindow() const { return window_; }
    void RequestQuit() { quit_ = true; }
    // Re-apply window mode, size and monitor from the config (used by the options menu).
    void ApplyDisplaySettings();
    // [video] vsync to the swap interval: 0 off, 1 on, 2 adaptive (-1), which falls back to on where
    // the driver refuses it. What took effect is read back (VSyncInEffect).
    void ApplyVSync();
    bool AdaptiveVSyncSupported() const { return adaptiveSupported_; }
    int VSyncInEffect() const { return vsyncInEffect_; }  // 0, 1 or 2
    // What the frame rate says about the driver (NVIDIA Control Panel, AMD Software) overriding
    // VSync: +1 = VSync is on here but frames come faster than the monitor refreshes; -1 = it is
    // off here but frames stop at the refresh rate; 0 = nothing seen. Judged only while frames are
    // drawn freely (Smooth motion on, no frame limit, the menu closed), from two seconds running.
    int VSyncOverride() const { return vsyncOverride_; }
    // The room a window's inside has on a monitor (0 = primary, as [video] display): its usable
    // area (without the taskbar) less the window's title bar and borders.
    void WindowFitArea(int display, int* w, int* h) const;
    // The largest 4:3 window from kWindowScales (multiples of the game's 640 x 480) that fits
    // that monitor; at least 640 x 480. The first start and "Reset all settings" use it.
    void DefaultWindowSize(int display, int* w, int* h) const;
    // Monitors by [video] display number (1..n in SDL's order; 0 = the primary).
    static std::string DisplayName(int index);
    static int DisplayCount();
    // Choose a monitor: its number and its name (kept, so it is found again under another number).
    static void SetDisplay(int index);
    // display_name, when that monitor is connected, decides the number (at start and whenever a
    // monitor is plugged in or out); when it isn't, the primary is used for now.
    static void ResolveDisplay();
    static constexpr float kWindowScales[] = {1.f, 1.25f, 1.5f, 1.6f, 2.f, 2.5f, 3.f, 3.5f, 4.f, 4.5f, 5.f, 6.f};
    bool OptionsOpen() const;
    void OpenOptions();  // e.g. from the title screen's OPTIONS entry
    // Hooks for the options/pause menu, implemented by the game.
    enum class UISound { Move, Confirm };
    virtual void PlayUISound(UISound s) { (void)s; }
    virtual bool CanReturnToMainMenu() const { return false; }
    virtual void ReturnToMainMenu() {}
    virtual void ReturnToLevelSelect() {}  // from a level (offered beside the main menu when InLevel)
    virtual bool InLevel() const { return false; }
    // Whether F10 / Start may open the options menu right now (false during the intro logo,
    // the ending and the fades, and in a level whenever no mage is under the player's control).
    virtual bool CanOpenOptions() const { return true; }
    virtual void ReloadMusic() {}                       // music source changed: restart the current track
    virtual void ApplyModChanges() {}                   // mods or level pack changed (title screen only)
    virtual const char* MusicSource() const { return ""; }

protected:
    virtual HRESULT OneTimeSceneInit() { return S_OK; }
    virtual HRESULT InitDeviceObjects() { return S_OK; }
    virtual HRESULT DeleteDeviceObjects() { return S_OK; }
    virtual HRESULT Render() { return S_OK; }
    virtual HRESULT FrameMove(DWORD) { return S_OK; }
    virtual HRESULT RestoreSurfaces() { return S_OK; }
    virtual HRESULT FinalCleanup() { return S_OK; }
    // Window-message equivalents for derived classes
    virtual void OnActivateApp(bool active) { (void)active; }
    virtual void OnKeyDown(int dik) { (void)dik; }
    virtual void OnMidiNotifySuccessful() {}
    virtual void OnAutomation(const std::string& action, const std::string& arg) { (void)action; (void)arg; }

    bool m_bActive = false;
    bool m_bReady = false;
    bool m_bFrameMoving = true;
    bool m_bSingleStep = false;
    DWORD m_dwLastFrameTime = 0;  // when the latest tick came due (ms; lastTickNs_ to the ns)
    float m_fFPS = 0.f;
    const char* m_strWindowTitle = "D3D Application";
    bool m_bAppUseZBuffer = false;
    bool m_bShowStats = false;

private:
    HRESULT Render3DEnvironment(DWORD now, bool forceRender);
    // Ticks are counted on the nanosecond clock (still exactly 10 ms each), so the smooth motion
    // alpha has no millisecond steps. HoldTickPhase keeps the blend where it is while the game
    // stands still (the options menu, P), so the picture doesn't jump when it moves again.
    void SetLastTick(uint64_t ns) {
        lastTickNs_ = ns;
        m_dwLastFrameTime = (DWORD)(ns / 1000000);
    }
    void HoldTickPhase(uint64_t nowNs, uint64_t tickNow);
    uint64_t lastTickNs_ = 0;
    bool heldLastFrame_ = false;  // the last frame was drawn with the game standing still
    void UpdateStats();
    void ToggleFullscreen();
    void HandleAutomation(DWORD now);
    // A window event (resized, moved, maximized, restored, another monitor): once things have
    // been still for kWindowSettleMs, the window's state is read and saved (SaveWindowState).
    void WindowChanged(DWORD now) { windowChangedAt_ = now ? now : 1; }
    void SaveWindowState();
    void PlaceWindow();
    static constexpr DWORD kWindowSettleMs = 500;
    DWORD windowChangedAt_ = 0;  // 0 = nothing to save

    SDL_Window* window_ = nullptr;
    void* glContext_ = nullptr;
    BackBuffer backBuffer_;
    bool quit_ = false;
    DWORD statsFrames_ = 0;
    float statsLastTime_ = 0.f;
    void JudgeVSync();  // each second's frame rate against the monitor's refresh rate
    bool adaptiveSupported_ = false;
    int vsyncInEffect_ = 1;
    int vsyncOverride_ = 0;
    int vsyncVote_ = 0, vsyncVotes_ = 0;  // the last second's verdict and how many in a row
    int vsyncLogged_ = -1;

    // scripted run for automated testing: "time_ms:action[=arg]" entries
    struct AutoStep { DWORD t; std::string action, arg; bool done; };
    std::vector<AutoStep> auto_;
    DWORD autoStart_ = 0;
    std::vector<int> autoHeldKeys_;
    void* autoPad_ = nullptr;  // SDL_Joystick* of the test-only virtual controller
    std::unique_ptr<OptionsMenu> menu_;
};

// Timer used by the framework (milliseconds, like timeGetTime)
DWORD AppTimeMs();
// The same clock in nanoseconds (AppTimeMs() == AppTimeNs() / 1,000,000)
uint64_t AppTimeNs();
// Fraction (0..1) of the current 10 ms logic tick that has elapsed at render time.
float AppTickAlpha();
// Inject a DIK key state for automation (pressed = true/false)
void AppInjectKey(int dik, bool pressed);
bool AppInjectedKey(int dik);
