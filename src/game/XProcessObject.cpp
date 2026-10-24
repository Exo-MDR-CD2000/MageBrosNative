#include "XProcessObject.h"
#include "game/JSprite.h"
#include <algorithm>

namespace {
const uint16_t kBitmapFade = 0x1f7;  // XApplication::Create makes it without a colour key
constexpr int kScreenWidth = 640;
constexpr int kStripHeight = 16;
}  // namespace

XProcessObject::XProcessObject(XApplication* app) : app_(app) {
    for (auto& s : strips_) s = std::make_unique<JSprite>(kBitmapFade, 1);
}
XProcessObject::~XProcessObject() = default;

void XProcessObject::StartFadeIn(int processType) {
    for (int i = 0; i < kStrips; i++) {
        strips_[i]->SetPosition(0, i * kStripHeight);
        strips_[i]->SetDrawSize(kScreenWidth, kStripHeight);
    }
    processType_ = processType;
    state_ = FADEIN;
    tick_ = 0;
}

void XProcessObject::StartFadeOut(int processType) {
    for (int i = 0; i < kStrips; i++) {
        strips_[i]->SetPosition(0, i * kStripHeight);
        strips_[i]->SetDrawSize(kScreenWidth, 0);
    }
    processType_ = processType;
    state_ = FADEOUT;
    tick_ = 0;
}

void XProcessObject::StartProcess(int processType) {
    processType_ = processType;
    state_ = PROCESS;
    tick_ = 0;
}

// ProcessFadeIn / ProcessFadeOut: strip i starts 20 + 5i ticks in and covers up to 64 ticks,
// a row every 4 ticks
void XProcessObject::ProcessFadeIn() {
    int t = std::max((int)tick_ - 20, 0);
    for (int i = 0; i < kStrips; i++) {
        int rows = t > i * 5 ? std::min(t - i * 5, 64) / 4 : 0;
        strips_[i]->SetPosition(0, i * kStripHeight + rows);  // the strip opens downwards
        strips_[i]->SetDrawSize(kScreenWidth, kStripHeight - rows);
    }
}

void XProcessObject::ProcessFadeOut() {
    int t = std::max((int)tick_ - 20, 0);
    for (int i = 0; i < kStrips; i++) {
        int rows = t > i * 5 ? std::min(t - i * 5, 64) / 4 : 0;
        strips_[i]->SetDrawSize(kScreenWidth, rows);  // the strip closes downwards
    }
}

void XProcessObject::RenderFade(BackBuffer* bb) const {
    if (state_ != FADEIN && state_ != FADEOUT) return;
    for (const auto& s : strips_) s->Draw(bb);
}
