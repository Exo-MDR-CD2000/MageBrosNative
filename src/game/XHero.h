// XHeroBase / XHeroA (XHero.cpp of the original): player 1's mage.
//
// Reconstructed so far: input, walking and turning, gravity, jumping, landing, the map
// collision, falling out of the level, the dying sequence, the animation frames and the wand,
// shots and magic points, damage and knockback, the warp, standing on static objects (the torch
// posts and the wall candles), riding platforms and sinking in water. Still to come: the other
// hero (stage 5).
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "core/d3dtypes.h"
#include "game/JModel.h"
#include "game/XCollideBox.h"
#include <cstdint>
#include <memory>

class Device;
class MapCollision;

// XHeroStatus: kept by the application across levels. XHeroStatus::Reset gives the defaults.
struct XHeroStatus {
    int16_t gold = 0;        // 0..9999
    int8_t life = 5;         // 0..5
    int8_t magicPoint = 10;  // 0..10
    int8_t magicLevel = 0;   // 0..7
    int8_t wandType = 0;     // 0..5
};

class XHero;

// What the hero asks of the game while it runs (XGameObject and its lists in the original).
class XHeroWorld {
public:
    virtual ~XHeroWorld() = default;
    // XApplication::PlaySound, panned by the sound's Z
    virtual void PlaySound(uint16_t waveId, float z) = 0;
    // XHeroFireList::CreateFire
    virtual void CreateFire(const XHero& hero) = 0;
    // XMap::ReadyForWarp / GetGoalPos / CloseWarpZone
    virtual bool ReadyForWarp() const = 0;
    virtual D3DVECTOR GoalPos() const = 0;
    virtual void CloseWarpZone() = 0;
    // XStaticObjList::CollisionCheckX / Z / Y for the hero's box; true on a hit
    virtual bool StaticCollisionX(float& newX, const XCollideBox& box) { return false; }
    virtual bool StaticCollisionZ(float& newZ, const XCollideBox& box) { return false; }
    virtual bool StaticCollisionY(float& newY, const XCollideBox& box) { return false; }
    // XPlatformList::CollisionCheckX / Z / Y for the hero's box; true on a hit
    virtual bool PlatformCollisionX(float& newX, const XCollideBox& box) { return false; }
    virtual bool PlatformCollisionZ(float& newZ, const XCollideBox& box) { return false; }
    virtual bool PlatformCollisionY(float& newY, const XCollideBox& box) { return false; }
    // XCandleList::CollisionCheckX / Z / Y for the hero's box; true on a hit
    virtual bool CandleCollisionX(float& newX, const XCollideBox& box) { return false; }
    virtual bool CandleCollisionZ(float& newZ, const XCollideBox& box) { return false; }
    virtual bool CandleCollisionY(float& newY, const XCollideBox& box) { return false; }
    // XPlatformList::GetLastCollidePlatform()->SetHeroAAbove(1): the hero stands on that platform
    virtual void SetHeroAboveLastPlatform(bool heroA) {}
    // XHeroFireList::_CreateFire_Normal: one shot from the hero at an angle (Create8Way)
    virtual void CreateFireAt(const XHero& hero, float angle) {}
    // XGameObject::boss != nullptr
    virtual bool BossLevel() const { return false; }
    // XStatusDisplay::ShowLostGoldMessage
    virtual void ShowLostGoldMessage(bool heroA, int amount) {}
    // XParticleList::CreateEnemyExplode at the hero's centre
    virtual void CreateEnemyExplode(const D3DVECTOR& pos, int size) {}
    // XParticleList::CreateLandEffect / CreateHeroWarpPhoton
    virtual void CreateLandEffect(const D3DVECTOR& pos) {}
    virtual void CreateHeroWarpPhoton(bool heroA) {}
    // XMap::UseWater / XParticleList::CreateWaterWave
    virtual bool UseWater() const { return false; }
    virtual void CreateWaterWave(const D3DVECTOR& pos, bool large, bool sound) {}
};

// XHeroBase::XHeroInput: what _UpdateInput fills in each tick.
struct XHeroInput {
    bool keyRight = false, keyLeft = false, keyUp = false, keyDown = false;
    bool keyControlUsed = false;  // an arrow key is held: the stick is ignored
    bool button1Pressed = false, button1Down = false;  // fire
    bool button2Pressed = false, button2Down = false;  // jump
    float valueX = 0, valueY = 0;                      // stick, -1..1
};

class XHero {
public:
    enum LiveState { NON = 0, NORMAL = 1, WARP = 2, DYING = 3, DEAD = 4, COMPLETED = 5 };
    enum JumpState { LAND = 0, JUMP = 1, FALL = 2 };

    XHero();
    void SetStatus(XHeroStatus* status) { status_ = status; }
    void SetStartPos(const D3DVECTOR& p) { startPos_ = p; }
    void Reset();  // XHeroBase::Reset
    void ProcessFrame(const XHeroInput& in, const MapCollision& map, XHeroWorld& world);
    void Render(Device* d);

    void Damage(XHeroWorld& world);              // XHeroBase::Damage
    void KnockBack(const D3DVECTOR& from);       // XHeroBase::KnockBack
    void StartDying();                           // XHeroBase::StartDying
    void UpdateMagicLevel();                     // XHeroBase::UpdateMagicLevel
    // XHeroBase::IncreasePushX / Y / Z: added to the next tick's move (a platform carrying it)
    void IncreasePushX(float v) { push_.x += v; }
    void IncreasePushY(float v) { push_.y += v; }
    void IncreasePushZ(float v) { push_.z += v; }
    // XHeroBase::pushedByFan, set by a floor fan lifting the hero and cleared at the end of each
    // NORMAL tick; a hero being lifted doesn't land on the other hero (player 2: stage 5)
    void SetPushedByFan() { pushedByFan_ = true; }
    void SetPosY(float y);  // XHeroBase::SetPosY (a rising platform lifts the hero)
    // XHeroBase::CollisionCheckX / Z: another box (a moving platform) stopped by this hero's box
    // while the hero is in the NORMAL state
    bool CollisionCheckX(float& newX, const XCollideBox& box) const {
        return liveState_ == NORMAL && box_.CollisionCheckX(newX, box, false);
    }
    bool CollisionCheckZ(float& newZ, const XCollideBox& box) const {
        return liveState_ == NORMAL && box_.CollisionCheckZ(newZ, box, false);
    }

    const D3DVECTOR& GetPos() const { return pos_; }
    float GetYaw() const { return yaw_; }
    bool BeingDamage() const { return damageCounter_ > 0; }
    const XCollideBox& GetCollideBox() const { return box_; }
    const JObject3DX& BodyModel() const { return *body_; }
    LiveState GetLiveState() const { return liveState_; }
    JumpState GetJumpState() const { return jumpState_; }

private:
    void ProcessNormal(const XHeroInput& in, const MapCollision& map, XHeroWorld& world);
    void ProcessWarp(XHeroWorld& world);
    void ProcessDying(XHeroWorld& world);
    int GetKeyDir(const XHeroInput& in) const;  // _GetKeyDir: 0..7, in 45 degree steps
    void SetPosX(float x);
    void SetPosZ(float z);
    void UpdateWorldMatrix();  // _UpdateWorldMatrix

    LiveState liveState_ = NON;
    JumpState jumpState_ = LAND;
    unsigned localCounter_ = 0, walkCounter_ = 0;
    D3DVECTOR startPos_{0, 0, 0}, pos_{0, 0, 0};
    float yaw_ = 0, pitch_ = 0;
    bool wandOut_ = false;
    D3DVECTOR push_{0, 0, 0};
    bool pushedByFan_ = false;
    float verticalSpeed_ = 0;
    float warpRotSpeed_ = 0;
    bool beingKnockBack_ = false;
    float knockBackDir_ = 0, knockBackSpeed_ = 0;
    int mpRecoveryCounter_ = 0, mpRecoveryFrames_ = 80;
    int damageCounter_ = 0;
    XHeroStatus defaultStatus_;
    XHeroStatus* status_ = &defaultStatus_;
    std::unique_ptr<JObject3DX> body_, wand_;
    XCollideBox box_;
};
