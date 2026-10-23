#include "game/XCandle.h"
#include "framework/d3dutil.h"
#include "game/JModel.h"
#include "game/XGameContext.h"
#include "game/XMath.h"
#include "game/XParticle.h"

namespace {
constexpr uint16_t kModelCandle = 0x1f8;
}  // namespace

// ------------------------------------------------------------------------------------ XCandle

XCandle::XCandle(const XGameContext* game, const D3DVECTOR& pos) : game_(game), pos_(pos) {
    // on the back wall, 175 up; the flame 22 above that
    pos_.x = 19.8f;
    pos_.y = 175.0f;
    firePos_ = pos_;
    firePos_.y += 22.0f;
    model_ = std::make_unique<JObject3DX>(kModelCandle);
    model_->SetYaw(1.5708f);
    model_->SetPos(pos_);
    model_->UpdateWorldMatrix();
    collideBox_ = std::make_unique<XCollideBox>(25.0f, 12.0f);
    collideBox_->SetPos(pos_);
    game_->particles->CreateCandleFire1(D3DVECTOR(firePos_.x + 6.0f, firePos_.y + 6.0f, firePos_.z));
    localCounter_ = (unsigned)Random(15);  // the flickers out of step
}
XCandle::~XCandle() = default;

void XCandle::ProcessFrame() {
    localCounter_++;
    if (localCounter_ % 15 == 0) game_->particles->CreateCandleFire2(RandomPosXZ(firePos_, 2.0f));
}

void XCandle::Render(Device* d) {
    if (!game_->IsVisible(pos_)) return;
    model_->Render(d);
}

// -------------------------------------------------------------------------------- XCandleList

XCandleList::~XCandleList() = default;

void XCandleList::Empty() {
    for (auto& s : slots_) s.reset();
}

int XCandleList::GetNullIndex() const {
    for (int i = 0; i < kSlots; i++)
        if (!slots_[i]) return i;
    return -1;
}

void XCandleList::CreateCandle(const D3DVECTOR& pos) {
    int i = GetNullIndex();
    if (i >= 0) slots_[i] = std::make_unique<XCandle>(game_, pos);
}

void XCandleList::ProcessFrame() {
    for (auto& s : slots_) {
        if (!s) break;
        s->ProcessFrame();
    }
}

void XCandleList::Render(Device* d) {
    for (auto& s : slots_) {
        if (!s) break;
        s->Render(d);
    }
}

bool XCandleList::CollisionCheckX(float& newX, const XCollideBox& box, bool ignoreY) const {
    bool hit = false;
    for (auto& s : slots_) {
        if (!s) break;
        if (s->GetCollideBox().CollisionCheckX(newX, box, ignoreY)) hit = true;
    }
    return hit;
}

bool XCandleList::CollisionCheckZ(float& newZ, const XCollideBox& box, bool ignoreY) const {
    bool hit = false;
    for (auto& s : slots_) {
        if (!s) break;
        if (s->GetCollideBox().CollisionCheckZ(newZ, box, ignoreY)) hit = true;
    }
    return hit;
}

bool XCandleList::CollisionCheckY(float& newY, const XCollideBox& box) const {
    bool hit = false;
    for (auto& s : slots_) {
        if (!s) break;
        if (s->GetCollideBox().CollisionCheckY(newY, box)) hit = true;
    }
    return hit;
}

bool XCandleList::CollisionCheck(const D3DVECTOR& p) const {
    for (auto& s : slots_) {
        if (!s) break;
        if (s->GetCollideBox().CollisionCheck(p)) return true;
    }
    return false;
}
