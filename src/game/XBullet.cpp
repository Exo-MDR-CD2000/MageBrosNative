#include "game/XBullet.h"
#include "game/JModel.h"
#include "game/JObject3D.h"
#include "game/XGameContext.h"
#include "game/XHero.h"
#include "game/XLevelView.h"
#include "game/XMath.h"
#include "game/XParticle.h"
#include "game/XPlatform.h"
#include "game/XStaticObj.h"
#include <cmath>

namespace {
constexpr uint16_t kModelCannonBall = 0x18d, kModelCannonRBall = 0x1f2;
constexpr uint16_t kTexFlame = 0x192;
constexpr float kQuarterPi = 0.785398f;

// XBullet_Cannon: a ball flying straight along one of eight directions at 1.4 a tick, spinning
// forward 4 degrees a tick, with a puff of smoke every 11 ticks. After 15 ticks it bursts on the
// map, an object, a platform or a hero (where it was before this tick's move).
class Cannon : public XBulletBase {
public:
    Cannon(const XGameContext* g, const D3DVECTOR& p, int dir) : XBulletBase(g, p), dir_(dir) {
        model_ = std::make_unique<JObject3DX>(kModelCannonBall);
        yaw_ = dir_ * kQuarterPi;
        UpdateWorldMatrix();
        shadow_ = std::make_unique<XShadow>(17.0f);
        radius_ = 8.0f;
        liveState_ = NORMAL;
    }
    void ProcessFrame() override {
        XBulletBase::ProcessFrame();
        if (liveState_ != NORMAL) return;
        pitch_ = NormaliseAngle(pitch_ + 0.0698132f);
        const D3DVECTOR prev = pos_;
        float a = dir_ * kQuarterPi;
        pos_.x += std::sin(a) * 1.4f;
        pos_.z += std::cos(a) * 1.4f;
        if (localCounter_ % 11 == 0) game_->particles->CreateSmoke(pos_);
        if (localCounter_ > 15 && Hits()) {
            game_->particles->CreateBulletExplode(prev);
            liveState_ = NON;
        }
        UpdateWorldMatrix();
    }

private:
    bool Hits() {
        // the map, objects, platforms, then player 1 (and player 2: stage 5)
        return game_->level->Collision().IsBlockedFloat(pos_.x, pos_.z, pos_.y) || game_->statics->CollisionCheck(pos_) ||
               game_->platforms->CollisionCheck(pos_) || CollisionCheck_Hero(game_->heroA);
    }
    int dir_;
};

// XBullet_CannonRBall: a ball rolling along a yaw at 1 a tick, spinning forward 3.5 degrees a tick,
// falling (0.016 a tick, at most 4) onto the floor, with a puff of smoke every 15 ticks. After 25
// ticks it bursts like the straight ball; below -200 it is gone.
class CannonRBall : public XBulletBase {
public:
    CannonRBall(const XGameContext* g, const D3DVECTOR& p, float yaw) : XBulletBase(g, p) {
        model_ = std::make_unique<JObject3DX>(kModelCannonRBall);
        yaw_ = yaw;
        UpdateWorldMatrix();
        shadow_ = std::make_unique<XShadow>(19.0f);
        radius_ = 9.0f;
        liveState_ = NORMAL;
    }
    void ProcessFrame() override {
        XBulletBase::ProcessFrame();
        if (liveState_ != NORMAL) return;
        pitch_ = NormaliseAngle(pitch_ + 0.0610865f);
        const D3DVECTOR prev = pos_;
        verticalSpeed_ -= 0.016f;
        if (verticalSpeed_ < -4.0f) verticalSpeed_ = -4.0f;
        // the bottom of the ball against the floor (XMap::CollisionCheckY on a point)
        float bottom = pos_.y + verticalSpeed_ - radius_;
        if (game_->level->Collision().CollisionCheckY(bottom, pos_.x, pos_.z)) verticalSpeed_ = 0;
        pos_.y = bottom + radius_;
        pos_.x += std::sin(yaw_) * 1.0f;
        pos_.z += std::cos(yaw_) * 1.0f;
        if (localCounter_ % 15 == 0) game_->particles->CreateSmoke(pos_);
        if (localCounter_ > 25 && Hits()) {
            game_->particles->CreateBulletExplode(prev);
            liveState_ = NON;
        }
        if (pos_.y < -200.0f) liveState_ = NON;
        // on a water level, below -38: a wave (unless already below -43) and a burst
        if (game_->useWater && pos_.y < -38.0f) {
            if (pos_.y > -43.0f) game_->particles->CreateWaterWave(D3DVECTOR(pos_.x, -38.0f, pos_.z), false, true);
            game_->particles->CreateBulletExplode(prev);
            liveState_ = NON;
        }
        UpdateWorldMatrix();
    }

private:
    bool Hits() {
        return game_->level->Collision().IsBlockedFloat(pos_.x, pos_.z, pos_.y) || game_->statics->CollisionCheck(pos_) ||
               game_->platforms->CollisionCheck(pos_) || CollisionCheck_Hero(game_->heroA);
    }
    float verticalSpeed_ = 0;
};
// XBullet_Flame: the dragon's fireball, a 50-unit sprite (the explosion sheet, 4 ticks a frame)
// flying at 4.5 a tick along its yaw and pitch, slowing by 0.03 a tick. The map, an object or a
// platform stops it where it was. It burns a hero during its first 9 frames and is gone after 64
// ticks.
class Flame : public XBulletBase {
public:
    enum SubState { MOVE = 0, STOP = 1 };
    Flame(const XGameContext* g, const D3DVECTOR& p, float yaw, float pitch)
        : XBulletBase(g, p), dirYaw_(yaw), dirPitch_(pitch) {
        particle_ = std::make_unique<JObject3DRect>(JObject3DRect::Particle(1, 1));
        particle_->SetTexture(kTexFlame);
        particle_->Scale(50.0f);
        particle_->SetUV(0, 0.25f, 0, 0.25f, 0);
        particle_->SetWorldPos(pos_);
        radius_ = 10.0f;
        liveState_ = NORMAL;
        subState_ = MOVE;
    }
    void ProcessFrame() override {
        XBulletBase::ProcessFrame();
        if (liveState_ != NORMAL) return;
        if (subState_ == MOVE) {
            const D3DVECTOR prev = pos_;
            pos_.x += std::sin(dirYaw_) * speed_ * std::cos(-dirPitch_);
            pos_.z += std::cos(dirYaw_) * speed_ * std::cos(-dirPitch_);
            pos_.y += std::sin(-dirPitch_) * speed_;
            speed_ -= 0.03f;
            if (0 > speed_) speed_ = 0;
            if (game_->level->Collision().IsBlockedFloat(pos_.x, pos_.z, pos_.y) || game_->statics->CollisionCheck(pos_) ||
                game_->platforms->CollisionCheck(pos_)) {
                pos_ = prev;
                subState_ = STOP;
            }
        }
        frame_ = (int)((localCounter_ / 4) % 16);
        float u = (frame_ % 4) * 0.25f, v = (frame_ / 4) * 0.25f;
        particle_->SetUV(u, u + 0.25f, v, v + 0.25f, 0);
        if (frame_ < 9) CollisionCheck_Hero(game_->heroA);  // (and player 2: stage 5)
        if (localCounter_ >= 64) liveState_ = NON;
    }
    void Render(Device* d, const D3DMATRIX& view) override {
        if (liveState_ == NON) return;
        particle_->SetWorldPos(pos_);
        particle_->Render2(d, view);
    }

private:
    SubState subState_ = MOVE;
    float speed_ = 4.5f, dirYaw_, dirPitch_;
    int frame_ = 0;
    std::unique_ptr<JObject3DRect> particle_;
};
}  // namespace

// --------------------------------------------------------------------------------- XBulletBase

XBulletBase::XBulletBase(const XGameContext* game, const D3DVECTOR& pos) : game_(game), pos_(pos) {}
XBulletBase::~XBulletBase() = default;

void XBulletBase::Render(Device* d, const D3DMATRIX&) {
    if (liveState_ == NON) return;
    if (model_) model_->Render(d);
    if (shadow_) {
        shadow_->SetPos(pos_);
        shadow_->Render(d, game_->level->Collision());
    }
}

bool XBulletBase::CollisionCheck_Fire(const D3DVECTOR& pos, float radius) const {
    if (liveState_ != NORMAL) return false;
    if (std::fabs(pos_.z - pos.z) > radius + radius_) return false;
    if (std::fabs(pos_.x - pos.x) > radius + radius_) return false;
    return radius + radius_ > ApproxDistance(pos_, pos);
}

bool XBulletBase::CollisionCheck_Hero(XHero* hero) {
    if (liveState_ != NORMAL || !hero || hero->GetLiveState() != XHero::NORMAL) return false;
    const D3DVECTOR& hp = hero->GetPos();
    if (std::fabs(pos_.z - hp.z) > hero->GetCollideBox().WidthZ() + radius_) return false;
    if (std::fabs(pos_.x - hp.x) > hero->GetCollideBox().WidthX() + radius_) return false;
    if (!hero->BodyModel().CheckCollision_Sphere(pos_, radius_)) return false;
    hero->Damage(*game_->heroWorld);
    hero->KnockBack(pos_);
    return true;
}

void XBulletBase::UpdateWorldMatrix() {
    model_->SetYaw(yaw_);
    model_->SetPitch(pitch_);
    model_->SetPos(pos_);
    model_->UpdateWorldMatrix();
}

// --------------------------------------------------------------------------------- XBulletList

XBulletList::~XBulletList() = default;

void XBulletList::Empty() {
    for (auto& s : slots_) s.reset();
}

int XBulletList::GetNullIndex() const {
    for (int i = 0; i < kSlots; i++)
        if (!slots_[i]) return i;
    return -1;
}

void XBulletList::CreateCannonBall(const D3DVECTOR& pos, int dir) {
    int i = GetNullIndex();
    if (i >= 0) slots_[i] = std::make_unique<Cannon>(game_, pos, dir);
}

void XBulletList::CreateCannonRBall(const D3DVECTOR& pos, float yaw) {
    int i = GetNullIndex();
    if (i >= 0) slots_[i] = std::make_unique<CannonRBall>(game_, pos, yaw);
}

void XBulletList::CreateFlame(const D3DVECTOR& pos, float yaw, float pitch) {
    int i = GetNullIndex();
    if (i >= 0) slots_[i] = std::make_unique<Flame>(game_, pos, yaw, pitch);
}

void XBulletList::ProcessFrame() {
    for (auto& s : slots_) {
        if (!s) continue;
        s->ProcessFrame();
        if (s->GetState() == XBulletBase::NON) s.reset();
    }
}

void XBulletList::Render(Device* d, const D3DMATRIX& view) {
    for (auto& s : slots_)
        if (s) s->Render(d, view);
}

bool XBulletList::CollisionCheck_Fire(const D3DVECTOR& pos, float radius) const {
    for (const auto& s : slots_)
        if (s && s->CollisionCheck_Fire(pos, radius)) return true;
    return false;
}
