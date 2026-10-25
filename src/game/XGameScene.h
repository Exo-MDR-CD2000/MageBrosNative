// The game screen (XGameObject of the original): a level, the two heroes, the level loop and the
// in-game camera, run at the original's 100 ticks per second.
//
// Level flow as in XGameObject::ProcessFrame. Each level fades in (and is played while it does);
// once it is cleared the screen waits 200 ticks, once it is lost 300, then fades out: to the next
// level of the world, or after a world's last level to level select (the next world), or after
// the last world to the ending; a lost level goes back to level select. Escape makes every hero
// still playing give up. In a two-player game the level is cleared when one hero has warped and
// the other has warped or died, and lost only when both have died. Hero B always exists; in a
// one-player game it is never reset, so it stays NON (not drawn, not processed, ignored by
// everything that looks for a hero). While the shop (XShop) is open the level is paused.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "game/XCamera.h"
#include "game/XHero.h"
#include "game/XLevelLoop.h"
#include "game/XLevelView.h"
#include "game/XProcessObject.h"
#include <cstdint>
#include <memory>

class BackBuffer;
class Device;
class XShadow;
class XShop;

class XGameScene : public XProcessObject {
public:
    explicit XGameScene(XApplication* app);
    ~XGameScene() override;
    // XGameObject::XGameObject: the heroes' start positions and the status display, then the
    // level of the application's level status
    void Start();
    // Test automation only: drop hero A (or B) in at (x, z) as at a level start.
    void TestPlaceHero(float x, float z, bool heroA = true);
    void ProcessFrame(unsigned n) override;  // XGameObject::ProcessFrame
    void Render(Device* d, BackBuffer* bb) override;  // XGameObject::Render
    // XApplication::InitDeviceObjects: the projection, light and ambient the game draws with
    static void SetDeviceState(Device* d);

    const XLevelView& Level() const { return level_; }
    const XHero& Hero() const { return *heroA_; }
    // True when a level was loaded since the last call
    bool TakeLevelChanged() {
        bool c = levelChanged_;
        levelChanged_ = false;
        return c;
    }

private:
    bool InitLevel();  // XGameObject::InitLevel: the level of the level status, fading in
    void Tick();       // one 10 ms tick of play (the lists, the heroes, the camera)
    void UpdateInput();  // XHeroA::_UpdateInput and XHeroB::_UpdateInput
    void PlaySound(uint16_t waveId, float z);  // XApplication::PlaySound

    XLevelView level_;
    std::unique_ptr<XHero> heroA_, heroB_;
    XLevelLoop loop_;
    XCamera camera_;
    std::unique_ptr<XShop> shop_;  // XGameObject::shop (+0x9c), made by Start
    XHeroInput inputA_, inputB_;
    uint8_t prevKeys_[256] = {};
    uint8_t prevButtons_[2][2] = {};  // gamepad 1 (player 1) and 2 (player 2), buttons 1 and 2
    bool levelChanged_ = false;
    std::unique_ptr<XShadow> heroShadowA_, heroShadowB_;
};
