// Particle effects (XParticleList.cpp and XParticle_*.cpp of the original).
//
// XParticleList keeps 300 slots. A new particle takes the first free slot searching on from a
// rotating index; when every slot is busy the effect is simply not made. Each tick every
// particle runs (its local counter counts up first) and the finished ones are deleted; they are
// drawn after the level's objects and before the shots.
//
// The effects so far are the hero's, the shots', the torches', the coins', the goal's, the
// enemies' and the bullets' (smoke and explosions); the water's, candles' and bosses' kinds arrive
// with them.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "core/d3dtypes.h"
#include "game/JObject3D.h"
#include <functional>
#include <memory>

class Device;
class XParticleList;

class XParticleBase {
public:
    XParticleBase(XParticleList* list, const D3DVECTOR& pos) : list_(list), pos_(pos) {}
    virtual ~XParticleBase() = default;
    virtual void ProcessFrame();
    void Render(Device* d, const D3DMATRIX& view);
    bool Finished() const { return state_ == NON; }

protected:
    enum State { NON = 0, NORMAL = 1 };
    XParticleList* list_;
    unsigned localCounter_ = 0;
    State state_ = NON;
    D3DVECTOR pos_;
    bool billboard_ = true;  // Render2 facing the camera; otherwise the rect's own transform
    std::unique_ptr<JObject3DRect> particle_;
};

class XParticleList {
public:
    static constexpr int kSlots = 300;
    // where a hero is (the warp photons circle it); `heroA` picks player 1
    using HeroPosFn = std::function<D3DVECTOR(bool heroA)>;

    XParticleList();
    ~XParticleList();
    // XApplication::PlaySound, for the particles that make a sound
    using SoundFn = std::function<void(uint16_t waveId, float z)>;

    void SetHeroPos(HeroPosFn f) { heroPos_ = std::move(f); }
    D3DVECTOR HeroPos(bool heroA) const { return heroPos_ ? heroPos_(heroA) : D3DVECTOR(0, 0, 0); }
    void SetPlaySound(SoundFn f) { playSound_ = std::move(f); }
    void PlaySound(uint16_t waveId, float z) const {
        if (playSound_) playSound_(waveId, z);
    }

    void Empty();
    void ProcessFrame();
    void Render(Device* d, const D3DMATRIX& view);

    void CreateSmoke(const D3DVECTOR& pos);
    void CreateBulletExplode(const D3DVECTOR& pos);
    void CreateEnemyExplode(const D3DVECTOR& pos, bool large);
    void CreateHeroFireSmoke(const D3DVECTOR& pos, bool heroA, bool large);
    void CreateHeroFireFlash(const D3DVECTOR& pos, bool heroA, bool large);
    void CreateHeroWarpPhoton(bool heroA);
    void CreateTorchFire1(const D3DVECTOR& pos);
    void CreateTorchFire2(const D3DVECTOR& pos);
    void CreateWarpRing(const D3DVECTOR& pos);
    void CreateGetCoinEffect(const D3DVECTOR& pos);
    void CreateLandEffect(const D3DVECTOR& pos);

private:
    int GetNullIndex();
    void Add(std::unique_ptr<XParticleBase> p);

    std::unique_ptr<XParticleBase> slots_[kSlots];
    int arrayPointer_ = 0;
    HeroPosFn heroPos_;
    SoundFn playSound_;
};

// XShadowSimple: one size x size shadow on a fixed floor level (objects, enemies and coins that
// keep to one floor).
class XShadowSimple {
public:
    explicit XShadowSimple(float size);
    void SetPosXZ(const D3DVECTOR& p) { pos_.x = p.x, pos_.z = p.z; }
    void SetFloorLevel(int level) { pos_.y = level * 25.0f + 0.2f; }
    void SetScale(float s);
    void Render(Device* d);

private:
    D3DVECTOR pos_{0, 0, 0};
    std::unique_ptr<JObject3DRect> shadow_;
};

// XShadow: a size x size shadow under a position, split where it crosses floor cells of
// different heights so each part lies on its own floor (up to four parts).
class MapCollision;
class XShadow {
public:
    explicit XShadow(float size);
    void SetPos(const D3DVECTOR& p) { pos_ = p; }
    void Render(Device* d, const MapCollision& map);

private:
    float size_;
    D3DVECTOR pos_{0, 0, 0};
    std::unique_ptr<JObject3DRect> parts_[4];
};
