#include "game/XStaticObj.h"
#include "core/log.h"
#include "game/JModel.h"
#include "game/XGameContext.h"
#include "game/XParticle.h"

namespace {
// models
constexpr uint16_t kModelPillar = 0x170, kModelPillarWorld3 = 0x292, kModelPillarLow = 0x1ef;
constexpr uint16_t kModelPillarSmall = 0x1db, kModelBarrel = 0x1f6;
constexpr uint16_t kModelFenceStraight = 0x1d9, kModelFenceCorner = 0x1d8, kModelFenceT = 0x205;
constexpr float kHalfPi = 1.5708f, kPi = 3.14159f;

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

XStaticObjBase* XStaticObjList::CreateObject(const D3DVECTOR& pos, char code) {
    if (GetNullIndex() < 0) return nullptr;
    switch (code) {
        case 'O': return Put(std::make_unique<Barrel>(game_, pos));
        case 'P': return Put(std::make_unique<Pillar>(game_, pos));
        case 'Q': return Put(std::make_unique<PillarSmall>(game_, pos));
        default:
            // cannons (H I J K), pots (M N), rocket launcher (R), shop (S), tree (T), plant (U):
            // later chunks
            return nullptr;
    }
}

XStaticObjBase* XStaticObjList::CreateFence(const D3DVECTOR& pos, char shape) {
    if (GetNullIndex() < 0) return nullptr;
    int type = shape == 'X' ? 0 : shape == 'Z' ? 1 : (shape >= '1' && shape <= '8') ? shape - '1' + 2 : -1;
    if (type < 0) return nullptr;
    XStaticObjBase* f = Put(std::make_unique<Fence>(game_, pos, type, this));
    if (f) f->CreateExtraCollideBox();
    return f;
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
