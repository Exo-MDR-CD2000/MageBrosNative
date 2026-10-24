// XTitleObject (XTitleObject.cpp of the original): the title screen.
//
// Over a scrolling tiled background: the title logo (pulsing), two pictures that fade in after 2
// seconds, two captions, and the menu, one player / two players / quit, with a blinking arrow on
// each side of the chosen item. Up / Down (or W / S, or a gamepad) move the choice, Enter (or
// Shift, Ctrl, a gamepad button) takes it, Escape takes "quit". The screen fades out, then sets
// the one-player flag and goes to the save slots, or quits. The menu music (Music01) plays.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "game/XProcessObject.h"
#include <memory>

class JObject3DRect;
class JSprite;
class XCamera;

class XTitleObject : public XProcessObject {
public:
    explicit XTitleObject(XApplication* app);
    ~XTitleObject() override;
    void ProcessFrame(unsigned n) override;
    void Render(Device* d, BackBuffer* bb) override;

private:
    std::unique_ptr<XCamera> camera_;                   // +0x90
    std::unique_ptr<JSprite> cursorLeft_, cursorRight_;  // +0x94, +0x98: the blinking arrows
    std::unique_ptr<JSprite> items_[3];                 // +0x9c: one player, two players, quit
    std::unique_ptr<JSprite> caption1_, background_;    // +0xa8, +0xac (the 120 x 120 tile)
    std::unique_ptr<JSprite> caption2_, unregistered_;  // +0xb0, +0xb4 (drawn only if unregistered)
    std::unique_ptr<JObject3DRect> logo_;               // +0xb8
    std::unique_ptr<JObject3DRect> pictureLeft_, pictureRight_;  // +0xbc, +0xc0
    int cursor_ = -1;                                   // +0xc4: the chosen item (-1 while fading in)
    unsigned counter_ = 0;                              // +0xc8: ticks since the screen opened
};
