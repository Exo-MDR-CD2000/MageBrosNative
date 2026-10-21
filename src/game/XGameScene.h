// Stage 4 game scene: a level, player 1, the level loop and the in-game camera, run at the
// original's 100 ticks per second (XGameObject's in-level process).
//
// Level flow as in XGameObject::ProcessFrame: 200 ticks after a completed warp the next level
// starts; after a world's last level the original returns to level select (here: the next
// world's first level, until the front end exists). 300 ticks after a death the original returns
// to level select (here: the same level restarts). Escape makes the hero give up.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "game/XCamera.h"
#include "game/XHero.h"
#include "game/XLevelLoop.h"
#include "game/XLevelView.h"
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

class BackBuffer;
class Device;
class XStatusDisplay;
class XShadow;

class XGameScene {
public:
    XGameScene();
    ~XGameScene();
    bool Load(int world, int level);
    // Test automation only: drop the hero in at (x, z) as at a level start.
    void TestPlaceHero(float x, float z);
    void ProcessFrame();  // one 10 ms tick
    void Render(Device* d);

    const XLevelView& Level() const { return level_; }
    const XHero& Hero() const { return *hero_; }
    // True when the scene moved to another level by itself (the music may need to change).
    bool TakeLevelChanged() {
        bool c = levelChanged_;
        levelChanged_ = false;
        return c;
    }
    // XGameObject::Render's last step: the status display, blitted over the 3D scene
    void RenderStatus(BackBuffer* bb);

private:
    void UpdateInput();  // XHeroA::_UpdateInput
    void PlaySound(uint16_t waveId, float z);  // XApplication::PlaySound
    void PlayStereo(uint16_t waveId, float pan, float volume);  // JSoundList::PlayStereo

    XLevelView level_;
    std::unique_ptr<XHero> hero_;
    XLevelLoop loop_;
    XCamera camera_;
    XHeroInput input_;
    XHeroStatus status_;  // XApplication's hero status, kept across levels
    uint8_t prevKeys_[256] = {};
    uint8_t prevButtons_[2] = {};
    bool onePlayer_ = true;  // the one-player game: the left Ctrl / Shift keys work for player 1 too
    int processTicks_ = 0;   // ticks since the hero completed or died
    bool levelChanged_ = false;
    struct Sound {
        std::vector<int> buffers;  // the copies' audio buffers
        int next = 0;
    };
    std::map<uint16_t, Sound> sounds_;
    std::unique_ptr<XShadow> heroShadow_;
    std::unique_ptr<XStatusDisplay> statusDisplay_;  // XApplication's, made on the first Load
};
