// The level loop of one level (XGameObject's lists): the goal (XMap::ProcessFrame), hero shots
// (XHeroFireList), torches (XTorchList), coins (XCoinList), static objects (XStaticObjList),
// enemies (XEnemyList) and particles (XParticleList). See docs/REVERSE_ENGINEERING.md,
// "The level loop" and "Enemies and static objects".
//
// Also the enemies' bullets (XBulletList), the platforms (XPlatformList) and the water (XWater).
// Still to come: candles, bosses and the remaining objects and enemies; only the starting wand's shot pattern is
// made (the others come with the shop).
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "core/d3dtypes.h"
#include "game/XHero.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

class Device;
class XLevelView;
class JObject3DRect;
class XParticleList;
class XStaticObjList;
class XEnemyList;
class XEnemyBase;
class XBulletList;
class XPlatformList;
class XWater;
class XShadowSimple;
struct XGameContext;

class XLevelLoop : public XHeroWorld {
public:
    using SoundFn = std::function<void(uint16_t waveId, float z)>;
    using LostGoldFn = std::function<void(bool heroA, int amount)>;

    XLevelLoop();
    ~XLevelLoop() override;
    // Builds the level's objects. `onePlayer` sets the coin values.
    void Load(const XLevelView& level, XHero* hero, XHeroStatus* status, bool onePlayer, SoundFn sound);

    // In the original's order: the map (goal), particles, shots and bullets come before the hero;
    // the enemies, coins, torches, platforms and static objects after it. `cameraZ` is the camera's look-at Z
    // (what is near enough to move and to draw).
    void ProcessBeforeHero(float cameraZ);
    void ProcessAfterHero();
    // The level's objects (drawn after the heroes), then the particles and shots (drawn last;
    // `view` turns them to face the camera).
    void RenderObjects(Device* d);
    void RenderEffects(Device* d, const D3DMATRIX& view);

    // Whether this port builds the object with this level code yet (the rest stay markers).
    static bool IsReconstructed(char code);
    void SetLostGoldHandler(LostGoldFn f) { lostGold_ = std::move(f); }
    int NumTorches() const { return (int)torches_.size(); }
    int LitTorches() const { return litTorches_; }

    // XHeroWorld
    void PlaySound(uint16_t waveId, float z) override { sound_(waveId, z); }
    void CreateFire(const XHero& hero) override;
    void CreateFireAt(const XHero& hero, float angle) override;
    bool BossLevel() const override { return hasBoss_; }
    void CreateEnemyExplode(const D3DVECTOR& pos, int size) override;
    void CreateLandEffect(const D3DVECTOR& pos) override;
    void CreateHeroWarpPhoton(bool heroA) override;
    bool UseWater() const override { return useWater_; }
    void CreateWaterWave(const D3DVECTOR& pos, bool large, bool sound) override;
    void ShowLostGoldMessage(bool heroA, int amount) override {
        if (lostGold_) lostGold_(heroA, amount);
    }
    bool ReadyForWarp() const override { return readyForWarp_; }
    D3DVECTOR GoalPos() const override { return goalPos_; }
    void CloseWarpZone() override;
    bool StaticCollisionX(float& newX, const XCollideBox& box) override;
    bool StaticCollisionZ(float& newZ, const XCollideBox& box) override;
    bool StaticCollisionY(float& newY, const XCollideBox& box) override;
    bool PlatformCollisionX(float& newX, const XCollideBox& box) override;
    bool PlatformCollisionZ(float& newZ, const XCollideBox& box) override;
    bool PlatformCollisionY(float& newY, const XCollideBox& box) override;
    void SetHeroAboveLastPlatform(bool heroA) override;

private:
    struct Fire {
        bool live = false;
        unsigned localTime = 0;
        D3DVECTOR pos{0, 0, 0};
        float angle = 0, verticalSpeed = 0, radius = 8.0f;
    };
    struct Torch {
        bool large = false, fireOn = false;
        unsigned localCounter = 0;
        D3DVECTOR pos{0, 0, 0}, fireCheckPos{0, 0, 0}, fireSpawnPos{0, 0, 0};
        float fireCheckRadius = 14.0f, heroCheckRadius = 24.0f;
        std::unique_ptr<JObject3DX> model;
    };
    struct Coin {  // XCoin
        bool live = true;
        int type = 1;           // 0 small, 1 medium, 2 heart
        bool bouncing = false;  // MOVESTATE BOUNCE: an enemy's coin popping out
        unsigned localCounter = 0;
        D3DVECTOR pos{0, 0, 0};
        float yaw = 0, radius = 0, rotSpeed = 1.5f, verticalSpeed = 0;
        int floorLevel = -1;
        int gold = 0;
        std::unique_ptr<JObject3DX> model;
        std::unique_ptr<XShadowSimple> shadow;
    };

    void ProcessFire(Fire& f);
    bool TorchCollisionCheckFire(const Fire& f);  // XTorchList::CollisionCheck_Fire
    bool CoinTouchesHero(const Coin& c);          // XCoin::CollisionCheck_Hero
    Coin& CreateCoin(const D3DVECTOR& pos, int type);  // XCoin::XCoin
    void CreateEnemyCoin(const XEnemyBase& e, int type);  // XCoinList::CreateEnemyCoin / CreateHeart
    void ProcessCoin(Coin& c);

    const XLevelView* level_ = nullptr;
    XHero* hero_ = nullptr;
    XHeroStatus* status_ = nullptr;
    SoundFn sound_;
    LostGoldFn lostGold_;
    std::vector<Fire> fires_;  // 30 slots
    std::vector<Torch> torches_;
    std::vector<Coin> coins_;
    int litTorches_ = 0;
    bool hasBoss_ = false;
    unsigned mapCounter_ = 0;
    bool readyForWarp_ = false;
    D3DVECTOR goalPos_{0, 0, 0};
    bool onePlayer_ = true;
    int barrelCounter_ = 0;
    std::unique_ptr<XGameContext> game_;
    std::unique_ptr<XParticleList> particles_;
    std::unique_ptr<XStaticObjList> statics_;
    std::unique_ptr<XEnemyList> enemies_;
    std::unique_ptr<XBulletList> bullets_;
    std::unique_ptr<XPlatformList> platforms_;
    std::unique_ptr<XWater> water_;
    bool useWater_ = false;
    std::unique_ptr<JObject3DRect> fireSprite_;  // drawn at each live shot
};
