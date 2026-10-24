// Bosses (XBossBase.cpp and XBoss_*.cpp of the original). One per level at most, made for the
// codes '6' Dragon, '7' Genie, '8' Nest and '9' Wizard (XMap::_ConstructObjects); it runs after
// the enemies and is drawn after them. A boss has a main health shown as hearts on the status
// display, each heart worth a number of hits (the sub health). When the last one goes the music
// stops; once the boss has gone (NON) for 500 ticks the goal opens.
//
// Built so far: the dragon, World 1's boss (and World 4-2's), the genie, World 2's (and World
// 4-5's), the nest and its birds, World 3's (and World 4-8's), and the wizard with its blade,
// World 4's (its wand: stage 5 chunk 15b).
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

class XBoss_Nest;

// XBoss_Bird: one of the nest's 40 birds. It sits in the nest (WAIT) until the nest sends it out
// (StartAttack): GO rises out of the nest along a spline, ATTACK_HERO chases a point near the
// closer hero for 1800 ticks, FLY_ABOVE_NEST circles points above the nest for 400, BACK_TO_NEST
// glides back in along a spline, and it waits again. It can't be hurt and doesn't stop shots.
class XBoss_Bird : public XBossBase {
public:
    enum MoveState { WAIT = 0, GO = 1, ATTACK_HERO = 2, FLY_ABOVE_NEST = 3, BACK_TO_NEST = 4 };

    XBoss_Bird(const XGameContext* game, XBoss_Nest* nest);
    ~XBoss_Bird() override;
    void Render(Device* d) override;
    void ProcessFrame() override;
    void StartAttack();
    void StartDead();

private:
    void Wait() {}
    void Go();
    void AttackHero();
    void FlyAboveNest();
    void BackToNest();
    void Fly(float limitFloor);       // the steering ATTACK_HERO and FLY_ABOVE_NEST share
    void FollowSplineTurning();       // a step along the spline, facing the way it moves
    void UpdateTargetPos_Hero();      // _UpdateTargetPos_Hero
    void UpdateTargetPos_Nest();      // _UpdateTargetPos_Nest
    static void LimitPosition_Wall(D3DVECTOR& p, float margin);   // _LimitPosition_Wall
    static void LimitPosition_Floor(D3DVECTOR& p, float margin);  // _LimitPosition_Floor

    MoveState moveState_ = WAIT;
    XBoss_Nest* nest_;
    std::unique_ptr<XShadow> bodyShadow_;
    JSpline spline_;
    unsigned birdCounter_ = 0;
    float travel_ = 0, travelSpeed_ = 0;
    D3DVECTOR targetPos_{0, 0, 0};
    int chirpTimer_ = 0;
};

// XBoss_Nest ('8'): a nest floating in the water, turning slowly, with 40 birds in it. Each hit
// bobs it (SHAKE, with three water rings), sends the next bird out after the hero and turns it a
// little faster. 10 hearts of 15 hits (World 4: 20, but the first heart still holds 15); when it
// dies every bird out of the nest bursts and the ones still in it vanish.
class XBoss_Nest : public XBossBase {
public:
    static constexpr int kBirds = 40;
    enum MoveState { WAIT = 0, SHAKE = 1 };

    XBoss_Nest(const XGameContext* game, const D3DVECTOR& pos);
    ~XBoss_Nest() override;
    void Render(Device* d) override;
    void ProcessFrame() override;

private:
    void Damage() override;
    void Shake();  // _ProcessFrame_Shake

    MoveState moveState_ = WAIT;
    int currentBirdIndex_ = 0;
    float refPosY_ = 0, rotSpeed_ = 0;
    std::unique_ptr<XBoss_Bird> birdArray_[kBirds];
};

class XBoss_Wizard;

// XBoss_WizardBlade: a great blade the wizard holds out 38 in front of it, spinning up, holding
// at full speed for 800 ticks and spinning down. It has no box and can't hurt: it blows the hero
// along Z, away from the side the wizard is on (up to 1.375 a tick). The wizard runs and draws it.
class XBoss_WizardBlade : public XBossBase {
public:
    enum MoveState { NON = 0, APPEAR = 1, ROTATE_FASTER = 2, ROTATE_STABLE = 3, ROTATE_SLOWER = 4, DISAPPEAR = 5 };

    XBoss_WizardBlade(const XGameContext* game, XBoss_Wizard* parent);
    void ProcessFrame() override;
    void Start(int windDir);  // windDir 0 blows along +Z, 4 along -Z

private:
    void Appear();
    void RotateFaster();
    void RotateStable();
    void RotateSlower();
    void Disappear();
    void UpdatePosAndYaw();     // _UpdatePosAndYaw
    void PushHero(XHero* hero);  // _PushHero

    int windDir_ = -1;
    XBoss_Wizard* parent_;
    MoveState moveState_ = NON;
    float rotateSpeed_ = 0;
};

// XBoss_Wizard ('9'): World 4's boss (4-10). It waits under the floor until a hero comes within 110
// (a roar from 300), rises at the right over 100 ticks and bobs there until 300, then loops: it
// glides in to 100 off the middle (150 ticks), circles the middle (speeding up to 6 degrees a
// tick, closing to 40 away, holding 300 ticks, widening and slowing) and stops on the other side,
// glides out to 200 off the middle and attacks with its blade or its wand, chosen at random.
// 10 hearts of 40 hits.
class XBoss_Wizard : public XBossBase {
public:
    enum MoveState { NON = 0, HIDE, APPEAR, BLADE, WAND, MOVEIN, MOVEOUT, ROTATE };
    enum RotateState { GET_FASTER = 0, GET_CLOSER, STAY, GET_FURTHER, GET_SLOWER, STOP };
    enum PosState { CENTER = 0, LEFT, RIGHT, LEFT_CENTER, RIGHT_CENTER, UP_CENTER, DOWN_CENTER };

    XBoss_Wizard(const XGameContext* game, const D3DVECTOR& pos);
    ~XBoss_Wizard() override;
    void Render(Device* d) override;
    void ProcessFrame() override;
    D3DVECTOR GetRefPos(PosState s) const;  // GetRefPos
    float GetYaw() const { return yaw_; }

private:
    void UpdatePosY();                    // _UpdatePosY: the bob
    void PlayRotateSound(float from, float to);  // _PlayRotateSound: at each quarter turn passed
    void Hide();
    void Appear();
    void Blade();
    void Wand();
    void MoveIn();
    void MoveOut();
    void Rotate();
    void Glide(PosState fromState, PosState toState);  // MoveIn and MoveOut's shared step

    MoveState moveState_ = NON;
    RotateState rotateState_ = GET_FASTER;
    PosState posState_ = CENTER;
    std::unique_ptr<XShadow> bodyShadow_;
    std::unique_ptr<XBoss_WizardBlade> blade_;
    D3DVECTOR toPos_{0, 0, 0}, fromPos_{0, 0, 0};
    float rotateAngle_ = 0, rotateSpeed_ = 0, rotateRadius_ = 0;
    unsigned subLocalCounter_ = 0, offsetCounter_ = 0;
    bool roarSoundPlayed_ = false;
};

