// Platforms (XPlatformList.cpp, XPlatformBase.cpp, XPlatform_*.cpp and XShadowOnPlatform.cpp of
// the original).
//
// XPlatformList keeps 35 slots, filled in order; its loops stop at the first empty slot. Codes
// (XPlatformList::CreatePlatform): 'A' elevator, 'B' elevator that only goes up, 'C' shuttling
// along Z, 'D' shuttling along X, 'E' moving right (+Z) once stood on, 'F' dropping once stood on,
// 'G' staying put. Each has a box the heroes, enemies, bullets and shots stop against.
//
// A hero landing on a platform marks it (SetHeroAAbove) for the platform's next tick, which runs
// after the hero's; a moving platform carries the heroes standing on it by adding its move to
// their push (applied at the start of their next tick), and a rising one lifts them out of its
// box at once. Every platform clears its marks at the end of its tick.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "core/d3dtypes.h"
#include "game/XCollideBox.h"
#include <memory>

class Device;
class JObject3DRect;
class JObject3DX;
class XHero;
struct XGameContext;

// XShadowOnPlatform: a hero's 35-unit shadow on the top of one platform's box, clipped to the
// box's 40 x 40 top (one per hero).
class XShadowOnPlatform {
public:
    explicit XShadowOnPlatform(const XGameContext* game);
    ~XShadowOnPlatform();
    void SetPlatformBoxPtr(const XCollideBox* box) { platformBox_ = box; }
    void Render(Device* d);

private:
    void RenderShadow(Device* d, const XHero* hero, JObject3DRect& shadow);
    const XGameContext* game_;
    const XCollideBox* platformBox_ = nullptr;
    std::unique_ptr<JObject3DRect> shadow_[2];
};

class XPlatformBase {
public:
    XPlatformBase(const XGameContext* game, int oid, const D3DVECTOR& pos);
    virtual ~XPlatformBase();
    virtual void ProcessFrame() = 0;
    virtual void Render(Device* d);

    int GetOID() const { return oid_; }
    const XCollideBox& GetCollideBox() const { return *collideBox_; }
    void SetFloorLevel(int level) { floorLevel_ = level; }
    void SetHeroAAbove(bool above) { heroAAbove_ = above; }
    void SetHeroBAbove(bool above) { heroBAbove_ = above; }

protected:
    // _UpdatePosX / _UpdatePosZ: against the floor's edge, the other platforms, objects and the
    // heroes, then moved; true if the floor's edge stopped it
    bool UpdatePosX(float newX);
    bool UpdatePosZ(float newZ);
    void SetPosX(float x);
    void SetPosY(float y);
    void SetPosZ(float z);
    void CollisionCheckY_Hero();  // _CollisionCheckY_Hero: lift the heroes out of the box
    void PushHeroX(float dx);     // _PushHeroX / Y / Z: carry the heroes standing on it
    void PushHeroY(float dy);
    void PushHeroZ(float dz);
    void UpdateWorldMatrix();     // _UpdateWorldMatrix

    const XGameContext* game_;
    int oid_;
    unsigned localCounter_ = 0;
    D3DVECTOR startPos_, pos_;
    int floorLevel_ = 0;
    float moveSpeed_ = 0;
    bool heroAAbove_ = false, heroBAbove_ = false;
    std::unique_ptr<JObject3DX> model_;
    std::unique_ptr<XCollideBox> collideBox_;
    std::unique_ptr<XShadowOnPlatform> shadowOnPlatform_;
};

class XPlatformList {
public:
    static constexpr int kSlots = 35;
    explicit XPlatformList(const XGameContext* game) : game_(game) {}
    ~XPlatformList();

    void Empty();
    // XPlatformList::CreatePlatform for a level code 'A'..'G' (nullptr if not made)
    XPlatformBase* CreatePlatform(const D3DVECTOR& pos, char code);
    void ProcessFrame();
    void Render(Device* d);

    // A box moving against the platforms; each corrects the coordinate in place and returns true
    // on a hit. CollisionCheckY remembers the last platform hit (GetLastCollidePlatform).
    bool CollisionCheckX(float& newX, const XCollideBox& box, bool ignoreY) const;
    bool CollisionCheckZ(float& newZ, const XCollideBox& box, bool ignoreY) const;
    bool CollisionCheckY(float& newY, const XCollideBox& box);
    // A platform moving against the others (heights ignored).
    bool CollisionCheckX(float& newX, const XPlatformBase& mover) const;
    bool CollisionCheckZ(float& newZ, const XPlatformBase& mover) const;
    bool CollisionCheck(const D3DVECTOR& p) const;  // a point inside a platform
    XPlatformBase* GetLastCollidePlatform() const { return lastCollidePlatform_; }

private:
    int GetNullIndex() const;

    const XGameContext* game_;
    std::unique_ptr<XPlatformBase> slots_[kSlots];
    XPlatformBase* lastCollidePlatform_ = nullptr;
    int lastOID_ = 0;
};
