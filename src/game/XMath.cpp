#include "XMath.h"
#include <cmath>
#include <cstdlib>

float MakeValueCloseTo(float cur, float target, float factor, float minStep, float maxStep) {
    float gap = std::fabs(cur - target);
    if (gap < minStep) return target;
    float step = gap * factor;
    if (step < minStep) step = minStep;
    else if (step > maxStep) step = maxStep;
    return target > cur ? cur + step : cur - step;
}

float AngleDifference(float a, float b) {
    a = NormaliseAngle(a);
    b = NormaliseAngle(b);
    if (std::fabs(a - b) > 3.14159f) a += b > a ? 6.28319f : -6.28319f;
    return std::fabs(a - b);
}

float NormaliseAngle(float a) {
    if (a >= kTwoPi) {
        int turns = (int)(a / kTwoPi);
        return a - turns * kTwoPi;
    }
    if (a < 0.0f) {
        int turns = (int)(a / kTwoPi - 1.0f);
        a -= turns * kTwoPi;
        return a == kTwoPi ? 0.0f : a;
    }
    return a;
}

float MakeAngleCloseTo(float cur, float target, float factor, float minStep, float maxStep) {
    cur = NormaliseAngle(cur);
    target = NormaliseAngle(target);
    if (std::fabs(cur - target) > kPi) {
        if (target > cur) cur += kTwoPi;
        else cur -= kTwoPi;
    }
    return NormaliseAngle(MakeValueCloseTo(cur, target, factor, minStep, maxStep));
}

float MakeValueCloseTo_Constant(float cur, float target, float step) {
    if (std::fabs(cur - target) <= step) return target;
    return target > cur ? cur + step : cur - step;
}

float MakeAngleCloseTo_Constant(float cur, float target, float step) {
    cur = NormaliseAngle(cur);
    target = NormaliseAngle(target);
    if (std::fabs(cur - target) > 3.14159f) cur += target > cur ? 6.28319f : -6.28319f;
    return NormaliseAngle(MakeValueCloseTo_Constant(cur, target, step));
}

float FindYaw(float a, float b) { return NormaliseAngle(std::atan2(a, b)); }

float Interpolate(float a, float b, float t) { return (b - a) * t + a; }

D3DVECTOR D3DVectorInterpolate(const D3DVECTOR& a, const D3DVECTOR& b, float t) {
    return D3DVECTOR(Interpolate(a.x, b.x, t), Interpolate(a.y, b.y, t), Interpolate(a.z, b.z, t));
}

float FindPitch(const D3DVECTOR& from, const D3DVECTOR& to) {
    float dx = to.x - from.x, dz = to.z - from.z;
    float d = std::sqrt(dx * dx + dz * dz);  // ExactDistanceXZ
    float dy = to.y - from.y;
    if (d == 0.0f) return dy >= 0.0f ? 1.5708f : -1.5708f;
    return std::atan(dy / d);
}

float ApproxDistance(const D3DVECTOR& a, const D3DVECTOR& b) {
    float dx = std::fabs(b.x - a.x), dy = std::fabs(b.y - a.y), dz = std::fabs(b.z - a.z);
    float d = dx + dy + dz;
    if (d > 0.0f) {
        float sq = dx * dx + dy * dy + dz * dz;
        d = (sq / d + d) * 0.5f;
        d = (sq / d + d) * 0.5f;
    }
    return d;
}

float UnitRand() { return (float)(std::rand() % 1000) / 1000.0f; }

float RandomF(float lo, float hi) { return UnitRand() * (hi - lo) + lo; }

D3DVECTOR RandomPosXZ(const D3DVECTOR& centre, float radius) {
    float angle = UnitRand() * 6.28319f;
    float dist = UnitRand() * radius;
    return D3DVECTOR(std::sin(angle) * dist + centre.x, centre.y, std::cos(angle) * dist + centre.z);
}
