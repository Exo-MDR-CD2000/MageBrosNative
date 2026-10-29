#include "dxgi_present.h"
#include "../core/log.h"
#include <SDL3/SDL.h>

#if defined(_WIN32)
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_5.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>

// WGL_NV_DX_interop / WGL_NV_DX_interop2 (the second adds Direct3D 10 and 11)
#define WGL_ACCESS_WRITE_DISCARD_NV 0x0002
typedef HANDLE(WINAPI* PFN_wglDXOpenDeviceNV)(void* dxDevice);
typedef BOOL(WINAPI* PFN_wglDXCloseDeviceNV)(HANDLE hDevice);
typedef HANDLE(WINAPI* PFN_wglDXRegisterObjectNV)(HANDLE hDevice, void* dxObject, GLuint name, GLenum type, GLenum access);
typedef BOOL(WINAPI* PFN_wglDXUnregisterObjectNV)(HANDLE hDevice, HANDLE hObject);
typedef BOOL(WINAPI* PFN_wglDXLockObjectsNV)(HANDLE hDevice, GLint count, HANDLE* hObjects);
typedef BOOL(WINAPI* PFN_wglDXUnlockObjectsNV)(HANDLE hDevice, GLint count, HANDLE* hObjects);
typedef const char*(WINAPI* PFN_wglGetExtensionsStringARB)(HDC hdc);
typedef HDC(WINAPI* PFN_wglGetCurrentDC)(void);

namespace {

PFN_wglDXOpenDeviceNV pOpenDevice;
PFN_wglDXCloseDeviceNV pCloseDevice;
PFN_wglDXRegisterObjectNV pRegister;
PFN_wglDXUnregisterObjectNV pUnregister;
PFN_wglDXLockObjectsNV pLock;
PFN_wglDXUnlockObjectsNV pUnlock;

constexpr DXGI_FORMAT kFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

struct State {
    bool active = false;
    HWND hwnd = nullptr;
    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    IDXGISwapChain1* swap = nullptr;
    UINT swapFlags = 0;
    HANDLE interopDev = nullptr;
    // the shared picture: a D3D11 texture that OpenGL sees as a texture
    ID3D11Texture2D* tex = nullptr;
    HANDLE interopTex = nullptr;
    GLuint glTex = 0, fbo = 0;
    int w = 0, h = 0;
    bool locked = false;
    // at most one frame queued: the frame-latency waitable object, waited on once per frame
    HANDLE waitable = nullptr;
    bool waited = false;
    LONGLONG freq = 1, lastPresent = 0;  // QueryPerformanceCounter ticks
    float hz = 0.f;                      // the monitor's refresh rate as reported
    // when presents reached the screen (frame statistics), to see a fixed refresh grid
    struct Sample {
        UINT refresh;
        LONGLONG qpc;
    };
    std::deque<Sample> shown;
    bool grid = false;   // the screen is seen refreshing on a fixed grid
    double period = 0.;  // its period (ticks), measured
    LONGLONG anchor = 0; // a refresh on it
    int gridLogged = -1, gridChanges = 0;
    // MBN_PRESENT_TRACE=<file>: one line per present (test hook, as chunk D's frame log)
    FILE* trace = nullptr;
    UINT presents = 0;
} S;

LONGLONG Qpc() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}

// After each present: the refreshes the frame statistics report over the last second, judged
// only with VSync (without it pictures don't wait for refreshes, and frames aren't aligned).
void UpdateGrid(bool judge) {
    DXGI_FRAME_STATISTICS st{};
    if (FAILED(S.swap->GetFrameStatistics(&st)) || !st.SyncQPCTime.QuadPart) return;
    if (S.trace)
        fprintf(S.trace, "%u,%lld,%u,%lld,%u,%d\n", S.presents, S.lastPresent, st.PresentCount, st.SyncQPCTime.QuadPart,
                st.SyncRefreshCount, S.grid ? 1 : 0);
    if (!judge) {
        S.shown.clear();
        S.grid = false;
        return;
    }
    if (!S.shown.empty() && st.SyncRefreshCount < S.shown.back().refresh) S.shown.clear();
    if (S.shown.empty() || st.SyncRefreshCount != S.shown.back().refresh)
        S.shown.push_back({st.SyncRefreshCount, st.SyncQPCTime.QuadPart});
    while (S.shown.size() > 2 && S.shown.back().qpc - S.shown[1].qpc > S.freq) S.shown.pop_front();
    if (S.shown.size() < 8) return;
    LONGLONG span = S.shown.back().qpc - S.shown.front().qpc;
    UINT refreshes = S.shown.back().refresh - S.shown.front().refresh;
    if (span < S.freq / 2 || refreshes == 0) return;  // (not enough seen yet: keep the verdict)
    // a fixed grid: every picture shown on a refresh of one steady period. (Not necessarily the
    // monitor's own: Windows 10 composes a window on any monitor on the primary monitor's
    // refreshes.) With G-Sync / FreeSync the screen refreshes when a picture comes instead.
    double period = (double)span / refreshes;
    LONGLONG anchor = S.shown.back().qpc;
    double worst = 0.;
    for (const State::Sample& s : S.shown) {
        double n = (double)(anchor - s.qpc) / period;
        worst = std::max(worst, std::fabs(n - std::round(n)) * period);
    }
    bool grid = period > S.freq / 500.0 && period < S.freq / 20.0 && worst < S.freq * 0.0003;
    S.grid = grid;
    S.period = period;
    S.anchor = anchor;
    if ((int)S.grid != S.gridLogged && S.gridChanges < 20) {
        double ms = period * 1000.0 / S.freq;
        if (S.grid) Log("Present: pictures reach the screen on a fixed grid, every %.3f ms (%.2f Hz): with a frame limit, frames start just after a refresh", ms, 1000.0 / ms);
        else Log("Present: no fixed refresh grid (G-Sync / FreeSync, or VSync off): frames start when due");
        if (++S.gridChanges == 20) Log("Present: (further changes of the refresh grid are not logged)");
        S.gridLogged = S.grid;
    }
}

template <class T> void Release(T*& p) {
    if (p) p->Release();
    p = nullptr;
}

// The DXGI adapter of the GPU OpenGL runs on, by its vendor (a laptop or a desktop with
// integrated graphics has two; the default adapter may be the other one).
IDXGIAdapter1* AdapterForGL(IDXGIFactory1* factory) {
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    UINT want = 0;
    if (vendor) {
        if (strstr(vendor, "NVIDIA")) want = 0x10DE;
        else if (strstr(vendor, "ATI") || strstr(vendor, "AMD") || strstr(vendor, "Advanced Micro")) want = 0x1002;
        else if (strstr(vendor, "Intel")) want = 0x8086;
    }
    IDXGIAdapter1* found = nullptr;
    for (UINT i = 0; want; i++) {
        IDXGIAdapter1* a = nullptr;
        if (factory->EnumAdapters1(i, &a) == DXGI_ERROR_NOT_FOUND) break;
        DXGI_ADAPTER_DESC1 d{};
        a->GetDesc1(&d);
        if (d.VendorId == want && !(d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
            found = a;
            break;
        }
        a->Release();
    }
    return found;
}

void DestroyTarget() {
    if (S.locked && S.interopTex) pUnlock(S.interopDev, 1, &S.interopTex);
    S.locked = false;
    if (S.interopTex) pUnregister(S.interopDev, S.interopTex);
    S.interopTex = nullptr;
    if (S.fbo) glDeleteFramebuffers(1, &S.fbo);
    if (S.glTex) glDeleteTextures(1, &S.glTex);
    S.fbo = S.glTex = 0;
    Release(S.tex);
    S.w = S.h = 0;
}

bool CreateTarget(int w, int h) {
    DestroyTarget();
    if (FAILED(S.swap->ResizeBuffers(0, (UINT)w, (UINT)h, DXGI_FORMAT_UNKNOWN, S.swapFlags))) {
        Log("Present: DXGI ResizeBuffers to %dx%d failed", w, h);
        return false;
    }
    D3D11_TEXTURE2D_DESC td{};
    td.Width = (UINT)w;
    td.Height = (UINT)h;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = kFormat;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(S.dev->CreateTexture2D(&td, nullptr, &S.tex))) {
        Log("Present: the shared %dx%d texture could not be made", w, h);
        return false;
    }
    glGenTextures(1, &S.glTex);
    S.interopTex = pRegister(S.interopDev, S.tex, S.glTex, GL_TEXTURE_2D, WGL_ACCESS_WRITE_DISCARD_NV);
    if (!S.interopTex) {
        Log("Present: wglDXRegisterObjectNV failed (error %lu)", GetLastError());
        return false;
    }
    glGenFramebuffers(1, &S.fbo);
    GLenum status = 0;
    // (attached and checked with the object locked for OpenGL)
    if (pLock(S.interopDev, 1, &S.interopTex)) {
        glBindFramebuffer(GL_FRAMEBUFFER, S.fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, S.glTex, 0);
        status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        pUnlock(S.interopDev, 1, &S.interopTex);
    }
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        Log("Present: the shared framebuffer is incomplete (status 0x%04x)", status);
        return false;
    }
    S.w = w;
    S.h = h;
    return true;
}

bool Fail(const char* why) {
    Log("Present: DXGI not used (%s); using OpenGL's own", why);
    DxgiPresent::Shutdown();
    return false;
}

}  // namespace

namespace DxgiPresent {

bool Init(SDL_Window* window) {
    S.hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    if (!S.hwnd) return Fail("no window handle");

    // the interop extension, from the WGL extension string of the current context
    HMODULE gl = GetModuleHandleA("opengl32.dll");
    auto getDC = gl ? (PFN_wglGetCurrentDC)(void*)GetProcAddress(gl, "wglGetCurrentDC") : nullptr;
    auto getExt = (PFN_wglGetExtensionsStringARB)SDL_GL_GetProcAddress("wglGetExtensionsStringARB");
    const char* ext = getExt && getDC ? getExt(getDC()) : nullptr;
    if (!ext || !strstr(ext, "WGL_NV_DX_interop2")) return Fail("the graphics driver has no WGL_NV_DX_interop2");
    pOpenDevice = (PFN_wglDXOpenDeviceNV)SDL_GL_GetProcAddress("wglDXOpenDeviceNV");
    pCloseDevice = (PFN_wglDXCloseDeviceNV)SDL_GL_GetProcAddress("wglDXCloseDeviceNV");
    pRegister = (PFN_wglDXRegisterObjectNV)SDL_GL_GetProcAddress("wglDXRegisterObjectNV");
    pUnregister = (PFN_wglDXUnregisterObjectNV)SDL_GL_GetProcAddress("wglDXUnregisterObjectNV");
    pLock = (PFN_wglDXLockObjectsNV)SDL_GL_GetProcAddress("wglDXLockObjectsNV");
    pUnlock = (PFN_wglDXUnlockObjectsNV)SDL_GL_GetProcAddress("wglDXUnlockObjectsNV");
    if (!pOpenDevice || !pCloseDevice || !pRegister || !pUnregister || !pLock || !pUnlock)
        return Fail("the interop functions are missing");

    // Direct3D 11 on the GPU OpenGL runs on
    IDXGIFactory2* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory2), (void**)&factory))) return Fail("no DXGI 1.2 factory");
    IDXGIAdapter1* adapter = AdapterForGL(factory);
    D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};
    HRESULT hr = D3D11CreateDevice(adapter, adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                                   levels, 3, D3D11_SDK_VERSION, &S.dev, nullptr, &S.ctx);
    if (adapter) {
        DXGI_ADAPTER_DESC1 d{};
        adapter->GetDesc1(&d);
        char name[128];
        WideCharToMultiByte(CP_UTF8, 0, d.Description, -1, name, sizeof name, nullptr, nullptr);
        Log("Present: Direct3D 11 on %s", name);
        adapter->Release();
    }
    if (FAILED(hr)) {
        factory->Release();
        return Fail("Direct3D 11 could not start");
    }

    // tearing allowed when the system has it (Windows 10 1709 on): VSync off, and adaptive's late frames
    IDXGIFactory5* f5 = nullptr;
    if (SUCCEEDED(factory->QueryInterface(__uuidof(IDXGIFactory5), (void**)&f5))) {
        BOOL tearing = FALSE;
        if (SUCCEEDED(f5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing, sizeof tearing)) && tearing)
            S.swapFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        f5->Release();
    }
    S.swapFlags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    int w = 1, h = 1;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    DXGI_SWAP_CHAIN_DESC1 sd{};
    sd.Width = (UINT)(w > 0 ? w : 1);
    sd.Height = (UINT)(h > 0 ? h : 1);
    sd.Format = kFormat;
    sd.SampleDesc.Count = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.Scaling = DXGI_SCALING_STRETCH;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    sd.Flags = S.swapFlags;
    hr = factory->CreateSwapChainForHwnd(S.dev, S.hwnd, &sd, nullptr, nullptr, &S.swap);
    if (SUCCEEDED(hr)) factory->MakeWindowAssociation(S.hwnd, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);
    factory->Release();
    if (FAILED(hr)) {
        char why[96];
        snprintf(why, sizeof why, "the flip swapchain was refused, 0x%08lx", (unsigned long)hr);
        return Fail(why);
    }

    IDXGISwapChain2* swap2 = nullptr;
    if (SUCCEEDED(S.swap->QueryInterface(__uuidof(IDXGISwapChain2), (void**)&swap2))) {
        swap2->SetMaximumFrameLatency(1);
        S.waitable = swap2->GetFrameLatencyWaitableObject();
        swap2->Release();
    }
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    S.freq = f.QuadPart;
    if (const char* t = SDL_getenv("MBN_PRESENT_TRACE")) {
        S.trace = fopen(t, "w");
        if (S.trace) fprintf(S.trace, "# qpc frequency %lld\npresent,present_qpc,shown_present,shown_qpc,shown_refresh,grid\n", S.freq);
    }

    S.interopDev = pOpenDevice(S.dev);
    if (!S.interopDev) return Fail("wglDXOpenDeviceNV failed");
    if (!CreateTarget(sd.Width, sd.Height)) return Fail("the shared picture could not be set up");
    S.active = true;
    Log("Present: DXGI flip swapchain %dx%d, %s%s", S.w, S.h, S.waitable ? "one frame queued" : "no frame-latency object",
        (S.swapFlags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) ? ", tearing supported" : "");
    return true;
}

void Shutdown() {
    if (S.interopDev) {
        DestroyTarget();
        pCloseDevice(S.interopDev);
    }
    S.interopDev = nullptr;
    if (S.trace) fclose(S.trace);
    S.trace = nullptr;
    if (S.waitable) CloseHandle(S.waitable);
    S.waitable = nullptr;
    Release(S.swap);
    if (S.ctx) S.ctx->ClearState();
    Release(S.ctx);
    Release(S.dev);
    S.active = false;
    S.swapFlags = 0;
    S.shown.clear();
    S.grid = false;
    S.gridLogged = -1;
    S.gridChanges = 0;
}

bool Active() { return S.active; }

bool CanTear() { return (S.swapFlags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) != 0; }

bool WaitForFrame() {
    if (!S.active || !S.waitable || S.waited) return false;
    WaitForSingleObjectEx(S.waitable, 100, TRUE);  // (never stuck: at most 100 ms)
    S.waited = true;
    return true;
}

void SetRefreshRate(float hz) {
    if (hz == S.hz) return;
    S.hz = hz;
    S.shown.clear();  // (another monitor, or another mode: seen afresh)
    S.grid = false;
}

uint64_t NsUntilFrameStart() {
    if (!S.active || !S.grid || S.period <= 0.) return 0;
    LONGLONG now = Qpc();
    if (now - S.anchor > S.freq) return 0;  // (nothing shown for a second: no grid to go by)
    double phase = std::fmod((double)(now - S.anchor), S.period);
    double window = S.freq * 0.001;  // a frame starting within 1 ms of a refresh goes now
    if (phase <= window) return 0;
    double wait = S.period - phase + S.freq * 0.0002;  // to 0.2 ms past the next refresh
    return (uint64_t)(wait * 1e9 / S.freq);
}

GLuint BeginFrame(int w, int h) {
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    if ((w != S.w || h != S.h) && !CreateTarget(w, h)) {
        Log("Present: DXGI stopped after a resize; this run can't show pictures, restart with present_mode = legacy");
        S.active = false;
        return 0;
    }
    S.locked = pLock(S.interopDev, 1, &S.interopTex) != FALSE;
    return S.fbo;
}

void Present(int vsync) {
    if (!S.active) return;
    LONGLONG now = Qpc();
    int syncInterval = vsync == 0 ? 0 : 1;
    // adaptive: a frame that comes more than a refresh and a half after the last one is late, and
    // shows at once rather than waiting for the next refresh
    if (vsync == 2 && S.lastPresent && S.hz > 0.f && now - S.lastPresent > (LONGLONG)(1.5 * S.freq / S.hz))
        syncInterval = 0;
    if (S.locked) pUnlock(S.interopDev, 1, &S.interopTex);
    S.locked = false;
    ID3D11Texture2D* back = nullptr;
    if (SUCCEEDED(S.swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back))) {
        S.ctx->CopyResource(back, S.tex);
        back->Release();
    }
    UINT flags = syncInterval == 0 && (S.swapFlags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) ? DXGI_PRESENT_ALLOW_TEARING : 0;
    S.swap->Present((UINT)syncInterval, flags);
    S.waited = false;
    S.lastPresent = now;
    S.presents++;
    UpdateGrid(vsync != 0);
}

}  // namespace DxgiPresent

#else

namespace DxgiPresent {
bool Init(SDL_Window*) { return false; }
void Shutdown() {}
bool Active() { return false; }
bool CanTear() { return false; }
bool WaitForFrame() { return false; }
GLuint BeginFrame(int, int) { return 0; }
void Present(int) {}
void SetRefreshRate(float) {}
uint64_t NsUntilFrameStart() { return 0; }
}  // namespace DxgiPresent

#endif
