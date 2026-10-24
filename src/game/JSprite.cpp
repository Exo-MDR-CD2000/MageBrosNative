#include "game/JSprite.h"
#include "framework/d3dapp.h"
#include "framework/d3dtextr.h"
#include "gfx/device.h"
#include <cstdio>
#include <map>
#include <memory>

namespace {
struct Entry {
    std::unique_ptr<Surface> surface;
    int width = 0, height = 0;
};
std::map<uint16_t, Entry>& Surfaces() {
    static std::map<uint16_t, Entry> s;
    return s;
}
}  // namespace

namespace JSurfaceList {
void Create(uint16_t resId, bool useColorKey) {
    auto& all = Surfaces();
    if (all.count(resId)) return;  // FindIndexOfResId: made once
    Entry e;
    e.surface.reset(DDLoadBitmap(resId, &e.width, &e.height));
    if (e.surface && useColorKey) DDSetColorKey(e.surface.get(), CLR_INVALID);  // keyed on the top-left pixel
    all.emplace(resId, std::move(e));
}

Surface* Get(uint16_t resId, int* width, int* height) {
    auto& all = Surfaces();
    auto it = all.find(resId);
    if (it == all.end()) {
        Create(resId, true);
        it = all.find(resId);
    }
    if (width) *width = it->second.width;
    if (height) *height = it->second.height;
    return it->second.surface.get();
}
void Clear() { Surfaces().clear(); }
}  // namespace JSurfaceList

JSprite::JSprite(uint16_t resId, int numFrames) : resId_(resId), numFrames_(numFrames < 1 ? 1 : numFrames) {
    int w = 0, h = 0;
    JSurfaceList::Get(resId, &w, &h);
    width_ = w > 0 ? w : 0;
    frameHeight_ = h > 0 ? h / numFrames_ : 0;
}

void JSprite::SetCenterPos(int x, int y) {
    posX_ = (float)(x - width_ / 2);
    posY_ = (float)(y - frameHeight_ / 2);
}

void JSprite::Draw(BackBuffer* bb) const {
    if (!bb) return;
    Surface* s = JSurfaceList::Get(resId_, nullptr, nullptr);
    if (!s) return;
    int x = GetPosIntX(), y = GetPosIntY();
    int w = drawWidth_ >= 0 ? drawWidth_ : width_;
    int h = drawHeight_ >= 0 ? drawHeight_ : frameHeight_;
    if (w <= 0 || h <= 0) return;  // (DirectDraw refuses an empty rectangle)
    RECT src{0, current_ * frameHeight_, width_, current_ * frameHeight_ + frameHeight_};
    RECT dst{x, y, x + w, y + h};
    bb->Blt(&dst, s, &src, s->KeyEnabled());
}

namespace {
// Round: half away from zero
int Round(float v) { return v >= 0.0f ? (int)(v + 0.5f) : (int)(v - 0.5f); }
}  // namespace

void JSprite::DrawScaleXY(BackBuffer* bb, float sx, float sy) const {
    if (!bb) return;
    Surface* s = JSurfaceList::Get(resId_, nullptr, nullptr);
    if (!s) return;
    int w = Round(width_ * sx), h = Round(frameHeight_ * sy);
    int x = GetCenterIntX() - w / 2, y = GetCenterIntY() - h / 2;
    if (w <= 0 || h <= 0) return;
    RECT src{0, current_ * frameHeight_, width_, current_ * frameHeight_ + frameHeight_};
    RECT dst{x, y, x + w, y + h};
    bb->Blt(&dst, s, &src, s->KeyEnabled());
}

JNumber::JNumber(uint16_t digitsResId) : digits_(digitsResId, 10) {}

void JNumber::Draw(BackBuffer* bb) {
    char text[34];
    std::snprintf(text, sizeof text, "%d", number_);  // _ltoa(number, buf, 10)
    int len = 0;
    while (text[len]) len++;
    int x = x_;
    // leading zeros up to the digit count (drawn only with SetDisplayZero(true))
    if (numDigits_ > 0 && len < numDigits_) {
        for (int i = 0; i < numDigits_ - len; i++) {
            digits_.SetCurrentFrame(0);
            digits_.SetPosition(x, y_);
            if (displayZero_) digits_.Draw(bb);
            x += GetDigitWidth();
        }
    }
    // the digits, skipping any beyond the digit count on the left
    for (int i = 0; i < len; i++) {
        if (len - i > numDigits_) continue;
        digits_.SetCurrentFrame(text[i] - '0');
        digits_.SetPosition(x, y_);
        digits_.Draw(bb);
        x += GetDigitWidth();
    }
}
