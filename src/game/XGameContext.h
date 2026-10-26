// What the level's objects reach through XGameObject in the original: the map, the heroes, the
// other object lists, the particles, sound and the camera.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "core/d3dtypes.h"
#include <cstdint>
#include <functional>

class XLevelView;
class XHero;
class XHeroWorld;
class XStaticObjList;
class XEnemyList;
class XEnemyBase;
class XBulletList;
class XPlatformList;
class XBossBase;
class XCandleList;
class XParticleList;

struct XGameContext {
    const XLevelView* level = nullptr;
    XHero* heroA = nullptr;  // player 1
    XHero* heroB = nullptr;  // player 2 (always there; NON for the whole level in a one-player game)
    XHeroWorld* heroWorld = nullptr;  // what XHero::Damage reports to
    XStaticObjList* statics = nullptr;
    XEnemyList* enemies = nullptr;
    XBulletList* bullets = nullptr;
    XPlatformList* platforms = nullptr;
    XCandleList* candles = nullptr;
    XBossBase* boss = nullptr;  // XGameObject::boss (one per level at most)
    XParticleList* particles = nullptr;
    std::function<void(uint16_t waveId, float z)> playSound;  // XApplication::PlaySound
    std::function<void(uint16_t waveId, float volume)> playMono;  // JSoundList::PlayMono (unpanned)
    std::function<void()> stopMusic;  // JMidiList::Stop ("close all")
    std::function<void(float magnitude)> startShaking;  // XCamera::StartShaking
    std::function<void(XHero* hero)> startShop;         // XShop::Start
    std::function<void(const XEnemyBase& e)> createEnemyCoin;  // XCoinList::CreateEnemyCoin
    std::function<void(const XEnemyBase& e)> createHeart;      // XCoinList::CreateHeart
    int* barrelCounter = nullptr;  // XGameObject's count of barrels broken (every third gives a heart)
    float cameraZ = 0;      // XCamera's look-at Z
    bool onePlayer = true;  // XApplication's one-player flag
    int world = 0;          // XLevelStatus: 0-based world
    bool useWater = false;  // XMap::UseWater

    // XCamera::IsVisible / IsUpdatable: within 400 / 480 units of the look-at point along Z. The
    // original's 4:3 view never reaches 400, so its drawing test is never seen. Native: the draw
    // range (only ever used to decide what to draw) widens with the 3D view, so in widescreen
    // nothing on screen is left out (XGameScene::Render sets it each frame: 400 at 4:3). The
    // update range, which pauses some enemies while far away, is game logic and stays 480.
    float visibleRange = 400.0f;
    bool IsVisible(const D3DVECTOR& p) const { return Abs(cameraZ - p.z) < visibleRange; }
    bool IsUpdatable(const D3DVECTOR& p) const { return Abs(cameraZ - p.z) < 480.0f; }

    // XGameObject::GetCloserHero: hero A unless hero B is strictly closer; only heroes in the
    // NORMAL state count, and with neither, hero A
    XHero* GetCloserHero(const D3DVECTOR& p) const;
    // XGameObject::FindDistanceToCloserHero / FindDirToCloserHero: heroes in the NORMAL state
    float DistanceToCloserHero(const D3DVECTOR& p) const;
    float DirToCloserHero(const D3DVECTOR& p) const;
    float DistanceToCloserHeroXZ(const D3DVECTOR& p) const;  // FindDistanceToCloserHeroXZ
    // XMap::CollisionCheckX_Floor / Z_Floor: a box kept to cells of its own floor level and out
    // of '!' cells (the invisible blocks that fence enemies in)
    bool CollisionCheckX_Floor(float& newX, const class XCollideBox& box, int floorLevel) const;
    bool CollisionCheckZ_Floor(float& newZ, const class XCollideBox& box, int floorLevel) const;

private:
    static float Abs(float v) { return v < 0 ? -v : v; }
};
