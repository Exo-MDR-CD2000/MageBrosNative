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
constexpr uint16_t kModelCannonBall = 0x18d, kModelCannonRBall = 0x1f2, kModelNeedleBall = 0x1f0;
constexpr uint16_t kModelRocket = 0x196, kSoundRocketLaunch = 0x262;
constexpr uint16_t kModelWizardFire = 0x29d;
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
        // the map, objects, platforms, then player 1 and player 2
        return game_->level->Collision().IsBlockedFloat(pos_.x, pos_.z, pos_.y) || game_->statics->CollisionCheck(pos_) ||
               game_->platforms->CollisionCheck(pos_) || CollisionCheck_Hero(game_->heroA) ||
               CollisionCheck_Hero(game_->heroB);
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
               game_->platforms->CollisionCheck(pos_) || CollisionCheck_Hero(game_->heroA) ||
               CollisionCheck_Hero(game_->heroB);
    }
    float verticalSpeed_ = 0;
};
// XBullet_NeedleBall: the genie's spiked ball (radius 9, shadow 19). It falls (0.04 a tick, at
// most 4) onto the floor and rolls along its yaw at 0.5 x its speed factor, which grows by 0.01
// a tick while it touches the floor (from 1, at most 5), spinning to match. For its first 500
// ticks it turns towards the hero that was closer when it was fired (0.3 degrees a tick). Smoke
// every 15 ticks. After 20 ticks it bursts like the rolling cannonball; below -200 it is gone.
class NeedleBall : public XBulletBase {
public:
    NeedleBall(const XGameContext* g, const D3DVECTOR& p, float yaw) : XBulletBase(g, p) {
        model_ = std::make_unique<JObject3DX>(kModelNeedleBall);
        yaw_ = yaw;
        UpdateWorldMatrix();
        shadow_ = std::make_unique<XShadow>(19.0f);
        radius_ = 9.0f;
        targetHero_ = g->GetCloserHero(pos_);  // XGameObject::GetCloserHero
        liveState_ = NORMAL;
    }
    void ProcessFrame() override {
        XBulletBase::ProcessFrame();
        if (liveState_ != NORMAL) return;
        const D3DVECTOR prev = pos_;
        verticalSpeed_ -= 0.04f;
        if (verticalSpeed_ < -4.0f) verticalSpeed_ = -4.0f;
        float bottom = pos_.y + verticalSpeed_ - radius_;
        if (game_->level->Collision().CollisionCheckY(bottom, pos_.x, pos_.z)) {
            verticalSpeed_ = 0;
            speedFactor_ += 0.01f;
            if (speedFactor_ > 5.0f) speedFactor_ = 5.0f;
        }
        pos_.y = bottom + radius_;
        if (localCounter_ < 500 && targetHero_) {
            const D3DVECTOR& hp = targetHero_->GetPos();
            D3DVECTOR c(hp.x, hp.y + 14.5f, hp.z);  // XHeroBase::GetCenterPos
            yaw_ = MakeAngleCloseTo_Constant(yaw_, FindYaw(c.x - pos_.x, c.z - pos_.z), 0.00523599f);
        }
        pitch_ = NormaliseAngle(pitch_ + 0.0331613f * speedFactor_);
        pos_.x += 0.5f * speedFactor_ * std::sin(yaw_);
        pos_.z += 0.5f * speedFactor_ * std::cos(yaw_);
        if (localCounter_ % 15 == 0) game_->particles->CreateSmoke(pos_);
        if (localCounter_ > 20 && Hits()) {
            game_->particles->CreateBulletExplode(prev);
            liveState_ = NON;
            return;
        }
        if (pos_.y < -200.0f) {
            liveState_ = NON;
            return;
        }
        // on a water level, below -38: a wave (unless already below -43) and a burst
        if (game_->useWater && pos_.y < -38.0f) {
            if (pos_.y > -43.0f) game_->particles->CreateWaterWave(D3DVECTOR(pos_.x, -38.0f, pos_.z), false, true);
            game_->particles->CreateBulletExplode(prev);
            liveState_ = NON;
            return;
        }
        UpdateWorldMatrix();
    }

private:
    bool Hits() {
        return game_->level->Collision().IsBlockedFloat(pos_.x, pos_.z, pos_.y) || game_->statics->CollisionCheck(pos_) ||
               game_->platforms->CollisionCheck(pos_) || CollisionCheck_Hero(game_->heroA) ||
               CollisionCheck_Hero(game_->heroB);
    }
    float speedFactor_ = 1.0f, verticalSpeed_ = 0;
    XHero* targetHero_ = nullptr;
};

// XBullet_Rocket: the rocket launcher's rocket (radius 9, shadow 19), pointing straight up. MOVEUP:
// it rises 0.085 a tick out of the launcher for 500 ticks, turning (1.3 degrees a tick) to face
// the closer hero, bursting if it touches a hero. Then LAUNCH (sound 0x262, a puff of smoke): it
// flies at 3, slowing by 0.017 a tick to 0.9, steering towards the closer hero's centre (yaw 0.75
// degrees a tick for 450 ticks, pitch 1 degree a tick from tick 11 to 449), with smoke every 15
// ticks. It bursts like the rolling cannonball, and on its own after 3000 ticks.
class Rocket : public XBulletBase {
public:
    enum SubState { MOVEUP = 0, LAUNCH = 1 };
    Rocket(const XGameContext* g, const D3DVECTOR& p) : XBulletBase(g, p) {
        model_ = std::make_unique<JObject3DX>(kModelRocket);
        pitch_ = -1.5708f;
        UpdateWorldMatrix();
        shadow_ = std::make_unique<XShadow>(19.0f);
        radius_ = 9.0f;
        liveState_ = NORMAL;
    }
    void ProcessFrame() override {
        XBulletBase::ProcessFrame();
        if (liveState_ != NORMAL) return;
        if (subState_ == MOVEUP) MoveUp();
        else if (subState_ == LAUNCH) Launch();
        UpdateWorldMatrix();
    }

private:
    void MoveUp() {
        pos_.y += 0.085f;
        yaw_ = MakeAngleCloseTo_Constant(yaw_, game_->DirToCloserHero(pos_), 0.0226893f);
        if (CollisionCheck_Hero(game_->heroA) || CollisionCheck_Hero(game_->heroB)) {
            game_->particles->CreateBulletExplode(pos_);
            liveState_ = NON;
            return;
        }
        if (localCounter_ < 500) return;
        game_->playSound(kSoundRocketLaunch, pos_.z);
        game_->particles->CreateSmoke(pos_);
        speed_ = 3.0f;
        localCounter_ = 0;
        subState_ = LAUNCH;
    }
    void Launch() {
        const D3DVECTOR prev = pos_;
        XHero* hero = game_->GetCloserHero(pos_);
        if (hero) {
            const D3DVECTOR& hp = hero->GetPos();
            D3DVECTOR c(hp.x, hp.y + 14.5f, hp.z);  // XHeroBase::GetCenterPos
            if (localCounter_ < 450) yaw_ = MakeAngleCloseTo_Constant(yaw_, FindYaw(c.x - pos_.x, c.z - pos_.z), 0.01309f);
            if (localCounter_ > 10 && localCounter_ < 450)
                pitch_ = MakeValueCloseTo_Constant(pitch_, -FindPitch(pos_, c), 0.0174533f);
        }
        pos_.x += std::sin(yaw_) * speed_ * std::cos(-pitch_);
        pos_.z += std::cos(yaw_) * speed_ * std::cos(-pitch_);
        pos_.y += std::sin(-pitch_) * speed_;
        speed_ -= 0.017f;
        if (speed_ < 0.9f) speed_ = 0.9f;
        if (localCounter_ % 15 == 0) game_->particles->CreateSmoke(pos_);
        if (game_->level->Collision().IsBlockedFloat(pos_.x, pos_.z, pos_.y) || game_->statics->CollisionCheck(pos_) ||
            game_->platforms->CollisionCheck(pos_) || CollisionCheck_Hero(game_->heroA) ||
               CollisionCheck_Hero(game_->heroB)) {
            game_->particles->CreateBulletExplode(prev);
            liveState_ = NON;
        } else if (pos_.y < -200.0f) {
            liveState_ = NON;
        } else if (localCounter_ >= 3000) {
            game_->particles->CreateBulletExplode(prev);
            liveState_ = NON;
        } else if (game_->useWater && pos_.y < -38.0f) {
            // a wave (unless already below -43) and a burst
            if (pos_.y > -43.0f) game_->particles->CreateWaterWave(D3DVECTOR(pos_.x, -38.0f, pos_.z), false, true);
            game_->particles->CreateBulletExplode(prev);
            liveState_ = NON;
        }
    }
    SubState subState_ = MOVEUP;
    float speed_ = 0;
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
        if (frame_ < 9) {
            CollisionCheck_Hero(game_->heroA);
            CollisionCheck_Hero(game_->heroB);
        }
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

// XBullet_WizardFire: the wizard's fireball (model 0x29d, radius 9, shadow 18). It grows from
// nothing by 0.015 a tick and homes on the closer hero's centre, turning 1.2 degrees and pitching
// 0.8 degrees a tick; after 40 ticks it speeds up from 0.8 by 0.008 a tick to 3, with a puff of
// smoke every 12. The map, an object or a hero stops it with a burst where it was; below -200 it
// just goes; on a water level below the surface it makes a ring (unless already below -43) and
// bursts.
class WizardFire : public XBulletBase {
public:
    WizardFire(const XGameContext* g, const D3DVECTOR& p, float yaw) : XBulletBase(g, p) {
        model_ = std::make_unique<JObject3DX>(kModelWizardFire);
        yaw_ = yaw;
        model_->SetScale(0.0f);
        UpdateWorldMatrix();
        radius_ = 9.0f;
        shadow_ = std::make_unique<XShadow>(radius_ * 2.0f);
        speed_ = 0.8f;
        liveState_ = NORMAL;
    }
    void ProcessFrame() override {
        XBulletBase::ProcessFrame();
        if (liveState_ != NORMAL) return;
        const D3DVECTOR prev = pos_;
        XHero* hero = game_->GetCloserHero(pos_);
        if (localCounter_ > 40) {
            speed_ += 0.008f;
            if (speed_ > 3.0f) speed_ = 3.0f;
            if (localCounter_ % 12 == 0) game_->particles->CreateSmoke(pos_);
        }
        const D3DVECTOR& hp = hero->GetPos();
        D3DVECTOR c(hp.x, hp.y + 14.5f, hp.z);  // XHeroBase::GetCenterPos
        yaw_ = MakeAngleCloseTo_Constant(yaw_, FindYaw(c.x - pos_.x, c.z - pos_.z), 0.020944f);
        pitch_ = MakeValueCloseTo_Constant(pitch_, -FindPitch(pos_, c), 0.0139626f);
        pos_.x += std::sin(yaw_) * speed_ * std::cos(-pitch_);
        pos_.z += std::cos(yaw_) * speed_ * std::cos(-pitch_);
        pos_.y += std::sin(-pitch_) * speed_;
        scale_ += 0.015f;
        if (scale_ > 1.0f) scale_ = 1.0f;
        model_->SetScale(scale_);
        UpdateWorldMatrix();
        if (game_->level->Collision().IsBlockedFloat(pos_.x, pos_.z, pos_.y) || game_->statics->CollisionCheck(pos_) ||
            CollisionCheck_Hero(game_->heroA) || CollisionCheck_Hero(game_->heroB)) {
            game_->particles->CreateBulletExplode(prev);
            liveState_ = NON;
        } else if (pos_.y < -200.0f) {
            liveState_ = NON;
        } else if (game_->useWater && pos_.y < -38.0f) {
            if (pos_.y > -43.0f) game_->particles->CreateWaterWave(D3DVECTOR(pos_.x, -38.0f, pos_.z), false, true);
            game_->particles->CreateBulletExplode(prev);
            liveState_ = NON;
        }
    }

private:
    float speed_ = 0, scale_ = 0;
};
}  // namespace

// --------------------------------------------------------------------------------- XBulletBase

XBulletBase::XBulletBase(const XGameContext* game, const D3DVECTOR& pos) : game_(game), pos_(pos) {}
XBulletBase::~XBulletBase() = default;

void XBulletBase::Render(Device* d, const D3DMATRIX&) {
    if (liveState_ == NON) return;
    if (model_) model_->Render(d);
    if (shadow_) {
        // (native: where the model is drawn)
        shadow_->SetPos(model_ ? pos_ + model_->BlendOffset() : pos_);
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
void XBulletList::CreateNeedleBall(const D3DVECTOR& pos, float yaw) {
    int i = GetNullIndex();
    if (i >= 0) slots_[i] = std::make_unique<NeedleBall>(game_, pos, yaw);
}
void XBulletList::CreateRocket(const D3DVECTOR& pos) {
    int i = GetNullIndex();
    if (i >= 0) slots_[i] = std::make_unique<Rocket>(game_, pos);
}

void XBulletList::CreateWizardFire(const D3DVECTOR& pos, float yaw) {
    int i = GetNullIndex();
    if (i >= 0) slots_[i] = std::make_unique<WizardFire>(game_, pos, yaw);
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
