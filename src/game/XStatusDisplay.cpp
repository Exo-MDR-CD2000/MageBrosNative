#include "game/XStatusDisplay.h"
#include "game/XHero.h"

namespace {
// bitmap resources
constexpr uint16_t kLevel = 185, kTorch = 417, kBoss = 527, kDash = 419, kSlash = 418;
constexpr uint16_t kGoldA = 409, kGoldB = 410, kLifeA = 411, kLifeB = 412, kMpA = 413, kMpB = 414;
constexpr uint16_t kHeart = 416, kBossHeart = 526, kMpLight = 415, kFaceA = 509, kFaceB = 510;
constexpr uint16_t kLostGoldA = 0x230, kLostGoldB = 0x231, kDigits = 0xba;
constexpr int kLostGoldTicks = 500;
}  // namespace

XStatusDisplay::XStatusDisplay() {
    level_ = std::make_unique<JSprite>(kLevel, 1);
    torch_ = std::make_unique<JSprite>(kTorch, 1);
    boss_ = std::make_unique<JSprite>(kBoss, 1);
    dash_ = std::make_unique<JSprite>(kDash, 1);
    slash_ = std::make_unique<JSprite>(kSlash, 1);
    goldA_ = std::make_unique<JSprite>(kGoldA, 1);
    goldB_ = std::make_unique<JSprite>(kGoldB, 1);
    lifeA_ = std::make_unique<JSprite>(kLifeA, 1);
    lifeB_ = std::make_unique<JSprite>(kLifeB, 1);
    mpA_ = std::make_unique<JSprite>(kMpA, 1);
    mpB_ = std::make_unique<JSprite>(kMpB, 1);
    heart_ = std::make_unique<JSprite>(kHeart, 4);
    bossHeart_ = std::make_unique<JSprite>(kBossHeart, 4);
    mpLight_ = std::make_unique<JSprite>(kMpLight, 2);
    faceA_ = std::make_unique<JSprite>(kFaceA, 1);
    faceB_ = std::make_unique<JSprite>(kFaceB, 1);
    world_ = std::make_unique<JNumber>(kDigits);
    levelNum_ = std::make_unique<JNumber>(kDigits);
    litTorch_ = std::make_unique<JNumber>(kDigits);
    numTorch_ = std::make_unique<JNumber>(kDigits);
    goldNumA_ = std::make_unique<JNumber>(kDigits);
    goldNumB_ = std::make_unique<JNumber>(kDigits);
    world_->SetNumDigits(1);
    levelNum_->SetNumDigits(2);
    litTorch_->SetNumDigits(2);
    numTorch_->SetNumDigits(2);
    goldNumA_->SetNumDigits(4);
    goldNumB_->SetNumDigits(4);

    // row A: LEVEL w - ll
    int x = 8, y = 4;
    level_->SetPosition(x, y);
    x += level_->GetWidth() + 4;
    world_->SetPosition(x, y);
    x += world_->GetWidth();
    dash_->SetPosition(x, y);
    x += dash_->GetWidth() / 3;
    levelNum_->SetPosition(x, y);
    // row B: TORCH ll / tt
    y = level_->GetHeight() + 10;
    x = 8;
    torch_->SetPosition(x, y);
    x += torch_->GetWidth() + 4;
    litTorch_->SetPosition(x, y);
    x += litTorch_->GetWidth();
    slash_->SetPosition(x, y);
    x += slash_->GetWidth() / 3;
    numTorch_->SetPosition(x, y);
    // row B on a boss level: BOSS and the boss's hearts
    boss_->SetPosition(8, level_->GetHeight() + 10);

    // player 1
    faceA_->SetPosition(450, 4);
    lifeA_->SetPosition(492, 4);
    goldA_->SetPosition(492, lifeA_->GetPosIntY() + lifeA_->GetHeight() + 2);
    goldNumA_->SetPosition(goldA_->GetPosIntX() + goldA_->GetWidth() + 4, goldA_->GetPosIntY());
    mpA_->SetPosition(492, goldA_->GetPosIntY() + goldA_->GetHeight() + 2);
    // player 2
    faceB_->SetPosition(260, 4);
    lifeB_->SetPosition(302, 4);
    goldB_->SetPosition(302, lifeB_->GetPosIntY() + lifeB_->GetHeight() + 2);
    goldNumB_->SetPosition(goldB_->GetPosIntX() + goldB_->GetWidth() + 4, goldB_->GetPosIntY());
    mpB_->SetPosition(302, goldB_->GetPosIntY() + goldB_->GetHeight() + 2);

    // the lost-gold messages, centred on the screen, the amount 198 pixels in
    lostGoldA_ = std::make_unique<JSprite>(kLostGoldA, 1);
    lostGoldB_ = std::make_unique<JSprite>(kLostGoldB, 1);
    lostGoldNumA_ = std::make_unique<JNumber>(kDigits);
    lostGoldNumB_ = std::make_unique<JNumber>(kDigits);
    lostGoldNumA_->SetNumDigits(3);
    lostGoldNumB_->SetNumDigits(3);
    lostGoldA_->SetCenterPos(320, 240);
    lostGoldNumA_->SetPosition(lostGoldA_->GetPosIntX() + 198, lostGoldA_->GetPosIntY() + 10);
    lostGoldB_->SetCenterPos(320, 267);
    lostGoldNumB_->SetPosition(lostGoldB_->GetPosIntX() + 198, lostGoldB_->GetPosIntY() + 10);
}

XStatusDisplay::~XStatusDisplay() = default;

void XStatusDisplay::InitForGame() {
    lostGoldTimerA_ = 0;
    lostGoldTimerB_ = 0;
    counter_ = 0;
}

void XStatusDisplay::ProcessFrame() {
    counter_++;
    if (lostGoldTimerA_ > 0) lostGoldTimerA_--;
    if (lostGoldTimerB_ > 0) lostGoldTimerB_--;
}

void XStatusDisplay::ShowLostGoldMessage(bool heroA, int amount) {
    if (heroA) {
        lostGoldTimerA_ = kLostGoldTicks;
        lostGoldNumA_->SetNumber(amount);
    } else {
        lostGoldTimerB_ = kLostGoldTicks;
        lostGoldNumB_->SetNumber(amount);
    }
}

void XStatusDisplay::Render(BackBuffer* bb, const Info& info) {
    int heartFrame = (int)((counter_ >> 4) % 4);

    level_->Draw(bb);
    dash_->Draw(bb);
    world_->SetNumber(info.world + 1);
    world_->Draw(bb);
    levelNum_->SetNumber(info.level + 1);
    levelNum_->Draw(bb);

    if (info.inGame) {
        if (!info.bossLevel) {
            torch_->Draw(bb);
            slash_->Draw(bb);
            litTorch_->SetNumber(info.litTorches);
            litTorch_->Draw(bb);
            numTorch_->SetNumber(info.numTorches);
            numTorch_->Draw(bb);
        } else {
            boss_->Draw(bb);
            int y = boss_->GetPosIntY() + 1;
            int x = boss_->GetPosIntX() + boss_->GetWidth() + 4;
            for (int i = 0; i < info.bossHealth; i++) {
                bossHeart_->SetPosition(x, y);
                bossHeart_->SetCurrentFrame(heartFrame);
                bossHeart_->Draw(bb);
                x += bossHeart_->GetWidth() / 2 - 8;
            }
        }
    }

    // one player's panel: face, gold, life hearts, magic lights
    auto panel = [&](const XHeroStatus* st, bool dead, JSprite& face, JSprite& gold, JNumber& goldNum,
                     JSprite& life, JSprite& mp) {
        if (!st) return;
        face.Draw(bb);
        gold.Draw(bb);
        goldNum.SetNumber(st->gold);
        goldNum.Draw(bb);
        life.Draw(bb);
        if (!(info.inGame && dead)) {
            int y = life.GetPosIntY() + 1;
            int x = life.GetPosIntX() + life.GetWidth() + 4;
            for (int i = 0; i < st->life; i++) {
                heart_->SetPosition(x, y);
                heart_->SetCurrentFrame(heartFrame);
                heart_->Draw(bb);
                x += heart_->GetWidth() / 2 - 6;
            }
        }
        if (info.inGame) {
            mp.Draw(bb);
            int y = mp.GetPosIntY() + 2;
            int x = mp.GetPosIntX() + mp.GetWidth() + 4;
            for (int i = 0; i < 10; i++) {
                mpLight_->SetPosition(x, y);
                mpLight_->SetCurrentFrame(!dead && i < st->magicPoint ? 1 : 0);
                mpLight_->Draw(bb);
                x += mpLight_->GetWidth() / 4;
            }
        }
    };
    panel(info.statusA, info.heroADead, *faceA_, *goldA_, *goldNumA_, *lifeA_, *mpA_);
    if (!info.onePlayer) panel(info.statusB, info.heroBDead, *faceB_, *goldB_, *goldNumB_, *lifeB_, *mpB_);

    if (lostGoldTimerB_ > 0) {
        lostGoldB_->Draw(bb);
        lostGoldNumB_->Draw(bb);
    }
    if (lostGoldTimerA_ > 0) {
        lostGoldA_->Draw(bb);
        lostGoldNumA_->Draw(bb);
    }
}
