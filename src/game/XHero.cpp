#include "XHero.h"
#include "game/XMapCollision.h"
#include "game/XMath.h"
#include <cmath>

namespace {
// XHeroA::XHeroA: MageA01, then frames MageA02, MageA01, MageA03, MageAJump, MageAFall;
// XHeroB::XHeroB: the same frames of MageB
const uint16_t kBodyFirstA = 0x163;
const uint16_t kBodyFramesA[] = {0x164, 0x163, 0x165, 0x167, 0x166};
const uint16_t kBodyFirstB = 0x168;
const uint16_t kBodyFramesB[] = {0x169, 0x168, 0x16a, 0x16c, 0x16b};
// XHeroBase::XHeroBase: the wand, two frames (down, raised) per wand type
const uint16_t kWandFirst = 0x1b7;
const uint16_t kWandFrames[] = {0x1b8, 0x1bf, 0x1c3, 0x1c1, 0x1c2, 0x1bd, 0x1be, 0x1c4, 0x1c5, 0x1c6, 0x1c7};

// Sounds (WAVE resources)
const uint16_t kSoundDamage = 0x263;
const uint16_t kSoundJump = 0x266;
const uint16_t kSoundFireA = 0x269;
const uint16_t kSoundBurst = 0x26e;
const uint16_t kSoundWarp = 0x272;
const uint16_t kSoundDie = 0x27e;
const uint16_t kSoundLand = 0x2ad;

constexpr float kGravity = 0.08f;
constexpr float kMaxFallSpeed = -4.0f;
constexpr float kJumpSpeed = 3.5f;
constexpr float kWalkSpeed = 1.2f;
constexpr float kQuarterPi = 0.785398f;
constexpr float kTurnMinStep = 0.0523599f;  // 3 degrees
constexpr float kPitchJump = 0.139626f;      // 8 degrees
constexpr float kPitchFall = -0.279253f;     // -16 degrees
constexpr float kPitchWarp = -0.174533f;     // -10 degrees
constexpr float kDegree = 0.0174533f;

// XHeroBase::UpdateMagicLevel: ticks per recovered magic point, by magic level
const int kRecoveryFrames[8] = {80, 70, 60, 50, 40, 32, 24, 16};
}  // namespace

XHero::XHero(bool heroA) : heroA_(heroA), box_(26.0f, 29.0f) {
    body_ = std::make_unique<JObject3DX>(heroA ? kBodyFirstA : kBodyFirstB);
    for (uint16_t f : heroA ? kBodyFramesA : kBodyFramesB) body_->AppendFrame(f);
    body_->SetCollisionModel(0x179);  // MageCOL: what enemies and coins touch
    wand_ = std::make_unique<JObject3DX>(kWandFirst);
    for (uint16_t f : kWandFrames) wand_->AppendFrame(f);
}

void XHero::SetPosX(float x) {
    pos_.x = x;
    box_.SetPosX(x);
}
void XHero::SetPosY(float y) {
    pos_.y = y;
    box_.SetPosY(y);
}
void XHero::SetPosZ(float z) {
    pos_.z = z;
    box_.SetPosZ(z);
}

void XHero::UpdateMagicLevel() {
    if (status_->magicLevel < 0) status_->magicLevel = 0;
    else if (status_->magicLevel > 7) status_->magicLevel = 7;
    mpRecoveryFrames_ = kRecoveryFrames[status_->magicLevel];
}

void XHero::Reset() {
    UpdateMagicLevel();
    pos_ = startPos_;
    box_.SetPos(pos_);
    SetPosY(800.0f);  // the hero drops into the level
    yaw_ = 0;
    pitch_ = 0;
    UpdateWorldMatrix();
    wandOut_ = false;
    beingKnockBack_ = false;
    knockBackDir_ = 0;
    knockBackSpeed_ = 0;
    push_ = D3DVECTOR(0, 0, 0);
    pushedByFan_ = false;
    heroAbove_ = false;
    verticalSpeed_ = 0;
    warpRotSpeed_ = 0;
    body_->SetCurrentFrame(0);
    wand_->SetCurrentFrame(status_->wandType * 2);
    mpRecoveryCounter_ = 0;
    damageCounter_ = 0;
    // native: the original's Reset leaves the flash flag alone (its heroes are rebuilt for each
    // level); this port reuses the hero, so a level restarted mid-flash must not stay white
    body_->UseWhiteMaterial(false);
    wand_->UseWhiteMaterial(false);
    walkCounter_ = 0;
    localCounter_ = 0;
    jumpState_ = LAND;
    liveState_ = NORMAL;
}

void XHero::UpdateWorldMatrix() {
    for (JObject3DX* m : {body_.get(), wand_.get()}) {
        m->SetPitch(pitch_);
        m->SetYaw(yaw_);
        m->SetPos(pos_);
        m->UpdateWorldMatrix();
    }
}

int XHero::GetKeyDir(const XHeroInput& in) const {
    int dir = 0;
    if (in.keyRight) {
        if (in.keyUp) dir = 7;
        else if (in.keyDown) dir = 1;
    } else if (in.keyLeft) {
        dir = 4;
        if (in.keyUp) dir = 5;
        else if (in.keyDown) dir = 3;
    } else if (in.keyUp) {
        dir = 6;
    } else if (in.keyDown) {
        dir = 2;
    }
    return dir;
}

void XHero::Damage(XHeroWorld& world) {
    if (damageCounter_ > 0) return;  // still flashing from the last hit
    world.PlaySound(kSoundDamage, pos_.z);
    status_->life--;
    if (status_->life > 0) {
        damageCounter_ = 400;
    } else {
        localCounter_ = 0;
        liveState_ = DYING;
    }
}

void XHero::KnockBack(const D3DVECTOR& from) {
    knockBackSpeed_ = beingKnockBack_ ? 1.6f : 2.4f;
    // FindDirectionXZ(from, pos): away from the source
    knockBackDir_ = FindYaw(pos_.x - from.x, pos_.z - from.z);
    beingKnockBack_ = true;
}

void XHero::StartDying() {
    localCounter_ = 0;
    liveState_ = DYING;
}

void XHero::ProcessFrame(const XHeroInput& in, const MapCollision& map, XHeroWorld& world, XHero* other) {
    if (liveState_ == NON || liveState_ == DEAD || liveState_ == COMPLETED) return;
    localCounter_++;
    if (damageCounter_ > 0) {
        damageCounter_--;
        // the damage flash: white for 12 ticks, normal for 12, until the counter runs out
        bool white = damageCounter_ > 0 && (damageCounter_ / 12) % 2 == 0;
        body_->UseWhiteMaterial(white);
        wand_->UseWhiteMaterial(white);
    }
    if (liveState_ == NORMAL) ProcessNormal(in, map, world, other);
    else if (liveState_ == WARP) ProcessWarp(world);
    else if (liveState_ == DYING) ProcessDying(world);
}

void XHero::ProcessNormal(const XHeroInput& in, const MapCollision& map, XHeroWorld& world, XHero* other) {
    // new position = old + push (cleared once used); the old one is kept to carry the other hero
    const D3DVECTOR oldPos = pos_;
    D3DVECTOR np(pos_.x + push_.x, pos_.y + push_.y, pos_.z + push_.z);
    push_ = D3DVECTOR(0, 0, 0);

    // knockback
    if (beingKnockBack_) {
        np.x += std::sin(knockBackDir_) * knockBackSpeed_;
        np.z += std::cos(knockBackDir_) * knockBackSpeed_;
        yaw_ = MakeAngleCloseTo(yaw_, knockBackDir_ + kPi, 0.1f, 0.0f, kFloatMax);
        knockBackSpeed_ -= 0.06f;
        if (knockBackSpeed_ <= 0.0f) beingKnockBack_ = false;
    }

    // steering, once knockback has slowed below 1
    if (knockBackSpeed_ < 1.0f) {
        float strength = 0;
        if (in.keyControlUsed) {
            strength = 0.9f;
        } else {
            strength = std::sqrt(in.valueX * in.valueX + in.valueY * in.valueY);
            if (strength > 0.9f) strength = 0.9f;
        }
        if (strength >= 0.5f) {
            float dir = in.keyControlUsed ? GetKeyDir(in) * kQuarterPi : FindYaw(in.valueY, in.valueX);
            yaw_ = MakeAngleCloseTo(yaw_, dir, 0.15f, kTurnMinStep, kFloatMax);
            float speed = ((strength - 0.5f) / 0.4f) * 0.8f + 0.2f;
            np.x += kWalkSpeed * speed * std::sin(dir);
            np.z += kWalkSpeed * speed * std::cos(dir);
            walkCounter_++;
        } else {
            walkCounter_ = 0;
        }
    } else {
        walkCounter_ = 0;
    }

    // gravity
    verticalSpeed_ -= kGravity;
    if (verticalSpeed_ < kMaxFallSpeed) verticalSpeed_ = kMaxFallSpeed;
    np.y += verticalSpeed_;

    // X, then Z: the other hero, static objects, candles, platforms, then the map
    if (other) other->CollisionCheckX(np.x, box_);
    world.StaticCollisionX(np.x, box_);
    world.CandleCollisionX(np.x, box_);
    world.PlatformCollisionX(np.x, box_);
    map.CollisionCheckX(np.x, box_);
    SetPosX(np.x);
    if (other) other->CollisionCheckZ(np.z, box_);
    world.StaticCollisionZ(np.z, box_);
    world.CandleCollisionZ(np.z, box_);
    world.PlatformCollisionZ(np.z, box_);
    map.CollisionCheckZ(np.z, box_);
    SetPosZ(np.z);
    // a hero standing on this one moves with it, next tick (everything this one moved, its own
    // push included; the X and Z only)
    if (heroAbove_ && other) {
        other->IncreasePushX(pos_.x - oldPos.x);
        other->IncreasePushZ(pos_.z - oldPos.z);
    }

    // Y. Falling: the other hero, static objects, platforms, the map, candles; landing on a
    // platform marks it, and the other hero learns whether this one stands on it (not while a
    // floor fan lifts this one: the box still stops it, but it doesn't count as standing).
    // Rising: the other hero, static objects, candles; a platform's underside does not stop a jump.
    bool landed = false, headHit = false;
    if (verticalSpeed_ < 0.0f) {
        bool onHero = other && other->CollisionCheckY(np.y, box_) && !pushedByFan_;
        bool onStatic = world.StaticCollisionY(np.y, box_);
        bool onPlatform = world.PlatformCollisionY(np.y, box_);
        bool onMap = map.CollisionCheckY(np.y, box_);
        bool onCandle = world.CandleCollisionY(np.y, box_);
        landed = onHero || onStatic || onPlatform || onMap || onCandle;
        if (other) other->SetHeroAbove(onHero);
        if (onPlatform) world.SetHeroAboveLastPlatform(heroA_);  // SetHeroAAbove / SetHeroBAbove
    } else {
        bool onHero = other && other->CollisionCheckY(np.y, box_);
        bool onStatic = world.StaticCollisionY(np.y, box_);
        bool onCandle = world.CandleCollisionY(np.y, box_);
        headHit = onHero || onStatic || onCandle;
        if (other) other->SetHeroAbove(false);
    }
    SetPosY(np.y);

    // the wand: a fresh press with the wand down and magic to spare fires
    if (in.button1Pressed && !wandOut_ && status_->magicPoint > 0) {
        world.PlaySound(kSoundFireA, pos_.z);
        status_->magicPoint--;
        mpRecoveryCounter_ = 0;
        world.CreateFire(*this);
    }
    wandOut_ = in.button1Down;
    if (++mpRecoveryCounter_ > mpRecoveryFrames_) {
        mpRecoveryCounter_ = 0;
        if (++status_->magicPoint > 10) status_->magicPoint = 10;
    }

    // jumping
    if (jumpState_ == LAND) {
        pitch_ = MakeAngleCloseTo(pitch_, 0.0f, 0.2f, 0.0f, kFloatMax);
        if (landed) {
            verticalSpeed_ = 0;
            if (in.button2Pressed) {
                world.PlaySound(kSoundJump, pos_.z);
                verticalSpeed_ = kJumpSpeed;
                jumpState_ = JUMP;
            }
        } else {
            jumpState_ = FALL;
        }
    } else if (jumpState_ == JUMP) {
        pitch_ = MakeAngleCloseTo(pitch_, kPitchJump, 0.1f, 0.0f, kFloatMax);
        if (headHit) {
            if (verticalSpeed_ > 0.0f) verticalSpeed_ = 0;
            jumpState_ = FALL;
        } else if (verticalSpeed_ <= 0.0f) {
            jumpState_ = FALL;
        }
    } else if (jumpState_ == FALL) {
        pitch_ = MakeAngleCloseTo(pitch_, kPitchFall, 0.1f, 0.0f, kFloatMax);
        if (landed) {
            if (verticalSpeed_ <= kMaxFallSpeed) {  // a hard landing: a thud and a ring of dust
                world.PlaySound(kSoundLand, pos_.z);
                world.CreateLandEffect(D3DVECTOR(pos_.x, pos_.y + 0.5f, pos_.z));
            }
            verticalSpeed_ = 0;
            jumpState_ = LAND;
        }
    }
    UpdateWorldMatrix();

    // falling out of the level: below -200, or on a water level below -58 with a large wave
    // (and its sound) on the surface
    if (!world.UseWater() && pos_.y < -200.0f) {
        localCounter_ = 0;
        liveState_ = DYING;
        return;
    }
    if (world.UseWater() && pos_.y < -58.0f) {
        world.CreateWaterWave(D3DVECTOR(pos_.x, -38.0f, pos_.z), true, true);
        localCounter_ = 0;
        liveState_ = DYING;
        return;
    }
    // the warp: standing at the open goal
    if (world.ReadyForWarp() && ApproxDistance(pos_, world.GoalPos()) < 20.0f) {
        world.CloseWarpZone();
        localCounter_ = 0;
        liveState_ = WARP;
        return;
    }
    pushedByFan_ = false;
}

void XHero::ProcessWarp(XHeroWorld& world) {
    if (localCounter_ == 1) {
        world.PlaySound(kSoundWarp, pos_.z);
        verticalSpeed_ = 0.8f;
        warpRotSpeed_ = 8.0f;
        jumpState_ = FALL;
    }
    pitch_ = MakeAngleCloseTo(pitch_, kPitchWarp, 0.05f, 0.0f, kFloatMax);
    yaw_ = NormaliseAngle(yaw_ - kDegree * warpRotSpeed_);
    SetPosY(pos_.y + verticalSpeed_);
    UpdateWorldMatrix();
    if (localCounter_ % 25 == 1) world.CreateHeroWarpPhoton(heroA_);
    if (pos_.y > 320.0f) {
        localCounter_ = 0;
        liveState_ = COMPLETED;
    }
}

void XHero::ProcessDying(XHeroWorld& world) {
    if (localCounter_ == 1) {
        world.PlaySound(kSoundDie, pos_.z);
        pitch_ = 0;
        verticalSpeed_ = 5.0f;
        jumpState_ = LAND;
    }
    SetPosY(pos_.y + verticalSpeed_);
    yaw_ = NormaliseAngle(yaw_ + 0.244346f);
    verticalSpeed_ -= kGravity;
    if (verticalSpeed_ < -3.0f) {
        world.CreateEnemyExplode(D3DVECTOR(pos_.x, pos_.y + 14.5f, pos_.z), 1);  // GetCenterPos
        // XHeroFireList::Create8Way: twelve shots, 30 degrees apart starting at 15
        for (int i = 0; i < 12; i++) world.CreateFireAt(*this, (i * 30.0f + 15.0f) * 0.0174533f);
        world.PlaySound(kSoundBurst, pos_.z);
        // the gold lost: everything up to (5 - life) * 3 (none on a boss level), plus a tenth of
        // the rest
        int base = world.BossLevel() ? 0 : (5 - status_->life) * 3;
        int lost = status_->gold <= base ? status_->gold : (int)((status_->gold - base) * 0.1f) + base;
        int gold = status_->gold - lost;  // XHeroBase::DecreaseGold
        status_->gold = (int16_t)(gold < 0 ? 0 : gold);
        world.ShowLostGoldMessage(heroA_, lost);
        status_->life = 5;
        status_->magicPoint = 10;
        localCounter_ = 0;
        liveState_ = DEAD;
        return;
    }
    UpdateWorldMatrix();
}

void XHero::Render(Device* d) {
    if (liveState_ == NON || liveState_ == DEAD || liveState_ == COMPLETED) return;
    if (jumpState_ == JUMP) {
        body_->SetCurrentFrame(4);
    } else if (jumpState_ == FALL) {
        body_->SetCurrentFrame(5);
    } else if (walkCounter_ == 0) {
        body_->SetCurrentFrame(0);
    } else {
        body_->SetCurrentFrame((int)(((walkCounter_ - 1) / 9 + 1) % 4));
    }
    wand_->SetCurrentFrame(status_->wandType * 2 + (wandOut_ ? 1 : 0));
    body_->Render(d);
    wand_->Render(d);
    // (the shadow: chunk 4c)
}
