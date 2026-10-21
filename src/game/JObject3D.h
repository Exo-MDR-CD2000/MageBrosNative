// The J library's lit-vertex objects (JObject3D.cpp of the original): JObject3DBase's transform,
// JObject3D's vertices and render states, and the rectangle kinds the game uses.
//
// A JObject3D_Rect is a w x h quad lying in the XZ plane, centred on the origin, texture v
// running from +Z (0) to -Z (1). Scale and Rotate change the vertices themselves; the transform
// (position, scale, yaw, pitch, roll) is applied when drawing. The game makes three kinds:
//   particle (JObject3D_Particle): turned upright (-90 degrees about X), drawn camera-facing with
//     Render2, blended source-alpha / destination-alpha (additive on the alpha-less back buffer);
//   shadow (JObject3D_Shadow): flat, multiplied onto the scene (blend zero / source colour);
//   plain rects, for the flat effects (rings, flashes), blended like particles.
// Every vertex starts white and opaque; SetAlphaFactor fades it.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "core/d3dtypes.h"
#include <cstdint>

class Device;

class JObject3DRect {
public:
    enum AlphaState { ALPHA_NONE = 0, ALPHA_PARTICLE = 1, ALPHA_SHADOW = 2, ALPHA_TRANSPARENT = 3 };
    enum FilterState { FILTER_NONE = 0, FILTER_POINT = 1, FILTER_LINEAR = 2 };

    JObject3DRect(float width, float height);           // JObject3D_Rect
    static JObject3DRect Particle(float width, float height);  // JObject3D_Particle
    static JObject3DRect Shadow(float width, float height);    // JObject3D_Shadow

    // JObject3D: vertex edits
    void Scale(float sx, float sy, float sz);
    void Scale(float s) { Scale(s, s, s); }
    void Rotate(int axis, float angle);  // 0 = X, 1 = Y, 2 = Z
    // u from u0 to u1 left to right, v from v0 to v1 front (+Z) to back, each pulled in by `inset`
    void SetUV(float u0, float u1, float v0, float v1, float inset);
    void SetColor(int r, int g, int b, int a);
    void SetAlpha(int a);
    void SetAlphaFactor(float f);  // 0..1
    void SetTexture(uint16_t resId);
    void SetAlphaState(AlphaState s) { alphaState_ = s; }
    void SetFilterState(FilterState s) { filterState_ = s; }
    void UsePerspective(bool p) { usePerspective_ = p; }

    // JObject3DBase: the transform
    void SetWorldPos(const D3DVECTOR& p) { worldPos_ = p; }
    void SetScaleVector(const D3DVECTOR& s) { scale_ = s; }
    void SetScaleVector(float s) { scale_ = D3DVECTOR(s, s, s); }
    void UpdateWorldMatrix();  // scale * roll * pitch * yaw * translation

    // JObject3D::Render with the world matrix; JObject3D_Rect::Render2 facing the camera (the
    // inverse of `view`, moved to the world position)
    void Render(Device* d) const;
    void Render2(Device* d, const D3DMATRIX& view) const;

private:
    void Draw(Device* d, const D3DMATRIX& world) const;

    D3DLVERTEX v_[4];
    uint16_t texture_ = 0;
    AlphaState alphaState_ = ALPHA_NONE;
    FilterState filterState_ = FILTER_NONE;
    bool usePerspective_ = false;
    D3DVECTOR worldPos_{0, 0, 0}, scale_{1, 1, 1};
    float yaw_ = 0, pitch_ = 0, roll_ = 0;
    D3DMATRIX world_;
};
