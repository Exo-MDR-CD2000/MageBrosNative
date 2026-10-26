#include "XProcessObject.h"
#include "game/JSprite.h"
#include "gfx/device.h"
#include <algorithm>

namespace {
const uint16_t kBitmapFade = 0x1f7;  // XApplication::Create makes it without a colour key
constexpr int kStripHeight = 16;

// native (widescreen): the view's x range, in the original's 640 x 480 pixels
void ViewXRange(int* left, int* right) { GetDevice().ViewXRange(left, right); }
}  // namespace

XProcessObject::XProcessObject(XApplication* app) : app_(app) {
    for (auto& s : strips_) s = std::make_unique<JSprite>(kBitmapFade, 1);
}
XProcessObject::~XProcessObject() = default;

void XProcessObject::StartFadeIn(int processType) {
    // native: the strips span the whole view, the widescreen sides too (the original's 640 at 4:3)
    int left, right;
    ViewXRange(&left, &right);
    for (int i = 0; i < kStrips; i++) {
        strips_[i]->SetPosition(left, i * kStripHeight);
        strips_[i]->SetDrawSize(right - left, kStripHeight);
    }
    processType_ = processType;
    state_ = FADEIN;
    tick_ = 0;
}

void XProcessObject::StartFadeOut(int processType) {
    int left, right;
    ViewXRange(&left, &right);
    for (int i = 0; i < kStrips; i++) {
        strips_[i]->SetPosition(left, i * kStripHeight);
        strips_[i]->SetDrawSize(right - left, 0);
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
// a row every 4 ticks (native: across the view's width as it is now, should the window change)
void XProcessObject::ProcessFadeIn() {
    int left, right;
    ViewXRange(&left, &right);
    int t = std::max((int)tick_ - 20, 0);
    for (int i = 0; i < kStrips; i++) {
        int rows = t > i * 5 ? std::min(t - i * 5, 64) / 4 : 0;
        strips_[i]->SetPosition(left, i * kStripHeight + rows);  // the strip opens downwards
        strips_[i]->SetDrawSize(right - left, kStripHeight - rows);
    }
}

void XProcessObject::ProcessFadeOut() {
    int left, right;
    ViewXRange(&left, &right);
    int t = std::max((int)tick_ - 20, 0);
    for (int i = 0; i < kStrips; i++) {
        int rows = t > i * 5 ? std::min(t - i * 5, 64) / 4 : 0;
        strips_[i]->SetPosition(left, i * kStripHeight);
        strips_[i]->SetDrawSize(right - left, rows);  // the strip closes downwards
    }
}

void XProcessObject::RenderFade(BackBuffer* bb) const {
    if (state_ != FADEIN && state_ != FADEOUT) return;
    for (const auto& s : strips_) s->Draw(bb);
}

void XProcessObject::RenderTiledBackground(JSprite& tile, unsigned counter, BackBuffer* bb) {
    int scroll = (int)((float)counter * 0.25f) % 120;
    // the columns covering the view: the original's 0 to 6, and further out in widescreen
    int left, right;
    ViewXRange(&left, &right);
    int first = std::min(0, -((-left + 119 - scroll) / 120));
    int last = std::max(6, (right + scroll + 119) / 120 - 1);
    // (DirectDraw dropped the tiles that crossed the screen edge; the port clips them, as it does
    // every blit: see Device::BltToBackBuffer)
    for (int y = 0; y < 5; y++)
        for (int x = first; x <= last; x++) {
            tile.SetPosition(x * 120 - scroll, y * 120 - scroll);
            tile.Draw(bb);
        }
}
