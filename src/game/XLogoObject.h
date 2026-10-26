// XLogoObject (XLogoObject.cpp of the original): the logo screen the game opens with.
//
// It stops the music, then shows the logo bitmap (0x2d1, not colour-keyed) centred on the screen
// over a 600 x 600 backdrop (texture 0x13c) standing upright in front of the title camera. At
// tick 50 it plays the jingle (0x2d0, unpanned); at tick 950 it hands over to the title. It has
// no fade of its own and nothing skips it.
//
// Native (widescreen): the backdrop is widened with the view, so the white reaches the sides.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "game/XProcessObject.h"
#include <memory>

class JObject3DRect;
class JSprite;
class XCamera;

class XLogoObject : public XProcessObject {
public:
    explicit XLogoObject(XApplication* app);
    ~XLogoObject() override;
    void ProcessFrame(unsigned n) override;
    void Render(Device* d, BackBuffer* bb) override;

private:
    std::unique_ptr<XCamera> camera_;           // +0x90
    std::unique_ptr<JSprite> logo_;             // +0x94
    std::unique_ptr<JObject3DRect> backdrop_;   // +0x98
};
