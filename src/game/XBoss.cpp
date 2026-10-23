#include "game/XBoss.h"
#include "framework/d3dutil.h"
#include "game/JModel.h"
#include "game/XBullet.h"
#include "game/XGameContext.h"
#include "game/XHero.h"
#include "game/XLevelView.h"
#include "game/XMath.h"
#include "game/XParticle.h"
#include "game/XStaticObj.h"
#include <cfloat>
#include <cmath>

namespace {
constexpr uint16_t kSoundHurt = 0x270, kSoundBurst = 0x26b, kSoundFinalBurst = 0x27c, kSoundFlame = 0x27a;
constexpr uint16_t kModelDragonHead = 0x1d2, kModelDragonBody = 0x1d3, kModelDragonTail = 0x1d1;
constexpr uint16_t kColDragonHead = 0x20b, kColDragonBody = 0x20c, kColDragonTail = 0x209;
constexpr uint16_t kModelGenie = 0x1ee, kColGenie = 0x1eb;
constexpr uint16_t kSoundGenieAppear = 0x2a1, kSoundGenieFire = 0x2a2, kSoundGenieSteam = 0x2a3,
                   kSoundGenieMove = 0x2a4;

// FindDirectionXZ: the yaw from `from` towards `to`
float FindDirectionXZ(const D3DVECTOR& from, const D3DVECTOR& to) { return FindYaw(to.x - from.x, to.z - from.z); }
}  // namespace

// ----------------------------------------------------------------------------------- XBossBase

XBossBase::XBossBase(const XGameContext* game, const D3DVECTOR& pos)
    : game_(game), startPos_(pos), pos_(pos), explodeCenterPos_(pos) {}
XBossBase::~XBossBase() = default;

void XBossBase::Render(Device* d) {
    if (liveState_ == NON || liveState_ == DEAD) return;
    model_->SetCurrentFrame(frame_);
    model_->Render(d);
    if (shadow_) {
        shadow_->SetScale(scale_);
        shadow_->SetPosXZ(pos_);
        shadow_->Render(d);
    }
}

void XBossBase::ProcessFrame() {
    // (the base boss's dying: explosions all over it for 300 ticks; the dragon has its own)
    localCounter_++;
    if (liveState_ == NON) return;
    CheckBattleStarted();
    ProcessDamage();
    if (liveState_ != DEAD) return;
    if (localCounter_ == 1) game_->particles->CreateEnemyExplode(explodeCenterPos_, true);
    if (localCounter_ % 5 == 1) {
        float r = RandomF(0.2f, 1.0f) * explodeRadius_;
        float a = UnitRand() * 6.28319f;
        float b = UnitRand() * 3.14159f - 1.5708f;
        D3DVECTOR p = explodeCenterPos_;
        p.x += std::sin(a) * r * std::cos(b);
        p.z += std::cos(a) * r * std::cos(b);
        p.y += std::sin(b) * r;
        game_->particles->CreateEnemyExplode(p, false);
        if (localCounter_ % 15 == 1) game_->playSound(kSoundBurst, p.z);
    }
    if (localCounter_ >= 300) {
        if (game_->playMono) game_->playMono(kSoundFinalBurst, 1.0f);
        localCounter_ = 0;
        liveState_ = NON;
    }
}

bool XBossBase::CollisionCheck_Fire(const D3DVECTOR& pos, float radius) {
    if (0 >= scale_ || liveState_ != NORMAL) return false;
    if (std::fabs(pos_.z - pos.z) > collideBox_->WidthZ() + radius) return false;
    if (std::fabs(pos_.x - pos.x) > collideBox_->WidthZ() + radius) return false;  // (Z width, as the original)
    if (!model_->CheckCollision_Sphere(pos, radius)) return false;
    Damage();
    return true;
}

void XBossBase::Damage() {
    if (!damagable_) return;
    game_->playSound(kSoundHurt, pos_.z);
    damageCounter_ = 12;
    model_->UseWhiteMaterial(true);
    if (--healthSub_ > 0) return;
    if (--healthMain_ > 0) {
        healthSub_ = healthSubMax_;
        return;
    }
    // defeated: the music stops (JMidiList::Stop) and the dying begins where it is
    if (game_->stopMusic) game_->stopMusic();
    explodeCenterPos_ = pos_;
    localCounter_ = 0;
    liveState_ = DEAD;
}

void XBossBase::CheckBattleStarted() {
    if (!battleStarted_) battleStarted_ = true;
}

void XBossBase::ProcessDamage() {
    if (damageCounter_ > 0 && --damageCounter_ <= 0) model_->UseWhiteMaterial(false);
}

bool XBossBase::CollisionCheck_Hero(XHero* hero) {
    if (0 >= scale_ || liveState_ != NORMAL || !hero || hero->GetLiveState() != XHero::NORMAL) return false;
    const D3DVECTOR& hp = hero->GetPos();
    if (std::fabs(pos_.z - hp.z) > collideBox_->WidthHalfZ() + hero->GetCollideBox().WidthHalfZ()) return false;
    if (std::fabs(pos_.x - hp.x) > collideBox_->WidthHalfX() + hero->GetCollideBox().WidthHalfX()) return false;
    if (!model_->CheckCollision(hero->BodyModel())) return false;
    hero->Damage(*game_->heroWorld);
    hero->KnockBack(pos_);
    return true;
}

void XBossBase::SetPos(const D3DVECTOR& p) {
    pos_ = p;
    collideBox_->SetPos(p);
}

void XBossBase::UpdateWorldMatrix() {
    model_->SetScale(scale_);
    model_->SetRoll(roll_);
    model_->SetPitch(pitch_);
    model_->SetYaw(yaw_);
    model_->SetPos(pos_);
    model_->UpdateWorldMatrix();
}

// ---------------------------------------------------------------------------- XBoss_DragonBody

XBoss_DragonBody::XBoss_DragonBody(const XGameContext* game, XBoss_Dragon* head, int id)
    : XBossBase(game, head->GetPos()), id_(id), head_(head) {
    if (id_ >= 1 && id_ < 16) {
        model_ = std::make_unique<JObject3DX>(kModelDragonBody);
        model_->SetCollisionModel(kColDragonBody);
        nextBody_ = std::make_unique<XBoss_DragonBody>(game, head, id_ + 1);
    } else if (id_ == 16) {
        model_ = std::make_unique<JObject3DX>(kModelDragonTail);
        model_->SetCollisionModel(kColDragonTail);
    }
    bodyShadow_ = std::make_unique<XShadow>(38.0f);
    collideBox_ = std::make_unique<XCollideBox>(45.0f, 40.0f);
    startPos_ = head->StartPos();
    SetPos(head->GetPos());
    yaw_ = head->YawRec(0);
    pitch_ = head->PitchRec(0);
    UpdateWorldMatrix();
    damagable_ = false;
    liveState_ = NORMAL;
}
XBoss_DragonBody::~XBoss_DragonBody() = default;

void XBoss_DragonBody::StartDead() {
    localCounter_ = 0;
    liveState_ = DEAD;
}

void XBoss_DragonBody::ProcessFrame() {
    if (nextBody_) nextBody_->ProcessFrame();
    localCounter_++;
    if (liveState_ == NON) return;
    if (liveState_ == DEAD) {
        // one segment after another bursts, 16 ticks apart; the tail ends it with 0x27c
        if (localCounter_ == 1) {
            game_->particles->CreateEnemyExplode(pos_, false);
            game_->playSound(kSoundBurst, pos_.z);
        }
        if (localCounter_ > 16) {
            if (nextBody_) nextBody_->StartDead();
            else if (game_->playMono) game_->playMono(kSoundFinalBurst, 1.0f);
            localCounter_ = 0;
            liveState_ = NON;
        }
        return;
    }
    if (head_->GetLiveState() != NORMAL) return;
    // the head's record 28 x id ticks ago
    int i = (head_->RecIndex() - id_ * 28 + XBoss_Dragon::kRecords) % XBoss_Dragon::kRecords;
    const D3DVECTOR prev = pos_;
    SetPos(head_->PosRec(i));
    yaw_ = head_->YawRec(i);
    pitch_ = head_->PitchRec(i);
    head_->CheckSplash(prev, pos_, false);
    UpdateWorldMatrix();
    CollisionCheck_Hero(game_->heroA);
}

void XBoss_DragonBody::Render(Device* d) {
    if (nextBody_) nextBody_->Render(d);
    if (liveState_ == NON || liveState_ == DEAD) return;
    model_->Render(d);
    bodyShadow_->SetPos(pos_);
    bodyShadow_->Render(d, game_->level->Collision());
}

bool XBoss_DragonBody::CollisionCheck_Fire(const D3DVECTOR& pos, float radius) {
    if (XBossBase::CollisionCheck_Fire(pos, radius)) return true;
    return nextBody_ && nextBody_->CollisionCheck_Fire(pos, radius);
}

// -------------------------------------------------------------------------------- XBoss_Dragon

XBoss_Dragon::XBoss_Dragon(const XGameContext* game, const D3DVECTOR& pos) : XBossBase(game, pos), prevPos_(pos) {
    model_ = std::make_unique<JObject3DX>(kModelDragonHead);
    model_->AppendFrame(0x20a);  // jaw opening
    model_->AppendFrame(0x20d);  // jaw open
    model_->SetCollisionModel(kColDragonHead);
    bodyShadow_ = std::make_unique<XShadow>(38.0f);
    collideBox_ = std::make_unique<XCollideBox>(60.0f, 40.0f);
    InitFloorRefPos();
    InitSwimRefPos1();
    InitSwimRefPos2();
    SetPos(swimRefPos1_[0]);
    yaw_ = 0;
    pitch_ = 0;
    UpdateWorldMatrix();
    for (int i = 0; i < kRecords; i++) {
        posRec_[i] = pos_;
        yawRec_[i] = yaw_;
        pitchRec_[i] = pitch_;
    }
    firstBody_ = std::make_unique<XBoss_DragonBody>(game, this, 1);
    healthSubMax_ = 4;
    healthSub_ = healthSubMax_;
    liveState_ = NORMAL;
    moveState_ = SWIM1;
    // (World 4: 6 hits a heart from the second heart on; the first keeps 4, as the original)
    if (game->world == 3) healthSubMax_ = 6;
}
XBoss_Dragon::~XBoss_Dragon() = default;

void XBoss_Dragon::InitFloorRefPos() {
    // the floor points: 3 columns (x - 80, x, x + 80) from 200 ahead of the start, 80 apart
    // along Z (the middle one 40 further on), all 20 up
    D3DVECTOR* f = floorRefPos_;
    f[0] = D3DVECTOR(startPos_.x - 80.0f, 20.0f, startPos_.z + 200.0f);
    for (int i = 1; i <= 3; i++) f[i] = D3DVECTOR(f[i - 1].x, 20.0f, f[i - 1].z + 80.0f);
    f[4] = D3DVECTOR(f[0].x + 80.0f, 20.0f, f[0].z + 40.0f);
    for (int i = 5; i <= 6; i++) f[i] = D3DVECTOR(f[i - 1].x, 20.0f, f[i - 1].z + 80.0f);
    f[7] = D3DVECTOR(f[0].x + 160.0f, 20.0f, f[0].z);
    for (int i = 8; i <= 10; i++) f[i] = D3DVECTOR(f[i - 1].x, 20.0f, f[i - 1].z + 80.0f);
}

namespace {
// _InitSwimRefPos1 / 2: a pool of 8 points round a centre, alternately 30 and 70 under the floor
void InitSwimRefPos(D3DVECTOR* s, float cx, float cz) {
    s[0] = D3DVECTOR(cx - 80.0f, -30.0f, cz - 80.0f);
    s[1] = D3DVECTOR(cx - 80.0f, -70.0f, cz);
    s[2] = D3DVECTOR(cx - 80.0f, -30.0f, cz + 80.0f);
    s[3] = D3DVECTOR(cx, -70.0f, cz - 160.0f);
    s[4] = D3DVECTOR(cx, -70.0f, cz + 160.0f);
    s[5] = D3DVECTOR(cx + 80.0f, -30.0f, cz - 80.0f);
    s[6] = D3DVECTOR(cx + 80.0f, -70.0f, cz);
    s[7] = D3DVECTOR(cx + 80.0f, -30.0f, cz + 80.0f);
}
}  // namespace

void XBoss_Dragon::InitSwimRefPos1() { InitSwimRefPos(swimRefPos1_, startPos_.x, startPos_.z); }
void XBoss_Dragon::InitSwimRefPos2() { InitSwimRefPos(swimRefPos2_, startPos_.x, startPos_.z + 640.0f); }

D3DVECTOR XBoss_Dragon::GetRandomCenterPos() const {
    // the middle floor point, or 40 behind it, 40 in front or (twice as likely) 80 in front
    D3DVECTOR p = floorRefPos_[5];
    switch (Random(5)) {
        case 1: p.x -= 40.0f; break;
        case 2: p.x += 40.0f; break;
        case 3: p.x += 80.0f; break;
        case 4: p.x += 80.0f; break;
        default: break;
    }
    return p;
}

void XBoss_Dragon::CheckBattleStarted() {
    // a hero passing 160 ahead of the start (swimRefPos1[4])
    if (battleStarted_) return;
    XHero* a = game_->heroA;
    if (a && a->GetLiveState() == XHero::NORMAL && a->GetPos().z > swimRefPos1_[4].z) battleStarted_ = true;
    // (or player 2: stage 5)
}

void XBoss_Dragon::CheckSplash(const D3DVECTOR& a, const D3DVECTOR& b, bool sound) const {
    if (!game_->useWater) return;
    if ((a.y < -38.0f) == (b.y < -38.0f)) return;
    game_->particles->CreateWaterWave(D3DVECTOR(b.x, -38.0f, b.z), true, sound);
}

void XBoss_Dragon::StartFlame() {
    flameState_ = STARTFLAME;
    flameCounter_ = 0;
}

void XBoss_Dragon::EndFlame() {
    flameState_ = ENDFLAME;
    flameCounter_ = 0;
}

bool XBoss_Dragon::FollowSpline() {
    travel_ += travelSpeed_;
    if (travel_ > 1.0f) travel_ = 1.0f;
    SetPos(spline_.GetPosition(travel_));
    return travel_ >= 1.0f;
}

void XBoss_Dragon::Swim1() {
    // round the first pool (a closed curve), until the battle has started
    if (localCounter_ == 1) {
        const D3DVECTOR* s = swimRefPos1_;
        spline_.Init(9);
        for (int i : {0, 1, 2, 4, 7, 6, 5, 3, 0}) spline_.AddNode(s[i]);
        spline_.MergeEnds();
        travel_ = 0;
        travelSpeed_ = 1.0f / spline_.GetTotalDistance();
    }
    if (FollowSpline()) {
        spline_.Deinit();
        localCounter_ = 0;
        if (battleStarted_) moveState_ = Random(2) == 0 ? FLOORWAVE1 : FLOORSTRAIGHT1;
        else moveState_ = SWIM1;
    }
}

void XBoss_Dragon::Swim2() {
    // round the second pool, then back over the floor
    if (localCounter_ == 1) {
        const D3DVECTOR* s = swimRefPos2_;
        spline_.Init(9);
        for (int i : {2, 1, 0, 3, 5, 6, 7, 4, 2}) spline_.AddNode(s[i]);
        spline_.MergeEnds();
        travel_ = 0;
        travelSpeed_ = 1.0f / spline_.GetTotalDistance();
    }
    if (FollowSpline()) {
        spline_.Deinit();
        localCounter_ = 0;
        moveState_ = Random(2) == 0 ? FLOORWAVE2 : FLOORSTRAIGHT2;
    }
}

void XBoss_Dragon::FloorWave1() {
    // out of the first pool, zigzagging across the floor (back, front, back, front), into the second
    if (localCounter_ == 1) {
        D3DVECTOR leap = swimRefPos1_[2];
        leap.z += 60.0f;
        leap.y = 80.0f;
        const D3DVECTOR* f = floorRefPos_;
        const D3DVECTOR* s = swimRefPos2_;
        spline_.Init(12);
        for (const D3DVECTOR& p : {pos_, swimRefPos1_[1], leap, f[0], f[8], f[2], f[10], s[5], s[6], s[7], s[4], s[2]})
            spline_.AddNode(p);
        travel_ = 0;
        travelSpeed_ = 1.0f / spline_.GetTotalDistance();
    }
    if (FollowSpline()) {
        spline_.Deinit();
        localCounter_ = 0;
        moveState_ = SWIM2;
    }
}

void XBoss_Dragon::FloorWave2() {
    // the same back from the second pool to the first
    if (localCounter_ == 1) {
        D3DVECTOR leap = swimRefPos2_[0];
        leap.z -= 60.0f;
        leap.y = 80.0f;
        const D3DVECTOR* f = floorRefPos_;
        const D3DVECTOR* s = swimRefPos1_;
        spline_.Init(12);
        for (const D3DVECTOR& p : {pos_, swimRefPos2_[1], leap, f[3], f[9], f[1], f[7], s[7], s[6], s[5], s[3], s[0]})
            spline_.AddNode(p);
        travel_ = 0;
        travelSpeed_ = 1.0f / spline_.GetTotalDistance();
    }
    if (FollowSpline()) {
        spline_.Deinit();
        localCounter_ = 0;
        moveState_ = SWIM1;
    }
}

void XBoss_Dragon::FloorStraight1() {
    // under the start, up through the middle of the floor breathing fire (from 20% to 50% of
    // the way), and down into the second pool
    if (localCounter_ == 1) {
        D3DVECTOR under(startPos_.x, -70.0f, startPos_.z);
        D3DVECTOR rise = swimRefPos1_[4];
        rise.y = 60.0f;
        D3DVECTOR land = swimRefPos2_[3];
        land.y = 20.0f;
        D3DVECTOR dive(startPos_.x, -70.0f, startPos_.z + 640.0f);
        D3DVECTOR centre = GetRandomCenterPos();
        const D3DVECTOR* f = floorRefPos_;
        const D3DVECTOR* s = swimRefPos2_;
        spline_.Init(12);
        for (const D3DVECTOR& p : {pos_, swimRefPos1_[1], under, rise, f[4], centre, f[6], land, dive, s[7], s[4], s[2]})
            spline_.AddNode(p);
        travel_ = 0;
        travelSpeed_ = 1.0f / spline_.GetTotalDistance();
    }
    float before = travel_;
    bool end = FollowSpline();
    if (0.2f > before && travel_ >= 0.2f) StartFlame();
    if (0.5f > before && travel_ >= 0.5f) EndFlame();
    if (end) {
        spline_.Deinit();
        localCounter_ = 0;
        moveState_ = SWIM2;
    }
}

void XBoss_Dragon::FloorStraight2() {
    // the same back from the second pool to the first
    if (localCounter_ == 1) {
        D3DVECTOR under(startPos_.x, -70.0f, startPos_.z + 640.0f);
        D3DVECTOR rise = swimRefPos2_[3];
        rise.y = 60.0f;
        D3DVECTOR land = swimRefPos1_[4];
        land.y = 20.0f;
        D3DVECTOR dive(startPos_.x, -70.0f, startPos_.z);
        D3DVECTOR centre = GetRandomCenterPos();
        const D3DVECTOR* f = floorRefPos_;
        const D3DVECTOR* s = swimRefPos1_;
        spline_.Init(12);
        for (const D3DVECTOR& p : {pos_, swimRefPos2_[1], under, rise, f[6], centre, f[4], land, dive, s[5], s[3], s[0]})
            spline_.AddNode(p);
        travel_ = 0;
        travelSpeed_ = 1.0f / spline_.GetTotalDistance();
    }
    float before = travel_;
    bool end = FollowSpline();
    if (0.2f > before && travel_ >= 0.2f) StartFlame();
    if (0.5f > before && travel_ >= 0.5f) EndFlame();
    if (end) {
        spline_.Deinit();
        localCounter_ = 0;
        moveState_ = SWIM1;
    }
}

void XBoss_Dragon::ProcessFrame() {
    if (firstBody_) firstBody_->ProcessFrame();
    localCounter_++;
    if (liveState_ == XBossBase::NON) return;
    CheckBattleStarted();
    ProcessDamage();
    if (liveState_ == DEAD) {
        // the head bursts, then 16 ticks later the body after it
        if (localCounter_ == 1) {
            game_->particles->CreateEnemyExplode(pos_, true);
            game_->playSound(kSoundBurst, pos_.z);
        }
        if (localCounter_ > 16) {
            if (firstBody_) firstBody_->StartDead();
            localCounter_ = 0;
            liveState_ = XBossBase::NON;
        }
        return;
    }
    if (liveState_ != NORMAL) return;
    prevPos_ = pos_;
    switch (moveState_) {
        case SWIM1: Swim1(); break;
        case SWIM2: Swim2(); break;
        case FLOORWAVE1: FloorWave1(); break;
        case FLOORWAVE2: FloorWave2(); break;
        case FLOORSTRAIGHT1: FloorStraight1(); break;
        case FLOORSTRAIGHT2: FloorStraight2(); break;
        default: break;
    }
    yaw_ = MakeAngleCloseTo(yaw_, FindDirectionXZ(prevPos_, pos_), 0.1f, 0.0f, FLT_MAX);
    pitch_ = MakeAngleCloseTo(pitch_, -FindPitch(prevPos_, pos_), 0.05f, 0.0f, FLT_MAX);
    CheckSplash(prevPos_, pos_, true);
    UpdateWorldMatrix();
    CollisionCheck_Hero(game_->heroA);
    // record this tick for the body
    posRec_[recIndex_] = pos_;
    yawRec_[recIndex_] = yaw_;
    pitchRec_[recIndex_] = pitch_;
    recIndex_ = (recIndex_ + 1) % kRecords;
    // the flame: the jaw opens (frame 1 for 15 ticks, 2 until 50), then a flame every 5 ticks from
    // 20 ahead of the head until the end, then it closes (frame 2 until 35, 1 until 50)
    flameCounter_++;
    if (flameState_ == NONFLAME) {
        frame_ = 0;
    } else if (flameState_ == STARTFLAME) {
        if (flameCounter_ < 15) {
            frame_ = 1;
        } else if (flameCounter_ < 50) {
            frame_ = 2;
        } else {
            game_->playSound(kSoundFlame, pos_.z);
            flameCounter_ = 0;
            flameState_ = DOFLAME;
        }
    } else if (flameState_ == DOFLAME) {
        frame_ = 2;
        if (flameCounter_ % 5 == 1) {
            D3DVECTOR p = pos_;
            p.x += std::sin(yaw_) * 20.0f * std::cos(-pitch_);
            p.z += std::cos(yaw_) * 20.0f * std::cos(-pitch_);
            p.y += std::sin(-pitch_) * 20.0f;
            game_->bullets->CreateFlame(p, yaw_, pitch_);
        }
    } else if (flameState_ == ENDFLAME) {
        if (flameCounter_ < 35) {
            frame_ = 2;
        } else if (flameCounter_ < 50) {
            frame_ = 1;
        } else {
            flameCounter_ = 0;
            flameState_ = NONFLAME;
        }
    }
}

void XBoss_Dragon::Render(Device* d) {
    if (firstBody_) firstBody_->Render(d);
    if (liveState_ == XBossBase::NON || liveState_ == DEAD) return;
    model_->SetCurrentFrame(frame_);
    model_->Render(d);
    bodyShadow_->SetPos(pos_);
    bodyShadow_->Render(d, game_->level->Collision());
}

bool XBoss_Dragon::CollisionCheck_Fire(const D3DVECTOR& pos, float radius) {
    if (XBossBase::CollisionCheck_Fire(pos, radius)) return true;
    return firstBody_ && firstBody_->CollisionCheck_Fire(pos, radius);
}

// --------------------------------------------------------------------------------- XBoss_Genie

XBoss_Genie::XBoss_Genie(const XGameContext* game, const D3DVECTOR& pos) : XBossBase(game, pos) {
    model_ = std::make_unique<JObject3DX>(kModelGenie);
    model_->AppendFrame(0x1ec);  // mouth half open
    model_->AppendFrame(0x1ed);  // mouth open
    model_->SetCollisionModel(kColGenie);
    shadow_ = std::make_unique<XShadowSimple>(140.0f);
    collideBox_ = std::make_unique<XCollideBox>(200.0f, 200.0f);
    scale_ = 0;
    UpdateWorldMatrix();
    genieLamp_ = game->statics->CreateGenieLamp(startPos_);
    explodeRadius_ = 80.0f;
    healthSubMax_ = 15;
    healthSub_ = healthSubMax_;
    liveState_ = NORMAL;
    moveState_ = HIDE;
    if (game->world == 3) healthSubMax_ = 20;  // (after healthSub was set: the first heart takes 15)
}

void XBoss_Genie::Render(Device* d) {
    if (moveState_ != HIDE) XBossBase::Render(d);
}

void XBoss_Genie::ProcessFrame() {
    XBossBase::ProcessFrame();
    if (liveState_ != NORMAL) return;
    switch (moveState_) {
        case HIDE: Hide(); break;
        case STEAM: Steam(); break;
        case APPEAR: Appear(); break;
        case IDLE: Idle(); break;
        case MOVE: Move(); break;
        case OPENMOUTH: OpenMouth(); break;
        case CLOSEMOUTH: CloseMouth(); break;
        case ATTACK: Attack(); break;
        default: break;
    }
    UpdateWorldMatrix();
    CollisionCheck_Hero(game_->heroA);
}

void XBoss_Genie::Damage() {
    XBossBase::Damage();
    if (moveState_ == IDLE) idleFrames_ -= 50;  // each hit shortens the rest
}

D3DVECTOR XBoss_Genie::GetMouthPos() const {
    return D3DVECTOR(pos_.x + std::sin(yaw_) * 34.0f, pos_.y + 62.0f, pos_.z + std::cos(yaw_) * 34.0f);
}

void XBoss_Genie::UpdatePosY() {
    offsetCounter_++;
    float t = (float)(offsetCounter_ % 200) / 200.0f;
    pos_.y = refPosY_ + std::sin(t * 6.28319f + 3.14159f) * 8.0f;
}

void XBoss_Genie::Hide() {
    if (game_->DistanceToCloserHeroXZ(pos_) < 80.0f) Next(STEAM);
}

void XBoss_Genie::Steam() {
    if (localCounter_ == 1) {
        game_->playSound(kSoundGenieSteam, pos_.z);
        genieLamp_->StartShaking();
    }
    if (genieLamp_->GetMoveState() == XStaticObj_GenieLamp::NORMAL) Next(APPEAR);
}

// 500 ticks: from the spout, growing (sin of a quarter turn), rising to 80 above the start on an
// arc 120 high, spinning from 15 degrees a tick to a stop. Only pos.y changes after the first tick.
void XBoss_Genie::Appear() {
    float holeY = genieLamp_->GetHolePos().y - startPos_.y;
    if (localCounter_ == 1) {
        game_->playSound(kSoundGenieAppear, pos_.z);
        yaw_ = 0.654503f;
        scale_ = 0;
        SetPos(genieLamp_->GetHolePos());
    }
    float t = (float)localCounter_ / 500.0f;
    scale_ = std::sin(t * 1.5708f);
    pos_.y = startPos_.y + (holeY + (80.0f - holeY) * t) + std::sin(t * 3.14159f) * 120.0f;
    yaw_ = NormaliseAngle(yaw_ + 0.0174533f * ((1.0f - t) * 15.0f));
    if (localCounter_ >= 500) {
        refPosY_ = pos_.y;
        Next(IDLE);
    }
}

void XBoss_Genie::Idle() {
    if (localCounter_ == 1) idleFrames_ = (int)(Random(5) + 4) * 100;  // RandomI(4, 8) x 100
    UpdatePosY();
    yaw_ = MakeAngleCloseTo(yaw_, game_->DirToCloserHero(pos_), 0.05f, 0.0f, 0.0139626f);
    if ((int)localCounter_ >= idleFrames_) Next(OPENMOUTH);
}

// It glides along Z only (pos.z directly: its box stays where it appeared) to the centre or 380
// either side, never the same place twice running.
void XBoss_Genie::Move() {
    if (localCounter_ == 1) {
        game_->playSound(kSoundGenieMove, pos_.z);
        moveDest_ = (MoveDest)((moveDest_ + Random(2) + 1) % 3);
        destPos_ = startPos_;
        if (moveDest_ == LEFT) destPos_.z -= 380.0f;
        else if (moveDest_ == RIGHT) destPos_.z += 380.0f;
        destYaw_ = FindDirectionXZ(pos_, destPos_);
    }
    UpdatePosY();
    pos_.z = MakeValueCloseTo(pos_.z, destPos_.z, 0.05f, 0.1f, 2.5f);
    yaw_ = MakeAngleCloseTo(yaw_, destYaw_, 0.1f, 0.0f, FLT_MAX);
    if (pos_.z == destPos_.z) Next(IDLE);
}

void XBoss_Genie::OpenMouth() {
    if (localCounter_ == 10) frame_ = 1;
    if (localCounter_ == 40) frame_ = 2;
    UpdatePosY();
    yaw_ = MakeAngleCloseTo(yaw_, game_->DirToCloserHero(pos_), 0.05f, 0.0f, 0.0279253f);
    if (localCounter_ >= 60) Next(ATTACK);
}

void XBoss_Genie::CloseMouth() {
    if (localCounter_ == 10) frame_ = 1;
    if (localCounter_ == 40) frame_ = 0;
    UpdatePosY();
    yaw_ = MakeAngleCloseTo(yaw_, game_->DirToCloserHero(pos_), 0.05f, 0.0f, 0.0279253f);
    if (localCounter_ >= 100) Next(MOVE);
}

// Rounds of ROTATE (30 ticks turning to the hero) and FIRE (20 ticks, a shot at 5), 30 shots in
// all, or until a hero comes within 80 (XZ).
void XBoss_Genie::Attack() {
    if (localCounter_ == 1) {
        fireCount_ = 0;
        attackCounter_ = 0;
        attackState_ = ATTACK_ROTATE;
    }
    attackCounter_++;
    UpdatePosY();
    if (attackState_ == ATTACK_ROTATE) {
        yaw_ = MakeAngleCloseTo(yaw_, game_->DirToCloserHero(pos_), 0.05f, 0.0f, FLT_MAX);
        if (attackCounter_ >= 30) {
            attackCounter_ = 0;
            attackState_ = ATTACK_FIRE;
        }
    } else if (attackState_ == ATTACK_FIRE) {
        if (attackCounter_ == 5) {
            game_->playSound(kSoundGenieFire, pos_.z);
            game_->bullets->CreateNeedleBall(GetMouthPos(), yaw_);
            fireCount_++;
        }
        if (attackCounter_ >= 20) {
            attackCounter_ = 0;
            attackState_ = ATTACK_ROTATE;
        }
    }
    if (fireCount_ >= 30) Next(CLOSEMOUTH);
    else if (game_->DistanceToCloserHeroXZ(pos_) < 80.0f) Next(CLOSEMOUTH);
}

