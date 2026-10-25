// Angle and distance helpers of the original (JStandard / JAscii era utilities used by the X*
// classes). See docs/REVERSE_ENGINEERING.md, "The hero: Helpers".
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "core/d3dtypes.h"

constexpr float kPi = 3.14159265f;
constexpr float kTwoPi = 6.28318531f;
constexpr float kFloatMax = 3.40282347e+38f;

// Eases `cur` toward `target` by `factor` of the gap per call, at least `minStep` and at most
// `maxStep`, snapping once closer than `minStep` (MakeValueCloseTo).
float MakeValueCloseTo(float cur, float target, float factor, float minStep, float maxStep);
// Wraps an angle into [0, 2pi).
float NormaliseAngle(float a);
// Eases angle `cur` towards `target` the short way round (MakeValueCloseTo on angles).
float MakeAngleCloseTo(float cur, float target, float factor, float minStep, float maxStep);
// MakeValueCloseTo_Constant / MakeAngleCloseTo_Constant: a fixed step towards the target (the
// angle the short way round).
float MakeValueCloseTo_Constant(float cur, float target, float step);
float MakeAngleCloseTo_Constant(float cur, float target, float step);
// AngleDifference: the unsigned angle between two angles, 0..pi (both normalised first)
float AngleDifference(float a, float b);
// atan2(a, b), normalised: the yaw of a stick direction is FindYaw(y, x).
float FindYaw(float a, float b);
// FindDirectionXZ: the yaw from `from` towards `to`
float FindDirectionXZ(const D3DVECTOR& from, const D3DVECTOR& to);
// Interpolate: a + (b - a) * t, and D3DVectorInterpolate on each coordinate.
float Interpolate(float a, float b, float t);
D3DVECTOR D3DVectorInterpolate(const D3DVECTOR& a, const D3DVECTOR& b, float t);
// FindPitch: atan(dy / distance in XZ) from `from` to `to`; straight up or down (+-90 degrees)
// when they share X and Z.
float FindPitch(const D3DVECTOR& from, const D3DVECTOR& to);
// Manhattan distance refined by two Newton steps of the square root (not exact, on purpose).
float ApproxDistance(const D3DVECTOR& a, const D3DVECTOR& b);
float ApproxDistanceXZ(const D3DVECTOR& a, const D3DVECTOR& b);  // the same, ignoring Y
// JStandard's random helpers, on the C library's rand() like the original:
// UnitRand = (rand() % 1000) / 1000; RandomF = lo + UnitRand * (hi - lo);
// RandomPosXZ = a point at a random angle and a random distance up to `radius` around `centre`.
float UnitRand();
float RandomF(float lo, float hi);
D3DVECTOR RandomPosXZ(const D3DVECTOR& centre, float radius);
