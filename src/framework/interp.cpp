#include "interp.h"
#include "d3dapp.h"
#include "../core/config.h"
#include <cmath>

namespace {
unsigned g_tick = 0;
bool g_scope = false;
float g_held = -1.f;
}  // namespace

namespace Interp {

void BeginTick() { g_tick++; }
unsigned CurrentTick() { return g_tick; }

bool Enabled() { return GetConfig().interpolation; }

float Alpha() {
    if (!g_scope || !Enabled()) return 1.f;
    return g_held >= 0.f ? g_held : AppTickAlpha();
}

void SetScope(bool on) { g_scope = on; }

void HoldAlpha(float a) { g_held = a > 1.f ? 1.f : a; }

float LerpAngle(float a, float b, float t) {
    if (t >= 1.f) return b;
    const float kTwoPi = 6.2831853f;
    float d = std::fmod(b - a, kTwoPi);
    if (d > kTwoPi * 0.5f) d -= kTwoPi;
    else if (d < -kTwoPi * 0.5f) d += kTwoPi;
    return a + d * t;
}

}  // namespace Interp
