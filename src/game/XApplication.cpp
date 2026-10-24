#include "XApplication.h"
#include "game/JSprite.h"
#include "game/XLogoObject.h"
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
    game_ = std::make_unique<XGameScene>(this);
    state_ = LOGO;
}

void XApplication::FrameMove(unsigned n) {
    if (n > 10) n = 10;
    if (state_ == RUNNING && current_) current_->ProcessFrame(n);
    switch (state_) {
        case LOGO:
            logo_ = std::make_unique<XLogoObject>(this);
            current_ = logo_.get();
            state_ = RUNNING;
            break;
        case GAME:
        // stand-ins until their screens exist: the game at the current level status
        case TITLE:
        case SELECT_SLOT:
        case SELECT_LEVEL:
        case REGISTER_INFO:
            EnterGame();
            break;
        case ENDING:  // stand-in: the ending goes back to the logo, as the real one does
            state_ = LOGO;
            break;
        case QUIT:
            quit_ = true;
            state_ = NON;
            break;
        default:
            break;
    }
}

void XApplication::EnterGame() {
    logo_.reset();
    current_ = game_.get();
    game_->Start();  // XGameObject::XGameObject
    state_ = RUNNING;
}

void XApplication::StartGameAt(int world, int level) {
    levelStatus_.currentWorld = (int8_t)world;
    levelStatus_.currentLevel = (int8_t)level;
    EnterGame();
}

void XApplication::Render(Device* d, BackBuffer* bb) {
    if (state_ == RUNNING && current_) current_->Render(d, bb);
}
