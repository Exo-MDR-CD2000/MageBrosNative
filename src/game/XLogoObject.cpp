#include "XLogoObject.h"
#include "game/JObject3D.h"
#include "game/JSoundList.h"
#include "game/JSprite.h"
#include "game/XApplication.h"
#include "game/XCamera.h"

namespace {
const uint16_t kBitmapLogo = 0x2d1;
const uint16_t kTextureBackdrop = 0x13c;
const uint16_t kSoundJingle = 0x2d0;
constexpr unsigned kJingleTick = 50;   // 0x32
constexpr unsigned kTitleTick = 950;   // 0x3b6
}  // namespace

XLogoObject::XLogoObject(XApplication* app) : XProcessObject(app) {
    app_->StopMusic();  // JMidiList::Stop
    JSurfaceList::Create(kBitmapLogo, false);
    camera_ = std::make_unique<XCamera>();
    camera_->ResetForTitle();
    logo_ = std::make_unique<JSprite>(kBitmapLogo, 1);
    backdrop_ = std::make_unique<JObject3DRect>(600.0f, 600.0f);
    backdrop_->Rotate(0, -1.5708f);  // upright, facing the camera
    backdrop_->UsePerspective(false);
    backdrop_->SetTexture(kTextureBackdrop);
    StartProcess(0);
}
XLogoObject::~XLogoObject() = default;

void XLogoObject::ProcessFrame(unsigned n) {
    if (state_ == NON) return;
    for (unsigned i = 0; i < n; i++) {
        tick_++;
        if (tick_ == kJingleTick) app_->Sounds().PlayMono(kSoundJingle, 1.0f);
        if (tick_ >= kTitleTick) {
            app_->SetState(XApplication::TITLE);
            state_ = NON;
            return;
        }
    }
}

void XLogoObject::Render(Device* d, BackBuffer* bb) {
    if (state_ == NON) return;
    camera_->DoSetTransform(d);
    backdrop_->Render(d);
    // centred on the 640 x 480 screen (CD3DApplication::GetScreenWidth / Height)
    logo_->SetCenterPos(640 / 2, 480 / 2);
    logo_->DrawScale(bb, 1.0f);
}
