// Bosses (XBossBase.cpp and XBoss_*.cpp of the original). One per level at most, made for the
// codes '6' Dragon, '7' Genie, '8' Nest and '9' Wizard (XMap::_ConstructObjects); it runs after
// the enemies and is drawn after them. A boss has a main health shown as hearts on the status
// display, each heart worth a number of hits (the sub health). When the last one goes the music
// stops; once the boss has gone (NON) for 500 ticks the goal opens.
//
// Built so far: the dragon, World 1's boss (and World 4-2's), and the genie, World 2's (and
// World 4-5's).
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "core/d3dtypes.h"
#include "game/JSpline.h"
#include "game/XCollideBox.h"
#include <memory>

class Device;
class JObject3DX;
class XHero;
class XShadow;
class XShadowSimple;
class XStaticObj_GenieLamp;
struct XGameContext;

class XBossBase {
public:
    enum LiveState { NON = 0, NORMAL = 1, DEAD = 2 };

    XBossBase(const XGameContext* game, const D3DVECTOR& pos);
    virtual ~XBossBase();
    virtual void Render(Device* d);
    virtual void ProcessFrame();
    // A hero's shot at `pos` with `radius`: true if it hit (the boss is hurt if it can be).
    virtual bool CollisionCheck_Fire(const D3DVECTOR& pos, float radius);

    // XBossBase::BossDefeated: gone for more than 500 ticks
    bool BossDefeated() const { return liveState_ == NON && localCounter_ > 500; }
    int GetHealthMain() const { return healthMain_; }
    LiveState GetLiveState() const { return liveState_; }
    const D3DVECTOR& GetPos() const { return pos_; }

protected:
    virtual void Damage();              // _Damage
    virtual void CheckBattleStarted();  // _CheckBattleStarted
    bool CollisionCheck_Hero(XHero* hero);  // _CollisionCheck_Hero
    void ProcessDamage();               // _ProcessDamage: the white flash wears off
    void SetPos(const D3DVECTOR& p);    // _SetPos
    void UpdateWorldMatrix();           // _UpdateWorldMatrix

    const XGameContext* game_;
    LiveState liveState_ = NON;
    unsigned localCounter_ = 0;
    D3DVECTOR startPos_, pos_;
    float dir_ = 0, roll_ = 0, pitch_ = 0, yaw_ = 0, scale_ = 1.0f;
    int healthMain_ = 10, healthMainMax_ = 10, healthSub_ = 0, healthSubMax_ = 0;
    int frame_ = 0, damageCounter_ = 0;
    bool damagable_ = true, battleStarted_ = false;
    D3DVECTOR explodeCenterPos_;
    float explodeRadius_ = 0;
    std::unique_ptr<JObject3DX> model_;
    std::unique_ptr<XShadowSimple> shadow_;
    std::unique_ptr<XCollideBox> collideBox_;
};

class XBoss_Dragon;

// XBoss_DragonBody: one of the 16 segments behind the dragon's head (the last is its tail). Each
// replays the head's recorded positions, 28 records further back than the one before it. They
// can't be hurt but stop shots; they burst one after another when the head dies.
class XBoss_DragonBody : public XBossBase {
public:
    XBoss_DragonBody(const XGameContext* game, XBoss_Dragon* head, int id);
    ~XBoss_DragonBody() override;
    void Render(Device* d) override;
    void ProcessFrame() override;
    bool CollisionCheck_Fire(const D3DVECTOR& pos, float radius) override;
    void StartDead();

private:
    int id_;
    std::unique_ptr<XShadow> bodyShadow_;
    XBoss_Dragon* head_;
    std::unique_ptr<XBoss_DragonBody> nextBody_;
};

// XBoss_Dragon ('6'): a sea dragon. It swims round a pool of 8 points by its start until a hero
// passes 160 ahead of it, then alternates between crossing the floor (in a zigzag, or straight
// through the middle breathing fire) and swimming round a second pool 640 ahead, all along
// JSpline curves. 10 hearts of 4 hits (6 in World 4).
class XBoss_Dragon : public XBossBase {
public:
    static constexpr int kRecords = 500;  // posRec / yawRec / pitchRec
    enum MoveState { NON = 0, SWIM1 = 1, SWIM2 = 2, FLOORWAVE1 = 3, FLOORWAVE2 = 4, FLOORSTRAIGHT1 = 5, FLOORSTRAIGHT2 = 6 };
    enum FlameState { NONFLAME = 0, STARTFLAME = 1, DOFLAME = 2, ENDFLAME = 3 };

    XBoss_Dragon(const XGameContext* game, const D3DVECTOR& pos);
    ~XBoss_Dragon() override;
    void Render(Device* d) override;
    void ProcessFrame() override;
    bool CollisionCheck_Fire(const D3DVECTOR& pos, float radius) override;

    // for the body segments
    int RecIndex() const { return recIndex_; }
    const D3DVECTOR& PosRec(int i) const { return posRec_[i]; }
    float YawRec(int i) const { return yawRec_[i]; }
    float PitchRec(int i) const { return pitchRec_[i]; }
    const D3DVECTOR& StartPos() const { return startPos_; }
    // _CheckSplash: a large wave where something moving from `a` to `b` crosses the surface
    void CheckSplash(const D3DVECTOR& a, const D3DVECTOR& b, bool sound) const;

private:
    void CheckBattleStarted() override;
    void InitFloorRefPos();
    void InitSwimRefPos1();
    void InitSwimRefPos2();
    D3DVECTOR GetRandomCenterPos() const;
    void StartFlame();
    void EndFlame();
    // the movements: a spline is built on the state's first tick, then followed
    void Swim1();
    void Swim2();
    void FloorWave1();
    void FloorWave2();
    void FloorStraight1();
    void FloorStraight2();
    bool FollowSpline();  // one step along the spline; true at its end

    MoveState moveState_ = NON;
    FlameState flameState_ = NONFLAME;
    D3DVECTOR prevPos_;
    JSpline spline_;
    float travel_ = 0, travelSpeed_ = 0;
    int flameCounter_ = 0;
    std::unique_ptr<XShadow> bodyShadow_;
    std::unique_ptr<XBoss_DragonBody> firstBody_;
    int recIndex_ = 0;
    D3DVECTOR floorRefPos_[11], swimRefPos1_[8], swimRefPos2_[8];
    D3DVECTOR posRec_[kRecords];
    float yawRec_[kRecords] = {}, pitchRec_[kRecords] = {};
};

// XBoss_Genie ('7'): a genie in a lamp (XStaticObj_GenieLamp, made at its start). It hides until
// a hero comes within 80 (XZ), the lamp shakes and steams for 500 ticks, then it grows out of the
// spout over 500 ticks, rising to 80 up and spinning to a stop. Then it cycles: idle 400-800
// ticks (50 fewer for each hit taken meanwhile) bobbing 8 and turning to the hero; opening its
// mouth (60); attacking (rounds of turning 30 ticks then firing, 30 shots, or until a hero is
// within 80); closing its mouth (100); gliding to the middle or 380 to either side. 10 hearts of
// 15 hits (World 4: 20, but the first heart still holds 15).
class XBoss_Genie : public XBossBase {
public:
    enum MoveState { NON = 0, HIDE, STEAM, APPEAR, IDLE, MOVE, OPENMOUTH, CLOSEMOUTH, ATTACK };
    enum MoveDest { CENTER = 0, LEFT = 1, RIGHT = 2 };
    enum AttackState { ATTACK_NON = 0, ATTACK_ROTATE = 1, ATTACK_FIRE = 2 };

    XBoss_Genie(const XGameContext* game, const D3DVECTOR& pos);
    void Render(Device* d) override;
    void ProcessFrame() override;

private:
    void Damage() override;
    D3DVECTOR GetMouthPos() const;  // _GetMouthPos
    void UpdatePosY();              // _UpdatePosY: the bob about refPosY
    void Hide();
    void Steam();
    void Appear();
    void Idle();
    void Move();
    void OpenMouth();
    void CloseMouth();
    void Attack();
    void Next(MoveState s) {
        localCounter_ = 0;
        moveState_ = s;
    }

    MoveState moveState_ = NON;
    MoveDest moveDest_ = CENTER;
    AttackState attackState_ = ATTACK_NON;
    float refPosY_ = 0, destYaw_ = 0;
    D3DVECTOR destPos_{0, 0, 0};
    unsigned offsetCounter_ = 0, attackCounter_ = 0;
    int idleFrames_ = 0;
    unsigned fireCount_ = 0;
    XStaticObj_GenieLamp* genieLamp_ = nullptr;  // owned by the static object list
};

