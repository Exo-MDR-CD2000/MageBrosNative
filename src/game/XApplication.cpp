#include "XApplication.h"
#include "core/log.h"
#include "core/saves.h"
#include <ctime>
#include "game/JSprite.h"
#include "game/XEndingObject.h"
#include "game/XLogoObject.h"
#include "game/XSelectLevelObject.h"
#include "game/XSelectSlot.h"
#include "game/XTitleObject.h"
#include "audio/midi.h"
#include "audio/music_stream.h"
#include "game/XFPSDisplay.h"
#include "game/XStatusDisplay.h"

namespace {
const uint16_t kBitmapFade = 0x1f7;
}  // namespace

XApplication::XApplication() = default;
XApplication::~XApplication() = default;

void XApplication::Init() {
    // XApplication::Create: the fade strips' bitmap, without a colour key; the first screen is
    // the logo
    JSurfaceList::Create(kBitmapFade, false);
    statusDisplay_ = std::make_unique<XStatusDisplay>();
    fpsDisplay_ = std::make_unique<XFPSDisplay>();
    game_ = std::make_unique<XGameScene>(this);
    state_ = LOGO;
}

void XApplication::FrameMove(unsigned n) {
    if (n > 10) n = 10;
    if (state_ == RUNNING && current_) current_->ProcessFrame(n);
    switch (state_) {
        case LOGO:
            DropScreens();
            logo_ = std::make_unique<XLogoObject>(this);
            current_ = logo_.get();
            state_ = RUNNING;
            break;
        case TITLE:
            DropScreens();
            title_ = std::make_unique<XTitleObject>(this);
            current_ = title_.get();
            state_ = RUNNING;
            break;
        case SELECT_SLOT:
            DropScreens();
            selectSlot_ = std::make_unique<XSelectSlot>(this);
            current_ = selectSlot_.get();
            state_ = RUNNING;
            break;
        case SELECT_LEVEL:
            DropScreens();
            selectLevel_ = std::make_unique<XSelectLevelObject>(this);
            current_ = selectLevel_.get();
            state_ = RUNNING;
            break;
        case GAME:
        case REGISTER_INFO:  // (never asked for: the freeware build is registered)
            EnterGame();
            break;
        case ENDING:
            DropScreens();
            ending_ = std::make_unique<XEndingObject>(this);
            current_ = ending_.get();
            state_ = RUNNING;
            break;
        case QUIT:
            quit_ = true;
            state_ = NON;
            break;
        default:
            break;
    }
}

void XApplication::DropScreens() {
    // the original deletes the old screen when it makes the next one
    current_ = nullptr;
    logo_.reset();
    title_.reset();
    selectSlot_.reset();
    selectLevel_.reset();
    ending_.reset();
}

void XApplication::StopMusic() {
    currentMusic_ = 0;
    musicRequest_ = 0;
    Midi::Close();  // "close all"
    MusicStream::Close();  // (and an MP3/FLAC replacement playing instead)
}

void XApplication::EnterGame() {
    DropScreens();
    current_ = game_.get();
    game_->Start();  // XGameObject::XGameObject
    state_ = RUNNING;
}

void XApplication::DoSaveData() {
    if (slot_ < 0) return;  // (the original would write slot "-1")
    std::time_t now = std::time(nullptr);
    if (const std::tm* t = std::localtime(&now)) {  // GetLocalTime
        saveData_.month = (int8_t)(t->tm_mon + 1);
        saveData_.day = (int8_t)t->tm_mday;
    }
    Saves::PackSave file;
    if (!Saves::Load("original", &file)) file.pack = "original";
    saveData_.WriteSlot(file, onePlayer_, slot_);
    if (!Saves::Store(file)) Log("Saves: could not save %s", XSaveData::SlotName(onePlayer_, slot_).c_str());
}

void XApplication::StartGameAt(int world, int level) {
    saveData_.levelStatus.currentWorld = (int8_t)world;
    saveData_.levelStatus.currentLevel = (int8_t)level;
    slot_ = -1;  // a test hook's game is never saved
    EnterGame();
}

void XApplication::Render(Device* d, BackBuffer* bb, bool showFps, int fps) {
    if (state_ != RUNNING || !current_) return;
    current_->Render(d, bb);
    if (showFps) fpsDisplay_->Render(bb, fps);  // XFPSDisplay::Render (RegInfo_GetDisplayFPS)
}

bool XApplication::OnLogoOrTitle() const {
    if (state_ == NON || state_ == LOGO || state_ == TITLE) return true;
    return state_ == RUNNING && current_ && (current_ == logo_.get() || current_ == title_.get());
}

bool XApplication::PlayerHasControl() const {
    if (state_ != RUNNING || !current_ || current_->GetState() != XProcessObject::PROCESS) return false;
    if (current_ == logo_.get() || current_ == ending_.get()) return false;
    return current_ != game_.get() || game_->PlayerHasControl();
}
