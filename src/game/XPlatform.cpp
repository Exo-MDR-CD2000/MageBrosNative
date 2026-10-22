#include "game/XPlatform.h"
#include "game/JModel.h"
#include "game/JObject3D.h"
#include "game/XGameContext.h"
#include "game/XHero.h"
#include "game/XStaticObj.h"
#include <algorithm>

namespace {
constexpr uint16_t kModelPlatform = 0x183;
constexpr uint16_t kModelElevator = 0x181, kModelElevatorBase = 0x182;
constexpr uint16_t kModelElevatorUp = 0x275, kModelElevatorUpBase = 0x274;
constexpr uint16_t kSoundElevator = 0x273;
constexpr uint16_t kTexShadow = 0xa3;
constexpr float kHalfPi = 1.5708f;

// The plain platform: model 0x183 turned a quarter, a 40 x 40 box 18 tall.
void MakePlainPlatform(std::unique_ptr<JObject3DX>& model, std::unique_ptr<XCollideBox>& box,
                       XShadowOnPlatform& shadow, const D3DVECTOR& at) {
    model = std::make_unique<JObject3DX>(kModelPlatform);
    model->SetYaw(kHalfPi);
    model->SetPos(at);
    model->UpdateWorldMatrix();
    box = std::make_unique<XCollideBox>(40.0f, 18.0f);
    box->SetPos(at);
    shadow.SetPlatformBoxPtr(box.get());
}

// XPlatform_Elevator ('A'; 'B' goes up only): a 40 x 40 pillar 200 tall that starts 170 below its
// cell and rises (0.5 a tick, sound 0x273 every 20) while a hero stands on it. At the top, or
// when nobody stands on it, the plain one falls back (0.08 a tick, at most 4) to the bottom; the
// up-only one stops where it is. Its base (model 0x182 / 0x274) stays at the cell.
class Elevator : public XPlatformBase {
public:
    enum MoveState { NON = 0, WAIT = 1, MOVEUP = 2, MOVEDOWN = 3 };
    Elevator(const XGameContext* g, int oid, const D3DVECTOR& p, bool upOnly) : XPlatformBase(g, oid, p), upOnly_(upOnly) {
        model_ = std::make_unique<JObject3DX>(upOnly_ ? kModelElevatorUp : kModelElevator);
        model_->SetYaw(kHalfPi);
        model_->SetPos(startPos_);
        model_->UpdateWorldMatrix();
        collideBox_ = std::make_unique<XCollideBox>(40.0f, 200.0f);
        collideBox_->SetPos(startPos_);
        shadowOnPlatform_->SetPlatformBoxPtr(collideBox_.get());
        baseModel_ = std::make_unique<JObject3DX>(upOnly_ ? kModelElevatorUpBase : kModelElevatorBase);
        baseModel_->SetYaw(kHalfPi);
        baseModel_->SetPos(startPos_);
        baseModel_->UpdateWorldMatrix();
        minPosY_ = pos_.y - 170.0f;
        SetPosY(minPosY_);
        UpdateWorldMatrix();
        moveState_ = WAIT;
    }
    void ProcessFrame() override {
        localCounter_++;
        if (moveState_ == WAIT) Wait();
        if (moveState_ == MOVEUP) MoveUp();
        if (moveState_ == MOVEDOWN) MoveDown();
        UpdateWorldMatrix();
        heroAAbove_ = heroBAbove_ = false;
    }
    void Render(Device* d) override;

private:
    void Wait() {  // _ProcessFrame_Wait
        if (heroAAbove_ || heroBAbove_) {
            localCounter_ = 0;
            moveState_ = MOVEUP;
        }
    }
    void MoveUp() {  // _ProcessFrame_MoveUp
        float y = pos_.y + 0.5f;
        if (y > startPos_.y) {
            SetPosY(startPos_.y);
        } else {
            if (localCounter_ % 20 == 1) game_->playSound(kSoundElevator, pos_.z);
            SetPosY(y);
        }
        CollisionCheckY_Hero();
        if (y > startPos_.y || (!heroAAbove_ && !heroBAbove_)) {
            localCounter_ = 0;
            moveState_ = upOnly_ ? WAIT : MOVEDOWN;
        }
    }
    void MoveDown() {  // _ProcessFrame_MoveDown
        if (localCounter_ == 1) verticalSpeed_ = 0;
        verticalSpeed_ -= 0.08f;
        if (verticalSpeed_ < -4.0f) verticalSpeed_ = -4.0f;
        float y = pos_.y + verticalSpeed_;
        if (minPosY_ > y) verticalSpeed_ = minPosY_ - pos_.y;
        SetPosY(pos_.y + verticalSpeed_);
        PushHeroY(verticalSpeed_);
        if (minPosY_ > y) {
            localCounter_ = 0;
            moveState_ = WAIT;
        }
    }

    MoveState moveState_ = NON;
    std::unique_ptr<JObject3DX> baseModel_;
    bool upOnly_;
    float verticalSpeed_ = 0, minPosY_ = 0;
};

// XPlatform_AutoZ ('C'): shuttles along Z at 0.35 + 0.05 per world, turning back where the
// floor's edge stops it, carrying the heroes on it.
class AutoZ : public XPlatformBase {
public:
    AutoZ(const XGameContext* g, int oid, const D3DVECTOR& p) : XPlatformBase(g, oid, p) {
        MakePlainPlatform(model_, collideBox_, *shadowOnPlatform_, startPos_);
        moveSpeed_ = g->world * 0.05f + 0.35f;
    }
    void ProcessFrame() override {
        localCounter_++;
        float old = pos_.z;
        if (UpdatePosZ(increasingPos_ ? old + moveSpeed_ : old - moveSpeed_)) increasingPos_ = !increasingPos_;
        PushHeroZ(pos_.z - old);
        UpdateWorldMatrix();
        heroAAbove_ = heroBAbove_ = false;
    }

private:
    bool increasingPos_ = true;
};

// XPlatform_AutoX ('D'): shuttles along X at 0.35 the same way.
class AutoX : public XPlatformBase {
public:
    AutoX(const XGameContext* g, int oid, const D3DVECTOR& p) : XPlatformBase(g, oid, p) {
        MakePlainPlatform(model_, collideBox_, *shadowOnPlatform_, startPos_);
        moveSpeed_ = 0.35f;
    }
    void ProcessFrame() override {
        localCounter_++;
        float old = pos_.x;
        if (UpdatePosX(increasingPos_ ? old + moveSpeed_ : old - moveSpeed_)) increasingPos_ = !increasingPos_;
        PushHeroX(pos_.x - old);
        UpdateWorldMatrix();
        heroAAbove_ = heroBAbove_ = false;
    }

private:
    bool increasingPos_ = true;
};

// XPlatform_RightOnly ('E'): waits for a hero, then moves along +Z at 0.35 + 0.1 per world until
// the floor's edge stops it for good.
class RightOnly : public XPlatformBase {
public:
    enum MoveState { NON = 0, WAIT = 1, MOVE = 2, STOP = 3 };
    RightOnly(const XGameContext* g, int oid, const D3DVECTOR& p) : XPlatformBase(g, oid, p) {
        MakePlainPlatform(model_, collideBox_, *shadowOnPlatform_, startPos_);
        moveSpeed_ = g->world * 0.1f + 0.35f;
        moveState_ = WAIT;
    }
    void ProcessFrame() override {
        localCounter_++;
        if (moveState_ == WAIT) {
            if (heroAAbove_ || heroBAbove_) {
                localCounter_ = 0;
                moveState_ = MOVE;
            }
        } else if (moveState_ == MOVE) {
            float old = pos_.z;
            bool hit = UpdatePosZ(old + moveSpeed_);
            PushHeroZ(pos_.z - old);
            UpdateWorldMatrix();
            if (hit) {
                localCounter_ = 0;
                moveState_ = STOP;
            }
        }
        heroAAbove_ = heroBAbove_ = false;
    }

private:
    MoveState moveState_ = NON;
};

// XPlatform_Drop ('F'): 20 ticks after a hero first stands on it, it falls (0.08 a tick, at most
// 4) carrying the heroes down, and stops below -500. (Water levels hold it at -38 with a wave:
// comes with the water.)
class Drop : public XPlatformBase {
public:
    enum MoveState { NON = 0, WAIT = 1, DROP = 2 };
    Drop(const XGameContext* g, int oid, const D3DVECTOR& p) : XPlatformBase(g, oid, p) {
        MakePlainPlatform(model_, collideBox_, *shadowOnPlatform_, startPos_);
        moveState_ = WAIT;
    }
    void ProcessFrame() override {
        localCounter_++;
        if (moveState_ == WAIT) {
            if (heroAAbove_ || heroBAbove_) {
                localCounter_ = 0;
                moveState_ = DROP;
            }
        } else if (moveState_ == DROP && localCounter_ > 20) {
            verticalSpeed_ -= 0.08f;
            if (verticalSpeed_ < -4.0f) verticalSpeed_ = -4.0f;
            SetPosY(pos_.y + verticalSpeed_);
            PushHeroY(verticalSpeed_);
            UpdateWorldMatrix();
            if (pos_.y < -500.0f) {
                localCounter_ = 0;
                moveState_ = NON;
            }
        }
        heroAAbove_ = heroBAbove_ = false;
    }

private:
    MoveState moveState_ = NON;
    float verticalSpeed_ = 0;
};

// XPlatform_Stay ('G'): never moves.
class Stay : public XPlatformBase {
public:
    Stay(const XGameContext* g, int oid, const D3DVECTOR& p) : XPlatformBase(g, oid, p) {
        MakePlainPlatform(model_, collideBox_, *shadowOnPlatform_, startPos_);
    }
    void ProcessFrame() override { heroAAbove_ = heroBAbove_ = false; }
};

void Elevator::Render(Device* d) {
    if (!game_->IsVisible(pos_)) return;
    XPlatformBase::Render(d);
    baseModel_->Render(d);
}
}  // namespace

// --------------------------------------------------------------------------- XShadowOnPlatform

XShadowOnPlatform::XShadowOnPlatform(const XGameContext* game) : game_(game) {
    for (auto& s : shadow_) {
        s = std::make_unique<JObject3DRect>(JObject3DRect::Shadow(35.0f, 35.0f));
        s->SetTexture(kTexShadow);
    }
}
XShadowOnPlatform::~XShadowOnPlatform() = default;

void XShadowOnPlatform::Render(Device* d) {
    RenderShadow(d, game_->heroA, *shadow_[0]);
    // (player 2's shadow on shadow_[1]: stage 5)
}

void XShadowOnPlatform::RenderShadow(Device* d, const XHero* hero, JObject3DRect& shadow) {
    if (!hero || !platformBox_) return;
    if (hero->GetLiveState() != XHero::NORMAL && hero->GetLiveState() != XHero::DYING) return;
    const D3DVECTOR& hp = hero->GetPos();
    if (platformBox_->Node2().y > hp.y) return;  // the hero is below the top
    // the hero's 35-unit square and the platform's 40-unit top
    float hx0 = hp.x - 17.5f, hx1 = hp.x + 17.5f, hz0 = hp.z - 17.5f, hz1 = hp.z + 17.5f;
    const D3DVECTOR& bp = platformBox_->Pos();
    float px0 = bp.x - 20.0f, px1 = bp.x + 20.0f, pz0 = bp.z - 20.0f, pz1 = bp.z + 20.0f;
    if (px0 >= hx1 || hx0 >= px1 || pz0 >= hz1 || hz0 >= pz1) return;
    // the part of the shadow texture over the top
    float u0 = std::max(0.0f, (px0 - hx0) / 35.0f);
    float u1 = std::min(1.0f, 1.0f - (hx1 - px1) / 35.0f);
    float v0 = std::max(0.0f, (pz0 - hz0) / 35.0f);
    float v1 = std::min(1.0f, 1.0f - (hz1 - pz1) / 35.0f);
    float xa = std::max(px0, hx0), xb = std::min(px1, hx1);
    float za = std::max(pz0, hz0), zb = std::min(pz1, hz1);
    shadow.SetWorldPos(D3DVECTOR((xb - xa) * 0.5f + xa, platformBox_->Node2().y + 0.2f, (zb - za) * 0.5f + za));
    shadow.SetUV(u0, u1, 1.0f - v1, 1.0f - v0, 0);
    shadow.SetScaleVector(D3DVECTOR(u1 - u0, 1.0f, (1.0f - v0) - (1.0f - v1)));
    shadow.UpdateWorldMatrix();
    shadow.Render(d);
}

// ------------------------------------------------------------------------------- XPlatformBase

XPlatformBase::XPlatformBase(const XGameContext* game, int oid, const D3DVECTOR& pos)
    : game_(game), oid_(oid), startPos_(pos), pos_(pos), shadowOnPlatform_(std::make_unique<XShadowOnPlatform>(game)) {}
XPlatformBase::~XPlatformBase() = default;

void XPlatformBase::Render(Device* d) {
    if (!game_->IsVisible(pos_)) return;
    model_->Render(d);
    shadowOnPlatform_->Render(d);
}

void XPlatformBase::UpdateWorldMatrix() {
    model_->SetPos(pos_);
    model_->UpdateWorldMatrix();
}

void XPlatformBase::SetPosX(float x) {
    pos_.x = x;
    collideBox_->SetPosX(x);
}
void XPlatformBase::SetPosY(float y) {
    pos_.y = y;
    collideBox_->SetPosY(y);
}
void XPlatformBase::SetPosZ(float z) {
    pos_.z = z;
    collideBox_->SetPosZ(z);
}

bool XPlatformBase::UpdatePosX(float newX) {
    bool hit = game_->CollisionCheckX_Floor(newX, *collideBox_, floorLevel_);
    game_->platforms->CollisionCheckX(newX, *this);
    game_->statics->CollisionCheckX(newX, *collideBox_, true);
    if (game_->heroA) game_->heroA->CollisionCheckX(newX, *collideBox_);
    // (and player 2: stage 5)
    SetPosX(newX);
    return hit;
}

bool XPlatformBase::UpdatePosZ(float newZ) {
    bool hit = game_->CollisionCheckZ_Floor(newZ, *collideBox_, floorLevel_);
    game_->platforms->CollisionCheckZ(newZ, *this);
    game_->statics->CollisionCheckZ(newZ, *collideBox_, true);
    if (game_->heroA) game_->heroA->CollisionCheckZ(newZ, *collideBox_);
    SetPosZ(newZ);
    return hit;
}

void XPlatformBase::CollisionCheckY_Hero() {
    XHero* a = game_->heroA;
    if (a && a->GetLiveState() == XHero::NORMAL) {
        float y = a->GetPos().y;
        if (collideBox_->CollisionCheckY(y, a->GetCollideBox())) a->SetPosY(y);
        // (then player 2 out of player 1's box, and the same for player 2: stage 5)
    }
}

void XPlatformBase::PushHeroX(float dx) {
    if (heroAAbove_ && game_->heroA) game_->heroA->IncreasePushX(dx);
}
void XPlatformBase::PushHeroY(float dy) {
    // (a hero standing on the one being carried goes too: stage 5)
    if (heroAAbove_ && game_->heroA) game_->heroA->IncreasePushY(dy);
}
void XPlatformBase::PushHeroZ(float dz) {
    if (heroAAbove_ && game_->heroA) game_->heroA->IncreasePushZ(dz);
}

// ------------------------------------------------------------------------------- XPlatformList

XPlatformList::~XPlatformList() = default;

void XPlatformList::Empty() {
    for (auto& s : slots_) s.reset();
    lastCollidePlatform_ = nullptr;
}

int XPlatformList::GetNullIndex() const {
    for (int i = 0; i < kSlots; i++)
        if (!slots_[i]) return i;
    return -1;
}

XPlatformBase* XPlatformList::CreatePlatform(const D3DVECTOR& pos, char code) {
    int i = GetNullIndex();
    if (i < 0) return nullptr;
    std::unique_ptr<XPlatformBase> p;
    switch (code) {
        case 'A': p = std::make_unique<Elevator>(game_, lastOID_, pos, false); break;
        case 'B': p = std::make_unique<Elevator>(game_, lastOID_, pos, true); break;
        case 'C': p = std::make_unique<AutoZ>(game_, lastOID_, pos); break;
        case 'D': p = std::make_unique<AutoX>(game_, lastOID_, pos); break;
        case 'E': p = std::make_unique<RightOnly>(game_, lastOID_, pos); break;
        case 'F': p = std::make_unique<Drop>(game_, lastOID_, pos); break;
        case 'G': p = std::make_unique<Stay>(game_, lastOID_, pos); break;
        default: return nullptr;
    }
    lastOID_++;
    slots_[i] = std::move(p);
    return slots_[i].get();
}

void XPlatformList::ProcessFrame() {
    for (auto& s : slots_) {
        if (!s) break;
        s->ProcessFrame();
    }
}

void XPlatformList::Render(Device* d) {
    for (auto& s : slots_) {
        if (!s) break;
        s->Render(d);
    }
}

bool XPlatformList::CollisionCheckX(float& newX, const XCollideBox& box, bool ignoreY) const {
    bool hit = false;
    for (auto& s : slots_) {
        if (!s) break;
        if (s->GetCollideBox().CollisionCheckX(newX, box, ignoreY)) hit = true;
    }
    return hit;
}

bool XPlatformList::CollisionCheckZ(float& newZ, const XCollideBox& box, bool ignoreY) const {
    bool hit = false;
    for (auto& s : slots_) {
        if (!s) break;
        if (s->GetCollideBox().CollisionCheckZ(newZ, box, ignoreY)) hit = true;
    }
    return hit;
}

bool XPlatformList::CollisionCheckY(float& newY, const XCollideBox& box) {
    lastCollidePlatform_ = nullptr;
    bool hit = false;
    for (auto& s : slots_) {
        if (!s) break;
        if (s->GetCollideBox().CollisionCheckY(newY, box)) {
            lastCollidePlatform_ = s.get();
            hit = true;
        }
    }
    return hit;
}

bool XPlatformList::CollisionCheckX(float& newX, const XPlatformBase& mover) const {
    bool hit = false;
    for (auto& s : slots_) {
        if (!s) break;
        if (s->GetOID() == mover.GetOID()) continue;
        if (s->GetCollideBox().CollisionCheckX(newX, mover.GetCollideBox(), true)) hit = true;
    }
    return hit;
}

bool XPlatformList::CollisionCheckZ(float& newZ, const XPlatformBase& mover) const {
    bool hit = false;
    for (auto& s : slots_) {
        if (!s) break;
        if (s->GetOID() == mover.GetOID()) continue;
        if (s->GetCollideBox().CollisionCheckZ(newZ, mover.GetCollideBox(), true)) hit = true;
    }
    return hit;
}

bool XPlatformList::CollisionCheck(const D3DVECTOR& p) const {
    for (auto& s : slots_) {
        if (!s) break;
        if (s->GetCollideBox().CollisionCheck(p)) return true;
    }
    return false;
}
