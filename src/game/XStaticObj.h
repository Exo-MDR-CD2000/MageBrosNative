// Static objects (XStaticObjList.cpp and XStaticObj_*.cpp of the original): pillars, barrels,
// fences and invisible collision boxes, each with a collision box the heroes and enemies stop
// against and (by default) the shots stop at.
//
// XStaticObjList keeps 150 slots, filled in order; its loops stop at the first empty slot.
// Object codes (XStaticObjList::CreateObject): 'H' 'I' 'J' cannon (facing 4, 0, 2 eighths of a
// turn), 'K' turning cannon, 'O' barrel, 'P' pillar, 'Q' small pillar; the pots, rocket launcher,
// shop, tree and plant arrive with their chunks. Fences ('/', and
// the fence doors 'v' 'w' as neighbours) are shaped by XMap::_ConstructObjects from their four
// neighbours.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "core/d3dtypes.h"
#include "game/XCollideBox.h"
#include <memory>

class Device;
class JObject3DX;
class XShadowSimple;
struct XGameContext;

class XStaticObjBase {
public:
    XStaticObjBase(const XGameContext* game, const D3DVECTOR& pos);
    virtual ~XStaticObjBase();
    virtual void ProcessFrame() { localCounter_++; }
    virtual void Render(Device* d);
    virtual void CreateExtraCollideBox() {}

    void SetFloorLevel(int level);
    void SetEnabled(bool e) { enabled_ = e; }
    void SetBlockFire(bool b) { blockFire_ = b; }
    bool Enabled() const { return enabled_; }
    bool BlockFire() const { return blockFire_; }
    const XCollideBox& GetCollideBox() const { return *collideBox_; }

protected:
    const XGameContext* game_;
    bool enabled_ = true, blockFire_ = true;
    unsigned localCounter_ = 0;
    D3DVECTOR pos_;
    float yaw_ = 0, pitch_ = 0;
    int floorLevel_ = 0;
    std::unique_ptr<JObject3DX> model_;
    std::unique_ptr<XShadowSimple> shadow_;
    std::unique_ptr<XCollideBox> collideBox_;
};

class XStaticObjList {
public:
    static constexpr int kSlots = 150;
    explicit XStaticObjList(const XGameContext* game) : game_(game) {}
    ~XStaticObjList();

    void Empty();
    // XStaticObjList::CreateObject for a level code 'H'..'Z' (nullptr if not made)
    XStaticObjBase* CreateObject(const D3DVECTOR& pos, char code);
    // XStaticObjList::CreateFence: shape 'X' 'Z' '1'..'8' (see XMap's fence rule)
    XStaticObjBase* CreateFence(const D3DVECTOR& pos, char shape);
    // XStaticObjList::CreateCollideBox: an invisible box (x width, height, z width)
    XStaticObjBase* CreateCollideBox(const D3DVECTOR& pos, float widthX, float height, float widthZ);

    void ProcessFrame();
    void Render(Device* d);

    // Each corrects the coordinate in place; true if any enabled object was hit.
    bool CollisionCheckX(float& newX, const XCollideBox& box, bool ignoreY) const;
    bool CollisionCheckZ(float& newZ, const XCollideBox& box, bool ignoreY) const;
    bool CollisionCheckY(float& newY, const XCollideBox& box) const;
    bool CollisionCheck(const D3DVECTOR& p) const;       // a point inside any enabled object
    bool CollisionCheck_Fire(const D3DVECTOR& p) const;  // ... that blocks shots

private:
    int GetNullIndex() const;
    XStaticObjBase* Put(std::unique_ptr<XStaticObjBase> o);

    const XGameContext* game_;
    std::unique_ptr<XStaticObjBase> slots_[kSlots];
};
