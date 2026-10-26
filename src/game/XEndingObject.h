// XEndingObject (XEndingObject.cpp of the original): the ending, after the last world's last
// level.
//
// The two mages walk side by side towards the camera down an endless corridor: a long floor
// and two walls whose textures scroll under their feet, between film-strip bands at the top and
// bottom of the screen that scroll left. The menu music plays. Five seconds after the fade-in four
// credit pictures, one after another, slide in from the right to the middle, hold there, and
// slide out to the left; the last one stays until a button (or Escape) is pressed, which fades
// out to the logo.
//
// Native (widescreen): the film strips run the view's whole width and the credits wait and leave
// past the wide view's edges (both the original's at 4:3).
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "game/XProcessObject.h"
#include <memory>

class JObject3DRect;
class JObject3DX;
class JSprite;
class XCamera;

class XEndingObject : public XProcessObject {
public:
    explicit XEndingObject(XApplication* app);
    ~XEndingObject() override;
    void ProcessFrame(unsigned n) override;
    void Render(Device* d, BackBuffer* bb) override;

private:
    // XEndingObject::CREDITSTATE
    enum CreditState { NONE = 0, WAIT1 = 1, APPEAR = 2, WAIT2 = 3, DISAPPEAR = 4, WAIT_FOR_INPUT = 5 };
    static constexpr int kCredits = 4;

    std::unique_ptr<XCamera> camera_;                             // +0x90
    std::unique_ptr<JSprite> filmUpper_, filmLower_;              // +0x94, +0x98
    std::unique_ptr<JSprite> credit_[kCredits];                   // +0x9c
    std::unique_ptr<JObject3DRect> wallLeft_, wallRight_, floor_; // +0xac, +0xb0, +0xb4
    std::unique_ptr<JObject3DX> mageBodyA_, mageBodyB_;           // +0xb8, +0xbc
    std::unique_ptr<JObject3DX> mageWandA_, mageWandB_;           // +0xc0, +0xc4
    std::unique_ptr<JObject3DRect> shadowA_, shadowB_;            // +0xc8, +0xcc
    float textureOffset_ = 0;    // +0xd0: the corridor's scroll, 0.008 a frame
    unsigned localCounter_ = 0;  // +0xd4: every tick
    int creditIndex_ = -1;       // +0xd8: the credit shown (-1 before the first)
    int creditPosX_ = 0;         // +0xdc: its centre's x
    int creditState_ = WAIT1;    // +0xe0
};
