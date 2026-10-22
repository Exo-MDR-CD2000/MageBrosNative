#include "game/XParticle.h"
#include "framework/d3dutil.h"
#include "game/XMapCollision.h"
#include "game/XMath.h"
#include <cmath>
#include <cstdlib>

namespace {
// textures
constexpr uint16_t kTexSmoke = 0x198, kTexExplode = 0x190;
constexpr uint16_t kTexHeroSmokeA = 0x15d, kTexHeroSmokeB = 0x185;
constexpr uint16_t kTexHeroFlashA = 0x194, kTexHeroFlashB = 0x195;
constexpr uint16_t kTexShotA = 0x15c, kTexShotB = 0x184;
constexpr uint16_t kTexTorchFire1 = 0x187, kTexTorchFire2 = 0x188;
constexpr uint16_t kTexWarpRing = 0x1a5, kTexCoin = 0x294, kTexLand = 0x295, kTexShadow = 0xa3;
constexpr uint16_t kTexBulletExplode = 0x192;
constexpr uint16_t kSoundBulletExplode = 0x279;
constexpr float kDegree = 0.0174533f;

// ---------------------------------------------------------------------------------- the kinds

// XParticle_Smoke: a grey puff that grows and fades (bullets, rockets)
class Smoke : public XParticleBase {
public:
    Smoke(XParticleList* l, const D3DVECTOR& p) : XParticleBase(l, p) {
        particle_ = std::make_unique<JObject3DRect>(JObject3DRect::Particle(1, 1));
        particle_->Scale(10);
        particle_->SetTexture(kTexSmoke);
        state_ = NORMAL;
    }
    void ProcessFrame() override {
        XParticleBase::ProcessFrame();
        if (state_ != NORMAL) return;
        alpha_ -= 0.015f;
        scaleIncr_ *= 0.9f;
        if (alpha_ > 0.05f) {
            particle_->Scale(1.0f + scaleIncr_);
            particle_->SetAlphaFactor(alpha_);
        } else {
            state_ = NON;
        }
    }

private:
    float scaleIncr_ = 0.12f, alpha_ = 1.0f;
};

// XParticle_EnemyExplode: a 16-frame explosion (4 x 4 sheet), 6 ticks a frame
class EnemyExplode : public XParticleBase {
public:
    EnemyExplode(XParticleList* l, const D3DVECTOR& p, bool large) : XParticleBase(l, p) {
        particle_ = std::make_unique<JObject3DRect>(JObject3DRect::Particle(1, 1));
        particle_->SetTexture(kTexExplode);
        particle_->Scale(large ? 180.0f : 100.0f);
        particle_->SetUV(0, 0.25f, 0, 0.25f, 0);
        state_ = NORMAL;
    }
    void ProcessFrame() override {
        XParticleBase::ProcessFrame();
        if (state_ != NORMAL) return;
        int frame = (int)((localCounter_ / 6) % 16);
        float u = (frame % 4) * 0.25f, v = (frame / 4) * 0.25f;
        particle_->SetUV(u, u + 0.25f, v, v + 0.25f, 0);
        if (localCounter_ >= 96) state_ = NON;
    }
};

// XParticle_BulletExplode: where a bullet hits; a 16-frame explosion (4 x 4 sheet) 60 across, 4
// ticks a frame, with sound 0x279
class BulletExplode : public XParticleBase {
public:
    BulletExplode(XParticleList* l, const D3DVECTOR& p) : XParticleBase(l, p) {
        particle_ = std::make_unique<JObject3DRect>(JObject3DRect::Particle(1, 1));
        particle_->SetTexture(kTexBulletExplode);
        particle_->Scale(60.0f);
        particle_->SetUV(0, 0.25f, 0, 0.25f, 0);
        state_ = NORMAL;
        l->PlaySound(kSoundBulletExplode, pos_.z);
    }
    void ProcessFrame() override {
        XParticleBase::ProcessFrame();
        if (state_ != NORMAL) return;
        int frame = (int)((localCounter_ / 4) % 16);
        float u = (frame % 4) * 0.25f, v = (frame / 4) * 0.25f;
        particle_->SetUV(u, u + 0.25f, v, v + 0.25f, 0);
        if (localCounter_ >= 64) state_ = NON;
    }
};

// XParticle_HeroFireSmoke: the trail behind a shot
class HeroFireSmoke : public XParticleBase {
public:
    HeroFireSmoke(XParticleList* l, const D3DVECTOR& p, bool heroA, bool large) : XParticleBase(l, p) {
        particle_ = std::make_unique<JObject3DRect>(JObject3DRect::Particle(1, 1));
        particle_->Scale(17);
        if (!large) particle_->Scale(0.8f);
        particle_->SetTexture(heroA ? kTexHeroSmokeA : kTexHeroSmokeB);
        state_ = NORMAL;
    }
    void ProcessFrame() override {
        XParticleBase::ProcessFrame();
        if (state_ != NORMAL) return;
        alpha_ -= 0.05f;
        if (alpha_ > 0.05f) {
            particle_->Scale(0.97f);
            particle_->SetAlphaFactor(alpha_);
        } else {
            state_ = NON;
        }
    }

private:
    float alpha_ = 1.0f;
};

// XParticle_HeroFireFlash: where a shot hits a wall; an upright 3 x 3 sheet facing the camera
// side (turned -90 degrees about Z), a random cell every tick
class HeroFireFlash : public XParticleBase {
public:
    HeroFireFlash(XParticleList* l, const D3DVECTOR& p, bool heroA, bool large) : XParticleBase(l, p) {
        particle_ = std::make_unique<JObject3DRect>(1.5f, 1.0f);
        particle_->Rotate(2, -1.5708f);
        particle_->UsePerspective(true);
        particle_->SetAlphaState(JObject3DRect::ALPHA_PARTICLE);
        particle_->SetFilterState(JObject3DRect::FILTER_LINEAR);
        particle_->Scale(70);
        if (!large) particle_->Scale(0.9f);
        particle_->SetTexture(heroA ? kTexHeroFlashA : kTexHeroFlashB);
        SetTextureRandomly();
        billboard_ = false;
        state_ = NORMAL;
    }
    void ProcessFrame() override {
        XParticleBase::ProcessFrame();
        if (state_ != NORMAL) return;
        alpha_ -= 0.06f;
        if (alpha_ > 0.05f) {
            particle_->Scale(0.94f);
            particle_->SetAlphaFactor(alpha_);
            SetTextureRandomly();
        } else {
            state_ = NON;
        }
    }

private:
    void SetTextureRandomly() {
        int r = (int)Random(9);
        float u = (r % 3) / 3.0f, v = (r / 3) / 3.0f;
        const float third = 0.333333f;
        particle_->SetUV(u, r % 3 == 2 ? 1.0f : u + third, v, r / 3 == 2 ? 1.0f : v + third, 0);
    }
    float alpha_ = 1.0f;
};

// XParticle_HeroWarpPhoton: lights circling the warping hero for 6 seconds, spiralling in over
// the first 3, each leaving a smoke trail
class HeroWarpPhoton : public XParticleBase {
public:
    HeroWarpPhoton(XParticleList* l, bool heroA) : XParticleBase(l, D3DVECTOR(0, 0, 0)), heroA_(heroA) {
        particle_ = std::make_unique<JObject3DRect>(JObject3DRect::Particle(1, 1));
        particle_->Scale(18);
        particle_->SetTexture(heroA ? kTexShotA : kTexShotB);
        heightOffset_ = 40.0f * UnitRand() + 0.0f;
        distFactor_ = RandomF(0.9f, 1.2f);
        distToHero_ = 70.0f * distFactor_;
        rotSpeedFactor_ = RandomF(0.6f, 1.0f);
        rotSpeed_ = 2.0f * rotSpeedFactor_;
        UpdatePosition();
        state_ = NORMAL;
    }
    void ProcessFrame() override {
        XParticleBase::ProcessFrame();
        if (state_ != NORMAL) return;
        if (localCounter_ < 300) {
            float s = std::sin((float)localCounter_ / 300.0f * 1.5708f);
            distToHero_ = (1.0f - s) * 48.0f + 22.0f;
            rotSpeed_ = 6.0f * s + 2.0f;
        } else {
            distToHero_ = 22.0f;
            rotSpeed_ = 8.0f;
        }
        distToHero_ *= distFactor_;
        rotSpeed_ *= rotSpeedFactor_;
        anglePos_ = NormaliseAngle(anglePos_ - kDegree * rotSpeed_);
        UpdatePosition();
        if (localCounter_ < 100) alpha_ = (float)localCounter_ / 100.0f;
        else if (localCounter_ >= 500) alpha_ = (float)(600 - localCounter_) / 100.0f;
        else alpha_ = 1.0f;
        alpha_ *= 0.5f;
        particle_->SetAlphaFactor(alpha_);
        if (localCounter_ % 5 == 0) list_->CreateHeroFireSmoke(pos_, heroA_, true);
        if (localCounter_ >= 600) state_ = NON;
    }

private:
    void UpdatePosition() {
        D3DVECTOR h = list_->HeroPos(heroA_);
        pos_.x = std::sin(anglePos_) * distToHero_ + h.x;
        pos_.z = std::cos(anglePos_) * distToHero_ + h.z;
        pos_.y = h.y + heightOffset_;
    }
    bool heroA_;
    float anglePos_ = 0, heightOffset_ = 0, distToHero_ = 0, distFactor_ = 0;
    float rotSpeed_ = 0, rotSpeedFactor_ = 0, alpha_ = 0;
};

// XParticle_TorchFire1: a lit torch's flame, flickering in size every 2 ticks (it stays)
class TorchFire1 : public XParticleBase {
public:
    TorchFire1(XParticleList* l, const D3DVECTOR& p) : XParticleBase(l, p) {
        particle_ = std::make_unique<JObject3DRect>(JObject3DRect::Particle(1, 1));
        particle_->SetTexture(kTexTorchFire1);
        SetRandomSize();
        state_ = NORMAL;
    }
    void ProcessFrame() override {
        XParticleBase::ProcessFrame();
        if (state_ == NORMAL && localCounter_ % 2 == 0) SetRandomSize();
    }

private:
    void SetRandomSize() {
        particle_->Scale(1.0f / prevScale_);
        float s = RandomF(0.9f, 1.1f) * 110.0f;
        particle_->Scale(s);
        prevScale_ = s;
    }
    float prevScale_ = 1.0f;
};

// XParticle_TorchFire2: a spark rising from a lit torch
class TorchFire2 : public XParticleBase {
public:
    TorchFire2(XParticleList* l, const D3DVECTOR& p) : XParticleBase(l, p) {
        particle_ = std::make_unique<JObject3DRect>(JObject3DRect::Particle(1, 1));
        particle_->SetTexture(kTexTorchFire2);
        particle_->Scale(34);
        state_ = NORMAL;
    }
    void ProcessFrame() override {
        XParticleBase::ProcessFrame();
        if (state_ != NORMAL) return;
        pos_.y += 1.6f;
        alpha_ -= 0.04f;
        if (alpha_ > 0.05f) {
            particle_->Scale(0.96f);
            particle_->SetAlphaFactor(alpha_);
        } else {
            state_ = NON;
        }
    }

private:
    float alpha_ = 1.0f;
};

// XParticle_WarpRing / XParticle_LandEffect: a flat ring spreading on the floor
class FlatRing : public XParticleBase {
public:
    FlatRing(XParticleList* l, const D3DVECTOR& p, uint16_t texture, float scale, float scaleIncr, float incrDecay,
             unsigned fadeAfter, float fade)
        : XParticleBase(l, p), scale_(scale), scaleIncr_(scaleIncr), incrDecay_(incrDecay), fadeAfter_(fadeAfter),
          fade_(fade) {
        particle_ = std::make_unique<JObject3DRect>(1.0f, 1.0f);
        particle_->UsePerspective(true);
        particle_->SetAlphaState(JObject3DRect::ALPHA_PARTICLE);
        particle_->SetFilterState(JObject3DRect::FILTER_LINEAR);
        particle_->SetTexture(texture);
        particle_->SetScaleVector(scale_);
        billboard_ = false;
        state_ = NORMAL;
    }
    void ProcessFrame() override {
        XParticleBase::ProcessFrame();
        if (state_ != NORMAL) return;
        scale_ += scaleIncr_;
        scaleIncr_ *= incrDecay_;
        particle_->SetScaleVector(scale_);
        if (localCounter_ > fadeAfter_) alpha_ *= fade_;
        if (alpha_ > 0.03f) particle_->SetAlphaFactor(alpha_);
        else state_ = NON;
    }

private:
    float scale_, scaleIncr_, incrDecay_;
    unsigned fadeAfter_;
    float fade_, alpha_ = 1.0f;
};

// XParticle_GetCoinParticle: one of three sparks flying up and out from a collected coin
class GetCoinParticle : public XParticleBase {
public:
    GetCoinParticle(XParticleList* l, const D3DVECTOR& p, const D3DVECTOR& dir) : XParticleBase(l, p) {
        particle_ = std::make_unique<JObject3DRect>(JObject3DRect::Particle(1, 1));
        particle_->Scale(18);
        particle_->SetTexture(kTexCoin);
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        dir_ = D3DVECTOR(dir.x / len, dir.y / len, dir.z / len);  // Normalize
        state_ = NORMAL;
    }
    void ProcessFrame() override {
        XParticleBase::ProcessFrame();
        if (state_ != NORMAL) return;
        pos_.x += dir_.x * 1.6f;
        pos_.y += dir_.y * 1.6f;
        pos_.z += dir_.z * 1.6f;
        if (localCounter_ > 20) alpha_ *= 0.9f;
        if (alpha_ > 0.03f) particle_->SetAlphaFactor(alpha_);
        else state_ = NON;
    }

private:
    D3DVECTOR dir_;
    float alpha_ = 1.0f;
};
}  // namespace

// ------------------------------------------------------------------------------- XParticleBase

void XParticleBase::ProcessFrame() {
    if (state_ != NON) localCounter_++;
}

void XParticleBase::Render(Device* d, const D3DMATRIX& view) {
    if (state_ == NON || !particle_) return;
    particle_->SetWorldPos(pos_);
    if (billboard_) {
        particle_->Render2(d, view);
    } else {
        particle_->UpdateWorldMatrix();
        particle_->Render(d);
    }
}

// ------------------------------------------------------------------------------- XParticleList

XParticleList::XParticleList() = default;
XParticleList::~XParticleList() = default;

void XParticleList::Empty() {
    for (auto& s : slots_) s.reset();
    arrayPointer_ = 0;
}

int XParticleList::GetNullIndex() {
    // from the rotating index onwards, wrapping once; -1 when every slot is busy
    int start = arrayPointer_;
    for (;;) {
        int found = slots_[arrayPointer_] ? -1 : arrayPointer_;
        arrayPointer_ = (arrayPointer_ + 1) % kSlots;
        if (found >= 0) return found;
        if (arrayPointer_ == start) return -1;
    }
}

void XParticleList::Add(std::unique_ptr<XParticleBase> p) {
    int i = GetNullIndex();
    if (i >= 0) slots_[i] = std::move(p);
}

void XParticleList::ProcessFrame() {
    for (auto& s : slots_) {
        if (!s) continue;
        s->ProcessFrame();
        if (s->Finished()) s.reset();
    }
}

void XParticleList::Render(Device* d, const D3DMATRIX& view) {
    for (auto& s : slots_)
        if (s) s->Render(d, view);
}

void XParticleList::CreateSmoke(const D3DVECTOR& pos) { Add(std::make_unique<Smoke>(this, pos)); }
void XParticleList::CreateBulletExplode(const D3DVECTOR& pos) {
    // (the sound plays as the particle is made, so not when every slot is busy, as the original)
    int i = GetNullIndex();
    if (i >= 0) slots_[i] = std::make_unique<BulletExplode>(this, pos);
}
void XParticleList::CreateEnemyExplode(const D3DVECTOR& pos, bool large) {
    Add(std::make_unique<EnemyExplode>(this, pos, large));
}
void XParticleList::CreateHeroFireSmoke(const D3DVECTOR& pos, bool heroA, bool large) {
    Add(std::make_unique<HeroFireSmoke>(this, pos, heroA, large));
}
void XParticleList::CreateHeroFireFlash(const D3DVECTOR& pos, bool heroA, bool large) {
    Add(std::make_unique<HeroFireFlash>(this, pos, heroA, large));
}
void XParticleList::CreateHeroWarpPhoton(bool heroA) { Add(std::make_unique<HeroWarpPhoton>(this, heroA)); }
void XParticleList::CreateTorchFire1(const D3DVECTOR& pos) { Add(std::make_unique<TorchFire1>(this, pos)); }
void XParticleList::CreateTorchFire2(const D3DVECTOR& pos) { Add(std::make_unique<TorchFire2>(this, pos)); }
void XParticleList::CreateWarpRing(const D3DVECTOR& pos) {
    // 15 units across, growing by 3 (x 0.985 a tick); fades (x 0.975) after 50 ticks
    Add(std::make_unique<FlatRing>(this, pos, kTexWarpRing, 15.0f, 3.0f, 0.985f, 50, 0.975f));
}
void XParticleList::CreateLandEffect(const D3DVECTOR& pos) {
    // 30 units across, growing by 3.5 (x 0.92 a tick); fades (x 0.92) after 10 ticks
    Add(std::make_unique<FlatRing>(this, pos, kTexLand, 30.0f, 3.5f, 0.92f, 10, 0.92f));
}
void XParticleList::CreateGetCoinEffect(const D3DVECTOR& pos) {
    // three sparks, 120 degrees apart, starting away from the camera, rising 3 for each 1 out
    float angle = -1.5708f;
    for (int i = 0; i < 3; i++) {
        Add(std::make_unique<GetCoinParticle>(this, pos, D3DVECTOR(std::sin(angle), 3.0f, std::cos(angle))));
        angle += 2.0944f;
    }
}

// ------------------------------------------------------------------------------- XShadowSimple

XShadowSimple::XShadowSimple(float size) {
    shadow_ = std::make_unique<JObject3DRect>(JObject3DRect::Shadow(size, size));
    shadow_->SetTexture(kTexShadow);
}

void XShadowSimple::SetScale(float s) { shadow_->SetScaleVector(s); }

void XShadowSimple::Render(Device* d) {
    shadow_->SetWorldPos(pos_);
    shadow_->UpdateWorldMatrix();
    shadow_->Render(d);
}

// ------------------------------------------------------------------------------------- XShadow

XShadow::XShadow(float size) : size_(size) {
    for (auto& p : parts_) {
        p = std::make_unique<JObject3DRect>(JObject3DRect::Shadow(size, size));
        p->SetTexture(kTexShadow);
    }
}

void XShadow::Render(Device* d, const MapCollision& map) {
    float half = size_ * 0.5f;
    float x0 = pos_.x - half, x1 = pos_.x + half, z0 = pos_.z - half, z1 = pos_.z + half;
    int mx0 = MapCollision::CellX(x0, true), mx1 = MapCollision::CellX(x1, false);
    int mz0 = MapCollision::CellZ(z0, true), mz1 = MapCollision::CellZ(z1, false);
    int f00 = map.GetFloorLevel(mx0, mz0), f10 = map.GetFloorLevel(mx1, mz0);
    int f01 = map.GetFloorLevel(mx0, mz1), f11 = map.GetFloorLevel(mx1, mz1);
    // split at the cell edge where the floors on either side differ
    float xs = x0, zs = z0;
    if (f00 != f10 || f01 != f11) xs = (mx0 + 1) * 40.0f;
    if (f00 != f01 || f10 != f11) zs = (mz0 + 1) * 40.0f;
    float fx = (xs - x0) / size_;  // share of the shadow before the X split
    float fz = (z1 - zs) / size_;  // share after the Z split (v runs from +Z)
    float top = pos_.y + 0.4f;
    struct Part {
        int floor;
        float xa, xb, za, zb, u0, u1, v0, v1, sx, sz;
    };
    const Part parts[4] = {
        {f00, x0, xs, z0, zs, 0, fx, fz, 1, fx, 1 - fz},
        {f10, xs, x1, z0, zs, fx, 1, fz, 1, 1 - fx, 1 - fz},
        {f01, x0, xs, zs, z1, 0, fx, 0, fz, fx, fz},
        {f11, xs, x1, zs, z1, fx, 1, 0, fz, 1 - fx, fz},
    };
    for (int i = 0; i < 4; i++) {
        const Part& p = parts[i];
        float h = p.floor * 25.0f + 0.2f;
        if (!(h > 0.0f && top > h)) continue;  // no floor here, or the floor is above the owner
        JObject3DRect& r = *parts_[i];
        r.SetWorldPos(D3DVECTOR((p.xb - p.xa) * 0.5f + p.xa, h, (p.zb - p.za) * 0.5f + p.za));
        r.SetUV(p.u0, p.u1, p.v0, p.v1, 0);
        r.SetScaleVector(D3DVECTOR(p.sx, 1.0f, p.sz));
        r.UpdateWorldMatrix();
        r.Render(d);
    }
}
