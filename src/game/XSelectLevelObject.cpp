#include "XSelectLevelObject.h"
#include "game/JDInput.h"
#include "game/JSoundList.h"
#include "game/JSprite.h"
#include "game/XApplication.h"
#include "game/XCamera.h"
#include "game/XStatusDisplay.h"
#include "input/input.h"

namespace {
// bitmaps (colour-keyed but the background tile)
const uint16_t kBitmapArrowLeft = 0xd0, kBitmapArrowRight = 0xd1;  // two frames each (blink)
const uint16_t kBitmapArrowUp = 0x236, kBitmapArrowDown = 0x235;
const uint16_t kBitmapItems[3] = {0xcd, 0xcc, 0xc8};  // two frames each: chosen, not chosen
const uint16_t kBitmapBackground = 0x1fc, kBitmapUnregistered = 0xf7, kBitmapHeading = 0x222;
const uint16_t kBitmapWorlds[4] = {0x226, 0x227, 0x228, 0x229};
const uint16_t kBitmapDot = 0x224, kBitmapLink = 0x223, kBitmapCleared = 0x22a, kBitmapPanel = 0x225;
const uint16_t kBitmapMarker = 0x1fd;  // made by the status display
// sounds
const uint16_t kSoundMove = 0xec, kSoundDecide = 0xed;
// the menu items, and processType after the fade-out
enum { START = 0, PICK = 1, BACK = 2 };
enum { TO_GAME = 0, TO_TITLE = 1 };
constexpr unsigned kFadeTicks = 254;
}  // namespace

XSelectLevelObject::XSelectLevelObject(XApplication* app) : XProcessObject(app) {
    app_->PlayMusic(1);  // JMidiList::Play(0): Music01
    for (uint16_t id : {kBitmapArrowLeft, kBitmapArrowRight, kBitmapArrowUp, kBitmapArrowDown})
        JSurfaceList::Create(id, true);
    JSurfaceList::Create(kBitmapBackground, false);
    for (uint16_t id : {kBitmapUnregistered, kBitmapHeading, kBitmapWorlds[0], kBitmapWorlds[1], kBitmapWorlds[2],
                        kBitmapWorlds[3], kBitmapDot, kBitmapLink, kBitmapCleared, kBitmapPanel, kBitmapItems[0],
                        kBitmapItems[1], kBitmapItems[2]})
        JSurfaceList::Create(id, true);
    camera_ = std::make_unique<XCamera>();
    camera_->ResetForTitle();
    arrowLeft_ = std::make_unique<JSprite>(kBitmapArrowLeft, 2);
    arrowRight_ = std::make_unique<JSprite>(kBitmapArrowRight, 2);
    arrowUp_ = std::make_unique<JSprite>(kBitmapArrowUp, 2);
    arrowDown_ = std::make_unique<JSprite>(kBitmapArrowDown, 2);
    background_ = std::make_unique<JSprite>(kBitmapBackground, 1);
    unregistered_ = std::make_unique<JSprite>(kBitmapUnregistered, 1);
    heading_ = std::make_unique<JSprite>(kBitmapHeading, 1);
    dot_ = std::make_unique<JSprite>(kBitmapDot, 2);
    link_ = std::make_unique<JSprite>(kBitmapLink, 1);
    cleared_ = std::make_unique<JSprite>(kBitmapCleared, 1);
    panel_ = std::make_unique<JSprite>(kBitmapPanel, 1);
    marker_ = std::make_unique<JSprite>(kBitmapMarker, 1);
    for (int i = 0; i < 4; i++) worlds_[i] = std::make_unique<JSprite>(kBitmapWorlds[i], 1);
    for (int i = 0; i < 3; i++) items_[i] = std::make_unique<JSprite>(kBitmapItems[i], 2);
    app_->StatusDisplay().InitForGame();  // InitForSelLevel: the same, with no game (Render's inGame)
    StartFadeIn(0);
}
XSelectLevelObject::~XSelectLevelObject() = default;

void XSelectLevelObject::ProcessFrame(unsigned n) {
    if (state_ == NON) return;
    JDInput& in = app_->GetDInput();
    in.UpdateInputState();
    tick_ += n;
    counter_ += n;
    if (state_ == FADEIN) {
        ProcessFadeIn();
        if (tick_ >= kFadeTicks) {
            choice_ = START;
            StartProcess(0);
        }
    } else if (state_ == FADEOUT) {
        ProcessFadeOut();
        if (tick_ >= kFadeTicks) {
            if (processType_ == TO_GAME) {
                // (the register screen when unregistered and past world 1: RegInfo_Registered is 1
                // in the freeware build)
                app_->SetState(XApplication::GAME);
            } else if (processType_ == TO_TITLE) {
                app_->DoSaveData();
                app_->SetState(XApplication::TITLE);
            }
            state_ = NON;
            return;
        }
    } else if (state_ == PROCESS) {
        // the four arrows blink every 25 ticks
        int frame = tick_ % 50 < 25 ? 0 : 1;
        for (JSprite* a : {arrowLeft_.get(), arrowRight_.get(), arrowUp_.get(), arrowDown_.get()})
            a->SetCurrentFrame(frame);
        bool up = CheckUpPressed(in), down = CheckDownPressed(in);
        bool left = CheckLeftPressed(in), right = CheckRightPressed(in);
        bool button = CheckButtonPressed(in);
        JSoundList& snd = app_->Sounds();
        if (picking_) {
            if (in.GetKeyboardPressed(DIK_ESCAPE)) button = true;  // Escape ends picking
            XLevelStatus& ls = app_->LevelStatus();
            if (left) {
                snd.PlayMono(kSoundMove, 1.0f);
                if (--ls.currentLevel < 0) ls.currentLevel = ls.maxSelLevel[ls.currentWorld];
            } else if (right) {
                snd.PlayMono(kSoundMove, 1.0f);
                if (++ls.currentLevel > ls.maxSelLevel[ls.currentWorld]) ls.currentLevel = 0;
            } else if (up) {
                snd.PlayMono(kSoundMove, 1.0f);
                if (--ls.currentWorld < 0) ls.currentWorld = ls.maxSelWorld;
                if (ls.currentLevel > ls.maxSelLevel[ls.currentWorld]) ls.currentLevel = ls.maxSelLevel[ls.currentWorld];
            } else if (down) {
                snd.PlayMono(kSoundMove, 1.0f);
                if (++ls.currentWorld > ls.maxSelWorld) ls.currentWorld = 0;
                if (ls.currentLevel > ls.maxSelLevel[ls.currentWorld]) ls.currentLevel = ls.maxSelLevel[ls.currentWorld];
            } else if (button) {
                snd.PlayMono(kSoundDecide, 1.0f);
                picking_ = false;
            }
        } else {
            if (in.GetKeyboardPressed(DIK_ESCAPE)) {  // Escape chooses back
                choice_ = BACK;
                button = true;
            }
            if (left) {
                if (choice_ > START) {
                    snd.PlayMono(kSoundMove, 1.0f);
                    choice_--;
                }
            } else if (right) {
                if (choice_ < BACK) {
                    snd.PlayMono(kSoundMove, 1.0f);
                    choice_++;
                }
            } else if (button) {
                snd.PlayMono(kSoundDecide, 1.0f);
                if (choice_ == START) StartFadeOut(TO_GAME);
                else if (choice_ == PICK) picking_ = true;
                else StartFadeOut(TO_TITLE);
            }
        }
    }
    // the status display runs on (its hearts beat)
    for (unsigned i = 0; i < n; i++) app_->StatusDisplay().ProcessFrame();
}

void XSelectLevelObject::Render(Device* d, BackBuffer* bb) {
    if (state_ == NON) return;
    camera_->DoSetTransform(d);
    // the title's background: the tile in a 7 x 5 grid, scrolling up and left a quarter pixel a
    // tick (clipped at the screen edge, as every blit of the port)
    int scroll = (int)((float)counter_ * 0.25f) % 120;
    for (int y = 0; y < 5; y++)
        for (int x = 0; x < 7; x++) {
            background_->SetPosition(x * 120 - scroll, y * 120 - scroll);
            background_->Draw(bb);
        }
    // the panel, the heading 134 above its centre, then the world banners down from 10 below the
    // heading, 34 apart
    const int cx = 320;
    int y = 250;
    panel_->SetCenterPos(cx, y);
    panel_->Draw(bb);
    y -= 134;
    heading_->SetCenterPos(cx, y);
    heading_->Draw(bb);
    y += heading_->GetHeight() + 10;
    for (auto& w : worlds_) {
        w->SetCenterPos(cx, y);
        w->Draw(bb);
        y += w->GetHeight() + 34;
    }
    // under each banner its ten levels: a dot (lit when it can be picked) and a link to the next,
    // the marker on the current level; the "cleared" badge on a cleared world's banner
    const XLevelStatus& ls = app_->LevelStatus();
    for (int w = 0; w < 4; w++) {
        const JSprite& banner = *worlds_[w];
        int x = 130;
        int rowY = banner.GetPosIntY() + banner.GetHeight() + 4;
        if (ls.bossDefeated[w] == 1) {
            cleared_->SetPosition(banner.GetPosIntX() + banner.GetWidth() - 10, banner.GetPosIntY() - 4);
            cleared_->Draw(bb);
        }
        for (int lv = 0; lv < 10; lv++) {
            dot_->SetCurrentFrame(w <= ls.maxSelWorld && lv <= ls.maxSelLevel[w] ? 1 : 0);
            dot_->SetPosition(x, rowY);
            dot_->Draw(bb);
            if (lv < 9) {
                x += dot_->GetWidth() / 2;
                link_->SetPosition(x, rowY);
                link_->Draw(bb);
                x += link_->GetWidth() / 2;
            }
            if (ls.currentWorld == w && ls.currentLevel == lv) {
                marker_->SetPosition(dot_->GetPosIntX() - 9, dot_->GetPosIntY() - 12);
                marker_->Draw(bb);
            }
        }
    }
    // the menu along the bottom from (136, 436), 40 apart; the chosen item on frame 0
    int ix = 136;
    for (int i = 0; i < 3; i++) {
        items_[i]->SetPosition(ix, 436);
        items_[i]->SetCurrentFrame(choice_ == i ? 0 : 1);
        items_[i]->Draw(bb);
        ix += items_[i]->GetWidth() + 40;
    }
    // (RegInfo_Registered is 1 in the freeware build, so the "unregistered" line is never drawn)

    if (state_ == PROCESS) {
        if (picking_) {
            // four arrows round the marker, 20 out (left and right shifted by a quarter of the
            // arrow's width, up and down by a quarter of theirs, as the original)
            int mx = marker_->GetCenterIntX(), my = marker_->GetCenterIntY() + 2;
            int a = arrowLeft_->GetWidth() / 4, b = arrowUp_->GetWidth() / 4;
            arrowLeft_->SetCenterPos(mx + a - 20, my);
            arrowLeft_->Draw(bb);
            arrowRight_->SetCenterPos(mx + a + 20, my);
            arrowRight_->Draw(bb);
            arrowUp_->SetCenterPos(mx + b, my - 20);
            arrowUp_->Draw(bb);
            arrowDown_->SetCenterPos(mx + b, my + 20);
            arrowDown_->Draw(bb);
        } else if (choice_ >= 0) {
            const JSprite& item = *items_[choice_];
            arrowLeft_->SetPosition(item.GetPosIntX() - arrowLeft_->GetWidth() / 2 - 2, item.GetPosIntY());
            arrowLeft_->Draw(bb);
            arrowRight_->SetPosition(item.GetPosIntX() + item.GetWidth() + 2, item.GetPosIntY());
            arrowRight_->Draw(bb);
        }
    }
    // the status display (level, gold and life; no game, so no magic), then the fade
    XStatusDisplay::Info info;
    info.inGame = false;
    info.world = ls.currentWorld;
    info.level = ls.currentLevel;
    info.onePlayer = app_->OnePlayer();
    info.statusA = &app_->HeroStatus(true);
    info.statusB = &app_->HeroStatus(false);
    app_->StatusDisplay().Render(bb, info);
    RenderFade(bb);
}
