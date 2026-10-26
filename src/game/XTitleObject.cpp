#include "XTitleObject.h"
#include "game/JDInput.h"
#include "game/JObject3D.h"
#include "game/JSoundList.h"
#include "game/JSprite.h"
#include "game/XApplication.h"
#include "game/XCamera.h"
#include "input/input.h"
#include <cmath>

namespace {
// bitmaps (all colour-keyed but the background tile)
const uint16_t kBitmapCursorLeft = 0xd0, kBitmapCursorRight = 0xd1;  // two frames each (blink)
const uint16_t kBitmapCaption1 = 0x1fa, kBitmapBackground = 0x1fc;
const uint16_t kBitmapCaption2 = 0xf8, kBitmapUnregistered = 0xf7;
const uint16_t kBitmapItems[3] = {0x233, 0x232, 0xc9};  // two frames each: chosen, not chosen
// textures
const uint16_t kTextureLogo = 0x1fb, kTexturePictureLeft = 0x208, kTexturePictureRight = 0x207;
// sounds
const uint16_t kSoundMove = 0xec, kSoundDecide = 0xed;
// processType after the fade-out: the menu item chosen
enum { ONE_PLAYER = 0, TWO_PLAYERS = 1, QUIT = 2 };
constexpr unsigned kFadeTicks = 254;
}  // namespace

XTitleObject::XTitleObject(XApplication* app) : XProcessObject(app) {
    app_->PlayMusic(1);  // JMidiList::Play(0): Music01
    for (uint16_t id : {kBitmapCursorLeft, kBitmapCursorRight, kBitmapCaption1}) JSurfaceList::Create(id, true);
    JSurfaceList::Create(kBitmapBackground, false);
    for (uint16_t id : {kBitmapCaption2, kBitmapUnregistered, kBitmapItems[0], kBitmapItems[1], kBitmapItems[2]})
        JSurfaceList::Create(id, true);
    camera_ = std::make_unique<XCamera>();
    camera_->ResetForTitle();
    cursorLeft_ = std::make_unique<JSprite>(kBitmapCursorLeft, 2);
    cursorRight_ = std::make_unique<JSprite>(kBitmapCursorRight, 2);
    caption1_ = std::make_unique<JSprite>(kBitmapCaption1, 1);
    background_ = std::make_unique<JSprite>(kBitmapBackground, 1);
    caption2_ = std::make_unique<JSprite>(kBitmapCaption2, 1);
    unregistered_ = std::make_unique<JSprite>(kBitmapUnregistered, 1);
    for (int i = 0; i < 3; i++) items_[i] = std::make_unique<JSprite>(kBitmapItems[i], 2);

    // the title logo: 555 x 208 (its texture's 0.375 aspect), high in the middle
    const float logoWidth = 555.0f;
    logo_ = std::make_unique<JObject3DRect>(JObject3DRect::Particle(logoWidth, logoWidth * 0.375f));
    logo_->SetTexture(kTextureLogo);
    logo_->UsePerspective(true);
    logo_->SetFilterState(JObject3DRect::FILTER_LINEAR);
    logo_->SetWorldPos(D3DVECTOR(0.0f, 120.0f, 0.0f));
    // the two pictures, 240 x 240, low on the left and right, invisible at first
    const float pictureSize = 240.0f;
    pictureLeft_ = std::make_unique<JObject3DRect>(JObject3DRect::Particle(pictureSize, pictureSize));
    pictureLeft_->SetTexture(kTexturePictureLeft);
    pictureLeft_->UsePerspective(true);
    pictureLeft_->SetFilterState(JObject3DRect::FILTER_LINEAR);
    pictureLeft_->SetAlphaFactor(0.0f);
    pictureLeft_->SetWorldPos(D3DVECTOR(-140.0f, -50.0f, 0.0f));
    pictureRight_ = std::make_unique<JObject3DRect>(JObject3DRect::Particle(pictureSize, pictureSize));
    pictureRight_->SetTexture(kTexturePictureRight);
    pictureRight_->UsePerspective(true);
    pictureRight_->SetFilterState(JObject3DRect::FILTER_LINEAR);
    pictureRight_->SetAlphaFactor(0.0f);
    pictureRight_->SetWorldPos(D3DVECTOR(140.0f, -50.0f, 0.0f));
    StartFadeIn(0);
}
XTitleObject::~XTitleObject() = default;

void XTitleObject::ProcessFrame(unsigned n) {
    if (state_ == NON) return;
    tick_ += n;
    counter_ += n;
    JDInput& in = app_->GetDInput();
    in.UpdateInputState();
    if (state_ == FADEIN) {
        ProcessFadeIn();
        if (tick_ >= kFadeTicks) {
            cursor_ = 0;
            StartProcess(0);
        }
    } else if (state_ == FADEOUT) {
        ProcessFadeOut();
        if (tick_ >= kFadeTicks) {
            if (processType_ == ONE_PLAYER || processType_ == TWO_PLAYERS) {
                app_->SetTwoPlayer(processType_ == TWO_PLAYERS);  // the one-player flag (+0x1a4)
                app_->SetState(XApplication::SELECT_SLOT);
            } else if (processType_ == QUIT) {
                app_->SetState(XApplication::QUIT);
            }
            state_ = NON;
        }
    } else if (state_ == PROCESS) {
        // the arrows blink every 25 ticks
        int frame = tick_ % 50 < 25 ? 0 : 1;
        cursorLeft_->SetCurrentFrame(frame);
        cursorRight_->SetCurrentFrame(frame);
        bool up = CheckUpPressed(in), down = CheckDownPressed(in), button = CheckButtonPressed(in);
        if (in.GetKeyboardPressed(DIK_ESCAPE)) {  // Escape chooses "quit" and takes it
            cursor_ = QUIT;
            button = true;
        }
        if (up) {
            if (cursor_ > 0) {
                app_->Sounds().PlayMono(kSoundMove, 1.0f);
                cursor_--;
            }
        } else if (down) {
            if (cursor_ < 2) {
                app_->Sounds().PlayMono(kSoundMove, 1.0f);
                cursor_++;
            }
        } else if (button) {
            app_->Sounds().PlayMono(kSoundDecide, 1.0f);
            StartFadeOut(cursor_);
        }
    }
}

void XTitleObject::Render(Device* d, BackBuffer* bb) {
    if (state_ == NON) return;
    camera_->DoSetTransform(d);
    // the background: the tile in a 7 x 5 grid, scrolling up and left a quarter pixel a tick
    // (native: wider in widescreen)
    RenderTiledBackground(*background_, counter_, bb);
    // the pictures fade in from tick 200 to 700, to 0.8
    float alpha = 0.0f;
    if (counter_ > 200) alpha = counter_ < 700 ? (float)(counter_ - 200) / 500.0f : 1.0f;
    alpha *= 0.8f;
    pictureLeft_->SetAlphaFactor(alpha);
    pictureLeft_->Render2(d, camera_->View());
    pictureRight_->SetAlphaFactor(alpha);
    pictureRight_->Render2(d, camera_->View());
    // the title logo pulses between 0.5 and 0.9 every 300 ticks
    float phase = (float)(counter_ % 300) / 300.0f;
    logo_->SetAlphaFactor((std::sin(phase * 6.28319f) * 0.5f + 0.5f) * 0.4f + 0.5f);
    logo_->Render2(d, camera_->View());
    // the captions
    caption1_->SetCenterPos(320, 100);
    caption1_->Draw(bb);
    caption2_->SetCenterPos(320, 180);
    caption2_->Draw(bb);
    // the menu from y 386 down, 4 pixels apart; the chosen item on frame 0
    int y = 386;
    for (int i = 0; i < 3; i++) {
        items_[i]->SetCenterPos(320, y);
        items_[i]->SetCurrentFrame(cursor_ == i ? 0 : 1);
        items_[i]->Draw(bb);
        y += items_[i]->GetHeight() + 4;
    }
    // (RegInfo_Registered is 1 in the freeware build, so the "unregistered" line is never drawn)
    // the arrows either side of the chosen item, while the menu runs
    if (state_ == PROCESS && cursor_ >= 0) {
        const JSprite& item = *items_[cursor_];
        int left = item.GetPosIntX() - cursorLeft_->GetWidth() / 2 - 2;
        int right = item.GetPosIntX() + item.GetWidth() + 2;
        cursorLeft_->SetPosition(left, item.GetPosIntY());
        cursorLeft_->Draw(bb);
        cursorRight_->SetPosition(right, item.GetPosIntY());
        cursorRight_->Draw(bb);
    }
    RenderFade(bb);
}
