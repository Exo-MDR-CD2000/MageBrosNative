// Smooth motion: drawing between the game's fixed 10 ms logic ticks. Purely visual: the ticks
// set every pose as always; objects that opt in keep their pose before the latest tick and are
// drawn blended between it and the current one. See docs/SMOOTH_MOTION.md.
#pragma once
#include "../core/d3dtypes.h"

namespace Interp {

// A tick of play is about to run (XGameScene calls it before each one); objects compare it with
// the tick of their last pose change.
void BeginTick();
unsigned CurrentTick();

bool Enabled();  // the Smooth motion setting
// How far the clock is between the latest two ticks: 0 = the previous tick's pose, 1 = the
// latest. Always 1 with Smooth motion off or outside a blending scope.
float Alpha();
// XGameScene::Render turns blending on for the level's 3D only.
void SetScope(bool on);
bool InScope();
// A level's picture is about to be drawn (XGameScene::Render); FramesDrawn counts them, so a
// model can tell whether it was drawn in the last one.
void BeginFrame();
unsigned FramesDrawn();
// Test hook (--auto alpha=): hold alpha at `a` (0..1); a negative value frees it.
void HoldAlpha(float a);

inline float Lerp(float a, float b, float t) { return t >= 1.f ? b : a + (b - a) * t; }
inline D3DVECTOR Lerp(const D3DVECTOR& a, const D3DVECTOR& b, float t) {
    if (t >= 1.f) return b;
    return D3DVECTOR(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
}
// an angle in radians, the short way round
float LerpAngle(float a, float b, float t);
// Where to draw something whose owner keeps its position from before tick `prevTick` (sprites:
// particles, shots, the flame): between `prev` and `cur` if that tick is the latest, else `cur`
// (and `cur` itself at alpha 1, or after a jump of more than 40 in the tick).
D3DVECTOR BlendPos(const D3DVECTOR& prev, const D3DVECTOR& cur, unsigned prevTick);

}  // namespace Interp
