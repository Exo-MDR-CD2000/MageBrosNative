// Enemies (XEnemyList.cpp, XEnemyBase.cpp and XEnemy_*.cpp of the original).
//
// XEnemyList keeps 110 slots, filled in order; its loops stop at the first empty slot and
// enemies are never removed (a defeated one just stops). Each enemy has a model with a ColDet
// collision mesh, an axis-aligned box, health, and a size (SMALL, KING, QUEEN: the big ones drop
// bigger coins and explode larger).
//
// Touching a hero: the boxes must overlap and then the meshes (ColDet); the hero takes damage
// and is knocked back from the enemy. A shot hits when it is close in Z and X (both tested
// against the box's Z width, as the original does) and its sphere touches the mesh; the shot
// stops there whether or not the enemy can be hurt. Hurt: sound 0x270 at the shooter, a white
// flash for 12 ticks, one health off; at 0 the enemy dies: a hop (4) while spinning 14 degrees a
// tick, then an explosion, a coin, sound 0x26b, gone.
//
// Codes so far: 'a' / '1' / '5' slime (small / king / queen), 'b' / '3' bee, 'd' / '0' wallaby,
// 'h' / '2' mushroom, 'x' / 'y' needle ball, 'c' / '4' squid, 'e' turtle, 'f' kiwi, 'k' hammer,
// 'q' 'r' 's' fan, 'j' barrel, 'm' needle fan, 'o' / 'p' needle (fixed / moving), 'z' jump needle,
// 'u' stalactite, 'v' / 'w' fence door, 'i' fish.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "core/d3dtypes.h"
#include "game/XCollideBox.h"
#include <memory>

class Device;
class JObject3DX;
class XHero;
class XShadowSimple;
struct XGameContext;

class XEnemyBase {
public:
    enum LiveState { NON = 0, NORMAL = 1, DEAD = 2 };
    enum SizeType { SMALL = 0, KING = 1, QUEEN = 2 };

    XEnemyBase(const XGameContext* game, int oid, const D3DVECTOR& pos, SizeType size);
    virtual ~XEnemyBase();
    virtual void ProcessFrame();
    virtual void Render(Device* d);

    // XEnemyBase::CollisionCheck_Fire: a shot at `pos` with `radius`, fired by a hero at `heroPos`
    bool CollisionCheck_Fire(const D3DVECTOR& pos, float radius, const D3DVECTOR& heroPos);

    LiveState GetLiveState() const { return liveState_; }
    SizeType GetSizeType() const { return sizeType_; }
    int GetOID() const { return oid_; }
    const D3DVECTOR& GetPos() const { return pos_; }
    int GetFloorLevel() const { return floorLevel_; }
    void SetFloorLevel(int level);
    bool Collidable() const { return collidable_; }
    const XCollideBox& GetCollideBox() const { return *collideBox_; }

protected:
    virtual void Damage(const D3DVECTOR& heroPos);  // XEnemyBase::_Damage
    bool CollisionCheck_Hero(XHero* hero);          // _CollisionCheck_Hero
    void SetPosX(float x);
    void SetPosY(float y);
    void SetPosZ(float z);
    bool UpdatePosX(float x);  // against other enemies, objects, platforms and the floor's edge
    bool UpdatePosZ(float z);
    bool UpdatePosY(float y) { SetPosY(y); return false; }
    void UpdateWorldMatrix();
    D3DVECTOR GenerateRandomPos() const;  // within 140 of the start along Z, x 20..180
    D3DVECTOR GenerateBehindPos() const;  // 40 away, roughly behind

    const XGameContext* game_;
    int oid_;
    SizeType sizeType_;
    LiveState liveState_ = NON;
    unsigned localCounter_ = 0;
    D3DVECTOR startPos_, pos_;
    float dir_ = 0, roll_ = 0, pitch_ = 0, yaw_ = 0;
    int health_ = 0, floorLevel_ = 0, frame_ = 0, damageCounter_ = 0;
    bool damagable_ = true, chasable_ = false, collidable_ = true, noCollisionWhenHeroDamaged_ = false;
    float verticalSpeed_ = 0;
    std::unique_ptr<JObject3DX> model_;
    std::unique_ptr<XShadowSimple> shadow_;
    std::unique_ptr<XCollideBox> collideBox_;
};

class XEnemyList {
public:
    static constexpr int kSlots = 110;
    explicit XEnemyList(const XGameContext* game) : game_(game) {}
    ~XEnemyList();

    void Empty();
    // XEnemyList::CreateEnemy for a level code (nullptr if not made)
    XEnemyBase* CreateEnemy(const D3DVECTOR& pos, char code);
    void ProcessFrame();
    void Render(Device* d);

    // A hero's shot: true if an enemy stopped it.
    bool CollisionCheck_Fire(const D3DVECTOR& pos, float radius, const D3DVECTOR& heroPos);
    // An enemy moving against the others (boxes only, heights ignored).
    bool CollisionCheckX(float& newX, const XEnemyBase& mover) const;
    bool CollisionCheckZ(float& newZ, const XEnemyBase& mover) const;

private:
    int GetNullIndex() const;

    const XGameContext* game_;
    std::unique_ptr<XEnemyBase> slots_[kSlots];
    int lastOID_ = 0;
};
