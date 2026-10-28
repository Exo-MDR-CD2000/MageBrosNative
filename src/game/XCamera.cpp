#include "XCamera.h"
#include "framework/d3dutil.h"
#include "framework/interp.h"
#include "game/XMath.h"
#include "gfx/device.h"
#include <cmath>

void XCamera::ResetForGame(int rows) {
    distance_ = 340.0f;
    vAngle_ = 0.610865f;
    hAngle_ = 1.5707964f;
    localTime_ = 0;
    minZ_ = 160.0f;
    maxZ_ = (rows - 4) * 40.0f;
    lookAt_ = D3DVECTOR(140.0f, 120.0f, minZ_);
    eyeBase_ = lookAt_;
    UpdateLookAtPos(minZ_);
    UpdateViewMatrix();
    Snap();
}

void XCamera::ResetForTitle() {
    distance_ = 500.0f;
    vAngle_ = 0.0f;
    hAngle_ = 3.14159f;
    lookAt_ = D3DVECTOR(0, 0, 0);
    eyeBase_ = D3DVECTOR(0, 0, 0);
    UpdateViewMatrix();
    Snap();
    localTime_ = 0;  // (and its state to 1, which only the game's ProcessFrame reads)
}

void XCamera::ResetForEnding() {
    distance_ = 500.0f;
    vAngle_ = 0.0f;
    hAngle_ = 3.14159f;
    lookAt_ = D3DVECTOR(0.0f, -60.0f, 0.0f);
    eyeBase_ = D3DVECTOR(0, 0, 0);
    UpdateViewMatrix();
    Snap();
    localTime_ = 0;  // (and its state to 1, as ResetForTitle)
}

void XCamera::ProcessFrame(float zA, bool aliveA, float zB, bool aliveB) {
    // native (smooth motion): the view before this tick, to draw from
    Snap();
    blendTick_ = Interp::CurrentTick();
    localTime_++;
    if (aliveA && !aliveB) {
        UpdateLookAtPos(zA);
    } else if (!aliveA && aliveB) {
        UpdateLookAtPos(zB);
    } else if (aliveA && aliveB) {
        // the further hero's Z less half their gap, the gap counted up to 240
        float front = zB > zA ? zB : zA;
        float gap = std::fabs(zA - zB);
        if (gap > 240.0f) gap = 240.0f;
        UpdateLookAtPos(front - gap * 0.5f);
    }
    UpdateShake();
    UpdateViewMatrix();
}

// The look-at point's height is 120 + sin(tick mod 8 / 8 turns) x magnitude, the magnitude
// falling 0.02 a tick to 0. (Its last value stays when the shaking ends, within 0.02 of 120.)
void XCamera::UpdateShake() {
    if (shakeMagnitude_ <= 0) return;
    float t = (float)(localTime_ % 8) / 8.0f;
    lookAt_.y = 120.0f + std::sin(t * 6.28319f) * shakeMagnitude_;
    shakeMagnitude_ -= 0.02f;
    if (shakeMagnitude_ < 0) shakeMagnitude_ = 0;
}

void XCamera::UpdateLookAtPos(float z) {
    // the low limit is checked first, so a map too short for the range stays at minZ
    if (minZ_ > z) z = minZ_;
    else if (z > maxZ_) z = maxZ_;
    lookAt_.z = MakeValueCloseTo(lookAt_.z, z, 0.1f, 0.1f, 5.0f);
    eyeBase_.z = lookAt_.z;
}

void XCamera::UpdateViewMatrix() { BuildView(view_, eyeBase_, lookAt_); }

void XCamera::BuildView(D3DMATRIX& view, const D3DVECTOR& eyeBase, const D3DVECTOR& lookAt) const {
    float rise = std::sin(vAngle_) * distance_;
    float flat = std::cos(vAngle_) * distance_;
    D3DVECTOR eye(eyeBase.x + std::sin(hAngle_) * flat, eyeBase.y + rise, eyeBase.z + std::cos(hAngle_) * flat);
    D3DVECTOR up(0.0f, 1.0f, 0.0f);
    D3DUtil_SetViewMatrix(view, eye, lookAt, up);
}

void XCamera::Snap() {
    prevLookAt_ = lookAt_;
    prevEyeBase_ = eyeBase_;
}

D3DMATRIX XCamera::BlendedView() const {
    // (only between the latest two ticks: a camera that didn't move in the latest tick is drawn
    // where it is)
    float a = Interp::Alpha();
    if (a >= 1.0f || blendTick_ != Interp::CurrentTick()) return view_;
    D3DMATRIX view;
    BuildView(view, Interp::Lerp(prevEyeBase_, eyeBase_, a), Interp::Lerp(prevLookAt_, lookAt_, a));
    return view;
}

void XCamera::DoSetTransform(Device* d) const { d->SetTransform(D3DTRANSFORMSTATE_VIEW, &view_); }
