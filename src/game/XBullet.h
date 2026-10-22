// Enemy bullets (XBulletList.cpp, XBulletBase.cpp and XBullet_*.cpp of the original).
//
// XBulletList keeps 50 slots. A new bullet takes the first free slot (none is made when they are
// all busy); each tick every bullet runs and the finished ones are deleted. Its loops skip the
// empty slots.
//
// A bullet hurts a hero it touches: close in Z and X (the hero box's widths plus the bullet's
// radius), then the bullet's sphere against the hero's mesh (ColDet); the hero takes damage and
// is knocked back from the bullet. A hero's shot that comes within reach of a bullet stops there;
// the bullet carries on.
//
// Kinds so far: the cannon's straight ball (XBullet_Cannon) and its rolling ball
// (XBullet_CannonRBall), and the dragon's flame (XBullet_Flame). The needle ball, rocket and
// wizard fire come with their owners.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "core/d3dtypes.h"
#include <memory>

class Device;
class JObject3DX;
class XHero;
class XShadow;
struct XGameContext;

class XBulletBase {
public:
    enum LiveState { NON = 0, NORMAL = 1 };

    XBulletBase(const XGameContext* game, const D3DVECTOR& pos);
    virtual ~XBulletBase();
    virtual void ProcessFrame() { localCounter_++; }
    // `view` turns sprite bullets (the flame) to face the camera
    virtual void Render(Device* d, const D3DMATRIX& view);

    // XBulletBase::CollisionCheck_Fire: a hero's shot at `pos` with `radius` reaches this bullet
    bool CollisionCheck_Fire(const D3DVECTOR& pos, float radius) const;
    LiveState GetState() const { return liveState_; }

protected:
    bool CollisionCheck_Hero(XHero* hero);  // _CollisionCheck_Hero
    void UpdateWorldMatrix();               // _UpdateWorldMatrix

    const XGameContext* game_;
    LiveState liveState_ = NON;
    unsigned localCounter_ = 0;
    D3DVECTOR pos_;
    float yaw_ = 0, pitch_ = 0, radius_ = 0;
    std::unique_ptr<JObject3DX> model_;
    std::unique_ptr<XShadow> shadow_;
};

class XBulletList {
public:
    static constexpr int kSlots = 50;
    explicit XBulletList(const XGameContext* game) : game_(game) {}
    ~XBulletList();

    void Empty();
    // XBulletList::CreateCannonBall: along one of the eight directions (45 degrees apart)
    void CreateCannonBall(const D3DVECTOR& pos, int dir);
    // XBulletList::CreateCannonRBall: a rolling ball at a yaw
    void CreateCannonRBall(const D3DVECTOR& pos, float yaw);
    // XBulletList::CreateFlame: a fireball along a yaw and pitch
    void CreateFlame(const D3DVECTOR& pos, float yaw, float pitch);
    void ProcessFrame();
    void Render(Device* d, const D3DMATRIX& view);

    // A hero's shot: true if a bullet stopped it.
    bool CollisionCheck_Fire(const D3DVECTOR& pos, float radius) const;

private:
    int GetNullIndex() const;

    const XGameContext* game_;
    std::unique_ptr<XBulletBase> slots_[kSlots];
};
