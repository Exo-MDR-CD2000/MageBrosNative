// The in-game status display (XStatusDisplay.cpp of the original): world and level, torches lit
// of the total (or BOSS and the boss's hearts on a boss level), and for each player a face,
// gold, life hearts and ten magic lights; plus the "lost gold" message after a death.
//
// Everything is a DirectDraw blit of the original bitmaps at the original 640x480 positions:
//   row A (y 4):  LEVEL  world - level           row B (y 39): TORCH lit / total, or BOSS hearts
//   player 1 (x 450): face, LIFE hearts (y 4), GOLD 0000 (y 31), MP lights (y 58)
//   player 2 (x 260): the same, two-player games only
//
// Native: in levels the HUD settings (Options > Video: HUD position, HUD max width, HUD size)
// move the groups out to the screen's edges in widescreen and resize them from their corners.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "game/JSprite.h"
#include <memory>

class BackBuffer;
struct XHeroStatus;

class XStatusDisplay {
public:
    // What Render reads from the application and the game object each frame.
    struct Info {
        bool inGame = true;  // the game object exists (InitForGame); false on level select
        int world = 0, level = 0;  // 0-based, shown 1-based
        bool bossLevel = false;
        int bossHealth = 0;  // XBossBase::GetHealthMain
        int litTorches = 0, numTorches = 0;
        bool onePlayer = true;
        const XHeroStatus* statusA = nullptr;  // player 1 (XApplication::GetHeroStatus(1))
        const XHeroStatus* statusB = nullptr;  // player 2
        bool heroADead = false, heroBDead = false;  // live state DEAD
    };

    XStatusDisplay();
    ~XStatusDisplay();

    void InitForGame();  // clears the counter and the lost-gold messages
    void ProcessFrame();  // one tick: animation counter, message timers
    // XStatusDisplay::ShowLostGoldMessage: shown for 500 ticks
    void ShowLostGoldMessage(bool heroA, int amount);
    void Render(BackBuffer* bb, const Info& info);

private:
    // native: the HUD size to draw at, the setting's but at most what keeps the level info and the
    // panels apart on this screen (`shift` = the edge shift)
    float FitScale(const Info& info, int shift);

    std::unique_ptr<JSprite> level_, torch_, boss_, dash_, slash_;
    std::unique_ptr<JSprite> goldA_, goldB_, lifeA_, lifeB_, mpA_, mpB_;
    std::unique_ptr<JSprite> heart_, bossHeart_, mpLight_, faceA_, faceB_;
    std::unique_ptr<JSprite> lostGoldA_, lostGoldB_;
    std::unique_ptr<JNumber> world_, levelNum_, litTorch_, numTorch_, goldNumA_, goldNumB_;
    std::unique_ptr<JNumber> lostGoldNumA_, lostGoldNumB_;
    unsigned counter_ = 0;
    int lostGoldTimerA_ = 0, lostGoldTimerB_ = 0;
    int shownWorld_ = -1, shownLevel_ = -1, mostBossHearts_ = 0;  // native: for FitScale
};
