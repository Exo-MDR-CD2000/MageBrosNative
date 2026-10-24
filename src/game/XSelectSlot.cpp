#include "XSelectSlot.h"
#include "core/log.h"
#include "core/saves.h"
#include "game/JDInput.h"
#include "game/JObject3D.h"
#include "game/JSoundList.h"
#include "game/JSprite.h"
#include "game/XApplication.h"
#include "game/XCamera.h"
#include "input/input.h"

namespace {
// bitmaps (colour-keyed but the background tile and the slot bars)
const uint16_t kBitmapCursorLeft = 0xd0, kBitmapCursorRight = 0xd1;  // two frames each (blink)
const uint16_t kBitmapBackground = 0x1fc, kBitmapUnregistered = 0xf7;
const uint16_t kBitmapHelp = 0x23a, kBitmapHeadingOne = 0x23d, kBitmapHeadingTwo = 0x23c;
const uint16_t kBitmapNoData = 0x238, kBitmapDate = 0x237, kBitmapWorld = 0xb9, kBitmapDash = 0x1a3;
const uint16_t kBitmapMonth = 0x239, kBitmapSlotNumber = 0x23e, kBitmapSlotBar = 0x23b, kBitmapBack = 0xc8;
const uint16_t kBitmapDigits = 0xba;
const uint16_t kBitmapQuestion = 0x27f, kBitmapYes = 0x281, kBitmapNo = 0x280;
const uint16_t kTextureQuestionBar = 0x218;
// sounds
const uint16_t kSoundMove = 0xec, kSoundDecide = 0xed;
const uint16_t kSoundAsk = 0x282, kSoundRefuse = 0x283, kSoundDeleted = 0x284;
// processType after the fade-out
enum { TO_LEVEL_SELECT = 0, TO_TITLE = 1 };
enum { YES = 0, NO = 1 };
constexpr unsigned kFadeTicks = 254;
}  // namespace

XSelectSlot::XSelectSlot(XApplication* app) : XProcessObject(app) {
    app_->PlayMusic(1);  // JMidiList::Play(0): Music01 (it carries on from the title)
    for (uint16_t id : {kBitmapCursorLeft, kBitmapCursorRight}) JSurfaceList::Create(id, true);
    JSurfaceList::Create(kBitmapBackground, false);
    for (uint16_t id : {kBitmapUnregistered, kBitmapHelp, kBitmapHeadingOne, kBitmapHeadingTwo, kBitmapNoData,
                        kBitmapDate, kBitmapMonth, kBitmapSlotNumber})
        JSurfaceList::Create(id, true);
    JSurfaceList::Create(kBitmapSlotBar, false);
    for (uint16_t id : {kBitmapBack, kBitmapQuestion, kBitmapYes, kBitmapNo}) JSurfaceList::Create(id, true);
    camera_ = std::make_unique<XCamera>();
    camera_->ResetForTitle();
    cursorLeft_ = std::make_unique<JSprite>(kBitmapCursorLeft, 2);
    cursorRight_ = std::make_unique<JSprite>(kBitmapCursorRight, 2);
    background_ = std::make_unique<JSprite>(kBitmapBackground, 1);
    unregistered_ = std::make_unique<JSprite>(kBitmapUnregistered, 1);
    help_ = std::make_unique<JSprite>(kBitmapHelp, 1);
    headingOne_ = std::make_unique<JSprite>(kBitmapHeadingOne, 1);
    headingTwo_ = std::make_unique<JSprite>(kBitmapHeadingTwo, 1);
    noData_ = std::make_unique<JSprite>(kBitmapNoData, 1);
    date_ = std::make_unique<JSprite>(kBitmapDate, 1);
    worldLabel_ = std::make_unique<JSprite>(kBitmapWorld, 1);
    dash_ = std::make_unique<JSprite>(kBitmapDash, 1);
    month_ = std::make_unique<JSprite>(kBitmapMonth, 12);
    slotNumber_ = std::make_unique<JSprite>(kBitmapSlotNumber, 8);
    back_ = std::make_unique<JSprite>(kBitmapBack, 2);
    for (auto& b : slotBar_) b = std::make_unique<JSprite>(kBitmapSlotBar, 2);
    day_ = std::make_unique<JNumber>(kBitmapDigits);
    day_->SetNumDigits(2);
    world_ = std::make_unique<JNumber>(kBitmapDigits);
    world_->SetNumDigits(1);
    level_ = std::make_unique<JNumber>(kBitmapDigits);
    level_->SetNumDigits(2);

    // the heading at the top, the slots below it 2 pixels apart, "back" 2 further down, the keys
    // at the bottom
    const int x = 320;
    int y = 36;
    headingOne_->SetCenterPos(x, y);
    headingTwo_->SetCenterPos(x, y);
    y += headingOne_->GetHeight() + 24;
    for (auto& b : slotBar_) {
        b->SetCenterPos(x, y);
        y += b->GetHeight() + 2;
    }
    y += 2;
    back_->SetCenterPos(x, y);
    help_->SetCenterPos(x, 464);

    // the delete question over a dark 600 x 70 bar, yes and no below it
    question_ = std::make_unique<JSprite>(kBitmapQuestion, 1);
    yes_ = std::make_unique<JSprite>(kBitmapYes, 2);
    no_ = std::make_unique<JSprite>(kBitmapNo, 2);
    questionBar_ = std::make_unique<JObject3DRect>(JObject3DRect::Particle(600.0f, 70.0f));
    questionBar_->SetTexture(kTextureQuestionBar);
    questionBar_->SetAlphaState(JObject3DRect::ALPHA_TRANSPARENT);
    questionBar_->SetAlphaFactor(0.9f);
    questionBar_->SetWorldPos(D3DVECTOR(0.0f, -177.0f, 0.0f));
    const int qy = 442;
    question_->SetCenterPos(x, qy - 15);
    yes_->SetCenterPos(x - 56, qy + 15);
    no_->SetCenterPos(x + 56, qy + 15);

    LoadSlots();
    StartFadeIn(0);
}
XSelectSlot::~XSelectSlot() = default;

void XSelectSlot::LoadSlots() {
    // the slots of this game mode; a missing or invalid one is empty (and reset)
    Saves::PackSave file;
    bool haveFile = Saves::Load("original", &file);
    for (int i = 0; i < kSlots; i++) empty_[i] = !(haveFile && slots_[i].ReadSlot(file, app_->OnePlayer(), i));
}

void XSelectSlot::DeleteSlot() {
    if (cursor_ < 0 || cursor_ >= kSlots) return;
    // JWinReg::DeleteValue: the slot's section goes from the save file
    Saves::PackSave file;
    if (Saves::Load("original", &file)) {
        file.RemoveSection(XSaveData::SlotName(app_->OnePlayer(), cursor_));
        if (!Saves::Store(file)) Log("Saves: could not delete %s", XSaveData::SlotName(app_->OnePlayer(), cursor_).c_str());
    }
    slots_[cursor_].Reset();
    empty_[cursor_] = true;
}

void XSelectSlot::ProcessFrame(unsigned n) {
    if (state_ == NON) return;
    JDInput& in = app_->GetDInput();
    in.UpdateInputState();
    tick_ += n;
    counter_ += n;
    if (state_ == FADEIN) {
        ProcessFadeIn();
        if (tick_ >= kFadeTicks) {
            cursor_ = 0;
            StartProcess(0);
        }
    } else if (state_ == FADEOUT) {
        ProcessFadeOut();
        if (tick_ >= kFadeTicks) {
            if (processType_ == TO_LEVEL_SELECT) {
                app_->SetSaveData(slots_[cursor_], cursor_);
                app_->SetState(XApplication::SELECT_LEVEL);
            } else if (processType_ == TO_TITLE) {
                app_->SetState(XApplication::TITLE);
            }
            state_ = NON;
        }
    } else if (state_ == PROCESS) {
        // the arrows blink every 25 ticks
        int frame = tick_ % 50 < 25 ? 0 : 1;
        cursorLeft_->SetCurrentFrame(frame);
        cursorRight_->SetCurrentFrame(frame);
        bool up = CheckUpPressed(in), down = CheckDownPressed(in);
        bool left = CheckLeftPressed(in), right = CheckRightPressed(in);
        bool button = CheckButtonPressed(in);
        JSoundList& snd = app_->Sounds();
        if (confirmDelete_) {
            if (in.GetKeyboardPressed(DIK_ESCAPE)) {  // Escape answers no
                deleteChoice_ = NO;
                button = true;
            }
            if (left) {
                if (deleteChoice_ == NO) {
                    snd.PlayMono(kSoundMove, 1.0f);
                    deleteChoice_ = YES;
                }
            } else if (right) {
                if (deleteChoice_ == YES) {
                    snd.PlayMono(kSoundMove, 1.0f);
                    deleteChoice_ = NO;
                }
            } else if (button) {
                if (deleteChoice_ == YES) {
                    snd.PlayMono(kSoundDeleted, 1.0f);
                    DeleteSlot();
                } else {
                    snd.PlayMono(kSoundDecide, 1.0f);
                }
                confirmDelete_ = false;
            }
            return;
        }
        if (in.GetKeyboardPressed(DIK_ESCAPE)) {  // Escape goes back
            cursor_ = kBack;
            button = true;
        }
        if (in.GetKeyboardPressed(DIK_DELETE)) {
            if (cursor_ >= kBack || empty_[cursor_]) {
                snd.PlayMono(kSoundRefuse, 1.0f);  // nothing to delete
            } else {
                snd.PlayMono(kSoundAsk, 1.0f);
                deleteChoice_ = NO;
                confirmDelete_ = true;
                return;
            }
        }
        if (up) {
            snd.PlayMono(kSoundMove, 1.0f);
            if (--cursor_ < 0) cursor_ = kBack;
        } else if (down) {
            snd.PlayMono(kSoundMove, 1.0f);
            if (++cursor_ > kBack) cursor_ = 0;
        } else if (button) {
            snd.PlayMono(kSoundDecide, 1.0f);
            StartFadeOut(cursor_ < kBack ? TO_LEVEL_SELECT : TO_TITLE);
        }
    }
}

void XSelectSlot::Render(Device* d, BackBuffer* bb) {
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
    (app_->OnePlayer() ? headingOne_ : headingTwo_)->Draw(bb);

    // each slot: its bar (frame 0 when chosen), its number, then "no data", or the date (month
    // and day) and the world and level reached
    for (int i = 0; i < kSlots; i++) {
        JSprite& bar = *slotBar_[i];
        bar.SetCurrentFrame(cursor_ == i ? 0 : 1);
        bar.Draw(bb);
        int x = bar.GetPosIntX() + 8;
        slotNumber_->SetPosition(x, bar.GetPosIntY() - 5);
        slotNumber_->SetCurrentFrame(i);
        slotNumber_->Draw(bb);
        x += slotNumber_->GetWidth() + 8;
        int y = bar.GetPosIntY() + 8;
        if (empty_[i]) {
            noData_->SetPosition(x, y);
            noData_->Draw(bb);
            continue;
        }
        const XSaveData& s = slots_[i];
        date_->SetPosition(x, y);
        date_->Draw(bb);
        x += date_->GetWidth() + 4;
        month_->SetPosition(x, y);
        month_->SetCurrentFrame(s.month - 1);
        month_->Draw(bb);
        x += month_->GetWidth();
        day_->SetPosition(x, y);
        day_->SetNumber(s.day);
        day_->Draw(bb);
        x += day_->GetWidth() + 10;
        worldLabel_->SetPosition(x, y);
        worldLabel_->Draw(bb);
        x += worldLabel_->GetWidth() + 4;
        world_->SetPosition(x, y);
        world_->SetNumber(s.levelStatus.currentWorld + 1);
        world_->Draw(bb);
        x += world_->GetWidth();
        dash_->SetPosition(x, y);
        dash_->Draw(bb);
        x += dash_->GetWidth() / 3;
        level_->SetPosition(x, y);
        level_->SetNumber(s.levelStatus.currentLevel + 1);
        level_->Draw(bb);
    }
    back_->SetCurrentFrame(cursor_ == kBack ? 0 : 1);
    back_->Draw(bb);
    help_->Draw(bb);
    // (RegInfo_Registered is 1 in the freeware build, so the "unregistered" line is never drawn)

    if (state_ == PROCESS) {
        // the arrows: either side of yes or no while asking, else of the chosen slot or "back"
        const JSprite* item = nullptr;
        int out = 2, down = 0;
        if (confirmDelete_) {
            questionBar_->Render2(d, camera_->View());
            question_->Draw(bb);
            yes_->SetCurrentFrame(deleteChoice_ == YES ? 0 : 1);
            yes_->Draw(bb);
            no_->SetCurrentFrame(deleteChoice_ == NO ? 0 : 1);
            no_->Draw(bb);
            item = deleteChoice_ == YES ? yes_.get() : no_.get();
        } else if (cursor_ >= 0 && cursor_ < kSlots) {
            item = slotBar_[cursor_].get();
            out = 6;
            down = 6;
        } else if (cursor_ == kBack) {
            item = back_.get();
        }
        if (item) {
            cursorLeft_->SetPosition(item->GetPosIntX() - cursorLeft_->GetWidth() / 2 - out, item->GetPosIntY() + down);
            cursorLeft_->Draw(bb);
            cursorRight_->SetPosition(item->GetPosIntX() + item->GetWidth() + out, item->GetPosIntY() + down);
            cursorRight_->Draw(bb);
        }
    }
    RenderFade(bb);
}
