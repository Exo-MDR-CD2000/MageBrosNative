// The J library's 2D sprites (JSprite.cpp, JNumber.cpp and JSurfaceList of the original).
//
// A JSprite is a bitmap resource drawn with a DirectDraw blit onto the back buffer. Its frames
// are stacked vertically in the bitmap (frame height = bitmap height / frames). JSurfaceList
// loads each bitmap once, colour-keyed on its top-left pixel. A JNumber draws a number with a
// ten-frame digit sprite, zero-padded to its digit count, each digit advancing by a third of the
// digit sprite's width.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include <cstdint>

class BackBuffer;
class Surface;

namespace JSurfaceList {
// JSurfaceList::CreateSurface(resId, useColorKey): load the bitmap once, colour-keyed on its
// top-left pixel or not (the fade strips and the logo are not). Get loads a bitmap not created
// yet colour-keyed, as most of the game's are.
void Create(uint16_t resId, bool useColorKey);
// JSurfaceList::GetByResId: the bitmap (nullptr if unreadable).
Surface* Get(uint16_t resId, int* width, int* height);
void Clear();
}  // namespace JSurfaceList

class JSprite {
public:
    JSprite(uint16_t resId, int numFrames);

    void SetPosition(int x, int y) { posX_ = (float)x, posY_ = (float)y; }
    // the sprite's centre at (x, y), in whole pixels
    void SetCenterPos(int x, int y);
    void SetCurrentFrame(int f) { if (f >= 0 && f < numFrames_) current_ = f; }
    int GetPosIntX() const { return (int)posX_; }
    int GetPosIntY() const { return (int)posY_; }
    int GetCenterIntX() const { return GetPosIntX() + width_ / 2; }
    int GetCenterIntY() const { return GetPosIntY() + frameHeight_ / 2; }
    // the size the frame is stretched to when drawn (a negative size keeps the bitmap's)
    void SetDrawSize(int w, int h) { drawWidth_ = w, drawHeight_ = h; }
    int GetWidth() const { return width_; }
    int GetHeight() const { return frameHeight_; }
    int GetNumFrames() const { return numFrames_; }

    void Draw(BackBuffer* bb) const;
    // DrawScale / DrawScaleXY: the frame stretched by (sx, sy) about the sprite's centre
    void DrawScale(BackBuffer* bb, float s) const { DrawScaleXY(bb, s, s); }
    void DrawScaleXY(BackBuffer* bb, float sx, float sy) const;

private:
    uint16_t resId_;
    float posX_ = 0, posY_ = 0;
    int width_ = 0, frameHeight_ = 0;
    int drawWidth_ = -1, drawHeight_ = -1;  // SetDrawSize
    int current_ = 0, numFrames_ = 1;
};

class JNumber {
public:
    // the digit sprite (ten frames, 0..9)
    explicit JNumber(uint16_t digitsResId);

    void SetNumber(int n) { number_ = n; }
    void SetNumDigits(int n) { numDigits_ = n; }
    void SetDisplayZero(bool z) { displayZero_ = z; }
    void SetPosition(int x, int y) { x_ = x, y_ = y; }
    int GetPosIntX() const { return x_; }
    int GetDigitWidth() const { return digits_.GetWidth() / 3; }
    int GetWidth() const { return GetDigitWidth() * numDigits_; }
    int GetHeight() const { return digits_.GetHeight(); }

    void Draw(BackBuffer* bb);

private:
    JSprite digits_;
    int x_ = 0, y_ = 0;
    bool displayZero_ = true;
    int numDigits_ = 0;
    int number_ = 0;
};
