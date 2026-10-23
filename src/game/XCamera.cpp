#include "XCamera.h"
#include "framework/d3dutil.h"
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
}

void XCamera::ProcessFrame(float heroZ, bool follow) {
    localTime_++;
    if (follow) UpdateLookAtPos(heroZ);
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

void XCamera::UpdateViewMatrix() {
    float rise = std::sin(vAngle_) * distance_;
    float flat = std::cos(vAngle_) * distance_;
    D3DVECTOR eye(eyeBase_.x + std::sin(hAngle_) * flat, eyeBase_.y + rise, eyeBase_.z + std::cos(hAngle_) * flat);
    D3DVECTOR up(0.0f, 1.0f, 0.0f);
    D3DUtil_SetViewMatrix(view_, eye, lookAt_, up);
}

void XCamera::DoSetTransform(Device* d) const { d->SetTransform(D3DTRANSFORMSTATE_VIEW, &view_); }
