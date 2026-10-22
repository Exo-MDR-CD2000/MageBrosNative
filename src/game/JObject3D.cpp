#include "game/JObject3D.h"
#include "framework/d3dtextr.h"
#include "framework/d3dutil.h"
#include "gfx/device.h"
#include <cstring>

namespace {
const WORD kIndices[6] = {0, 1, 3, 0, 3, 2};  // JObject3D_Rect's two triangles

void SetPos(D3DLVERTEX& v, float x, float y, float z) {  // SetLVertexFromVector: position only
    v.x = x;
    v.y = y;
    v.z = z;
}
}  // namespace

JObject3DRect::JObject3DRect(float width, float height) {
    std::memset(v_, 0, sizeof v_);
    float hw = width / 2.0f, hh = height / 2.0f;
    SetPos(v_[0], -hw, 0, hh);
    SetPos(v_[1], hw, 0, hh);
    SetPos(v_[2], -hw, 0, -hh);
    SetPos(v_[3], hw, 0, -hh);
    SetColor(0xff, 0xff, 0xff, 0xff);  // JObject3D's constructor
    SetUV(0, 1, 0, 1, 0);
    D3DUtil_SetIdentityMatrix(world_);
}

JObject3DRect JObject3DRect::Particle(float width, float height) {
    JObject3DRect r(width, height);
    r.Rotate(0, -1.5708f);  // upright, facing -Z (the camera, once turned by Render2)
    r.UsePerspective(false);
    r.SetAlphaState(ALPHA_PARTICLE);
    r.SetFilterState(FILTER_LINEAR);
    return r;
}

JObject3DRect JObject3DRect::Shadow(float width, float height) {
    JObject3DRect r(width, height);
    r.UsePerspective(true);
    r.SetAlphaState(ALPHA_SHADOW);
    r.SetFilterState(FILTER_LINEAR);
    return r;
}

void JObject3DRect::Scale(float sx, float sy, float sz) {
    for (auto& v : v_) SetPos(v, v.x * sx, v.y * sy, v.z * sz);
}

void JObject3DRect::Shift(const D3DVECTOR& d) {
    for (auto& v : v_) SetPos(v, v.x + d.x, v.y + d.y, v.z + d.z);
}

void JObject3DRect::Rotate(int axis, float angle) {
    D3DMATRIX m;
    if (axis == 0) D3DUtil_SetRotateXMatrix(m, angle);
    else if (axis == 1) D3DUtil_SetRotateYMatrix(m, angle);
    else D3DUtil_SetRotateZMatrix(m, angle);
    for (auto& v : v_) {
        D3DVECTOR in(v.x, v.y, v.z), out;
        D3DMath_VectorMatrixMultiply(out, in, m);
        SetPos(v, out.x, out.y, out.z);
    }
}

void JObject3DRect::SetUV(float u0, float u1, float v0, float v1, float inset) {
    v_[0].tu = u0 + inset, v_[0].tv = v0 + inset;
    v_[1].tu = u1 - inset, v_[1].tv = v0 + inset;
    v_[2].tu = u0 + inset, v_[2].tv = v1 - inset;
    v_[3].tu = u1 - inset, v_[3].tv = v1 - inset;
}

void JObject3DRect::SetColor(int r, int g, int b, int a) {
    D3DCOLOR c = ((DWORD)(a & 0xff) << 24) | ((DWORD)(r & 0xff) << 16) | ((DWORD)(g & 0xff) << 8) | (DWORD)(b & 0xff);
    for (auto& v : v_) v.color = c;
}

void JObject3DRect::SetAlpha(int a) {
    for (auto& v : v_) v.color = ((DWORD)(a & 0xff) << 24) | (v.color & 0xffffff);
}

void JObject3DRect::SetAlphaFactor(float f) {
    if (f < 0.0f) f = 0.0f;
    else if (f > 1.0f) f = 1.0f;
    SetAlpha((int)(f * 255.0f));
}

void JObject3DRect::SetTexture(uint16_t resId) {
    texture_ = resId;  // (made on first draw: the original creates every texture at start-up)
}

void JObject3DRect::UpdateWorldMatrix() {
    D3DMATRIX s, rz, rx, ry, t;
    D3DUtil_SetScaleMatrix(s, scale_.x, scale_.y, scale_.z);
    D3DUtil_SetRotateZMatrix(rz, roll_);
    D3DUtil_SetRotateXMatrix(rx, pitch_);
    D3DUtil_SetRotateYMatrix(ry, yaw_);
    D3DUtil_SetTranslateMatrix(t, worldPos_.x, worldPos_.y, worldPos_.z);
    D3DUtil_SetIdentityMatrix(world_);
    for (const D3DMATRIX* m : {&s, &rz, &rx, &ry, &t}) {
        D3DMATRIX r;
        D3DMath_MatrixMultiply(r, world_, *m);
        world_ = r;
    }
}

void JObject3DRect::Render(Device* d) const { Draw(d, world_); }

void JObject3DRect::Render2(Device* d, const D3DMATRIX& view) const {
    // JObject3D_Rect::_SetWorldTransform2: the camera's orientation, at the world position
    D3DMATRIX m;
    D3DMath_MatrixInvert(m, view);
    m._41 = worldPos_.x;
    m._42 = worldPos_.y;
    m._43 = worldPos_.z;
    Draw(d, m);
}

void JObject3DRect::Draw(Device* d, const D3DMATRIX& world) const {
    D3DMATRIX oldWorld;
    d->GetTransform(D3DTRANSFORMSTATE_WORLD, &oldWorld);
    d->SetTransform(D3DTRANSFORMSTATE_WORLD, &world);
    Texture* oldTex = nullptr;
    d->GetTexture(0, &oldTex);
    if (texture_) {
        Texture* t = D3DTextr_GetSurface(texture_);
        if (!t && SUCCEEDED(D3DTextr_CreateTextureFromResource(texture_, 0, 0))) t = D3DTextr_GetSurface(texture_);
        d->SetTexture(0, t);
    }

    // JObject3D::_SetRenderState: save what it changes, then set the object's states
    struct RS {
        D3DRENDERSTATETYPE s;
        DWORD old;
    };
    RS rs[] = {{D3DRENDERSTATE_LIGHTING, 0},     {D3DRENDERSTATE_TEXTUREPERSPECTIVE, 0},
               {D3DRENDERSTATE_ALPHABLENDENABLE, 0}, {D3DRENDERSTATE_SRCBLEND, 0},
               {D3DRENDERSTATE_DESTBLEND, 0},    {D3DRENDERSTATE_SPECULARENABLE, 0},
               {D3DRENDERSTATE_ZWRITEENABLE, 0}};
    for (auto& r : rs) d->GetRenderState(r.s, &r.old);
    struct TS {
        D3DTEXTURESTAGESTATETYPE s;
        DWORD old;
    };
    TS ts[] = {{D3DTSS_COLOROP, 0}, {D3DTSS_COLORARG1, 0}, {D3DTSS_COLORARG2, 0}, {D3DTSS_ALPHAOP, 0},
               {D3DTSS_ALPHAARG1, 0}, {D3DTSS_ALPHAARG2, 0}, {D3DTSS_MINFILTER, 0}, {D3DTSS_MAGFILTER, 0},
               {D3DTSS_MIPFILTER, 0}};
    for (auto& t : ts) d->GetTextureStageState(0, t.s, &t.old);

    d->SetRenderState(D3DRENDERSTATE_LIGHTING, 0);
    d->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, usePerspective_);
    auto modulate = [&] {
        d->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        d->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        d->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        d->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        d->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        d->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    };
    if (alphaState_ == ALPHA_PARTICLE) {
        d->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, 1);
        d->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
        d->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_DESTALPHA);
        d->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, 0);
        d->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, 0);
        modulate();
    } else if (alphaState_ == ALPHA_SHADOW) {
        d->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, 1);
        d->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ZERO);
        d->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_SRCCOLOR);
        d->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, 0);
        d->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, 0);
    } else if (alphaState_ == ALPHA_TRANSPARENT) {
        d->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, 1);
        d->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
        d->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
        d->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, 0);
        d->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, 0);
        modulate();
    }
    if (filterState_ == FILTER_POINT) {
        d->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFG_POINT);
        d->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_POINT);
        d->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_POINT);
    } else if (filterState_ == FILTER_LINEAR) {
        d->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFG_LINEAR);
        d->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
        d->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_LINEAR);
    }

    d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, D3DFVF_LVERTEX, v_, 4, kIndices, 6, 0);

    // _ResetRenderState
    for (auto& t : ts) d->SetTextureStageState(0, t.s, t.old);
    for (auto& r : rs) d->SetRenderState(r.s, r.old);
    d->SetTexture(0, oldTex);
    d->SetTransform(D3DTRANSFORMSTATE_WORLD, &oldWorld);
}
