// XCamera (XCamera.cpp of the original): the in-game camera.
//
// It looks at a point that slides along the level (Z) following the heroes, from a fixed
// distance and angles. See docs/REVERSE_ENGINEERING.md, "Level start, camera and music".
// Not yet reconstructed: the title/ending set-ups and the vertical-angle controls.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "core/d3dtypes.h"

class Device;

class XCamera {
public:
    // XCamera::ResetForGame, for a map `rows` long.
    void ResetForGame(int rows);
    // XCamera::ProcessFrame (game state): count the tick, ease the look-at point toward the heroes
    // that are alive (not NON, DEAD or COMPLETED), then shake. With one alive it follows that one;
    // with both, their midpoint, but never more than 120 behind the one further along.
    void ProcessFrame(float zA, bool aliveA, float zB, bool aliveB);
    // XCamera::StartShaking: bob the look-at point up and down by `magnitude`, fading out
    void StartShaking(float magnitude) { shakeMagnitude_ = magnitude; }
    void DoSetTransform(Device* d) const;  // XCamera::DoSetTransform: the view matrix
    const D3DVECTOR& LookAt() const { return lookAt_; }
    const D3DMATRIX& View() const { return view_; }

private:
    void UpdateLookAtPos(float z);  // _UpdateLookAtPos
    void UpdateShake();             // _UpdateShake
    void UpdateViewMatrix();        // _UpdateViewMatrix

    unsigned localTime_ = 0;      // +0x0c

    D3DVECTOR lookAt_{0, 0, 0};   // +0x10: where the camera looks
    D3DVECTOR eyeBase_{0, 0, 0};  // +0x1c: the point the eye is offset from
    float distance_ = 340.0f;     // +0x28
    float vAngle_ = 0.610865f;    // +0x2c: 35 degrees above the horizontal
    float hAngle_ = 1.5707964f;   // +0x30: pi/2, the camera side (+x)
    float minZ_ = 160.0f;         // +0x34
    float maxZ_ = 160.0f;         // +0x38: (rows - 4) * 40
    float shakeMagnitude_ = 0;    // +0x3c: set only by the constructor and StartShaking
    D3DMATRIX view_;
};
