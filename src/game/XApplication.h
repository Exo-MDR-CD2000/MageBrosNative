// XApplication's screen state machine (XApplication::FrameMove, SetState, Render of the
// original): one screen (an XProcessObject) runs at a time; a screen asks for the next one with
// SetState, and on the next frame the old screen is dropped and the new one made.
//
//   3 logo -> 4 title -> 5 save slots -> 6 level select -> 7 game -> 6 level select ...
//   game -> 9 ending (after the last world) -> 3 logo; title -> 1 quit
//   (8, the register screen, is never reached: RegInfo_Registered returns 1 in the freeware build)
//
// It also keeps what the original application keeps for the screens: the save being played
// (XSaveData: the level status and both players' status) and its slot, the one-player flag, the
// status display and the sound list. DoSaveData writes the save back to its slot after each level.
//
// Every screen is built: the logo, the title, the save slots, level select, the game and the
// ending. The register screen is never asked for (RegInfo_Registered is 1 in the freeware build).
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "game/JDInput.h"
#include "game/JSoundList.h"
#include "game/XGameScene.h"
#include "game/XHero.h"
#include "game/XLevelStatus.h"
#include "game/XSaveData.h"
#include <memory>

class BackBuffer;
class Device;
class XEndingObject;
class XLogoObject;
class XTitleObject;
class XSelectSlot;
class XSelectLevelObject;
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
    JDInput& GetDInput() { return dinput_; }  // the menus' input, updated by each screen
    // JMidiList::Play: Music01..06 (1-based here), unless that track is already the one playing;
    // the caller plays it (TakeMusicRequest)
    void PlayMusic(int track) {
        if (track == currentMusic_) return;
        currentMusic_ = track;
        musicRequest_ = track;
    }
    // JMidiList::Stop: the music stops, and the next PlayMusic starts its track again
    void StopMusic();
    int TakeMusicRequest() {
        int t = musicRequest_;
        musicRequest_ = 0;
        return t;
    }
    XStatusDisplay& StatusDisplay() { return *statusDisplay_; }
    // GetHeroStatus / GetLevelStatus: parts of the save being played
    XHeroStatus& HeroStatus(bool heroA) { return heroA ? saveData_.heroStatusA : saveData_.heroStatusB; }
    XLevelStatus& LevelStatus() { return saveData_.levelStatus; }
    // SetSaveData: play from this save, kept in `slot` of the current game mode
    void SetSaveData(const XSaveData& data, int slot) {
        saveData_ = data;
        slot_ = slot;
    }
    // DoSaveData: date the save and write it to its slot (nothing while no slot is chosen: the
    // test hooks that start a level directly never touch the save file)
    void DoSaveData();
    bool OnePlayer() const { return onePlayer_; }
    // the one-player flag (+0x1a4), which the title sets; F2 and --two-player are test hooks
    void SetTwoPlayer(bool twoPlayer) { onePlayer_ = !twoPlayer; }

    XGameScene& Game() { return *game_; }
    bool InGame() const { return current_ == game_.get() && state_ == RUNNING; }
    // Test hook (N / B, F2, --auto level=): straight into the game at that level, now, without a
    // save slot (so nothing it does is saved)
    void StartGameAt(int world, int level);

private:
    void EnterGame();
    void DropScreens();

    State state_ = NON;  // +0x1ac
    bool quit_ = false;
    XProcessObject* current_ = nullptr;  // +0x1c8
    std::unique_ptr<XLogoObject> logo_;
    std::unique_ptr<XTitleObject> title_;
    std::unique_ptr<XSelectSlot> selectSlot_;
    std::unique_ptr<XSelectLevelObject> selectLevel_;
    std::unique_ptr<XEndingObject> ending_;
    std::unique_ptr<XGameScene> game_;  // kept, and restarted on each entry (the original remakes it)
    JSoundList sounds_;
    JDInput dinput_;
    int musicRequest_ = 0;
    int currentMusic_ = 0;  // JMidiList's current track (0: none)
    std::unique_ptr<XStatusDisplay> statusDisplay_;
    XSaveData saveData_;  // +0x1d4
    int slot_ = -1;       // +0x1a8: the slot of the save being played, -1 for none
    bool onePlayer_ = true;
};
