#include "game/XWater.h"
#include "framework/interp.h"
#include "game/JObject3D.h"

namespace {
// water1: blended (alpha 0.4) at -38; water2: added (alpha 0.6) at -39. World 3 uses 0x28f / 0x290.
constexpr uint16_t kTexWater1 = 0x18e, kTexWater2 = 0x1a6;
constexpr uint16_t kTexWater1World3 = 0x28f, kTexWater2World3 = 0x290;
}  // namespace

XWater::XWater() {
    water1_ = std::make_unique<JObject3DRect>(1.0f, 1.0f);
    water1_->UsePerspective(true);
    water1_->SetFilterState(JObject3DRect::FILTER_LINEAR);
    water1_->SetAlphaState(JObject3DRect::ALPHA_TRANSPARENT);
    water1_->SetAlphaFactor(0.4f);
    water2_ = std::make_unique<JObject3DRect>(1.0f, 1.0f);
    water2_->UsePerspective(true);
    water2_->SetFilterState(JObject3DRect::FILTER_LINEAR);
    water2_->SetAlphaState(JObject3DRect::ALPHA_PARTICLE);
    water2_->SetAlphaFactor(0.6f);
}
XWater::~XWater() = default;

void XWater::Init(int cellsX, int cellsZ, int world) {
    // (XWater::XWater picks the textures by the world the game object is made for)
    water1_->SetTexture(world == 2 ? kTexWater1World3 : kTexWater1);
    water2_->SetTexture(world == 2 ? kTexWater2World3 : kTexWater2);
    cellsX_ = cellsX;
    cellsZ_ = cellsZ;
    float sx = cellsX * 40.0f, sz = cellsZ * 40.0f;
    // one texture repeat per cell, the sheet scaled over the map and centred on it
    for (auto* w : {water1_.get(), water2_.get()}) {
        w->SetUV(0, (float)cellsX, 0, (float)cellsZ, 0);
        w->SetScaleVector(D3DVECTOR(sx, 0.0f, sz));
    }
    water1_->SetWorldPos(D3DVECTOR(sx / 2.0f, -38.0f, sz / 2.0f));
    water1_->UpdateWorldMatrix();
    water2_->SetWorldPos(D3DVECTOR(sx / 2.0f, -39.0f, sz / 2.0f));
    water2_->UpdateWorldMatrix();
    waterU1_ = waterV1_ = waterU2_ = waterV2_ = 0;
    localCounter_ = 0;
    initialised_ = true;
}

void XWater::ProcessFrame() {
    if (!initialised_) return;
    // native (smooth motion): the drift before this tick, to draw from
    prevU1_ = waterU1_, prevV1_ = waterV1_, prevU2_ = waterU2_, prevV2_ = waterV2_;
    prevTick_ = Interp::CurrentTick();
    localCounter_++;
    // the textures drift: the top sheet twice as fast as the lower one
    auto drift = [](float& v, float step) {
        v += step;
        if (v >= 1.0f) v -= 1.0f;
    };
    drift(waterU1_, 0.0006f);
    drift(waterV1_, 0.0014f);
    drift(waterU2_, 0.0003f);
    drift(waterV2_, 0.0007f);
    water1_->SetUV(waterU1_, cellsX_ + waterU1_, waterV1_, cellsZ_ + waterV1_, 0);
    water2_->SetUV(waterU2_, cellsX_ + waterU2_, waterV2_, cellsZ_ + waterV2_, 0);
}

void XWater::Render(Device* d) {
    if (!initialised_) return;
    // native (smooth motion): drawn with the drift between the last two ticks, then the tick's put
    // back (the drift wraps at 1, so a smaller value has gone round once)
    float a = Interp::Alpha();
    bool blend = prevTick_ == Interp::CurrentTick() && a < 1.0f;
    if (blend) {
        auto mix = [a](float p, float c) { return p + (c < p ? c + 1.0f - p : c - p) * a; };
        float u1 = mix(prevU1_, waterU1_), v1 = mix(prevV1_, waterV1_);
        float u2 = mix(prevU2_, waterU2_), v2 = mix(prevV2_, waterV2_);
        water1_->SetUV(u1, cellsX_ + u1, v1, cellsZ_ + v1, 0);
        water2_->SetUV(u2, cellsX_ + u2, v2, cellsZ_ + v2, 0);
    }
    water2_->Render(d);
    water1_->Render(d);
    if (blend) {
        water1_->SetUV(waterU1_, cellsX_ + waterU1_, waterV1_, cellsZ_ + waterV1_, 0);
        water2_->SetUV(waterU2_, cellsX_ + waterU2_, waterV2_, cellsZ_ + waterV2_, 0);
    }
}
