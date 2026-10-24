// XApplication's screen state machine (XApplication::FrameMove, SetState, Render of the
// original): one screen (an XProcessObject) runs at a time; a screen asks for the next one with
// SetState, and on the next frame the old screen is dropped and the new one made.
//
//   3 logo -> 4 title -> 5 save slots -> 6 level select -> 7 game -> 6 level select ...
//   game -> 9 ending (after the last world) -> 3 logo; title -> 1 quit
//   (8, the register screen, is never reached: RegInfo_Registered returns 1 in the freeware build)
//
// It also keeps what the original application keeps for the screens: both players' status, the
// level status, the one-player flag, the status display and the sound list.
//
// Built so far (stage 6 chunk 1): the logo and the game. Stand-ins until their screens exist:
// the title, the save slots and level select go straight into the game at the current level
// status (so a death replays the level and a cleared world starts the next one), and the ending
// goes back to the logo.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "game/JSoundList.h"
#include "game/XGameScene.h"
#include "game/XHero.h"
#include "game/XLevelStatus.h"
#include <memory>

class BackBuffer;
class Device;
class XLogoObject;
class XProcessObject;
class XStatusDisplay;

class XApplication {
public:
    enum State {
        NON = 0, QUIT = 1, RUNNING = 2, LOGO = 3, TITLE = 4, SELECT_SLOT = 5, SELECT_LEVEL = 6, GAME = 7,
        REGISTER_INFO = 8, ENDING = 9
    };

    XApplication();
    ~XApplication();
    // XApplication::Create: the shared bitmaps, then the logo on the first frame
    void Init();
    void SetState(State s) { state_ = s; }
    // XApplication::FrameMove: `n` ticks of the running screen (at most 10), then any change of
    // screen
    void FrameMove(unsigned n);
    // XApplication::Render: the running screen (the device state set by the caller)
    void Render(Device* d, BackBuffer* bb);
    bool QuitRequested() const { return quit_; }

    JSoundList& Sounds() { return sounds_; }
    XStatusDisplay& StatusDisplay() { return *statusDisplay_; }
    XHeroStatus& HeroStatus(bool heroA) { return heroA ? statusA_ : statusB_; }  // GetHeroStatus
    XLevelStatus& LevelStatus() { return levelStatus_; }
    bool OnePlayer() const { return onePlayer_; }
    // the one-player flag (+0x1a4), which the title sets (stage 6 chunk 2); here also F2 and
    // --two-player
    void SetTwoPlayer(bool twoPlayer) { onePlayer_ = !twoPlayer; }

    XGameScene& Game() { return *game_; }
    bool InGame() const { return current_ == game_.get() && state_ == RUNNING; }
    // Test hook (N / B, --auto level=): straight into the game at that level, now
    void StartGameAt(int world, int level);

private:
    void EnterGame();

    State state_ = NON;  // +0x1ac
    bool quit_ = false;
    XProcessObject* current_ = nullptr;  // +0x1c8
    std::unique_ptr<XLogoObject> logo_;
    std::unique_ptr<XGameScene> game_;  // kept, and restarted on each entry (the original remakes it)
    JSoundList sounds_;
    std::unique_ptr<XStatusDisplay> statusDisplay_;
    XHeroStatus statusA_, statusB_;
    XLevelStatus levelStatus_;
    bool onePlayer_ = true;
};
