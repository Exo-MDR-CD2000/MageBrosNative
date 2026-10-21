#include "XCamera.h"
#include "framework/d3dutil.h"
#include "game/XMath.h"
#include "gfx/device.h"
#include <cmath>

void XCamera::ResetForGame(int rows) {
    distance_ = 340.0f;
    vAngle_ = 0.610865f;
    hAngle_ = 1.5707964f;
    minZ_ = 160.0f;
    maxZ_ = (rows - 4) * 40.0f;
    lookAt_ = D3DVECTOR(140.0f, 120.0f, minZ_);
    eyeBase_ = lookAt_;
    UpdateLookAtPos(minZ_);
    UpdateViewMatrix();
}

void XCamera::ProcessFrame(float heroZ) {
    UpdateLookAtPos(heroZ);
    UpdateViewMatrix();
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
