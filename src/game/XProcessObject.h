// XProcessObject (XProcessObject.cpp of the original): the base of every screen the application
// runs (the logo, the title, the save slots, level select, the game, the ending), with the
// screen-wide fade.
//
// The fade is a venetian blind of 30 black strips (bitmap 0x1f7, not colour-keyed), each the
// width of the 640 x 480 screen and 16 rows tall. After 20 ticks the strips open (fade in) or
// close (fade out) one after another from the top, 5 ticks apart, each over 64 ticks in steps of
// 4 ticks a row: 229 ticks in all. The screens wait 254 ticks before going on.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include <memory>

class BackBuffer;
class Device;
class JSprite;
class XApplication;

class XProcessObject {
public:
    enum STATE { NON = 0, FADEIN = 1, FADEOUT = 2, PROCESS = 3 };

    explicit XProcessObject(XApplication* app);
    virtual ~XProcessObject();
    // one frame of `n` ticks (XApplication::FrameMove passes at most 10)
    virtual void ProcessFrame(unsigned n) {}
    virtual void Render(Device* d, BackBuffer* bb) {}
    STATE GetState() const { return state_; }

protected:
    void StartFadeIn(int processType);
    void StartFadeOut(int processType);
    void StartProcess(int processType);
    void ProcessFadeIn();
    void ProcessFadeOut();
    void RenderFade(BackBuffer* bb) const;  // the strips, while fading in or out
    // The title's scrolling background (the title, the save slots and level select): the 120 x 120
    // tile in a 7 x 5 grid, scrolling up and left a quarter pixel a tick. Native: in widescreen
    // the grid takes in as many more columns as the view's width needs, on both sides, in step
    // with the original's (exactly the original's 7 x 5 at 4:3 and in Stretch).
    static void RenderTiledBackground(JSprite& tile, unsigned counter, BackBuffer* bb);

    XApplication* app_;
    STATE state_ = NON;    // +0x08
    unsigned tick_ = 0;    // +0x0c: ticks since the last Start*, counted by the screen
    int processType_ = 0;  // +0x88: what the screen does next (each screen its own meaning)

private:
    static constexpr int kStrips = 30;
    std::unique_ptr<JSprite> strips_[kStrips];  // +0x10
};
