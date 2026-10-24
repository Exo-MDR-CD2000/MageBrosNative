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
constexpr uint16_t kModelNest = 0x28d, kColNest = 0x28e;
constexpr uint16_t kModelBird1 = 0x289, kModelBird2 = 0x28a, kModelBird3 = 0x28b, kColBird = 0x28c;
constexpr uint16_t kSoundBirdChirp = 0x2a5, kSoundNestHit = 0x2a6;
constexpr uint16_t kModelWizard1 = 0x298, kModelWizard2 = 0x299, kModelWizard3 = 0x29a, kColWizard = 0x296;
constexpr uint16_t kModelWizardBlade = 0x29c, kModelWizardWand = 0x29b;
constexpr uint16_t kSoundWizardRotate = 0x2a7, kSoundWizardAttack = 0x2aa, kSoundWizardAppear = 0x2ab,
                   kSoundWizardRoar = 0x2b3, kSoundWizardFire = 0x2a8, kSoundWizardWandUp = 0x2a9;

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
    CollisionCheck_Hero(game_->heroB);
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
    XHero* b = game_->heroB;
    if (b && b->GetLiveState() == XHero::NORMAL && b->GetPos().z > swimRefPos1_[4].z) battleStarted_ = true;
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
    CollisionCheck_Hero(game_->heroB);
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
    CollisionCheck_Hero(game_->heroB);
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

// ---------------------------------------------------------------------------------- XBoss_Bird

XBoss_Bird::XBoss_Bird(const XGameContext* game, XBoss_Nest* nest) : XBossBase(game, nest->GetPos()), nest_(nest) {
    model_ = std::make_unique<JObject3DX>(kModelBird1);
    model_->AppendFrame(kModelBird2);
    model_->AppendFrame(kModelBird3);
    model_->AppendFrame(kModelBird2);
    model_->SetCollisionModel(kColBird);
    bodyShadow_ = std::make_unique<XShadow>(12.0f);
    collideBox_ = std::make_unique<XCollideBox>(20.0f, 20.0f);
    damagable_ = false;
    liveState_ = NORMAL;
    moveState_ = WAIT;
}
XBoss_Bird::~XBoss_Bird() = default;

void XBoss_Bird::Render(Device* d) {
    // (drawn in the nest too, with no visibility test)
    if (liveState_ == NON) return;
    model_->SetCurrentFrame(frame_);
    model_->Render(d);
    bodyShadow_->SetPos(pos_);
    bodyShadow_->Render(d, game_->level->Collision());
    // (the original also calls the box's debug draw)
}

void XBoss_Bird::ProcessFrame() {
    // (not XBossBase::ProcessFrame: no flash, no battle start, its own death)
    localCounter_++;
    if (liveState_ == NON) return;
    if (liveState_ == NORMAL) {
        birdCounter_++;
        switch (moveState_) {
            case WAIT: Wait(); break;
            case GO: Go(); break;
            case ATTACK_HERO: AttackHero(); break;
            case FLY_ABOVE_NEST: FlyAboveNest(); break;
            case BACK_TO_NEST: BackToNest(); break;
        }
        frame_ = (int)(birdCounter_ / 3 % 4);
        UpdateWorldMatrix();
        CollisionCheck_Hero(game_->heroA);
        CollisionCheck_Hero(game_->heroB);
    } else if (liveState_ == DEAD) {
        game_->particles->CreateEnemyExplode(pos_, false);
        localCounter_ = 0;
        liveState_ = NON;
    }
}

// XBoss_Bird::StartAttack: only a bird sitting in the nest goes
void XBoss_Bird::StartAttack() {
    if (moveState_ != WAIT) return;
    localCounter_ = 0;
    moveState_ = GO;
}

// XBoss_Bird::StartDead: a bird in the nest just goes; one out of it bursts
void XBoss_Bird::StartDead() {
    localCounter_ = 0;
    liveState_ = moveState_ == WAIT ? NON : DEAD;
}

void XBoss_Bird::LimitPosition_Wall(D3DVECTOR& p, float margin) {
    if (margin > p.x) p.x = margin;
    else if (p.x > 200.0f - margin) p.x = 200.0f - margin;
    if (margin > p.z) p.z = margin;  // (no limit at the far end)
}

void XBoss_Bird::LimitPosition_Floor(D3DVECTOR& p, float margin) {
    if (margin > p.y) p.y = margin;
}

// _UpdateTargetPos_Hero: a random point 50 from the closer hero's centre
void XBoss_Bird::UpdateTargetPos_Hero() {
    float a = UnitRand() * 6.28319f;
    float b = UnitRand() * 3.14159f - 1.5708f;
    XHero* hero = game_->GetCloserHero(nest_->GetPos());  // the hero closer to the nest
    const D3DVECTOR& hp = hero->GetPos();
    targetPos_ = D3DVECTOR(hp.x, hp.y + 14.5f, hp.z);  // XHeroBase::GetCenterPos
    targetPos_.x += std::sin(a) * 50.0f * std::cos(b);
    targetPos_.z += std::cos(a) * 50.0f * std::cos(b);
    targetPos_.y += std::sin(b) * 50.0f;
    LimitPosition_Wall(targetPos_, 30.0f);
    LimitPosition_Floor(targetPos_, 5.0f);
}

// _UpdateTargetPos_Nest: a random point 60 from the spot 80 above the nest, in its upper half
void XBoss_Bird::UpdateTargetPos_Nest() {
    float a = UnitRand() * 6.28319f;
    float b = RandomF(0.2f, 1.0f) * 1.5708f;
    targetPos_ = nest_->GetPos();
    targetPos_.x += std::sin(a) * 60.0f * std::cos(b);
    targetPos_.z += std::cos(a) * 60.0f * std::cos(b);
    targetPos_.y += std::sin(b) * 60.0f + 80.0f;
    LimitPosition_Wall(targetPos_, 30.0f);
}

// The flight of _ProcessFrame_AttackHero and _ProcessFrame_FlyAboveNest: turn 2 degrees a tick
// towards the target, pitch 1 degree a tick towards it (within 45 degrees), and fly along the
// facing at 0.8 to 1.8, faster the further the target is (full speed from 160 away).
void XBoss_Bird::Fly(float limitFloor) {
    yaw_ = MakeAngleCloseTo_Constant(yaw_, FindDirectionXZ(pos_, targetPos_), 0.0349066f);
    float p = -FindPitch(pos_, targetPos_);
    if (-0.785398f > p) p = -0.785398f;
    else if (p > 0.785398f) p = 0.785398f;
    pitch_ = MakeValueCloseTo_Constant(pitch_, p, 0.0174533f);
    float k = ApproxDistanceXZ(pos_, targetPos_) / 160.0f;
    if (k > 1.0f) k = 1.0f;
    float speed = 1.0f * k + 0.8f;
    D3DVECTOR np = pos_;
    np.x += std::sin(yaw_) * speed * std::cos(-pitch_);
    np.z += std::cos(yaw_) * speed * std::cos(-pitch_);
    np.y += std::sin(-pitch_) * speed;
    LimitPosition_Wall(np, 5.0f);
    LimitPosition_Floor(np, limitFloor);
    SetPos(np);
}

// A step along the spline (travel clamped at 1), turning to face the way it moved
void XBoss_Bird::FollowSplineTurning() {
    travel_ += travelSpeed_;
    if (travel_ > 1.0f) travel_ = 1.0f;
    D3DVECTOR prev = pos_;
    D3DVECTOR np = spline_.GetPosition(travel_);
    if (moveState_ == BACK_TO_NEST) {
        LimitPosition_Wall(np, 5.0f);
        LimitPosition_Floor(np, 5.0f);
    }
    SetPos(np);
    yaw_ = MakeAngleCloseTo(yaw_, FindDirectionXZ(prev, pos_), 0.16f, 0.0f, FLT_MAX);
    pitch_ = MakeAngleCloseTo(pitch_, -FindPitch(prev, pos_), 0.08f, 0.0f, FLT_MAX);
}

// XBoss_Bird::_ProcessFrame_Go: up out of the nest (60, 120) and off to a random side, 100 up,
// at 1.8 a tick along the spline
void XBoss_Bird::Go() {
    if (localCounter_ == 1) {
        const D3DVECTOR& n = nest_->GetPos();
        D3DVECTOR p1 = n, p2 = n, p3 = n;
        p1.y += 60.0f;
        p2.y += 120.0f;
        float a = UnitRand() * 6.28319f;
        p3.x += std::sin(a) * 60.0f;
        p3.z += std::cos(a) * 60.0f;
        p3.y += 100.0f;
        spline_.Init(4);
        spline_.AddNode(n);
        spline_.AddNode(p1);
        spline_.AddNode(p2);
        spline_.AddNode(p3);
        travel_ = 0;
        travelSpeed_ = 1.8f / spline_.GetTotalDistance();
    }
    FollowSplineTurning();
    if (travel_ >= 1.0f) {
        spline_.Deinit();
        localCounter_ = 0;
        moveState_ = ATTACK_HERO;
    }
}

// XBoss_Bird::_ProcessFrame_AttackHero: 1800 ticks after the hero, a new target every 130 ticks,
// chirping (sound 0x2a5) every 100 to 300 ticks
void XBoss_Bird::AttackHero() {
    if (localCounter_ == 1) {
        chirpTimer_ = (int)Random(201) + 100;  // RandomI(100, 300, inclusive)
        UpdateTargetPos_Hero();
    }
    Fly(5.0f);
    if (--chirpTimer_ <= 0) {
        game_->playSound(kSoundBirdChirp, pos_.z);
        chirpTimer_ = (int)Random(201) + 100;
    }
    if (localCounter_ % 130 == 0) UpdateTargetPos_Hero();
    if (localCounter_ >= 1800) {
        localCounter_ = 0;
        moveState_ = FLY_ABOVE_NEST;
    }
}

// XBoss_Bird::_ProcessFrame_FlyAboveNest: 400 ticks round points above the nest, a new one
// every 130 ticks
void XBoss_Bird::FlyAboveNest() {
    if (localCounter_ == 1) UpdateTargetPos_Nest();
    Fly(5.0f);
    if (localCounter_ % 130 == 0) UpdateTargetPos_Nest();
    if (localCounter_ >= 400) {
        localCounter_ = 0;
        moveState_ = BACK_TO_NEST;
    }
}

// XBoss_Bird::_ProcessFrame_BackToNest: a spline from 30 ahead, down through 100 and 60 above the
// nest and into it, at 1.2 a tick; then it waits to be sent out again
void XBoss_Bird::BackToNest() {
    if (localCounter_ == 1) {
        D3DVECTOR ahead = pos_;
        ahead.x += std::sin(yaw_) * 30.0f * std::cos(-pitch_);
        ahead.z += std::cos(yaw_) * 30.0f * std::cos(-pitch_);
        ahead.y += std::sin(-pitch_) * 30.0f;
        const D3DVECTOR& n = nest_->GetPos();
        D3DVECTOR p100 = n, p60 = n;
        p100.y += 100.0f;
        p60.y += 60.0f;
        spline_.Init(5);
        spline_.AddNode(pos_);
        spline_.AddNode(ahead);
        spline_.AddNode(p100);
        spline_.AddNode(p60);
        spline_.AddNode(n);
        travel_ = 0;
        travelSpeed_ = 1.2f / spline_.GetTotalDistance();
    }
    FollowSplineTurning();
    if (travel_ >= 1.0f) {
        spline_.Deinit();
        localCounter_ = 0;
        moveState_ = WAIT;
    }
}

// ---------------------------------------------------------------------------------- XBoss_Nest

XBoss_Nest::XBoss_Nest(const XGameContext* game, const D3DVECTOR& pos) : XBossBase(game, pos) {
    rotSpeed_ = 0.00174533f;
    model_ = std::make_unique<JObject3DX>(kModelNest);
    model_->SetCollisionModel(kColNest);
    collideBox_ = std::make_unique<XCollideBox>(140.0f, 60.0f);
    pos_.y += 12.0f;
    SetPos(pos_);
    UpdateWorldMatrix();
    refPosY_ = pos_.y;
    for (auto& b : birdArray_) b = std::make_unique<XBoss_Bird>(game, this);
    explodeRadius_ = 85.0f;
    healthSubMax_ = 15;
    healthSub_ = healthSubMax_;
    liveState_ = NORMAL;
    moveState_ = WAIT;
    if (game->world == 3) healthSubMax_ = 20;  // (after healthSub was set: the first heart takes 15)
}
XBoss_Nest::~XBoss_Nest() = default;

void XBoss_Nest::Render(Device* d) {
    for (auto& b : birdArray_) b->Render(d);
    XBossBase::Render(d);
}

void XBoss_Nest::ProcessFrame() {
    XBossBase::ProcessFrame();
    if (liveState_ == NORMAL) {
        if (moveState_ == SHAKE) Shake();
        yaw_ = NormaliseAngle(yaw_ - rotSpeed_);
        UpdateWorldMatrix();
        CollisionCheck_Hero(game_->heroA);
        CollisionCheck_Hero(game_->heroB);
    }
    for (auto& b : birdArray_) b->ProcessFrame();
}

// XBoss_Nest::_Damage: after the base's hit, the next bird goes after the hero (sound 0x2a6) and
// the nest turns 0.02 degrees a tick faster; on the last hit every bird dies instead
void XBoss_Nest::Damage() {
    XBossBase::Damage();
    if (liveState_ == DEAD) {
        for (auto& b : birdArray_) b->StartDead();
        return;
    }
    game_->playSound(kSoundNestHit, pos_.z);
    birdArray_[currentBirdIndex_]->StartAttack();
    currentBirdIndex_ = (currentBirdIndex_ + 1) % kBirds;
    rotSpeed_ += 0.000349066f;
    localCounter_ = 0;
    moveState_ = SHAKE;
}

// XBoss_Nest::_ProcessFrame_Shake: three large rings on the water 40 out at random angles, then
// 100 ticks bobbing 10 times, 3.5 at first and dying away
void XBoss_Nest::Shake() {
    if (localCounter_ == 1) {
        for (int i = 0; i < 3; i++) {
            float a = UnitRand() * 6.28319f;
            D3DVECTOR p(std::sin(a) * 40.0f + pos_.x, -38.0f, std::cos(a) * 40.0f + pos_.z);
            game_->particles->CreateWaterWave(p, true, false);
        }
    }
    float t = (float)localCounter_ / 100.0f;
    pos_.y = refPosY_ + std::sin(6.28319f * t * 10.0f) * ((1.0f - t) * 3.5f);
    SetPos(pos_);
    if (localCounter_ >= 100) {
        localCounter_ = 0;
        moveState_ = WAIT;
    }
}

// --------------------------------------------------------------------------- XBoss_WizardBlade

XBoss_WizardBlade::XBoss_WizardBlade(const XGameContext* game, XBoss_Wizard* parent)
    : XBossBase(game, parent->GetPos()), parent_(parent) {
    model_ = std::make_unique<JObject3DX>(kModelWizardBlade);  // (no collision model, no box)
}

void XBoss_WizardBlade::Start(int windDir) {
    windDir_ = windDir;
    scale_ = 0;
    rotateSpeed_ = 0;
    UpdatePosAndYaw();
    UpdateWorldMatrix();
    localCounter_ = 0;
    liveState_ = NORMAL;
    moveState_ = APPEAR;
}

void XBoss_WizardBlade::ProcessFrame() {
    XBossBase::ProcessFrame();
    if (liveState_ != NORMAL) return;
    switch (moveState_) {
        case APPEAR: Appear(); break;
        case ROTATE_FASTER: RotateFaster(); break;
        case ROTATE_STABLE: RotateStable(); break;
        case ROTATE_SLOWER: RotateSlower(); break;
        case DISAPPEAR: Disappear(); break;
        default: break;
    }
    UpdatePosAndYaw();
    PushHero(game_->heroA);
    PushHero(game_->heroB);
    UpdateWorldMatrix();
}

// _UpdatePosAndYaw: 38 in front of the wizard, facing its way (pos written directly: no box)
void XBoss_WizardBlade::UpdatePosAndYaw() {
    yaw_ = parent_->GetYaw();
    pos_ = parent_->GetPos();
    pos_.x += std::sin(yaw_) * 38.0f;
    pos_.z += std::cos(yaw_) * 38.0f;
}

// _PushHero: while it spins (not appearing or going), 1.375 a tick at full speed
void XBoss_WizardBlade::PushHero(XHero* hero) {
    if (liveState_ != NORMAL) return;
    if (!hero || hero->GetLiveState() != XHero::NORMAL) return;
    if (moveState_ == NON || moveState_ == APPEAR || moveState_ == DISAPPEAR) return;
    float push = rotateSpeed_ / 10.0f * 1.375f;
    if (windDir_ == 0) hero->IncreasePushZ(push);
    else if (windDir_ == 4) hero->IncreasePushZ(-push);
}

// grows over 120 ticks
void XBoss_WizardBlade::Appear() {
    scale_ = std::sin((float)localCounter_ / 120.0f * 1.5708f);
    if (localCounter_ >= 120) {
        localCounter_ = 0;
        moveState_ = ROTATE_FASTER;
    }
}

// spins up by 0.02 degrees a tick to 10
void XBoss_WizardBlade::RotateFaster() {
    if (localCounter_ == 1) rotateSpeed_ = 0;
    rotateSpeed_ += 0.02f;
    if (rotateSpeed_ > 10.0f) rotateSpeed_ = 10.0f;
    roll_ = NormaliseAngle(roll_ - 0.0174533f * rotateSpeed_);
    if (rotateSpeed_ >= 10.0f) {
        localCounter_ = 0;
        moveState_ = ROTATE_STABLE;
    }
}

// 10 degrees a tick for 800 ticks
void XBoss_WizardBlade::RotateStable() {
    roll_ = NormaliseAngle(roll_ - 0.174533f);
    if (localCounter_ >= 800) {
        localCounter_ = 0;
        moveState_ = ROTATE_SLOWER;
    }
}

// spins down by 0.02 degrees a tick to 0
void XBoss_WizardBlade::RotateSlower() {
    if (localCounter_ == 1) rotateSpeed_ = 10.0f;
    rotateSpeed_ -= 0.02f;
    if (0.0f > rotateSpeed_) rotateSpeed_ = 0.0f;
    roll_ = NormaliseAngle(roll_ - 0.0174533f * rotateSpeed_);
    if (0.0f >= rotateSpeed_) {
        localCounter_ = 0;
        moveState_ = DISAPPEAR;
    }
}

// shrinks over 40 ticks and goes
void XBoss_WizardBlade::Disappear() {
    scale_ = std::cos((float)localCounter_ / 40.0f * 1.5708f);
    if (localCounter_ >= 40) {
        localCounter_ = 0;
        moveState_ = NON;
        liveState_ = XBossBase::NON;
    }
}

// -------------------------------------------------------------------------------- XBoss_Wizard

XBoss_Wizard::XBoss_Wizard(const XGameContext* game, const D3DVECTOR& pos) : XBossBase(game, pos) {
    model_ = std::make_unique<JObject3DX>(kModelWizard1);
    model_->AppendFrame(kModelWizard2);
    model_->AppendFrame(kModelWizard1);
    model_->AppendFrame(kModelWizard3);
    model_->SetCollisionModel(kColWizard);
    collideBox_ = std::make_unique<XCollideBox>(200.0f, 80.0f);
    bodyShadow_ = std::make_unique<XShadow>(39.9f);
    blade_ = std::make_unique<XBoss_WizardBlade>(game, this);
    wand_ = std::make_unique<XBoss_WizardWand>(game, this);
    yaw_ = 3.14159f;
    pos_ = GetRefPos(RIGHT);
    pos_.y = -300.0f;  // under the floor
    SetPos(pos_);
    UpdateWorldMatrix();
    explodeRadius_ = 80.0f;
    healthSubMax_ = 40;
    healthSub_ = healthSubMax_;
    liveState_ = NORMAL;
    moveState_ = HIDE;
    posState_ = RIGHT;
}
XBoss_Wizard::~XBoss_Wizard() = default;

// GetRefPos: 87.5 up across the middle (x 100); along Z the middle, 200 or 100 to either side; or
// the middle at either edge (x 0 or 200)
D3DVECTOR XBoss_Wizard::GetRefPos(PosState s) const {
    D3DVECTOR p(100.0f, 87.5f, startPos_.z);
    switch (s) {
        case LEFT: p.z = startPos_.z - 200.0f; break;
        case RIGHT: p.z = startPos_.z + 200.0f; break;
        case LEFT_CENTER: p.z = startPos_.z - 100.0f; break;
        case RIGHT_CENTER: p.z = startPos_.z + 100.0f; break;
        case UP_CENTER: p.x -= 100.0f; break;
        case DOWN_CENTER: p.x += 100.0f; break;
        default: break;
    }
    return p;
}

void XBoss_Wizard::Render(Device* d) {
    if (liveState_ == XBossBase::NON || liveState_ == DEAD) return;
    model_->SetCurrentFrame(frame_);
    model_->Render(d);
    bodyShadow_->SetPos(pos_);
    bodyShadow_->Render(d, game_->level->Collision());
    // (the original also calls the box's debug draw)
    if (moveState_ == BLADE) blade_->Render(d);
    else if (moveState_ == WAND) wand_->Render(d);
}

void XBoss_Wizard::ProcessFrame() {
    XBossBase::ProcessFrame();
    if (liveState_ != NORMAL) return;
    frame_ = (int)(localCounter_ / 3 % 4);
    switch (moveState_) {
        case HIDE: Hide(); break;
        case APPEAR: Appear(); break;
        case BLADE: Blade(); break;
        case WAND: Wand(); break;
        case MOVEIN: MoveIn(); break;
        case MOVEOUT: MoveOut(); break;
        case ROTATE: Rotate(); break;
        default: break;
    }
    if (moveState_ != HIDE && moveState_ != APPEAR) UpdatePosY();
    UpdateWorldMatrix();
    CollisionCheck_Hero(game_->heroA);
    CollisionCheck_Hero(game_->heroB);
}

// _UpdatePosY: 87.5 up, bobbing 4.5 over 100 ticks (pos only: the box keeps its height)
void XBoss_Wizard::UpdatePosY() {
    offsetCounter_++;
    float t = (float)(offsetCounter_ % 100) / 100.0f;
    pos_.y = 87.5f + std::sin(t * 6.28319f + 3.14159f) * 4.5f;
}

// _PlayRotateSound: sound 0x2a7 when the angle passes 0, 90, 180 or 270 degrees
void XBoss_Wizard::PlayRotateSound(float from, float to) {
    bool play = from > to;
    if (1.5708f >= from && to >= 1.5708f) play = true;
    if (3.14159f >= from && to >= 3.14159f) play = true;
    if (4.71239f >= from && to >= 4.71239f) play = true;
    if (play) game_->playSound(kSoundWizardRotate, pos_.z);
}

// _ProcessFrame_Hide: a roar (0x2b3) when a hero first comes within 300 (XZ), out within 110
void XBoss_Wizard::Hide() {
    if (!roarSoundPlayed_ && game_->DistanceToCloserHeroXZ(pos_) < 300.0f) {
        if (game_->playMono) game_->playMono(kSoundWizardRoar, 1.0f);
        roarSoundPlayed_ = true;
    }
    if (game_->DistanceToCloserHeroXZ(pos_) < 110.0f) {
        localCounter_ = 0;
        moveState_ = APPEAR;
    }
}

// _ProcessFrame_Appear: sound 0x2ab; up from under the floor to 87.5 over 100 ticks, bobbing from
// then on, and in after 300
void XBoss_Wizard::Appear() {
    if (localCounter_ == 1 && game_->playMono) game_->playMono(kSoundWizardAppear, 1.0f);
    float k = (float)localCounter_ / 100.0f;
    if (k > 1.0f) k = 1.0f;
    D3DVECTOR p = pos_;
    p.y = std::sin(k * 1.5708f) * 387.5f + -300.0f;
    SetPos(p);
    if (localCounter_ >= 100) UpdatePosY();
    if (localCounter_ >= 300) {
        posState_ = RIGHT;
        localCounter_ = 0;
        moveState_ = MOVEIN;
    }
}

// _ProcessFrame_Blade: sound 0x2aa, the blade blowing away from this side; it faces the middle
// until the blade has gone
void XBoss_Wizard::Blade() {
    if (localCounter_ == 1) {
        game_->playSound(kSoundWizardAttack, pos_.z);
        int windDir = -1;
        if (posState_ == LEFT) windDir = 0;
        else if (posState_ == RIGHT) windDir = 4;
        blade_->Start(windDir);
        toPos_ = GetRefPos(CENTER);
    }
    yaw_ = MakeAngleCloseTo(yaw_, FindDirectionXZ(pos_, toPos_), 0.05f, 0.0f, FLT_MAX);
    blade_->ProcessFrame();
    if (blade_->GetLiveState() == XBossBase::NON) {
        localCounter_ = 0;
        moveState_ = MOVEIN;
    }
}

// _ProcessFrame_Wand: the same as the blade, with the wand
void XBoss_Wizard::Wand() {
    if (localCounter_ == 1) {
        game_->playSound(kSoundWizardAttack, pos_.z);
        int wandDir = -1;
        if (posState_ == LEFT) wandDir = 0;
        else if (posState_ == RIGHT) wandDir = 4;
        wand_->Start(wandDir);
        toPos_ = GetRefPos(CENTER);
    }
    yaw_ = MakeAngleCloseTo(yaw_, FindDirectionXZ(pos_, toPos_), 0.05f, 0.0f, FLT_MAX);
    wand_->ProcessFrame();
    if (wand_->GetLiveState() == XBossBase::NON) {
        localCounter_ = 0;
        moveState_ = MOVEIN;
    }
}

// ---------------------------------------------------------------------------- XBoss_WizardWand

XBoss_WizardWand::XBoss_WizardWand(const XGameContext* game, XBoss_Wizard* parent)
    : XBossBase(game, parent->GetPos()), parent_(parent) {
    model_ = std::make_unique<JObject3DX>(kModelWizardWand);  // (no collision model, no box)
}

void XBoss_WizardWand::Start(int wandDir) {
    wandDir_ = wandDir;
    scale_ = 0;
    pitch_ = 0;
    UpdatePosAndYaw();
    UpdateWorldMatrix();
    fireTimer_ = 120;
    localCounter_ = 0;
    liveState_ = NORMAL;
    moveState_ = APPEAR;
}

void XBoss_WizardWand::ProcessFrame() {
    XBossBase::ProcessFrame();
    if (liveState_ != NORMAL) return;
    switch (moveState_) {
        case APPEAR: Appear(); break;
        case STICK_OUT: StickOut(); break;
        case STICK_IN: StickIn(); break;
        case ATTACK: Attack(); break;
        case DISAPPEAR: Disappear(); break;
        default: break;
    }
    UpdatePosAndYaw();
    UpdateWorldMatrix();
}

// _UpdatePosAndYaw: 1 in front of the wizard, facing its way (pos written directly: no box)
void XBoss_WizardWand::UpdatePosAndYaw() {
    yaw_ = parent_->GetYaw();
    pos_ = parent_->GetPos();
    pos_.x += std::sin(yaw_) * 1.0f;
    pos_.z += std::cos(yaw_) * 1.0f;
}

D3DVECTOR XBoss_WizardWand::GetFirePos() const {
    D3DVECTOR p = pos_;
    p.x += std::sin(yaw_) * 50.0f;
    p.z += std::cos(yaw_) * 50.0f;
    p.y -= 20.0f;
    return p;
}

// grows over 100 ticks
void XBoss_WizardWand::Appear() {
    scale_ = std::sin((float)localCounter_ / 100.0f * 1.5708f);
    if (localCounter_ >= 100) {
        localCounter_ = 0;
        moveState_ = STICK_OUT;
    }
}

// tilts out to 60 degrees over 50 ticks
void XBoss_WizardWand::StickOut() {
    pitch_ = std::sin((float)localCounter_ / 50.0f * 1.5708f) * 1.0472f;
    if (localCounter_ >= 50) {
        pitch_ = 1.0472f;
        localCounter_ = 0;
        moveState_ = ATTACK;
    }
}

// _ProcessFrame_Attack: sound 0x2a9; a flash at the tip every 4 ticks; when the timer runs out
// (first after 120) sound 0x2a8 and a fireball along its yaw, the next in 160, 130 from tick 666
// or 105 from 1332; 2000 ticks in all
void XBoss_WizardWand::Attack() {
    if (localCounter_ == 1) game_->playSound(kSoundWizardWandUp, pos_.z);
    if (localCounter_ % 4 == 1) game_->particles->CreateWizardFireFlash(GetFirePos());
    if (--fireTimer_ <= 0) {
        game_->playSound(kSoundWizardFire, pos_.z);
        game_->bullets->CreateWizardFire(GetFirePos(), yaw_);
        if (localCounter_ < 666) fireTimer_ = 160;
        else if (localCounter_ < 1332) fireTimer_ = 130;
        else fireTimer_ = 105;
    }
    if (localCounter_ >= 2000) {
        localCounter_ = 0;
        moveState_ = STICK_IN;
    }
}

// tilts back over 50 ticks
void XBoss_WizardWand::StickIn() {
    pitch_ = (1.0f - std::sin((float)localCounter_ / 50.0f * 1.5708f)) * 1.0472f;
    if (localCounter_ >= 50) {
        pitch_ = 0;
        localCounter_ = 0;
        moveState_ = DISAPPEAR;
    }
}

// shrinks over 40 ticks and goes
void XBoss_WizardWand::Disappear() {
    scale_ = std::cos((float)localCounter_ / 40.0f * 1.5708f);
    if (localCounter_ >= 40) {
        localCounter_ = 0;
        moveState_ = NON;
        liveState_ = XBossBase::NON;
    }
}

// The glide of _ProcessFrame_MoveIn and _ProcessFrame_MoveOut: 150 ticks in a straight line,
// turning to face the way it goes
void XBoss_Wizard::Glide(PosState fromState, PosState toState) {
    if (localCounter_ == 1) {
        fromPos_ = GetRefPos(fromState);
        toPos_ = GetRefPos(toState);
    }
    float t = (float)localCounter_ / 150.0f;
    D3DVECTOR prev = pos_;
    D3DVECTOR p(fromPos_.x + (toPos_.x - fromPos_.x) * t, fromPos_.y + (toPos_.y - fromPos_.y) * t,
                fromPos_.z + (toPos_.z - fromPos_.z) * t);  // D3DVectorInterpolate
    SetPos(p);
    yaw_ = MakeAngleCloseTo(yaw_, FindDirectionXZ(prev, pos_), 0.05f, 0.0f, FLT_MAX);
}

// _ProcessFrame_MoveIn: from 200 off the middle to 100 off it, then circle
void XBoss_Wizard::MoveIn() {
    if (posState_ == LEFT) Glide(LEFT, LEFT_CENTER);
    else if (posState_ == RIGHT) Glide(RIGHT, RIGHT_CENTER);
    if (localCounter_ >= 150) {
        if (posState_ == LEFT) posState_ = LEFT_CENTER;
        else if (posState_ == RIGHT) posState_ = RIGHT_CENTER;
        localCounter_ = 0;
        moveState_ = ROTATE;
    }
}

// _ProcessFrame_MoveOut: from 100 off the middle to 200 off it, then the wand or the blade
void XBoss_Wizard::MoveOut() {
    if (posState_ == LEFT_CENTER) Glide(LEFT_CENTER, LEFT);
    else if (posState_ == RIGHT_CENTER) Glide(RIGHT_CENTER, RIGHT);
    if (localCounter_ >= 150) {
        if (posState_ == LEFT_CENTER) posState_ = LEFT;
        else if (posState_ == RIGHT_CENTER) posState_ = RIGHT;
        localCounter_ = 0;
        moveState_ = Random(2) == 0 ? WAND : BLADE;
    }
}

// _ProcessFrame_Rotate: round the middle at radius 100 facing it, from the side it came in on:
// speeding up by 0.016 degrees a tick to 6, closing in by 0.2 to 40, 300 ticks there, out again to
// 100, slowing to 1.2; then on to the other side (angle 2 pi or 3 pi) and out from there
void XBoss_Wizard::Rotate() {
    if (localCounter_ == 1) {
        if (posState_ == LEFT_CENTER) rotateAngle_ = 3.14159f;
        else if (posState_ == RIGHT_CENTER) rotateAngle_ = 0.0f;
        toPos_ = GetRefPos(CENTER);
        rotateRadius_ = 100.0f;
        rotateSpeed_ = 1.2f;
        subLocalCounter_ = 0;
        rotateState_ = GET_FASTER;
    }
    subLocalCounter_++;
    auto place = [this]() {
        D3DVECTOR p = toPos_;
        p.x += std::sin(rotateAngle_) * rotateRadius_;
        p.z += std::cos(rotateAngle_) * rotateRadius_;
        SetPos(p);
        yaw_ = FindDirectionXZ(pos_, toPos_);
    };
    if (rotateState_ != STOP) {
        float prev = rotateAngle_;
        rotateAngle_ = NormaliseAngle(rotateAngle_ + 0.0174533f * rotateSpeed_);
        if (localCounter_ > 10) PlayRotateSound(prev, rotateAngle_);
        place();
    }
    if (rotateState_ == GET_FASTER) {
        rotateSpeed_ += 0.016f;
        if (rotateSpeed_ > 6.0f) rotateSpeed_ = 6.0f;
        if (rotateSpeed_ >= 6.0f) {
            subLocalCounter_ = 0;
            rotateState_ = GET_CLOSER;
        }
    } else if (rotateState_ == GET_CLOSER) {
        rotateRadius_ -= 0.2f;
        if (40.0f > rotateRadius_) rotateRadius_ = 40.0f;
        if (40.0f >= rotateRadius_) {
            subLocalCounter_ = 0;
            rotateState_ = STAY;
        }
    } else if (rotateState_ == STAY) {
        if (subLocalCounter_ >= 300) {
            subLocalCounter_ = 0;
            rotateState_ = GET_FURTHER;
        }
    } else if (rotateState_ == GET_FURTHER) {
        rotateRadius_ += 0.2f;
        if (rotateRadius_ > 100.0f) rotateRadius_ = 100.0f;
        if (rotateRadius_ >= 100.0f) {
            subLocalCounter_ = 0;
            rotateState_ = GET_SLOWER;
        }
    } else if (rotateState_ == GET_SLOWER) {
        rotateSpeed_ -= 0.016f;
        if (1.2f > rotateSpeed_) rotateSpeed_ = 1.2f;
        if (1.2f >= rotateSpeed_) {
            subLocalCounter_ = 0;
            rotateState_ = STOP;
        }
    } else if (rotateState_ == STOP) {
        // (no longer normalised: it runs on to the stopping angle)
        rotateAngle_ += 0.0174533f * rotateSpeed_;
        if (posState_ == LEFT_CENTER && rotateAngle_ > 6.28319f) rotateAngle_ = 6.28319f;
        else if (posState_ == RIGHT_CENTER && rotateAngle_ > 9.42478f) rotateAngle_ = 9.42478f;
        place();
        if (posState_ == LEFT_CENTER) {
            if (rotateAngle_ >= 6.28319f) {
                posState_ = RIGHT_CENTER;
                localCounter_ = 0;
                moveState_ = MOVEOUT;
            }
        } else if (posState_ == RIGHT_CENTER && rotateAngle_ >= 9.42478f) {
            posState_ = LEFT_CENTER;
            localCounter_ = 0;
            moveState_ = MOVEOUT;
        }
    }
}

