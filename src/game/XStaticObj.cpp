#include "game/XStaticObj.h"
#include "core/log.h"
#include "game/JModel.h"
#include "game/JObject3D.h"
#include "game/XHero.h"
#include "game/XBullet.h"
#include "game/XGameContext.h"
#include "game/XMath.h"
#include "game/XParticle.h"
#include <cmath>

namespace {
// models
constexpr uint16_t kModelPillar = 0x170, kModelPillarWorld3 = 0x292, kModelPillarLow = 0x1ef;
constexpr uint16_t kModelPillarSmall = 0x1db, kModelBarrel = 0x1f6;
constexpr uint16_t kModelFenceStraight = 0x1d9, kModelFenceCorner = 0x1d8, kModelFenceT = 0x205;
constexpr uint16_t kModelCannon = 0x18c, kModelCannonRBarrel = 0x1f4, kModelCannonRBase = 0x1f3;
constexpr uint16_t kSoundCannon = 0x262;
constexpr uint16_t kModelShop = 0x1ab, kTexShopSign = 0x1ac;
constexpr uint16_t kModelTree = 0x1d6, kModelPlant = 0x1d7, kModelGenieLamp = 0x1ea;
constexpr uint16_t kModelPotLarge = 0x1d4, kModelPot = 0x1d5, kModelRocketLauncher = 0x197;
constexpr float kHalfPi = 1.5708f, kPi = 3.14159f, kQuarterPi = 0.785398f;

// XStaticObj_Pillar: 27 wide; 146.5 tall (165 in world 3, model 0x292). Over a hole (the floor
// level is negative) a taller pillar (180) stands from y = -38.
class Pillar : public XStaticObjBase {
public:
    Pillar(const XGameContext* g, const D3DVECTOR& p) : XStaticObjBase(g, p) {
        if (pos_.y >= 0) {
            if (g->world == 2) {
                model_ = std::make_unique<JObject3DX>(kModelPillarWorld3);
                collideBox_ = std::make_unique<XCollideBox>(27.0f, 165.0f);
            } else {
                model_ = std::make_unique<JObject3DX>(kModelPillar);
                collideBox_ = std::make_unique<XCollideBox>(27.0f, 146.5f);
            }
        } else {
            model_ = std::make_unique<JObject3DX>(kModelPillarLow);
            collideBox_ = std::make_unique<XCollideBox>(27.0f, 180.0f);
            pos_.y = -38.0f;
        }
        model_->SetPos(pos_);
        model_->UpdateWorldMatrix();
        collideBox_->SetPos(pos_);
    }
};

// XStaticObj_PillarSmall: 20 wide, 78 tall
class PillarSmall : public XStaticObjBase {
public:
    PillarSmall(const XGameContext* g, const D3DVECTOR& p) : XStaticObjBase(g, p) {
        model_ = std::make_unique<JObject3DX>(kModelPillarSmall);
        model_->SetPos(pos_);
        model_->UpdateWorldMatrix();
        collideBox_ = std::make_unique<XCollideBox>(20.0f, 78.0f);
        collideBox_->SetPos(pos_);
    }
};

// XStaticObj_Tree ('T', World 2): turned -90 degrees; a 35-wide, 5-tall box at its foot plus a
// 12-wide, 134.4-tall box for the trunk (CreateExtraCollideBox). No shadow.
class Tree : public XStaticObjBase {
public:
    Tree(const XGameContext* g, const D3DVECTOR& p, XStaticObjList* list) : XStaticObjBase(g, p), list_(list) {
        model_ = std::make_unique<JObject3DX>(kModelTree);
        model_->SetYaw(-kHalfPi);
        model_->SetPos(pos_);
        model_->UpdateWorldMatrix();
        collideBox_ = std::make_unique<XCollideBox>(35.0f, 5.0f);
        collideBox_->SetPos(pos_);
    }
    void CreateExtraCollideBox() override { list_->CreateCollideBox(pos_, 12.0f, 134.4f, 12.0f); }

private:
    XStaticObjList* list_;
};

// XStaticObj_Plant ('U', World 2): turned -90 degrees, 24 wide, 20 tall, with a 28-unit shadow
class Plant : public XStaticObjBase {
public:
    Plant(const XGameContext* g, const D3DVECTOR& p) : XStaticObjBase(g, p) {
        model_ = std::make_unique<JObject3DX>(kModelPlant);
        model_->SetYaw(-kHalfPi);
        model_->SetPos(pos_);
        model_->UpdateWorldMatrix();
        shadow_ = std::make_unique<XShadowSimple>(28.0f);
        shadow_->SetPosXZ(pos_);
        collideBox_ = std::make_unique<XCollideBox>(24.0f, 20.0f);
        collideBox_->SetPos(pos_);
    }
};

// XStaticObj_PotLarge ('M', World 3): 32 wide, 35 tall, with a 40-unit shadow, and a 17-wide,
// 80-tall box for its neck (CreateExtraCollideBox)
class PotLarge : public XStaticObjBase {
public:
    PotLarge(const XGameContext* g, const D3DVECTOR& p, XStaticObjList* list) : XStaticObjBase(g, p), list_(list) {
        model_ = std::make_unique<JObject3DX>(kModelPotLarge);
        model_->SetPos(pos_);
        model_->UpdateWorldMatrix();
        shadow_ = std::make_unique<XShadowSimple>(40.0f);
        shadow_->SetPosXZ(pos_);
        collideBox_ = std::make_unique<XCollideBox>(32.0f, 35.0f);
        collideBox_->SetPos(pos_);
    }
    void CreateExtraCollideBox() override { list_->CreateCollideBox(pos_, 17.0f, 80.0f, 17.0f); }

private:
    XStaticObjList* list_;
};

// XStaticObj_Pot ('N', World 3 and 4-6): 30 wide, 40 tall, with a 38-unit shadow
class Pot : public XStaticObjBase {
public:
    Pot(const XGameContext* g, const D3DVECTOR& p) : XStaticObjBase(g, p) {
        model_ = std::make_unique<JObject3DX>(kModelPot);
        model_->SetPos(pos_);
        model_->UpdateWorldMatrix();
        shadow_ = std::make_unique<XShadowSimple>(38.0f);
        shadow_->SetPosXZ(pos_);
        collideBox_ = std::make_unique<XCollideBox>(30.0f, 40.0f);
        collideBox_->SetPos(pos_);
    }
};

// XStaticObj_RocketLauncher ('R', World 3, 4-1, 4-4, 4-7): 34 wide, 40 tall, no shadow. Every 1000
// ticks it sends up a rocket from its own position.
class RocketLauncher : public XStaticObjBase {
public:
    RocketLauncher(const XGameContext* g, const D3DVECTOR& p) : XStaticObjBase(g, p) {
        model_ = std::make_unique<JObject3DX>(kModelRocketLauncher);
        model_->SetPos(pos_);
        model_->UpdateWorldMatrix();
        collideBox_ = std::make_unique<XCollideBox>(34.0f, 40.0f);
        collideBox_->SetPos(pos_);
    }
    void ProcessFrame() override {
        XStaticObjBase::ProcessFrame();
        if (localCounter_ % 1000 == 0) game_->bullets->CreateRocket(pos_);
    }
};

// XStaticObj_Barrel: 27 wide, 42.8 tall, with a 37-unit shadow
class Barrel : public XStaticObjBase {
public:
    Barrel(const XGameContext* g, const D3DVECTOR& p) : XStaticObjBase(g, p) {
        model_ = std::make_unique<JObject3DX>(kModelBarrel);
        model_->SetPos(pos_);
        model_->UpdateWorldMatrix();
        shadow_ = std::make_unique<XShadowSimple>(37.0f);
        shadow_->SetPosXZ(pos_);
        collideBox_ = std::make_unique<XCollideBox>(27.0f, 42.8f);
        collideBox_->SetPos(pos_);
    }
};

// XStaticObj_Cannon ('H' 'I' 'J', facing directions 4, 0 and 2: 45 degrees apart): a 33-wide,
// 34-tall cannon with a 38-unit shadow. Every 550 ticks it fires from 21 up, sound 0x262, and
// kicks its barrel up 8 degrees (easing back at 0.15 a tick): a rolling ball in worlds 1 and 2, a
// straight ball in worlds 3 and 4.
class Cannon : public XStaticObjBase {
public:
    Cannon(const XGameContext* g, const D3DVECTOR& p, int dir) : XStaticObjBase(g, p), dir_(dir) {
        model_ = std::make_unique<JObject3DX>(kModelCannon);
        model_->SetYaw(dir_ * kQuarterPi);
        model_->SetPos(pos_);
        model_->UpdateWorldMatrix();
        shadow_ = std::make_unique<XShadowSimple>(38.0f);
        shadow_->SetPosXZ(pos_);
        collideBox_ = std::make_unique<XCollideBox>(33.0f, 34.0f);
        collideBox_->SetPos(pos_);
    }
    void ProcessFrame() override {
        XStaticObjBase::ProcessFrame();
        if (localCounter_ % 550 == 0) {
            D3DVECTOR at(pos_.x, pos_.y + 21.0f, pos_.z);
            if (game_->world <= 1) game_->bullets->CreateCannonRBall(at, dir_ * kQuarterPi);
            else game_->bullets->CreateCannonBall(at, dir_);
            pitch_ = -0.139626f;
            game_->playSound(kSoundCannon, pos_.z);
        } else {
            pitch_ = MakeAngleCloseTo_Constant(pitch_, 0.0f, 0.00261799f);
        }
        model_->SetPitch(pitch_);
        model_->UpdateWorldMatrix();
    }

private:
    int dir_;
};

// XStaticObj_CannonR ('K'): a turning cannon (barrel 0x1f4 on base 0x1f3) with a 38-unit shadow,
// a 32-wide, 22-tall box from 30 up and an 18-wide, 40-tall box at its foot. It turns towards
// the closer hero (0.6 degrees a tick) for 430 ticks, then holds: 60 ticks in it fires a rolling
// ball from 5 ahead and 40 up along its yaw (sound 0x262, kick as the fixed cannon), and after
// 120 it turns again.
class CannonR : public XStaticObjBase {
public:
    enum MoveState { ROTATE = 0, FIRE = 1 };
    CannonR(const XGameContext* g, const D3DVECTOR& p, XStaticObjList* list) : XStaticObjBase(g, p), list_(list) {
        yaw_ = kHalfPi;
        model_ = std::make_unique<JObject3DX>(kModelCannonRBarrel);
        model_->SetYaw(yaw_);
        model_->SetPos(pos_);
        model_->UpdateWorldMatrix();
        shadow_ = std::make_unique<XShadowSimple>(38.0f);
        shadow_->SetPosXZ(pos_);
        collideBox_ = std::make_unique<XCollideBox>(32.0f, 22.0f);
        collideBox_->SetPos(D3DVECTOR(pos_.x, pos_.y + 30.0f, pos_.z));
        base_ = std::make_unique<JObject3DX>(kModelCannonRBase);
        base_->SetPos(pos_);
        base_->UpdateWorldMatrix();
    }
    void CreateExtraCollideBox() override { list_->CreateCollideBox(pos_, 18.0f, 40.0f, 18.0f); }
    void ProcessFrame() override {
        XStaticObjBase::ProcessFrame();
        if (moveState_ == ROTATE) {
            yaw_ = MakeAngleCloseTo_Constant(yaw_, game_->DirToCloserHero(pos_), 0.010472f);
            if (localCounter_ >= 430) {
                localCounter_ = 0;
                moveState_ = FIRE;
            }
        } else if (moveState_ == FIRE) {
            if (localCounter_ == 60) {
                D3DVECTOR at(pos_.x + std::sin(yaw_) * 5.0f, pos_.y + 40.0f, pos_.z + std::cos(yaw_) * 5.0f);
                game_->bullets->CreateCannonRBall(at, yaw_);
                pitch_ = -0.139626f;
                game_->playSound(kSoundCannon, pos_.z);
            }
            if (localCounter_ >= 120) {
                localCounter_ = 0;
                moveState_ = ROTATE;
            }
        }
        pitch_ = MakeAngleCloseTo_Constant(pitch_, 0.0f, 0.00261799f);
        model_->SetYaw(yaw_);
        model_->SetPitch(pitch_);
        model_->UpdateWorldMatrix();
    }
    void Render(Device* d) override {
        if (!enabled_ || !game_->IsVisible(pos_)) return;
        model_->Render(d);
        base_->Render(d);
        shadow_->Render(d);
    }

private:
    XStaticObjList* list_;
    MoveState moveState_ = ROTATE;
    std::unique_ptr<JObject3DX> base_;
};

// XStaticObj_Shop ('S'): the shop building (model 0x1ab turned a quarter) with a 112 x 1000 x 120
// box and its sign (texture 0x1ac, 84 x 36.96, upright and facing the camera, 56 out and 127.4
// up). A hero standing within 15 of the doorstep (60 in front) opens the shop; it can open again
// once the hero has stepped more than 25 away.
class Shop : public XStaticObjBase {
public:
    Shop(const XGameContext* g, const D3DVECTOR& p) : XStaticObjBase(g, p), checkPos_(p) {
        model_ = std::make_unique<JObject3DX>(kModelShop);
        model_->SetYaw(kHalfPi);
        model_->SetPos(pos_);
        model_->UpdateWorldMatrix();
        collideBox_ = std::make_unique<XCollideBox>(112.0f, 1000.0f, 120.0f);
        collideBox_->SetPos(pos_);
        float w = 84.0f;
        logo_ = std::make_unique<JObject3DRect>(w, w * 0.44f);
        logo_->SetTexture(kTexShopSign);
        logo_->UsePerspective(true);
        logo_->SetFilterState(JObject3DRect::FILTER_LINEAR);
        logo_->Rotate(1, -kHalfPi);
        logo_->Rotate(2, -kHalfPi);
        logo_->Shift(D3DVECTOR(56.0f, 127.4f, 0.0f));
        logo_->Shift(pos_);
        logo_->UpdateWorldMatrix();
        checkPos_.x += 60.0f;
    }
    void ProcessFrame() override {
        XStaticObjBase::ProcessFrame();
        CheckHero(game_->heroA, heroAFinished_);
        CheckHero(game_->heroB, heroBFinished_);
    }
    void Render(Device* d) override {
        if (!game_->IsVisible(pos_)) return;
        XStaticObjBase::Render(d);
        logo_->Render(d);
    }

private:
    // a hero in the NORMAL state within 15 of the door opens the shop; it opens again for that hero
    // only once it has gone 25 away
    void CheckHero(XHero* h, bool& finished) {
        if (!finished) {
            if (h && h->GetLiveState() == XHero::NORMAL && 15.0f > ApproxDistance(h->GetPos(), checkPos_)) {
                if (game_->startShop) game_->startShop(h);  // XShop::Start
                finished = true;
            }
        } else if (h && ApproxDistance(h->GetPos(), checkPos_) > 25.0f) {
            finished = false;
        }
    }

    std::unique_ptr<JObject3DRect> logo_;
    D3DVECTOR checkPos_;
    bool heroAFinished_ = false, heroBFinished_ = false;
};

// XStaticObj_CollideBox: no model, just the box
class CollideBox : public XStaticObjBase {
public:
    CollideBox(const XGameContext* g, const D3DVECTOR& p, float wx, float h, float wz) : XStaticObjBase(g, p) {
        collideBox_ = std::make_unique<XCollideBox>(wx, h, wz);
        collideBox_->SetPos(pos_);
    }
};

// XStaticObj_Fence: ten shapes. 0 'X' runs along X, 1 'Z' along Z (both 40 long); 2..9 are
// corners ('1'..'4', model 0x1d8) and T pieces ('5'..'8', model 0x205), each a 30-long piece
// half a cell to one side plus one or two extra boxes (CreateExtraCollideBox). All 100 tall and
// 10 thick.
class Fence : public XStaticObjBase {
public:
    Fence(const XGameContext* g, const D3DVECTOR& p, int type, XStaticObjList* list)
        : XStaticObjBase(g, p), type_(type), list_(list) {
        static const struct {
            uint16_t model;
            float yaw, wx, wz, dx;
        } kShapes[10] = {
            {kModelFenceStraight, 0, 40, 10, 0},       {kModelFenceStraight, -kHalfPi, 10, 40, 0},
            {kModelFenceCorner, kHalfPi, 30, 10, -10}, {kModelFenceCorner, kPi, 30, 10, -10},
            {kModelFenceCorner, 0, 30, 10, 10},        {kModelFenceCorner, -kHalfPi, 30, 10, 10},
            {kModelFenceT, 0, 30, 10, -10},            {kModelFenceT, kHalfPi, 30, 10, -10},
            {kModelFenceT, -kHalfPi, 30, 10, 10},      {kModelFenceT, kPi, 30, 10, 10},
        };
        const auto& s = kShapes[type < 0 || type > 9 ? 0 : type];
        model_ = std::make_unique<JObject3DX>(s.model);
        if (s.yaw != 0) model_->SetYaw(s.yaw);
        collideBox_ = std::make_unique<XCollideBox>(s.wx, 100.0f, s.wz);
        collideBox_->SetPos(D3DVECTOR(pos_.x + s.dx, pos_.y, pos_.z));
        model_->SetPos(pos_);
        model_->UpdateWorldMatrix();
    }
    void CreateExtraCollideBox() override {
        // the extra boxes, 30 long: along Z half a cell to one side, or along X
        auto alongZ = [&](float dz) { list_->CreateCollideBox(D3DVECTOR(pos_.x, pos_.y, pos_.z + dz), 10, 100, 30); };
        auto alongX = [&](float dx) { list_->CreateCollideBox(D3DVECTOR(pos_.x + dx, pos_.y, pos_.z), 30, 100, 10); };
        switch (type_) {
            case 2: alongZ(-10); break;
            case 3: alongZ(10); break;
            case 4: alongZ(-10); break;
            case 5: alongZ(10); break;
            case 6: alongZ(-10), alongX(10); break;
            case 7: alongZ(10), alongZ(-10); break;
            case 8: alongZ(-10), alongZ(10); break;
            case 9: alongZ(10), alongX(-10); break;
            default: break;
        }
    }

private:
    int type_;
    XStaticObjList* list_;
};
}  // namespace

// ------------------------------------------------------------------------------ XStaticObjBase

XStaticObjBase::XStaticObjBase(const XGameContext* game, const D3DVECTOR& pos) : game_(game), pos_(pos) {}
XStaticObjBase::~XStaticObjBase() = default;

void XStaticObjBase::SetFloorLevel(int level) {
    floorLevel_ = level;
    if (shadow_) shadow_->SetFloorLevel(level);
}

void XStaticObjBase::Render(Device* d) {
    if (!enabled_ || !game_->IsVisible(pos_)) return;
    if (model_) model_->Render(d);
    if (shadow_) shadow_->Render(d);
}

// ------------------------------------------------------------------------ XStaticObj_GenieLamp
// Model 0x1ea turned 180 degrees, a 30 x 24 box, a 36-unit shadow; the spout is 28 up and 30
// back along Z.

XStaticObj_GenieLamp::XStaticObj_GenieLamp(const XGameContext* game, const D3DVECTOR& pos) : XStaticObjBase(game, pos) {
    model_ = std::make_unique<JObject3DX>(kModelGenieLamp);
    model_->SetYaw(3.14159f);
    model_->SetPos(pos_);
    model_->UpdateWorldMatrix();
    shadow_ = std::make_unique<XShadowSimple>(36.0f);
    shadow_->SetPos(pos_);
    collideBox_ = std::make_unique<XCollideBox>(30.0f, 24.0f);
    collideBox_->SetPos(pos_);
    holePos_ = D3DVECTOR(pos_.x, pos_.y + 28.0f, pos_.z - 30.0f);
}

// Shaking: it rocks up to 4 degrees (50 times over the 500 ticks) and puffs steam every 16 ticks.
// (The last pitch, close to 0, is left when it stops.)
void XStaticObj_GenieLamp::ProcessFrame() {
    XStaticObjBase::ProcessFrame();
    if (moveState_ != SHAKE) return;
    float t = (float)localCounter_ / 500.0f;
    model_->SetPitch(std::sin(t * 100.0f * 3.14159f) * 0.0698132f);
    model_->UpdateWorldMatrix();
    if (localCounter_ % 16 == 0) game_->particles->CreateLampSteam(holePos_);
    if (localCounter_ >= 500) {
        localCounter_ = 0;
        moveState_ = NORMAL;
    }
}

// ------------------------------------------------------------------------------ XStaticObjList

XStaticObjList::~XStaticObjList() = default;

void XStaticObjList::Empty() {
    for (auto& s : slots_) s.reset();
}

int XStaticObjList::GetNullIndex() const {
    for (int i = 0; i < kSlots; i++)
        if (!slots_[i]) return i;
    return -1;
}

XStaticObjBase* XStaticObjList::Put(std::unique_ptr<XStaticObjBase> o) {
    int i = GetNullIndex();
    if (i < 0 || !o) return nullptr;
    slots_[i] = std::move(o);
    return slots_[i].get();
}

// XStaticObjList::CreateObject: makes the object, then calls its CreateExtraCollideBox (a no-op
// for most), as the original does for every code.
XStaticObjBase* XStaticObjList::CreateObject(const D3DVECTOR& pos, char code) {
    if (GetNullIndex() < 0) return nullptr;
    std::unique_ptr<XStaticObjBase> o;
    switch (code) {
        case 'O': o = std::make_unique<Barrel>(game_, pos); break;
        case 'P': o = std::make_unique<Pillar>(game_, pos); break;
        case 'Q': o = std::make_unique<PillarSmall>(game_, pos); break;
        case 'H': o = std::make_unique<Cannon>(game_, pos, 4); break;
        case 'I': o = std::make_unique<Cannon>(game_, pos, 0); break;
        case 'J': o = std::make_unique<Cannon>(game_, pos, 2); break;
        case 'K': o = std::make_unique<CannonR>(game_, pos, this); break;
        case 'M': o = std::make_unique<PotLarge>(game_, pos, this); break;
        case 'N': o = std::make_unique<Pot>(game_, pos); break;
        case 'R': o = std::make_unique<RocketLauncher>(game_, pos); break;
        case 'S': o = std::make_unique<Shop>(game_, pos); break;
        case 'T': o = std::make_unique<Tree>(game_, pos, this); break;
        case 'U': o = std::make_unique<Plant>(game_, pos); break;
        default:
            return nullptr;
    }
    XStaticObjBase* s = Put(std::move(o));
    if (s) s->CreateExtraCollideBox();
    return s;
}

XStaticObjBase* XStaticObjList::CreateFence(const D3DVECTOR& pos, char shape) {
    if (GetNullIndex() < 0) return nullptr;
    int type = shape == 'X' ? 0 : shape == 'Z' ? 1 : (shape >= '1' && shape <= '8') ? shape - '1' + 2 : -1;
    if (type < 0) return nullptr;
    XStaticObjBase* f = Put(std::make_unique<Fence>(game_, pos, type, this));
    if (f) f->CreateExtraCollideBox();
    return f;
}

XStaticObj_GenieLamp* XStaticObjList::CreateGenieLamp(const D3DVECTOR& pos) {
    return static_cast<XStaticObj_GenieLamp*>(Put(std::make_unique<XStaticObj_GenieLamp>(game_, pos)));
}

XStaticObjBase* XStaticObjList::CreateCollideBox(const D3DVECTOR& pos, float wx, float h, float wz) {
    return Put(std::make_unique<CollideBox>(game_, pos, wx, h, wz));
}

void XStaticObjList::ProcessFrame() {
    for (auto& s : slots_) {
        if (!s) break;
        s->ProcessFrame();
    }
}

void XStaticObjList::Render(Device* d) {
    for (auto& s : slots_) {
        if (!s) break;
        s->Render(d);
    }
}

bool XStaticObjList::CollisionCheckX(float& newX, const XCollideBox& box, bool ignoreY) const {
    bool hit = false;
    for (auto& s : slots_) {
        if (!s) break;
        if (s->Enabled() && s->GetCollideBox().CollisionCheckX(newX, box, ignoreY)) hit = true;
    }
    return hit;
}

bool XStaticObjList::CollisionCheckZ(float& newZ, const XCollideBox& box, bool ignoreY) const {
    bool hit = false;
    for (auto& s : slots_) {
        if (!s) break;
        if (s->Enabled() && s->GetCollideBox().CollisionCheckZ(newZ, box, ignoreY)) hit = true;
    }
    return hit;
}

bool XStaticObjList::CollisionCheckY(float& newY, const XCollideBox& box) const {
    bool hit = false;
    for (auto& s : slots_) {
        if (!s) break;
        if (s->Enabled() && s->GetCollideBox().CollisionCheckY(newY, box)) hit = true;
    }
    return hit;
}

bool XStaticObjList::CollisionCheck(const D3DVECTOR& p) const {
    for (auto& s : slots_) {
        if (!s) break;
        if (s->Enabled() && s->GetCollideBox().CollisionCheck(p)) return true;
    }
    return false;
}

bool XStaticObjList::CollisionCheck_Fire(const D3DVECTOR& p) const {
    for (auto& s : slots_) {
        if (!s) break;
        if (s->Enabled() && s->BlockFire() && s->GetCollideBox().CollisionCheck(p)) return true;
    }
    return false;
}
