// The level loop of one level (XGameObject's lists): the goal (XMap::ProcessFrame), hero shots
// (XHeroFireList), torches (XTorchList), coins (XCoinList), static objects (XStaticObjList),
// enemies (XEnemyList) and particles (XParticleList). See docs/REVERSE_ENGINEERING.md,
// "The level loop" and "Enemies and static objects".
//
// Also the enemies' bullets (XBulletList), the platforms (XPlatformList), the water (XWater) and
// the bosses and the wall candles (XCandleList). The shots follow the shooter's wand: one, two or
// three straight shots, or the same of homing shots (XHeroFire_Chase).
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
class XBossBase;
class XCandleList;
class XShadowSimple;
struct XGameContext;

class XLevelLoop : public XHeroWorld {
public:
    using SoundFn = std::function<void(uint16_t waveId, float z)>;
    using LostGoldFn = std::function<void(bool heroA, int amount)>;
    using MonoSoundFn = std::function<void(uint16_t waveId, float volume)>;
    using StopMusicFn = std::function<void()>;
    using ShakeFn = std::function<void(float magnitude)>;
    using ShopFn = std::function<void(XHero* hero)>;

    XLevelLoop();
    ~XLevelLoop() override;
    // Builds the level's objects. Hero B is always there (NON in a one-player game);
    // `onePlayer` sets the coin values.
    void Load(const XLevelView& level, XHero* heroA, XHero* heroB, bool onePlayer, SoundFn sound);

    // In the original's order: the map (goal), particles, shots and bullets come before the heroes;
    // the enemies, coins, torches, platforms and static objects after it. `cameraZ` is the camera's look-at Z
    // (what is near enough to move and to draw).
    void ProcessBeforeHero(float cameraZ);
    void ProcessAfterHero();
    // The level's objects (drawn after the heroes), then the particles and shots (drawn last;
    // `view` turns them to face the camera).
    void RenderObjects(Device* d, const D3DMATRIX& view);
    // Native (widescreen): how far along Z from the look-at point objects are drawn (400 at 4:3)
    void SetVisibleRange(float range);
    void RenderEffects(Device* d, const D3DMATRIX& view);

    // Whether this port builds the object with this level code yet (the rest stay markers).
    static bool IsReconstructed(char code);
    void SetLostGoldHandler(LostGoldFn f) { lostGold_ = std::move(f); }
    void SetMonoSoundHandler(MonoSoundFn f) { playMono_ = std::move(f); }
    void SetStopMusicHandler(StopMusicFn f) { stopMusic_ = std::move(f); }
    void SetShakeHandler(ShakeFn f) { shake_ = std::move(f); }
    void SetShopHandler(ShopFn f) { shop_ = std::move(f); }
    // XBossBase::GetHealthMain for the status display (0 without a boss)
    int BossHealth() const;
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
    bool CandleCollisionX(float& newX, const XCollideBox& box) override;
    bool CandleCollisionZ(float& newZ, const XCollideBox& box) override;
    bool CandleCollisionY(float& newY, const XCollideBox& box) override;

private:
    struct Fire {  // XHeroFire (and XHeroFire_Chase)
        bool live = false;
        const XHero* owner = nullptr;  // ownerHero: picks the colour and where enemies flinch from
        unsigned localTime = 0;
        D3DVECTOR pos{0, 0, 0};
        float angle = 0, verticalSpeed = 0, radius = 8.0f;
        bool large = true;  // the side shots of the triple wands are 0.8 the size (radius 6.4)
        // XHeroFire_Chase: the enemy, or else the boss, it turns towards (chosen when fired)
        bool chase = false;
        const XEnemyBase* targetEnemy = nullptr;
        const XBossBase* targetBoss = nullptr;
        D3DVECTOR prevPos{0, 0, 0};  // native (smooth motion): pos before tick prevTick
        unsigned prevTick = ~0u;
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

    // XHeroFireList::_CreateFire_Normal / _CreateFire_Chase: one shot at `angle`
    void CreateFire_Normal(const XHero& hero, float angle, bool large);
    void CreateFire_Chase(const XHero& hero, float angle, bool large);
    Fire* NewFire(const XHero& hero, float angle, bool large);  // the first free slot, or nullptr
    void ProcessFire(Fire& f);
    bool TorchCollisionCheckFire(const Fire& f);  // XTorchList::CollisionCheck_Fire
    bool CoinTouchesHero(const Coin& c, const XHero* hero);  // XCoin::CollisionCheck_Hero's test
    void CoinCollisionCheckHero(Coin& c, XHero* hero);       // XCoin::CollisionCheck_Hero
    void TorchCollisionCheckHero(const Torch& t, XHero* hero);  // XTorch::_CollisionCheck_Hero
    Coin& CreateCoin(const D3DVECTOR& pos, int type);  // XCoin::XCoin
    void CreateEnemyCoin(const XEnemyBase& e, int type);  // XCoinList::CreateEnemyCoin / CreateHeart
    void ProcessCoin(Coin& c);

    const XLevelView* level_ = nullptr;
    XHero* heroA_ = nullptr;
    XHero* heroB_ = nullptr;
    SoundFn sound_;
    LostGoldFn lostGold_;
    MonoSoundFn playMono_;
    StopMusicFn stopMusic_;
    ShakeFn shake_;
    ShopFn shop_;
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
    std::unique_ptr<XCandleList> candles_;
    std::unique_ptr<XBossBase> boss_;  // XGameObject::boss
    bool useWater_ = false;
    std::unique_ptr<JObject3DRect> fireSpriteA_, fireSpriteB_;  // drawn at each live shot, by its owner
    std::unique_ptr<JObject3DRect> fireSpriteSmallA_, fireSpriteSmallB_;  // the same at 0.8, for small shots
};
