#include "XLevelLoop.h"
#include "framework/d3dtextr.h"
#include "framework/d3dutil.h"
#include "game/XLevelView.h"
#include "game/XMath.h"
#include "game/XBullet.h"
#include "game/XEnemy.h"
#include "game/XGameContext.h"
#include "game/XParticle.h"
#include "game/XPlatform.h"
#include "game/XStaticObj.h"
#include "game/XWater.h"
#include "gfx/device.h"
#include <cmath>
#include <cstdlib>

namespace {
const uint16_t kSoundGoal = 0xea;       // the open goal's hum
const uint16_t kSoundCoin = 0x268;
const uint16_t kSoundTorchOn = 0x26c;
const uint16_t kTextureFireA = 0x15c;   // player 1's shot
const uint16_t kModelTorchLarge = 0x186;
const uint16_t kModelTorchSmall = 0x18b;
const uint16_t kModelCoin = 0x180, kModelHeart = 0x2ac;
const uint16_t kSoundCoinBounce = 0x27b;
constexpr int kMaxFires = 30;
constexpr float kFireSpeed = 2.6f;
}  // namespace

XLevelLoop::XLevelLoop()
    : game_(std::make_unique<XGameContext>()), particles_(std::make_unique<XParticleList>()) {
    statics_ = std::make_unique<XStaticObjList>(game_.get());
    enemies_ = std::make_unique<XEnemyList>(game_.get());
    bullets_ = std::make_unique<XBulletList>(game_.get());
    platforms_ = std::make_unique<XPlatformList>(game_.get());
    water_ = std::make_unique<XWater>();
    // XHeroFire's look: an 18-unit particle (0.8 of that for the small shots of some wands)
    fireSprite_ = std::make_unique<JObject3DRect>(JObject3DRect::Particle(18.0f, 18.0f));
    fireSprite_->SetTexture(kTextureFireA);
    particles_->SetHeroPos([this](bool) { return hero_ ? hero_->GetPos() : D3DVECTOR(0, 0, 0); });
    particles_->SetPlaySound([this](uint16_t id, float z) {
        if (sound_) sound_(id, z);
    });
}
XLevelLoop::~XLevelLoop() = default;

void XLevelLoop::Load(const XLevelView& level, XHero* hero, XHeroStatus* status, bool onePlayer, SoundFn sound) {
    level_ = &level;
    hero_ = hero;
    status_ = status;
    onePlayer_ = onePlayer;
    sound_ = std::move(sound);
    fires_.assign(kMaxFires, Fire());
    torches_.clear();
    coins_.clear();
    litTorches_ = 0;
    hasBoss_ = false;
    mapCounter_ = 0;
    readyForWarp_ = false;
    // XGameObject::InitLevel empties every list
    enemies_->Empty();
    bullets_->Empty();
    platforms_->Empty();
    statics_->Empty();
    particles_->Empty();
    // XWater::Deinit, then XWater::Init on a water level
    water_->Deinit();
    useWater_ = level.UseWater();
    if (useWater_) water_->Init(LevelGrid::kWidth, level.Depth(), level.World());

    XGameContext& g = *game_;
    g.level = &level;
    g.heroA = hero;
    g.heroWorld = this;
    g.statics = statics_.get();
    g.enemies = enemies_.get();
    g.bullets = bullets_.get();
    g.platforms = platforms_.get();
    g.particles = particles_.get();
    g.playSound = [this](uint16_t id, float z) { sound_(id, z); };
    g.createEnemyCoin = [this](const XEnemyBase& e) { CreateEnemyCoin(e, (int)e.GetSizeType()); };
    g.createHeart = [this](const XEnemyBase& e) { CreateEnemyCoin(e, 2); };
    g.barrelCounter = &barrelCounter_;  // (kept for the whole game, as XGameObject's)
    g.onePlayer = onePlayer;
    g.world = level.World();
    g.useWater = useWater_;

    // XMap::_ConstructObjects: each cell in turn, at the centre of the cell at its floor's height
    auto isFence = [&](int x, int z) {
        char c = level.ObjectCode(x, z);
        return c == '/' || c == 'v' || c == 'w';
    };
    for (const auto& o : level.Objects()) {
        D3DVECTOR at(o.pos.x, o.floor * 25.0f, o.pos.z);
        switch (o.kind) {
            case XLevelView::Kind::Goal:  // XMap::_CreateGoal
                goalPos_ = D3DVECTOR(o.pos.x, (o.floor < 0 ? 0 : o.floor) * 25.0f + 0.1f, o.pos.z);
                break;
            case XLevelView::Kind::Torch:
            case XLevelView::Kind::TorchSmall: {  // XTorch::XTorch
                Torch t;
                t.large = o.kind == XLevelView::Kind::Torch;
                t.pos = at;
                float h = t.large ? 76.0f : 50.0f;
                t.model = std::make_unique<JObject3DX>(t.large ? kModelTorchLarge : kModelTorchSmall);
                t.model->SetYaw(1.5707964f);
                t.model->SetPos(at);
                t.model->UpdateWorldMatrix();
                statics_->CreateCollideBox(at, 28.0f, h, 28.0f);  // the post, as tall as the torch
                t.fireCheckPos = D3DVECTOR(at.x, at.y + h, at.z);
                t.fireSpawnPos = D3DVECTOR(at.x + 8.0f, at.y + h + 14.5f, at.z);
                torches_.push_back(std::move(t));
                break;
            }
            case XLevelView::Kind::Coin: {  // '$': a medium coin (XCoin::SetFloorLevel)
                Coin& c = CreateCoin(at, 1);
                c.floorLevel = o.floor;
                c.shadow->SetFloorLevel(o.floor);
                break;
            }
            case XLevelView::Kind::Fence: {
                // the shape from the fence cells (fences and fence doors) on each side
                bool l = isFence(o.x - 1, o.z), r = isFence(o.x + 1, o.z);
                bool zm = isFence(o.x, o.z - 1), zp = isFence(o.x, o.z + 1);
                char shape = 'X';
                if (l && !r && !zp && zm) shape = '1';
                else if (l && !r && zp && !zm) shape = '2';
                else if (!l && r && !zp && zm) shape = '3';
                else if (!l && r && zp && !zm) shape = '4';
                else if (l && r && !zp && zm) shape = '5';
                else if (l && !r && zp && zm) shape = '6';
                else if (!l && r && zp && zm) shape = '7';
                else if (l && r && zp && !zm) shape = '8';
                else if ((l || r) && !zp && !zm) shape = 'X';
                else if (!l && !r && (zp || zm)) shape = 'Z';
                // (a lone fence or a four-way crossing leaves the original's shape undefined)
                if (XStaticObjBase* f = statics_->CreateFence(at, shape)) f->SetFloorLevel(o.floor);
                break;
            }
            case XLevelView::Kind::StaticObject:
                if (XStaticObjBase* so = statics_->CreateObject(at, o.code)) so->SetFloorLevel(o.floor);
                break;
            case XLevelView::Kind::Enemy:
                if (XEnemyBase* e = enemies_->CreateEnemy(at, o.code)) e->SetFloorLevel(o.floor);
                break;
            case XLevelView::Kind::Boss:
                hasBoss_ = true;
                break;
            case XLevelView::Kind::Platform:  // XPlatformList::CreatePlatform
                if (XPlatformBase* p = platforms_->CreatePlatform(at, o.code)) p->SetFloorLevel(o.floor);
                break;
            default:
                break;
        }
    }
}

bool XLevelLoop::IsReconstructed(char code) {
    static const char kDone[] = "#$*+/OPQa15opb3d0jmvwHIJKABCDEFGi";
    for (const char* c = kDone; *c; c++)
        if (*c == code) return true;
    return false;
}

// ---------------------------------------------------------------------------- the goal

void XLevelLoop::CloseWarpZone() {
    mapCounter_ = 0;
    readyForWarp_ = false;
}

// ---------------------------------------------------------------------------- shots

void XLevelLoop::CreateFire(const XHero& hero) {
    // _WandNormal: one shot along the hero's yaw (the other wands come with the shop)
    CreateFireAt(hero, hero.GetYaw());
}

void XLevelLoop::CreateFireAt(const XHero& hero, float angle) {
    // XHeroFireList::_CreateFire_Normal: the first free slot, 20 units out and 12 up
    for (Fire& f : fires_) {
        if (f.live) continue;
        const D3DVECTOR& p = hero.GetPos();
        f = Fire();
        f.live = true;
        f.angle = angle;
        f.pos = D3DVECTOR(p.x + std::sin(angle) * 20.0f, p.y + 12.0f, p.z + std::cos(angle) * 20.0f);
        return;
    }
}

void XLevelLoop::CreateEnemyExplode(const D3DVECTOR& pos, int size) { particles_->CreateEnemyExplode(pos, size != 0); }
void XLevelLoop::CreateLandEffect(const D3DVECTOR& pos) { particles_->CreateLandEffect(pos); }
void XLevelLoop::CreateHeroWarpPhoton(bool heroA) { particles_->CreateHeroWarpPhoton(heroA); }
void XLevelLoop::CreateWaterWave(const D3DVECTOR& pos, bool large, bool sound) {
    particles_->CreateWaterWave(pos, large, sound);
}

bool XLevelLoop::TorchCollisionCheckFire(const Fire& f) {
    for (Torch& t : torches_) {
        if (t.fireOn) continue;
        if (std::fabs(t.pos.z - f.pos.z) > 40.0f) continue;
        if (ApproxDistance(f.pos, t.fireCheckPos) < t.fireCheckRadius + f.radius) {
            // XTorch::_TurnOnFire
            sound_(kSoundTorchOn, t.pos.z);
            particles_->CreateTorchFire1(t.fireSpawnPos);
            t.fireOn = true;
            litTorches_++;
            return true;
        }
    }
    return false;
}

void XLevelLoop::ProcessFire(Fire& f) {
    f.localTime++;
    const D3DVECTOR prev = f.pos;  // the flash goes where the shot was before this move
    f.pos.x += std::sin(f.angle) * kFireSpeed;
    f.pos.z += std::cos(f.angle) * kFireSpeed;
    if (f.localTime > 20) {
        f.verticalSpeed -= 0.06f;
        f.pos.y += f.verticalSpeed;
    }
    if (f.localTime % 2 == 0) particles_->CreateHeroFireSmoke(f.pos, true, true);  // player 1, normal size
    // XHeroFire::ProcessFrame: the map, the heroes, objects, platforms, enemies, bullets,
    // (candles) stop the shot with a flash
    if (level_->Collision().IsBlockedFloat(f.pos.x, f.pos.z, f.pos.y) ||
        (hero_ && hero_->GetCollideBox().CollisionCheck(f.pos)) || statics_->CollisionCheck_Fire(f.pos) ||
        platforms_->CollisionCheck(f.pos) || enemies_->CollisionCheck_Fire(f.pos, f.radius, hero_ ? hero_->GetPos() : f.pos) ||
        bullets_->CollisionCheck_Fire(f.pos, f.radius)) {
        particles_->CreateHeroFireFlash(prev, true, true);
        f.live = false;
    } else if (TorchCollisionCheckFire(f)) {
        f.live = false;
    } else if (f.pos.y < -200.0f) {
        f.live = false;
    } else if (useWater_ && f.pos.y < -38.0f) {
        // into the water: a small wave, unless it was already deeper than -43
        if (f.pos.y > -43.0f) particles_->CreateWaterWave(D3DVECTOR(f.pos.x, -38.0f, f.pos.z), false, true);
        f.live = false;
    }
}

// ---------------------------------------------------------------------------- per tick

void XLevelLoop::ProcessBeforeHero(float cameraZ) {
    game_->cameraZ = cameraZ;
    // XMap::ProcessFrame
    mapCounter_++;
    if (!readyForWarp_) {
        if (mapCounter_ > 300 && !hasBoss_ && litTorches_ >= (int)torches_.size()) readyForWarp_ = true;
        // (a boss level opens when the boss is defeated: bosses come with stage 5)
    }
    if (readyForWarp_) {
        if (mapCounter_ % 60 == 0) particles_->CreateWarpRing(D3DVECTOR(goalPos_.x, goalPos_.y + 0.5f, goalPos_.z));
        if (mapCounter_ % 120 == 0) sound_(kSoundGoal, goalPos_.z);
    }
    water_->ProcessFrame();  // XWater::ProcessFrame
    particles_->ProcessFrame();
    for (Fire& f : fires_)
        if (f.live) ProcessFire(f);
    bullets_->ProcessFrame();  // XBulletList::ProcessFrame
}

bool XLevelLoop::CoinTouchesHero(const Coin& c) {
    if (!hero_ || hero_->GetLiveState() != XHero::NORMAL) return false;
    const D3DVECTOR& hp = hero_->GetPos();
    const XCollideBox& box = hero_->GetCollideBox();
    if (std::fabs(c.pos.z - hp.z) >= box.WidthZ() + c.radius) return false;
    if (std::fabs(c.pos.x - hp.x) >= box.WidthX() + c.radius) return false;
    // JObject3DX::CheckCollision_Sphere: the coin's sphere against the hero's mesh (MageCOL)
    return hero_->BodyModel().CheckCollision_Sphere(D3DVECTOR(c.pos.x, c.pos.y + c.radius, c.pos.z), c.radius);
}

XLevelLoop::Coin& XLevelLoop::CreateCoin(const D3DVECTOR& pos, int type) {
    // XCoin::XCoin: small coins (model 0x180 at 0.35) are worth 3 (5 in a two-player game), medium
    // ones (0.6) 30 (50); hearts (model 0x2ac at 0.6) give a life
    Coin c;
    c.type = type;
    c.pos = pos;
    float scale = type == 0 ? 0.35f : 0.6f;
    c.model = std::make_unique<JObject3DX>(type == 2 ? kModelHeart : kModelCoin);
    c.model->SetScale(scale);
    c.gold = type == 0 ? (onePlayer_ ? 3 : 5) : type == 1 ? (onePlayer_ ? 30 : 50) : 0;
    c.shadow = std::make_unique<XShadowSimple>(scale * 40.0f);
    c.radius = scale * 40.0f / 2.0f;
    c.rotSpeed = 1.5f;
    c.yaw = UnitRand() * 6.28319f;
    c.model->SetYaw(c.yaw);
    c.model->SetPos(c.pos);
    c.model->UpdateWorldMatrix();
    coins_.push_back(std::move(c));
    return coins_.back();
}

void XLevelLoop::CreateEnemyCoin(const XEnemyBase& e, int type) {
    // XCoinList::CreateEnemyCoin (by the enemy's size: small, medium, heart) and CreateHeart: a
    // coin spinning fast and bouncing out
    Coin& c = CreateCoin(e.GetPos(), type);
    c.floorLevel = e.GetFloorLevel();
    c.shadow->SetFloorLevel(c.floorLevel);
    c.rotSpeed = 15.0f;
    c.bouncing = true;
}

void XLevelLoop::ProcessCoin(Coin& c) {
    // XCoin::ProcessFrame
    c.localCounter++;
    if (!c.live) return;
    if (c.bouncing) {  // _ProcessFrame_Bounce
        if (c.localCounter == 1) c.verticalSpeed = 3.0f;
        c.pos.y += c.verticalSpeed;
        c.verticalSpeed -= 0.08f;
        if (c.verticalSpeed < -4.0f) c.verticalSpeed = -4.0f;
        if (c.floorLevel >= 0 && c.floorLevel * 25.0f > c.pos.y) {
            c.verticalSpeed = std::fabs(c.verticalSpeed) * 0.8f;
            if (c.verticalSpeed > 1.6f) sound_(kSoundCoinBounce, c.pos.z);
            if (c.verticalSpeed < 0.8f) {
                c.pos.y = c.floorLevel * 25.0f;
                c.bouncing = false;
            }
        }
        if (c.pos.y < -200.0f) c.live = false;
    }
    if (c.rotSpeed > 1.8f) c.rotSpeed -= 0.02f;
    c.yaw = NormaliseAngle(c.yaw + 0.0174533f * c.rotSpeed);
    c.model->SetYaw(c.yaw);
    c.model->SetPos(c.pos);
    c.model->UpdateWorldMatrix();
    if (CoinTouchesHero(c)) {
        sound_(kSoundCoin, c.pos.z);
        particles_->CreateGetCoinEffect(D3DVECTOR(c.pos.x, c.pos.y + c.radius, c.pos.z));
        if (c.type == 2) {
            if (++status_->life > 5) status_->life = 5;
        } else {
            int g = status_->gold + (int)c.gold;
            status_->gold = (int16_t)(g > 9999 ? 9999 : g);
        }
        c.live = false;
    }
}

void XLevelLoop::ProcessAfterHero() {
    // XGameObject::ProcessFrame after the heroes: enemies, coins, torches, platforms, objects
    enemies_->ProcessFrame();
    for (size_t i = 0; i < coins_.size(); i++) ProcessCoin(coins_[i]);
    // XTorchList::ProcessFrame
    for (Torch& t : torches_) {
        t.localCounter++;
        if (!t.fireOn) continue;
        if (t.localCounter % 12 == 0) particles_->CreateTorchFire2(RandomPosXZ(t.fireCheckPos, 2.5f));
        if (!hero_) continue;
        // XTorch::_CollisionCheck_Hero: the flame burns a hero standing in it
        if (hero_->GetLiveState() != XHero::NORMAL) continue;
        if (std::fabs(t.pos.z - hero_->GetPos().z) > 80.0f) continue;
        if (ApproxDistance(hero_->GetPos(), t.fireCheckPos) < t.heroCheckRadius) {
            hero_->Damage(*this);
            hero_->KnockBack(t.pos);
        }
    }
    platforms_->ProcessFrame();
    statics_->ProcessFrame();
    // (XCandleList::ProcessFrame: later)
}

// ---------------------------------------------------------------------------- static objects
// XStaticObjList::CollisionCheckX / Z / Y for the hero's box (heights counted)

bool XLevelLoop::StaticCollisionX(float& newX, const XCollideBox& box) { return statics_->CollisionCheckX(newX, box, false); }
bool XLevelLoop::StaticCollisionZ(float& newZ, const XCollideBox& box) { return statics_->CollisionCheckZ(newZ, box, false); }
bool XLevelLoop::StaticCollisionY(float& newY, const XCollideBox& box) { return statics_->CollisionCheckY(newY, box); }

// ---------------------------------------------------------------------------------- platforms
// XPlatformList::CollisionCheckX / Z / Y for the hero's box (heights counted)

bool XLevelLoop::PlatformCollisionX(float& newX, const XCollideBox& box) { return platforms_->CollisionCheckX(newX, box, false); }
bool XLevelLoop::PlatformCollisionZ(float& newZ, const XCollideBox& box) { return platforms_->CollisionCheckZ(newZ, box, false); }
bool XLevelLoop::PlatformCollisionY(float& newY, const XCollideBox& box) { return platforms_->CollisionCheckY(newY, box); }
void XLevelLoop::SetHeroAboveLastPlatform(bool heroA) {
    if (XPlatformBase* p = platforms_->GetLastCollidePlatform()) {
        if (heroA) p->SetHeroAAbove(true);
        else p->SetHeroBAbove(true);
    }
}

// ---------------------------------------------------------------------------- drawing

void XLevelLoop::RenderObjects(Device* d) {
    // XGameObject::Render, after the map and the heroes: the coins, then the torches
    d->SetRenderState(D3DRENDERSTATE_LIGHTING, 1);
    for (Coin& c : coins_) {  // XCoin::Render
        if (!c.live || !game_->IsVisible(c.pos)) continue;
        c.model->Render(d);
        c.shadow->SetPosXZ(c.pos);
        c.shadow->Render(d);
    }
    for (Torch& t : torches_) t.model->Render(d);
    // platforms, static objects, (candles,) bullets, enemies
    platforms_->Render(d);
    statics_->Render(d);
    bullets_->Render(d);
    enemies_->Render(d);
    water_->Render(d);  // XWater::Render, before the particles
}

void XLevelLoop::RenderEffects(Device* d, const D3DMATRIX& view) {
    // last: the particles, then the shots (XHeroFireList::Render)
    particles_->Render(d, view);
    for (const Fire& f : fires_) {
        if (!f.live) continue;
        fireSprite_->SetWorldPos(f.pos);
        fireSprite_->Render2(d, view);
    }
}
