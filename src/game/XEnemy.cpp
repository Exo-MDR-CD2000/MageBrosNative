#include "game/XEnemy.h"
#include "framework/d3dutil.h"
#include "game/JModel.h"
#include "game/XGameContext.h"
#include "game/XHero.h"
#include "game/XLevelView.h"
#include "game/XMath.h"
#include "game/XParticle.h"
#include "game/XPlatform.h"
#include "game/XStaticObj.h"
#include <cfloat>
#include <cmath>

namespace {
constexpr uint16_t kSoundEnemyHurt = 0x270, kSoundEnemyDie = 0x27d, kSoundEnemyBurst = 0x26b;
constexpr float kPi = 3.14159f, kHalfPi = 1.5708f;

// RandomI: lo..hi, including hi when `inclusive`
long RandomI(long lo, long hi, bool inclusive) { return Random(hi - lo + (inclusive ? 1 : 0)) + lo; }

// FindDirectionXZ: the yaw from `from` towards `to` (sin along X, cos along Z)
float FindDirectionXZ(const D3DVECTOR& from, const D3DVECTOR& to) { return FindYaw(to.x - from.x, to.z - from.z); }

// ------------------------------------------------------------------------------- XEnemy_Slime
// 'a' slime (1 health), '1' king slime (20), '5' queen slime (120, starts at x 100). It walks a
// short way, waits, and every few waits sits for 60 ticks then turns towards a random point (or
// jumps at a hero within 120).
class Slime : public XEnemyBase {
public:
    enum MoveState { NON = 0, WALK = 1, WAIT = 2, JUMP = 3, LONGWAIT = 4 };
    Slime(const XGameContext* g, int oid, const D3DVECTOR& p, SizeType size) : XEnemyBase(g, oid, p, size) {
        struct Look {
            uint16_t frames[4], col;
            float shadow, width, height;
            int health;
        };
        static const Look kLooks[3] = {
            {{0x16f, 0x16e, 0x16d, 0x16e}, 0x177, 34.0f, 34.0f, 18.0f, 1},
            {{0x1e1, 0x1e2, 0x1e3, 0x1e2}, 0x1e4, 68.0f, 68.0f, 36.0f, 20},
            {{0x2c2, 0x2c0, 0x2c1, 0x2c0}, 0x2bf, 163.2f, 163.2f, 86.4f, 120},
        };
        if (size == QUEEN) {
            startPos_.x = 100.0f;
            pos_ = startPos_;
        }
        const Look& l = kLooks[size];
        model_ = std::make_unique<JObject3DX>(l.frames[0]);
        for (int i = 1; i < 4; i++) model_->AppendFrame(l.frames[i]);
        model_->SetCollisionModel(l.col);
        shadow_ = std::make_unique<XShadowSimple>(l.shadow);
        collideBox_ = std::make_unique<XCollideBox>(l.width, l.height);
        collideBox_->SetPos(startPos_);
        health_ = l.health;
        dir_ = UnitRand() * 6.28319f;
        yaw_ = dir_;
        UpdateWorldMatrix();
        countdownLongWait_ = (int)Random(5) + 2;
        countdownJump_ = (int)Random(4) + 3;
        chasable_ = true;
        liveState_ = XEnemyBase::NORMAL;
        moveState_ = WALK;
    }

    void ProcessFrame() override {
        if (!game_->IsUpdatable(pos_)) return;
        XEnemyBase::ProcessFrame();
        if (liveState_ != XEnemyBase::NORMAL) return;
        switch (moveState_) {
            case WALK: Walk(); break;
            case WAIT: Wait(); break;
            case JUMP: Jump(); break;
            case LONGWAIT: LongWait(); break;
            default: break;
        }
        yaw_ = MakeAngleCloseTo(yaw_, dir_, sizeType_ == QUEEN ? 0.02f : 0.06f, 0.0f, FLT_MAX);
        UpdateWorldMatrix();
        CollisionCheck_Hero(game_->heroA);
    }

private:
    void Walk() {
        float speed = 0.5f;
        if (sizeType_ == KING) speed *= 1.5f;
        else if (sizeType_ == QUEEN) speed = 0.0f;
        if (localCounter_ == 1) prevPosForWalk_ = pos_;
        float nx = pos_.x + std::sin(dir_) * speed, nz = pos_.z + std::cos(dir_) * speed;
        UpdatePosX(nx);
        UpdatePosZ(nz);
        frame_ = (int)((localCounter_ / 10 + 1) % 4);
        if (localCounter_ >= 40) {
            frame_ = 0;
            // stuck against something: no long wait this time
            if (sizeType_ != QUEEN && ApproxDistance(prevPosForWalk_, pos_) < 10.0f) countdownLongWait_ = 0;
            localCounter_ = 0;
            moveState_ = WAIT;
        }
    }
    void Wait() {
        if (localCounter_ < 10) return;
        if (--countdownLongWait_ <= 0) {
            countdownLongWait_ = (int)Random(5) + 2;
            localCounter_ = 0;
            moveState_ = LONGWAIT;
        } else {
            localCounter_ = 0;
            moveState_ = WALK;
        }
    }
    void Jump() {
        unsigned duration = 100;
        float height = 60.0f, speedUp = 0.2f, speedDown = 1.1f;
        if (sizeType_ == KING) {
            height *= 1.5f, speedUp *= 1.5f, speedDown *= 1.5f;
        } else if (sizeType_ == QUEEN) {
            duration = 120;
            height *= 2.5f;
            speedUp = speedDown = 0.0f;
        }
        float nx = pos_.x, nz = pos_.z;
        if (localCounter_ < duration / 2) {  // crouching up: slowly, tilting back
            nx += std::sin(dir_) * speedUp;
            nz += std::cos(dir_) * speedUp;
            frame_ = 2;
            pitch_ = MakeAngleCloseTo(pitch_, -0.349066f, 0.06f, 0.0f, FLT_MAX);
        } else {  // coming down: fast, tilting forward
            nx += std::sin(dir_) * speedDown;
            nz += std::cos(dir_) * speedDown;
            frame_ = 1;
            pitch_ = MakeAngleCloseTo(pitch_, 0.349066f, 0.06f, 0.0f, FLT_MAX);
        }
        float ny = floorLevel_ * 25.0f + std::sin((float)localCounter_ / (float)duration * kPi) * height;
        UpdatePosX(nx);
        UpdatePosZ(nz);
        UpdatePosY(ny);
        if (localCounter_ >= duration) {
            frame_ = 0;
            pitch_ = 0;
            localCounter_ = 0;
            moveState_ = WAIT;
        }
    }
    void LongWait() {
        if (localCounter_ < 60) return;
        countdownJump_--;
        dir_ = MakeAngleCloseTo_Constant(dir_, FindDirectionXZ(pos_, GenerateRandomPos()), 2.0944f);
        if (countdownJump_ <= 0) {
            countdownJump_ = (int)Random(4) + 3;
            if (game_->DistanceToCloserHero(pos_) < 120.0f) dir_ = game_->DirToCloserHero(pos_);
            localCounter_ = 0;
            moveState_ = JUMP;
        } else {
            localCounter_ = 0;
            moveState_ = WALK;
        }
    }

    MoveState moveState_ = NON;
    int countdownLongWait_ = 0, countdownJump_ = 0;
    D3DVECTOR prevPosForWalk_{0, 0, 0};
};

// ------------------------------------------------------------------------------ XEnemy_Needle
// Floor spikes: 34 wide, 70 tall, sunk 40.5 into the floor. 'p' spikes sink 30 and rise again
// over 50 ticks each, waiting (200 - 20 x world) ticks in between; 'o' spikes stay. They can't be
// hurt, and don't hurt a hero who is still flashing.
class Needle : public XEnemyBase {
public:
    enum MoveState { NON = 0, WAIT = 1, WAIT2 = 2, MOVEUP = 3, MOVEDOWN = 4 };
    Needle(const XGameContext* g, int oid, const D3DVECTOR& p, bool movable)
        : XEnemyBase(g, oid, p, SMALL), movable_(movable) {
        model_ = std::make_unique<JObject3DX>(0x18a);
        model_->SetCollisionModel(0x1c8);
        collideBox_ = std::make_unique<XCollideBox>(34.0f, 70.0f);
        collideBox_->SetPos(startPos_);
        startPos_.y -= 40.5f;
        SetPosY(startPos_.y);
        UpdateWorldMatrix();
        noCollisionWhenHeroDamaged_ = true;
        damagable_ = false;
        liveState_ = XEnemyBase::NORMAL;
        moveState_ = WAIT;
    }

    void ProcessFrame() override {
        XEnemyBase::ProcessFrame();
        if (liveState_ != XEnemyBase::NORMAL) return;
        unsigned wait = (unsigned)(200 - game_->world * 20);
        switch (moveState_) {
            case WAIT:
                if (movable_ && localCounter_ >= wait) {
                    localCounter_ = 0;
                    moveState_ = MOVEDOWN;
                }
                break;
            case WAIT2:
                if (localCounter_ >= wait) {
                    localCounter_ = 0;
                    moveState_ = MOVEUP;
                }
                break;
            case MOVEUP:
                SetPosY(startPos_.y + std::sin((float)localCounter_ / 50.0f * kHalfPi) * 30.0f - 30.0f);
                if (localCounter_ >= 50) {
                    localCounter_ = 0;
                    moveState_ = WAIT;
                }
                break;
            case MOVEDOWN:
                SetPosY(startPos_.y - std::sin((float)localCounter_ / 50.0f * kHalfPi) * 30.0f);
                if (localCounter_ >= 50) {
                    localCounter_ = 0;
                    moveState_ = WAIT2;
                }
                break;
            default: break;
        }
        UpdateWorldMatrix();
        CollisionCheck_Hero(game_->heroA);
    }

private:
    bool movable_;
    MoveState moveState_ = NON;
};
// --------------------------------------------------------------------------------- XEnemy_Bee
// 'b' bee (31 x 55, 2 health), '3' king bee (62 x 110, 51 health). Hovers 5 above its floor,
// bobbing 4 up and down over 80 ticks, and drifts at 0.5 (king 0.6) for 120 ticks at a time
// towards a random point, or round behind itself after being stuck, turning at most 1 degree a
// tick; within 120 of a hero it turns towards the hero instead.
class Bee : public XEnemyBase {
public:
    Bee(const XGameContext* g, int oid, const D3DVECTOR& p, SizeType size) : XEnemyBase(g, oid, p, size) {
        bool king = size == KING;
        static const uint16_t kSmall[4] = {0x171, 0x174, 0x173, 0x174}, kKing[4] = {0x2b8, 0x2b7, 0x2b6, 0x2b7};
        const uint16_t* f = king ? kKing : kSmall;
        model_ = std::make_unique<JObject3DX>(f[0]);
        for (int i = 1; i < 4; i++) model_->AppendFrame(f[i]);
        model_->SetCollisionModel(king ? 0x2b9 : 0x178);
        shadow_ = std::make_unique<XShadowSimple>(king ? 62.0f : 31.0f);
        collideBox_ = king ? std::make_unique<XCollideBox>(62.0f, 110.0f) : std::make_unique<XCollideBox>(31.0f, 55.0f);
        collideBox_->SetPos(startPos_);
        health_ = king ? 51 : 2;
        dir_ = UnitRand() * 6.28319f;
        yaw_ = dir_;
        startPosY_ = startPos_.y + 5.0f;
        SetPosY(startPosY_);
        UpdateWorldMatrix();
        beeCounter_ = (unsigned)Random(80);
        chasable_ = true;
        liveState_ = XEnemyBase::NORMAL;
    }
    void ProcessFrame() override {
        if (!game_->IsUpdatable(pos_)) return;
        XEnemyBase::ProcessFrame();
        if (liveState_ != XEnemyBase::NORMAL) return;
        beeCounter_++;
        frame_ = (int)((beeCounter_ >> 2) % 4);
        UpdatePosY(startPosY_ + std::sin((float)(beeCounter_ % 80) / 80.0f * 6.28319f) * 4.0f);
        Walk();
        UpdateWorldMatrix();
        CollisionCheck_Hero(game_->heroA);
    }

private:
    void Walk() {
        float speed = 0.5f, turn = 0.0174533f;
        if (sizeType_ == KING) speed *= 1.2f;
        if (localCounter_ == 1) {
            prevPosForWalk_ = pos_;
            targetPos_ = turnAround_ ? GenerateBehindPos() : GenerateRandomPos();
        }
        if (!turnAround_ && game_->DistanceToCloserHero(pos_) < 120.0f)
            dir_ = MakeAngleCloseTo_Constant(dir_, game_->DirToCloserHero(pos_), turn);
        else
            dir_ = MakeAngleCloseTo_Constant(dir_, FindDirectionXZ(pos_, targetPos_), turn);
        yaw_ = dir_;
        UpdatePosX(pos_.x + std::sin(dir_) * speed);
        UpdatePosZ(pos_.z + std::cos(dir_) * speed);
        if (localCounter_ >= 120) {
            turnAround_ = ApproxDistance(prevPosForWalk_, pos_) < 10.0f;
            localCounter_ = 0;
        }
    }
    float startPosY_ = 0;
    unsigned beeCounter_ = 0;
    bool turnAround_ = false;
    D3DVECTOR targetPos_{0, 0, 0}, prevPosForWalk_{0, 0, 0};
};

// ----------------------------------------------------------------------------- XEnemy_Wallaby
// 'd' wallaby (35 x 44, 2 health), '0' king wallaby (70 x 88, 45 health). Waits 60-240 ticks,
// then hops 2-6 times: 70 ticks in the air (up to 44 high, 0.65 forward; king 66 and 0.975)
// plus 10 on the ground, turning up to 40 degrees (king 48) towards a random point (or behind
// itself after being stuck); if a hero was within 120 when it set off, 1.5 times as fast towards
// the hero.
class Wallaby : public XEnemyBase {
public:
    Wallaby(const XGameContext* g, int oid, const D3DVECTOR& p, SizeType size) : XEnemyBase(g, oid, p, size) {
        bool king = size == KING;
        static const uint16_t kSmall[3] = {0x1a7, 0x1a8, 0x1a9}, kKing[3] = {0x1e5, 0x1e6, 0x1e7};
        const uint16_t* f = king ? kKing : kSmall;
        model_ = std::make_unique<JObject3DX>(f[0]);
        for (int i = 1; i < 3; i++) model_->AppendFrame(f[i]);
        model_->SetCollisionModel(king ? 0x1e8 : 0x1aa);
        shadow_ = std::make_unique<XShadowSimple>(king ? 70.0f : 35.0f);
        collideBox_ = king ? std::make_unique<XCollideBox>(70.0f, 88.0f) : std::make_unique<XCollideBox>(35.0f, 44.0f);
        collideBox_->SetPos(startPos_);
        health_ = king ? 45 : 2;
        dir_ = UnitRand() * 6.28319f;
        yaw_ = dir_;
        UpdateWorldMatrix();
        chasable_ = true;
        liveState_ = XEnemyBase::NORMAL;
        moveState_ = WAIT;
    }
    void ProcessFrame() override {
        if (!game_->IsUpdatable(pos_)) return;
        XEnemyBase::ProcessFrame();
        if (liveState_ != XEnemyBase::NORMAL) return;
        if (moveState_ == HOP) Hop();
        else if (moveState_ == WAIT) Wait();
        yaw_ = MakeAngleCloseTo(yaw_, dir_, 0.1f, 0.0f, FLT_MAX);
        UpdateWorldMatrix();
        CollisionCheck_Hero(game_->heroA);
    }

private:
    enum MoveState { NON = 0, HOP = 1, WAIT = 2 };
    void Wait() {
        if (localCounter_ == 1) waitFrames_ = (unsigned)RandomI(60, 240, true);
        if (localCounter_ < waitFrames_) return;
        targetPos_ = turnAround_ ? GenerateBehindPos() : GenerateRandomPos();
        chaseHero_ = game_->DistanceToCloserHero(pos_) < 120.0f;
        countdownHop_ = (int)RandomI(2, 6, true);
        localCounter_ = 0;
        moveState_ = HOP;
    }
    void Hop() {
        float height = 44.0f, speed = 0.65f, turn = 0.698132f;
        if (sizeType_ == KING) height *= 1.5f, speed *= 1.5f, turn *= 1.2f;
        if (localCounter_ == 1) {
            prevPosForHop_ = pos_;
            if (!turnAround_ && chaseHero_)
                dir_ = MakeAngleCloseTo_Constant(dir_, game_->DirToCloserHero(pos_), turn * 1.5f);
            else
                dir_ = MakeAngleCloseTo_Constant(dir_, FindDirectionXZ(pos_, targetPos_), turn);
        }
        if (localCounter_ <= 70) {
            if (localCounter_ < 35) {
                frame_ = 1;
                pitch_ = MakeAngleCloseTo(pitch_, 0.174533f, 0.1f, 0.0f, FLT_MAX);
            } else {
                frame_ = 2;
                pitch_ = MakeAngleCloseTo(pitch_, -0.20944f, 0.1f, 0.0f, FLT_MAX);
            }
            float nx = pos_.x + std::sin(dir_) * speed, nz = pos_.z + std::cos(dir_) * speed;
            float ny = floorLevel_ * 25.0f;
            if (localCounter_ < 70) ny += std::sin((float)localCounter_ / 70.0f * kPi) * height;
            UpdatePosX(nx);
            UpdatePosZ(nz);
            UpdatePosY(ny);
        } else {
            frame_ = 0;
            pitch_ = MakeAngleCloseTo(pitch_, 0.0f, 0.2f, 0.0f, FLT_MAX);
        }
        if (localCounter_ < 80) return;
        if (!turnAround_ && ApproxDistance(prevPosForHop_, pos_) < 10.0f) {
            turnAround_ = true;  // stuck: wait, then turn round
            localCounter_ = 0;
            moveState_ = WAIT;
        } else if (--countdownHop_ <= 0) {
            turnAround_ = false;
            localCounter_ = 0;
            moveState_ = WAIT;
        } else {
            localCounter_ = 0;
            moveState_ = HOP;
        }
    }
    MoveState moveState_ = NON;
    unsigned waitFrames_ = 0;
    int countdownHop_ = 0;
    bool chaseHero_ = false, turnAround_ = false;
    D3DVECTOR targetPos_{0, 0, 0}, prevPosForHop_{0, 0, 0};
};

// ---------------------------------------------------------------------------- XEnemy_Mushroom
// 'h' mushroom (37 x 44, 3 health), '2' king mushroom (74 x 88, 30 health). The wallaby's hop
// with its own numbers: waits 50-200 ticks, then hops 3-5 times: 80 ticks in the air (up to 48
// high, 0.65 forward; king 72 and 0.975) plus 5 on the ground, turning up to 40 degrees (king
// 50) towards a random point (or behind itself after being stuck); if a hero was within 120 when
// it set off, 1.5 times as fast towards the hero.
class Mushroom : public XEnemyBase {
public:
    Mushroom(const XGameContext* g, int oid, const D3DVECTOR& p, SizeType size) : XEnemyBase(g, oid, p, size) {
        bool king = size == KING;
        static const uint16_t kSmall[3] = {0x25d, 0x25e, 0x25f}, kKing[3] = {0x285, 0x286, 0x287};
        const uint16_t* f = king ? kKing : kSmall;
        model_ = std::make_unique<JObject3DX>(f[0]);
        for (int i = 1; i < 3; i++) model_->AppendFrame(f[i]);
        model_->SetCollisionModel(king ? 0x288 : 0x260);
        shadow_ = std::make_unique<XShadowSimple>(king ? 74.0f : 37.0f);
        collideBox_ = king ? std::make_unique<XCollideBox>(74.0f, 88.0f) : std::make_unique<XCollideBox>(37.0f, 44.0f);
        collideBox_->SetPos(startPos_);
        health_ = king ? 30 : 3;
        dir_ = UnitRand() * 6.28319f;
        yaw_ = dir_;
        UpdateWorldMatrix();
        chasable_ = true;
        liveState_ = XEnemyBase::NORMAL;
        moveState_ = WAIT;
    }
    void ProcessFrame() override {
        if (!game_->IsUpdatable(pos_)) return;
        XEnemyBase::ProcessFrame();
        if (liveState_ != XEnemyBase::NORMAL) return;
        if (moveState_ == HOP) Hop();
        else if (moveState_ == WAIT) Wait();
        yaw_ = MakeAngleCloseTo(yaw_, dir_, 0.1f, 0.0f, FLT_MAX);
        UpdateWorldMatrix();
        CollisionCheck_Hero(game_->heroA);
    }

private:
    enum MoveState { NON = 0, HOP = 1, WAIT = 2 };
    void Wait() {
        if (localCounter_ == 1) waitFrames_ = (unsigned)RandomI(50, 200, true);
        if (localCounter_ < waitFrames_) return;
        targetPos_ = turnAround_ ? GenerateBehindPos() : GenerateRandomPos();
        chaseHero_ = game_->DistanceToCloserHero(pos_) < 120.0f;
        countdownHop_ = (int)RandomI(3, 5, true);
        localCounter_ = 0;
        moveState_ = HOP;
    }
    void Hop() {
        float height = 48.0f, speed = 0.65f, turn = 0.698132f;
        if (sizeType_ == KING) height *= 1.5f, speed *= 1.5f, turn *= 1.25f;
        if (localCounter_ == 1) {
            prevPosForHop_ = pos_;
            if (!turnAround_ && chaseHero_)
                dir_ = MakeAngleCloseTo_Constant(dir_, game_->DirToCloserHero(pos_), turn * 1.5f);
            else
                dir_ = MakeAngleCloseTo_Constant(dir_, FindDirectionXZ(pos_, targetPos_), turn);
        }
        if (localCounter_ <= 80) {
            if (localCounter_ < 40) {
                frame_ = 1;
                pitch_ = MakeAngleCloseTo(pitch_, 0.174533f, 0.1f, 0.0f, FLT_MAX);
            } else {
                frame_ = 2;
                pitch_ = MakeAngleCloseTo(pitch_, -0.20944f, 0.1f, 0.0f, FLT_MAX);
            }
            float nx = pos_.x + std::sin(dir_) * speed, nz = pos_.z + std::cos(dir_) * speed;
            float ny = floorLevel_ * 25.0f;
            if (localCounter_ < 80) ny += std::sin((float)localCounter_ / 80.0f * kPi) * height;
            UpdatePosX(nx);
            UpdatePosZ(nz);
            UpdatePosY(ny);
        } else {
            frame_ = 0;
            pitch_ = MakeAngleCloseTo(pitch_, 0.0f, 0.2f, 0.0f, FLT_MAX);
        }
        if (localCounter_ < 85) return;
        if (!turnAround_ && ApproxDistance(prevPosForHop_, pos_) < 10.0f) {
            turnAround_ = true;  // stuck: wait, then turn round
            localCounter_ = 0;
            moveState_ = WAIT;
        } else if (--countdownHop_ <= 0) {
            turnAround_ = false;
            localCounter_ = 0;
            moveState_ = WAIT;
        } else {
            localCounter_ = 0;
            moveState_ = HOP;
        }
    }
    MoveState moveState_ = NON;
    unsigned waitFrames_ = 0;
    int countdownHop_ = 0;
    bool chaseHero_ = false, turnAround_ = false;
    D3DVECTOR targetPos_{0, 0, 0}, prevPosForHop_{0, 0, 0};
};

// ------------------------------------------------------------------------------ XEnemy_Barrel
// 'j': a barrel to shoot open (10 health, model 0x1f6, a 27 x 42.8 box that also stops heroes and
// enemies but not shots). It bursts at once: a medium coin, or a heart for every third barrel
// broken in the game (sound 0x267). It never hurts.
class BarrelEnemy : public XEnemyBase {
public:
    BarrelEnemy(const XGameContext* g, int oid, const D3DVECTOR& p) : XEnemyBase(g, oid, p, SMALL) {
        model_ = std::make_unique<JObject3DX>(0x1f6);
        model_->SetCollisionModel(0x204);
        shadow_ = std::make_unique<XShadowSimple>(37.0f);
        collideBox_ = std::make_unique<XCollideBox>(27.0f, 42.8f);
        collideBox_->SetPos(pos_);
        blocker_ = g->statics->CreateCollideBox(startPos_, 27.0f, 42.8f, 27.0f);
        if (blocker_) blocker_->SetBlockFire(false);
        UpdateWorldMatrix();
        sizeType_ = KING;
        health_ = 10;
        chasable_ = false;
        liveState_ = XEnemyBase::NORMAL;
    }
    void ProcessFrame() override {
        if (liveState_ == XEnemyBase::NON) return;
        localCounter_++;
        if (damageCounter_ > 0 && --damageCounter_ <= 0) model_->UseWhiteMaterial(false);
        if (liveState_ != XEnemyBase::DEAD) return;
        pos_.y += collideBox_->Height() * 0.5f;
        game_->particles->CreateEnemyExplode(pos_, false);
        if (blocker_) blocker_->SetEnabled(false);
        int n = game_->barrelCounter ? ++*game_->barrelCounter : 1;
        if (n % 3 == 0) {
            if (game_->createHeart) game_->createHeart(*this);
        } else if (game_->createEnemyCoin) {
            game_->createEnemyCoin(*this);
        }
        game_->playSound(0x267, pos_.z);
        liveState_ = XEnemyBase::NON;
    }

private:
    XStaticObjBase* blocker_ = nullptr;
};

// --------------------------------------------------------------------------- XEnemy_FenceDoor
// 'v' (across X) / 'w' (along Z, turned -90 degrees): a breakable fence panel, 40 x 82 x 10
// (5 health, model 0x1dc). Its box also stops heroes and enemies but not shots. It just
// bursts (sound 0x267), with no coin.
class FenceDoor : public XEnemyBase {
public:
    FenceDoor(const XGameContext* g, int oid, const D3DVECTOR& p, bool alongZ) : XEnemyBase(g, oid, p, SMALL) {
        model_ = std::make_unique<JObject3DX>(0x1dc);
        model_->SetCollisionModel(0x1dd);
        float wx = alongZ ? 10.0f : 40.0f, wz = alongZ ? 40.0f : 10.0f;
        if (alongZ) yaw_ = -kHalfPi;
        collideBox_ = std::make_unique<XCollideBox>(wx, 82.0f, wz);
        collideBox_->SetPos(startPos_);
        blocker_ = g->statics->CreateCollideBox(startPos_, wx, 82.0f, wz);
        if (blocker_) blocker_->SetBlockFire(false);
        UpdateWorldMatrix();
        health_ = 5;
        chasable_ = false;
        liveState_ = XEnemyBase::NORMAL;
    }
    void ProcessFrame() override {
        if (liveState_ == XEnemyBase::NON) return;
        localCounter_++;
        if (damageCounter_ > 0 && --damageCounter_ <= 0) model_->UseWhiteMaterial(false);
        if (liveState_ != XEnemyBase::DEAD) return;
        pos_.y += collideBox_->Height() * 0.5f;
        game_->particles->CreateEnemyExplode(pos_, false);
        if (blocker_) blocker_->SetEnabled(false);
        game_->playSound(0x267, pos_.z);
        liveState_ = XEnemyBase::NON;
    }

private:
    XStaticObjBase* blocker_ = nullptr;
};

// ---------------------------------------------------------------------------- XEnemy_NeedleFan
// 'm': a spiked bar (model 0x17f, 200 wide and 80 tall) turning on a post (model 0x17e, a 19 x 87
// box) at (0.6 + 0.1 x world) degrees a tick. It can't be hurt.
class NeedleFan : public XEnemyBase {
public:
    NeedleFan(const XGameContext* g, int oid, const D3DVECTOR& p) : XEnemyBase(g, oid, p, SMALL) {
        model_ = std::make_unique<JObject3DX>(0x17f);
        model_->SetCollisionModel(0x17d);
        shadow_ = std::make_unique<XShadowSimple>(42.0f);
        collideBox_ = std::make_unique<XCollideBox>(200.0f, 80.0f);
        collideBox_->SetPos(startPos_);
        fanBase_ = std::make_unique<JObject3DX>(0x17e);
        fanBase_->SetPos(startPos_);
        fanBase_->UpdateWorldMatrix();
        g->statics->CreateCollideBox(startPos_, 19.0f, 87.0f, 19.0f);
        yaw_ = 0;
        UpdateWorldMatrix();
        damagable_ = false;
        liveState_ = XEnemyBase::NORMAL;
    }
    void ProcessFrame() override {
        XEnemyBase::ProcessFrame();
        if (liveState_ != XEnemyBase::NORMAL) return;
        yaw_ = NormaliseAngle(yaw_ + (0.6f + game_->world * 0.1f) * 0.0174533f);
        UpdateWorldMatrix();
        CollisionCheck_Hero(game_->heroA);
    }
    void Render(Device* d) override {
        // (drawn wherever it is: no visibility test)
        if (liveState_ == XEnemyBase::NON) return;
        model_->Render(d);
        fanBase_->Render(d);
        shadow_->SetPosXZ(startPos_);
        shadow_->Render(d);
    }

private:
    std::unique_ptr<JObject3DX> fanBase_;
};

// -------------------------------------------------------------------------------- XEnemy_Fish
// 'i': a fish swimming round a circle of radius 80 about (100, -62, cell z) under the water, a
// half turn at a time (1 degree a tick, the way picked at random), and after a few half turns
// (2 to 5 for type A, 1 to 2 for type B) jumping across the circle from one side to the other
// (x 20 to 180, 92 high at the top), with a wave each time it crosses the surface. Type A
// (model 0x203) lives in worlds 1-2, type B (0x2af) in 3-4. It can't be hurt or stopped; its
// 200 x 120 box stays at the centre, so the mesh decides what it touches.
class Fish : public XEnemyBase {
public:
    enum MoveState { NON = 0, SWIM1 = 1, SWIM2 = 2, JUMP1 = 3, JUMP2 = 4 };
    Fish(const XGameContext* g, int oid, const D3DVECTOR& p, bool typeA) : XEnemyBase(g, oid, p, SMALL), typeA_(typeA) {
        static const uint16_t kFramesA[5] = {0x203, 0x200, 0x203, 0x201, 0x202};
        static const uint16_t kFramesB[5] = {0x2af, 0x2b0, 0x2af, 0x2b1, 0x2b2};
        const uint16_t* f = typeA_ ? kFramesA : kFramesB;
        model_ = std::make_unique<JObject3DX>(f[0]);
        for (int i = 1; i < 5; i++) model_->AppendFrame(f[i]);
        model_->SetCollisionModel(0x1ff);
        startPos_.x = 100.0f;
        startPos_.y = -62.0f;
        collideBox_ = std::make_unique<XCollideBox>(200.0f, 120.0f);
        collideBox_->SetPos(startPos_);
        posAngle_ = 0;
        pos_ = GetPosAtAngle(posAngle_);
        UpdateWorldMatrix();
        clockwise_ = Random(2) == 0;
        jumpReadyTimer_ = typeA_ ? (int)RandomI(2, 5, true) : (int)RandomI(1, 2, true);
        damagable_ = false;
        chasable_ = false;
        collidable_ = false;
        liveState_ = XEnemyBase::NORMAL;
        moveState_ = SWIM1;
    }
    void ProcessFrame() override {
        if (!game_->IsUpdatable(pos_)) return;
        XEnemyBase::ProcessFrame();
        if (liveState_ != XEnemyBase::NORMAL) return;
        // one state a tick (a new state starts on the next)
        if (moveState_ == SWIM1) Swim(0.0f, kPi, -kPi, SWIM2, JUMP2);
        else if (moveState_ == SWIM2) Swim(kPi, 2.0f * kPi, 0.0f, SWIM1, JUMP1);
        else if (moveState_ == JUMP1 || moveState_ == JUMP2) Jump();
        UpdateWorldMatrix();
        CollisionCheck_Hero(game_->heroA);
    }

private:
    static constexpr float kPi = 3.14159f;

    // XEnemy_Fish::GetPosAtAngle: on the circle, angle 0 at the back (x 20), turning towards +Z
    D3DVECTOR GetPosAtAngle(float a) const {
        a -= 1.5708f;
        return D3DVECTOR(std::sin(a) * 80.0f + startPos_.x, startPos_.y, std::cos(a) * 80.0f + startPos_.z);
    }
    // _ProcessFrame_Swim1 (0 to +-180 degrees) and _ProcessFrame_Swim2 (180 to 360 or 0): half a
    // turn, then the next half turn or, when the countdown runs out, a jump
    void Swim(float from, float clockwiseEnd, float otherEnd, MoveState next, MoveState jump) {
        if (localCounter_ == 1) posAngle_ = from;
        if (clockwise_) {
            posAngle_ += 0.0174533f;
            if (posAngle_ >= clockwiseEnd) posAngle_ = clockwiseEnd;
        } else {
            posAngle_ -= 0.0174533f;
            if (otherEnd >= posAngle_) posAngle_ = otherEnd;
        }
        const D3DVECTOR prev = pos_;
        pos_ = GetPosAtAngle(posAngle_);
        yaw_ = MakeAngleCloseTo(yaw_, FindDirectionXZ(prev, pos_), 0.1f, 0.0f, FLT_MAX);
        pitch_ = MakeAngleCloseTo(pitch_, 0.0f, 0.1f, 0.0f, FLT_MAX);
        walkCounter_++;
        frame_ = (int)((walkCounter_ / 12) % 4);
        if (posAngle_ >= clockwiseEnd || otherEnd >= posAngle_) {
            localCounter_ = 0;
            moveState_ = --jumpReadyTimer_ <= 0 ? jump : next;
        }
    }
    // _ProcessFrame_Jump: JUMP1 from x 20 to 180, JUMP2 back, 0.008 of the way a tick, arcing 92 up
    void Jump() {
        if (localCounter_ == 1) {
            jumpTravel_ = 0;
            D3DVECTOR back(startPos_.x - 80.0f, startPos_.y, startPos_.z), front(startPos_.x + 80.0f, startPos_.y, startPos_.z);
            jumpStart_ = moveState_ == JUMP1 ? back : front;
            jumpEnd_ = moveState_ == JUMP1 ? front : back;
        }
        jumpTravel_ += 0.008f;
        if (jumpTravel_ >= 1.0f) jumpTravel_ = 1.0f;
        const D3DVECTOR prev = pos_;
        pos_ = D3DVectorInterpolate(jumpStart_, jumpEnd_, jumpTravel_);
        pos_.y += std::sin(kPi * jumpTravel_) * 92.0f;
        yaw_ = MakeAngleCloseTo(yaw_, FindDirectionXZ(prev, pos_), 0.1f, 0.0f, FLT_MAX);
        pitch_ = MakeAngleCloseTo(pitch_, -FindPitch(prev, pos_), 0.1f, 0.0f, FLT_MAX);
        if (jumpTravel_ > 0.3f && 0.7f > jumpTravel_) {
            frame_ = 4;  // mid-air
        } else {
            walkCounter_++;
            frame_ = (int)((walkCounter_ / 12) % 4);
        }
        // a wave where it crosses the surface
        if (game_->useWater && (prev.y < -38.0f) != (pos_.y < -38.0f))
            game_->particles->CreateWaterWave(D3DVECTOR(pos_.x, -38.0f, pos_.z), false, true);
        if (jumpTravel_ >= 1.0f) {
            clockwise_ = Random(2) == 0;
            jumpReadyTimer_ = typeA_ ? (int)RandomI(2, 5, true) : (int)RandomI(1, 2, true);
            localCounter_ = 0;
            moveState_ = moveState_ == JUMP1 ? SWIM2 : SWIM1;
        }
    }

    MoveState moveState_ = NON;
    bool typeA_, clockwise_ = true;
    float posAngle_ = 0, jumpTravel_ = 0;
    unsigned walkCounter_ = 0;
    int jumpReadyTimer_ = 0;
    D3DVECTOR jumpStart_{0, 0, 0}, jumpEnd_{0, 0, 0};
};
}  // namespace

// ---------------------------------------------------------------------------------- XGameContext

float XGameContext::DistanceToCloserHero(const D3DVECTOR& p) const {
    float d = FLT_MAX;
    if (heroA && heroA->GetLiveState() == XHero::NORMAL) d = ApproxDistance(p, heroA->GetPos());
    return d;
}

float XGameContext::DirToCloserHero(const D3DVECTOR& p) const {
    // GetCloserHero: player 1 unless player 2 is strictly closer
    return heroA ? FindDirectionXZ(p, heroA->GetPos()) : 0.0f;
}

namespace {
bool FloorEdge(const XGameContext& g, int x, int z, int level) {
    // XMap::IsFloorLevelDifferent (outside counts) or IsInvisibleBlock ('!')
    const MapCollision& m = g.level->Collision();
    if (x < 0 || x >= LevelGrid::kWidth || z < 0 || z >= m.Depth()) return true;
    return m.GetFloorLevel(x, z) != level || g.level->ObjectCode(x, z) == '!';
}
}  // namespace

bool XGameContext::CollisionCheckX_Floor(float& newX, const XCollideBox& box, int level) const {
    int x0 = MapCollision::CellX(newX - box.WidthHalfX(), true), x1 = MapCollision::CellX(newX + box.WidthHalfX(), false);
    int za = MapCollision::CellZ(box.Node1().z, true), zb = MapCollision::CellZ(box.Pos().z, true),
        zc = MapCollision::CellZ(box.Node2().z, false);
    if (box.Pos().x > newX) {
        if (FloorEdge(*this, x0, za, level) || FloorEdge(*this, x0, zb, level) || FloorEdge(*this, x0, zc, level)) {
            newX = (x0 + 1) * 40.0f + box.WidthHalfX();
            return true;
        }
    } else if (newX > box.Pos().x) {
        if (FloorEdge(*this, x1, za, level) || FloorEdge(*this, x1, zb, level) || FloorEdge(*this, x1, zc, level)) {
            newX = x1 * 40.0f - box.WidthHalfX();
            return true;
        }
    }
    return false;
}

bool XGameContext::CollisionCheckZ_Floor(float& newZ, const XCollideBox& box, int level) const {
    int z0 = MapCollision::CellZ(newZ - box.WidthHalfZ(), true), z1 = MapCollision::CellZ(newZ + box.WidthHalfZ(), false);
    int xa = MapCollision::CellX(box.Node1().x, true), xb = MapCollision::CellX(box.Pos().x, true),
        xc = MapCollision::CellX(box.Node2().x, false);
    if (box.Pos().z > newZ) {
        if (FloorEdge(*this, xa, z0, level) || FloorEdge(*this, xb, z0, level) || FloorEdge(*this, xc, z0, level)) {
            newZ = (z0 + 1) * 40.0f + box.WidthHalfZ();
            return true;
        }
    } else if (newZ > box.Pos().z) {
        if (FloorEdge(*this, xa, z1, level) || FloorEdge(*this, xb, z1, level) || FloorEdge(*this, xc, z1, level)) {
            newZ = z1 * 40.0f - box.WidthHalfZ();
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------------- XEnemyBase

XEnemyBase::XEnemyBase(const XGameContext* game, int oid, const D3DVECTOR& pos, SizeType size)
    : game_(game), oid_(oid), sizeType_(size), startPos_(pos), pos_(pos) {}
XEnemyBase::~XEnemyBase() = default;

void XEnemyBase::SetFloorLevel(int level) {
    floorLevel_ = level;
    if (shadow_) shadow_->SetFloorLevel(level);
}

void XEnemyBase::SetPosX(float x) {
    pos_.x = x;
    collideBox_->SetPosX(x);
}
void XEnemyBase::SetPosY(float y) {
    pos_.y = y;
    collideBox_->SetPosY(y);
}
void XEnemyBase::SetPosZ(float z) {
    pos_.z = z;
    collideBox_->SetPosZ(z);
}

bool XEnemyBase::UpdatePosX(float x) {
    bool a = game_->enemies->CollisionCheckX(x, *this);
    bool b = game_->statics->CollisionCheckX(x, *collideBox_, true);
    bool c = game_->platforms->CollisionCheckX(x, *collideBox_, true);
    bool d = game_->CollisionCheckX_Floor(x, *collideBox_, floorLevel_);
    SetPosX(x);
    return a || b || c || d;
}

bool XEnemyBase::UpdatePosZ(float z) {
    bool a = game_->enemies->CollisionCheckZ(z, *this);
    bool b = game_->statics->CollisionCheckZ(z, *collideBox_, true);
    bool c = game_->platforms->CollisionCheckZ(z, *collideBox_, true);
    bool d = game_->CollisionCheckZ_Floor(z, *collideBox_, floorLevel_);
    SetPosZ(z);
    return a || b || c || d;
}

void XEnemyBase::UpdateWorldMatrix() {
    model_->SetRoll(roll_);
    model_->SetPitch(pitch_);
    model_->SetYaw(yaw_);
    model_->SetPos(pos_);
    model_->UpdateWorldMatrix();
}

D3DVECTOR XEnemyBase::GenerateBehindPos() const {
    // 40 ahead along a direction 120..240 degrees off the current one
    float a = NormaliseAngle(dir_ + RandomF(120.0f, 240.0f) * 0.0174533f);
    return D3DVECTOR(pos_.x + std::sin(a) * 40.0f, pos_.y, pos_.z + std::cos(a) * 40.0f);
}

D3DVECTOR XEnemyBase::GenerateRandomPos() const {
    D3DVECTOR p;
    p.x = RandomF(20.0f, 180.0f);
    p.z = RandomF(startPos_.z - 140.0f, startPos_.z + 140.0f);
    p.y = startPos_.y;
    return p;
}

void XEnemyBase::ProcessFrame() {
    if (liveState_ == NON) return;
    localCounter_++;
    if (damageCounter_ > 0 && --damageCounter_ <= 0) model_->UseWhiteMaterial(false);
    if (liveState_ != DEAD) return;
    // dying: a hop while spinning, then the burst
    if (localCounter_ == 1) {
        game_->playSound(kSoundEnemyDie, pos_.z);
        verticalSpeed_ = 4.0f;
    }
    SetPosY(pos_.y + verticalSpeed_);
    yaw_ = NormaliseAngle(yaw_ + 0.244346f);
    verticalSpeed_ -= 0.08f;
    if (verticalSpeed_ < -3.0f) {
        pos_.y += collideBox_->Height() * 0.5f;
        game_->particles->CreateEnemyExplode(pos_, sizeType_ != SMALL);
        if (game_->createEnemyCoin) game_->createEnemyCoin(*this);
        game_->playSound(kSoundEnemyBurst, pos_.z);
        liveState_ = NON;
        return;
    }
    UpdateWorldMatrix();
}

void XEnemyBase::Render(Device* d) {
    if (liveState_ == NON || !game_->IsVisible(pos_)) return;
    if (shadow_) {
        shadow_->SetPosXZ(pos_);
        shadow_->Render(d);
    }
    model_->SetCurrentFrame(frame_);
    model_->Render(d);
}

bool XEnemyBase::CollisionCheck_Fire(const D3DVECTOR& pos, float radius, const D3DVECTOR& heroPos) {
    if (liveState_ != NORMAL) return false;
    if (std::fabs(pos_.z - pos.z) > collideBox_->WidthZ() + radius) return false;
    if (std::fabs(pos_.x - pos.x) > collideBox_->WidthZ() + radius) return false;  // (Z width, as the original)
    if (!model_->CheckCollision_Sphere(pos, radius)) return false;
    Damage(heroPos);
    return true;
}

void XEnemyBase::Damage(const D3DVECTOR& heroPos) {
    if (!damagable_) return;
    game_->playSound(kSoundEnemyHurt, heroPos.z);
    damageCounter_ = 12;
    model_->UseWhiteMaterial(true);
    if (--health_ <= 0) {
        localCounter_ = 0;
        liveState_ = DEAD;
    }
}

bool XEnemyBase::CollisionCheck_Hero(XHero* hero) {
    if (liveState_ != NORMAL || !hero || hero->GetLiveState() != XHero::NORMAL) return false;
    if (noCollisionWhenHeroDamaged_ && hero->BeingDamage()) return false;
    if (!collideBox_->CollisionCheck(hero->GetCollideBox())) return false;
    if (!model_->CheckCollision(hero->BodyModel())) return false;
    hero->Damage(*game_->heroWorld);
    hero->KnockBack(pos_);
    return true;
}

// ---------------------------------------------------------------------------------- XEnemyList

XEnemyList::~XEnemyList() = default;

void XEnemyList::Empty() {
    for (auto& s : slots_) s.reset();
    lastOID_ = 0;
}

int XEnemyList::GetNullIndex() const {
    for (int i = 0; i < kSlots; i++)
        if (!slots_[i]) return i;
    return -1;
}

XEnemyBase* XEnemyList::CreateEnemy(const D3DVECTOR& pos, char code) {
    int i = GetNullIndex();
    if (i < 0) return nullptr;
    std::unique_ptr<XEnemyBase> e;
    switch (code) {
        case 'a': e = std::make_unique<Slime>(game_, lastOID_, pos, XEnemyBase::SMALL); break;
        case '1': e = std::make_unique<Slime>(game_, lastOID_, pos, XEnemyBase::KING); break;
        case '5': e = std::make_unique<Slime>(game_, lastOID_, pos, XEnemyBase::QUEEN); break;
        case 'b': e = std::make_unique<Bee>(game_, lastOID_, pos, XEnemyBase::SMALL); break;
        case '3': e = std::make_unique<Bee>(game_, lastOID_, pos, XEnemyBase::KING); break;
        case 'd': e = std::make_unique<Wallaby>(game_, lastOID_, pos, XEnemyBase::SMALL); break;
        case '0': e = std::make_unique<Wallaby>(game_, lastOID_, pos, XEnemyBase::KING); break;
        case 'h': e = std::make_unique<Mushroom>(game_, lastOID_, pos, XEnemyBase::SMALL); break;
        case '2': e = std::make_unique<Mushroom>(game_, lastOID_, pos, XEnemyBase::KING); break;
        case 'j': e = std::make_unique<BarrelEnemy>(game_, lastOID_, pos); break;
        case 'm': e = std::make_unique<NeedleFan>(game_, lastOID_, pos); break;
        case 'v': e = std::make_unique<FenceDoor>(game_, lastOID_, pos, false); break;
        case 'w': e = std::make_unique<FenceDoor>(game_, lastOID_, pos, true); break;
        case 'o': e = std::make_unique<Needle>(game_, lastOID_, pos, false); break;
        case 'p': e = std::make_unique<Needle>(game_, lastOID_, pos, true); break;
        case 'i': e = std::make_unique<Fish>(game_, lastOID_, pos, game_->world < 2); break;
        default: break;  // the other enemies: stage 5
    }
    slots_[i] = std::move(e);
    lastOID_++;
    return slots_[i].get();
}

void XEnemyList::ProcessFrame() {
    for (auto& s : slots_) {
        if (!s) break;
        s->ProcessFrame();
    }
}

void XEnemyList::Render(Device* d) {
    for (auto& s : slots_) {
        if (!s) break;
        s->Render(d);
    }
}

bool XEnemyList::CollisionCheck_Fire(const D3DVECTOR& pos, float radius, const D3DVECTOR& heroPos) {
    for (auto& s : slots_) {
        if (!s) break;
        if (s->CollisionCheck_Fire(pos, radius, heroPos)) return true;
    }
    return false;
}

bool XEnemyList::CollisionCheckX(float& newX, const XEnemyBase& m) const {
    bool hit = false;
    for (auto& s : slots_) {
        if (!s) break;
        if (s->GetLiveState() != XEnemyBase::NORMAL || !s->Collidable() || s->GetOID() == m.GetOID()) continue;
        if (s->GetCollideBox().CollisionCheckX(newX, m.GetCollideBox(), true)) hit = true;
    }
    return hit;
}

bool XEnemyList::CollisionCheckZ(float& newZ, const XEnemyBase& m) const {
    bool hit = false;
    for (auto& s : slots_) {
        if (!s) break;
        if (s->GetLiveState() != XEnemyBase::NORMAL || !s->Collidable() || s->GetOID() == m.GetOID()) continue;
        if (s->GetCollideBox().CollisionCheckZ(newZ, m.GetCollideBox(), true)) hit = true;
    }
    return hit;
}
