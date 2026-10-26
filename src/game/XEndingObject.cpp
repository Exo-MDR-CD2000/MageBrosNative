#include "XEndingObject.h"
#include "game/JDInput.h"
#include "game/JModel.h"
#include "game/JObject3D.h"
#include "game/JSoundList.h"
#include "game/JSprite.h"
#include "game/XApplication.h"
#include "game/XCamera.h"
#include "gfx/device.h"
#include "input/input.h"
#include <algorithm>

namespace {
// bitmaps: the film strips and the last credit are not colour-keyed
const uint16_t kBitmapFilmUpper = 0x2cb, kBitmapFilmLower = 0x2ca;
const uint16_t kBitmapCredit[4] = {0x2cc, 0x2cd, 0x2ce, 0x2cf};
// textures and models
const uint16_t kTextureFloor = 0x2c8, kTextureWall = 0x2c7, kTextureShadow = 0xa3;
const uint16_t kModelMageA = 0x163, kModelMageB = 0x168, kModelWand = 0x1b7;
const uint16_t kSoundDecide = 0xed;
constexpr unsigned kFadeTicks = 254;
constexpr float kPi = 3.14159f, kHalfPi = 1.5708f;
}  // namespace

// XEndingObject::XEndingObject
XEndingObject::XEndingObject(XApplication* app) : XProcessObject(app) {
    app_->PlayMusic(1);  // JMidiList::Play(0): Music01
    JSurfaceList::Create(kBitmapFilmUpper, false);
    JSurfaceList::Create(kBitmapFilmLower, false);
    JSurfaceList::Create(kBitmapCredit[0], true);
    JSurfaceList::Create(kBitmapCredit[1], true);
    JSurfaceList::Create(kBitmapCredit[2], true);
    JSurfaceList::Create(kBitmapCredit[3], false);
    camera_ = std::make_unique<XCamera>();
    camera_->ResetForEnding();
    filmUpper_ = std::make_unique<JSprite>(kBitmapFilmUpper, 1);
    filmLower_ = std::make_unique<JSprite>(kBitmapFilmLower, 1);
    for (int i = 0; i < kCredits; i++) credit_[i] = std::make_unique<JSprite>(kBitmapCredit[i], 1);

    // the corridor, 12800 long: the floor 200 wide and 50 down, the walls 200 high on either side
    // (99 out), facing in
    floor_ = std::make_unique<JObject3DRect>(200.0f, 12800.0f);
    floor_->Shift(D3DVECTOR(0.0f, -50.0f, 0.0f));
    floor_->SetTexture(kTextureFloor);
    floor_->UsePerspective(true);
    floor_->SetFilterState(JObject3DRect::FILTER_LINEAR);
    floor_->UpdateWorldMatrix();
    wallLeft_ = std::make_unique<JObject3DRect>(12800.0f, 200.0f);
    wallLeft_->Rotate(0, -kHalfPi);
    wallLeft_->Rotate(1, -kHalfPi);
    wallLeft_->Shift(D3DVECTOR(-99.0f, 50.0f, 0.0f));
    wallLeft_->SetTexture(kTextureWall);
    wallLeft_->UsePerspective(true);
    wallLeft_->SetFilterState(JObject3DRect::FILTER_LINEAR);
    wallLeft_->UpdateWorldMatrix();
    wallRight_ = std::make_unique<JObject3DRect>(12800.0f, 200.0f);
    wallRight_->Rotate(0, -kHalfPi);
    wallRight_->Rotate(1, kHalfPi);
    wallRight_->Shift(D3DVECTOR(99.0f, 50.0f, 0.0f));
    wallRight_->SetTexture(kTextureWall);
    wallRight_->UsePerspective(true);
    wallRight_->SetFilterState(JObject3DRect::FILTER_LINEAR);
    wallRight_->UpdateWorldMatrix();

    // the mages 40 either side of the middle, on the floor, 280 in front of the camera's target,
    // turned to face the camera, each with its walk frames and a wand
    const D3DVECTOR posA(-40.0f, -50.0f, -280.0f), posB(40.0f, -50.0f, -280.0f);
    auto makeMage = [&](uint16_t first, const D3DVECTOR& pos) {
        auto m = std::make_unique<JObject3DX>(first);
        m->AppendFrame(first + 1);
        m->AppendFrame(first);
        m->AppendFrame(first + 2);
        m->SetPos(pos);
        m->SetYaw(kPi);
        m->SetScale(1.0f);
        m->UpdateWorldMatrix();
        return m;
    };
    auto makeWand = [&](const D3DVECTOR& pos) {
        auto w = std::make_unique<JObject3DX>(kModelWand);
        w->SetPos(pos);
        w->SetYaw(kPi);
        w->SetScale(1.0f);
        w->UpdateWorldMatrix();
        return w;
    };
    mageBodyA_ = makeMage(kModelMageA, posA);
    mageWandA_ = makeWand(posA);
    mageBodyB_ = makeMage(kModelMageB, posB);
    mageWandB_ = makeWand(posB);
    // a 40-unit shadow under each, 0.2 above the floor
    auto makeShadow = [&](const D3DVECTOR& pos) {
        auto s = std::make_unique<JObject3DRect>(JObject3DRect::Shadow(40.0f, 40.0f));
        s->SetTexture(kTextureShadow);
        s->SetWorldPos(D3DVECTOR(pos.x, pos.y + 0.2f, pos.z));
        s->UpdateWorldMatrix();
        return s;
    };
    shadowA_ = makeShadow(posA);
    shadowB_ = makeShadow(posB);

    creditIndex_ = -1;
    creditPosX_ = 0;
    creditState_ = WAIT1;
    StartFadeIn(0);
}

XEndingObject::~XEndingObject() = default;

// XEndingObject::ProcessFrame. The credits: 300 ticks after the fade-in the first one waits 200
// ticks off the right edge (x 820), slides to the middle over 200 (500 pixels), holds there 500,
// slides out over 200, and the next one starts; the last stops in the middle and waits for a
// button or Escape, which fades out to the logo.
void XEndingObject::ProcessFrame(unsigned n) {
    if (state_ == NON) return;
    tick_ += n;
    localCounter_ += n;
    textureOffset_ += 0.008f;  // (once a frame, not once a tick, as the original)
    JDInput& in = app_->GetDInput();
    in.UpdateInputState();
    if (state_ == FADEIN) {
        ProcessFadeIn();
        if (tick_ >= kFadeTicks) StartProcess(0);
    } else if (state_ == FADEOUT) {
        ProcessFadeOut();
        if (tick_ >= kFadeTicks) {
            app_->SetState(XApplication::LOGO);
            state_ = NON;
        }
    } else if (state_ == PROCESS) {
        if (creditIndex_ < 0 && tick_ >= 300) {
            creditIndex_ = 0;
            tick_ = 0;
            creditState_ = WAIT1;
        }
        if (creditIndex_ < 0 || creditIndex_ > 3) return;
        if (creditState_ == WAIT1) {
            creditPosX_ = 820;
            if (tick_ >= 200) {
                tick_ = 0;
                creditState_ = APPEAR;
            }
        } else if (creditState_ == APPEAR) {
            creditPosX_ = 820 - (int)((float)tick_ / 200.0f * 500.0f);
            if (tick_ >= 200) {
                tick_ = 0;
                creditState_ = creditIndex_ == 3 ? WAIT_FOR_INPUT : WAIT2;
            }
        } else if (creditState_ == WAIT2) {
            creditPosX_ = 320;
            if (tick_ >= 500) {
                tick_ = 0;
                creditState_ = DISAPPEAR;
            }
        } else if (creditState_ == DISAPPEAR) {
            creditPosX_ = 320 - (int)((float)tick_ / 200.0f * 500.0f);
            if (tick_ >= 200) {
                creditIndex_++;
                creditPosX_ = 820;
                tick_ = 0;
                creditState_ = WAIT1;
            }
        } else if (creditState_ == WAIT_FOR_INPUT) {
            if (CheckButtonPressed(in) || in.GetKeyboardPressed(DIK_ESCAPE)) {
                app_->Sounds().PlayMono(kSoundDecide, 1.0f);
                StartFadeOut(0);
            }
        }
    }
}

// XEndingObject::Render: the corridor (floor and walls with their textures scrolled by the
// offset), the shadows, the wands, the mages on their walk frame (one step every 9 ticks), the
// film strips (80-pixel tiles moving left a quarter pixel a tick) and the credit centred at
// (x, 240)
void XEndingObject::Render(Device* d, BackBuffer* bb) {
    if (state_ == NON) return;
    camera_->DoSetTransform(d);
    const float off = textureOffset_;
    floor_->SetUV(0.0f, 5.0f, off, 320.0f + off, 0.0f);
    floor_->Render(d);
    wallLeft_->SetUV(-off, 320.0f - off, 0.0f, 1.0f, 0.0f);
    wallLeft_->Render(d);
    wallRight_->SetUV(off, 320.0f + off, 0.0f, 1.0f, 0.0f);
    wallRight_->Render(d);
    shadowA_->Render(d);
    shadowB_->Render(d);
    mageWandA_->Render(d);
    mageWandB_->Render(d);
    const int frame = (int)(((localCounter_ - 1u) / 9u + 1u) % 4u);
    mageBodyA_->SetCurrentFrame(frame);
    mageBodyA_->Render(d);
    mageBodyB_->SetCurrentFrame(frame);
    mageBodyB_->Render(d);

    // native (widescreen): the film strips take in as many more tiles as the view's width needs on
    // both sides (the original's 9 at 4:3), so their black bands cover the corridor to the edges
    int viewLeft, viewRight;
    d->ViewXRange(&viewLeft, &viewRight);
    const int film = (int)((float)localCounter_ * 0.25f) % 80;
    const int first = std::min(0, -((-viewLeft + 79 - film) / 80));
    const int last = std::max(8, (viewRight + film + 79) / 80 - 1);
    for (int i = first; i <= last; i++) {
        filmUpper_->SetPosition(i * 80 - film, 0);
        filmUpper_->Draw(bb);
        filmLower_->SetPosition(i * 80 - film, 360);
        filmLower_->Draw(bb);
    }
    if (creditIndex_ >= 0 && creditIndex_ <= 3) {
        // native (widescreen): the credit waits and leaves as far past the view's edges as the
        // original's x 820 and -180 are past the 4:3 screen's, on the same ticks (so it slides a
        // little faster); the original's x at 4:3
        int x = creditPosX_;
        if (x > 320) x += (x - 320) * (viewRight - 640) / 500;
        else x += (320 - x) * viewLeft / 500;
        credit_[creditIndex_]->SetCenterPos(x, 240);
        credit_[creditIndex_]->Draw(bb);
    }
    RenderFade(bb);
}
