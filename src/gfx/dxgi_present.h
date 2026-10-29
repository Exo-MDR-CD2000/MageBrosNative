// Presenting through a DXGI flip-model swapchain (Windows; present_mode = dxgi).
//
// OpenGL's own SwapBuffers in a window or borderless is copied into the desktop by DWM, which
// adds a frame of delay and keeps G-Sync / FreeSync from working there. Here the picture goes
// out the way a Direct3D 11 game's does: the renderer's last pass draws into a D3D11 texture
// shared with OpenGL (WGL_NV_DX_interop2, on NVIDIA, AMD and Intel), which is copied to a flip
// swapchain's back buffer and presented. Nothing else in the renderer changes.
#pragma once
#include "gl.h"
#include <cstdint>

struct SDL_Window;

namespace DxgiPresent {

// Sets the path up on the window (the OpenGL context must be current). False, with the reason
// logged, when the system can't do it: the caller then keeps OpenGL's own SwapBuffers.
bool Init(SDL_Window* window);
void Shutdown();
bool Active();
// Whether presents can tear (VSync off and adaptive's late frames show at once).
bool CanTear();

// Waits until the swapchain can take another frame (at most one queued: the frame-latency
// waitable object), once per presented frame. True if it waited.
bool WaitForFrame();

// The framebuffer the frame's last pass draws into, w x h pixels (the window's drawable size),
// locked for OpenGL until Present. Its rows run top to bottom, unlike the window's own.
GLuint BeginFrame(int w, int h);
// Hands the frame to DXGI and presents it. vsync: 0 off, 1 on, 2 adaptive (on, but a frame
// later than a refresh shows at once).
void Present(int vsync);

// The monitor's refresh rate as the system reports it (Hz; 0 = unknown), for telling a fixed
// refresh from G-Sync / FreeSync.
void SetRefreshRate(float hz);
// While the screen is seen refreshing on a fixed grid (no G-Sync / FreeSync, or a frame rate at
// the refresh rate): how long to wait (ns) so that a frame due now starts just after a refresh.
// 0 when it already does, or when there is no fixed grid.
uint64_t NsUntilFrameStart();

}  // namespace DxgiPresent
