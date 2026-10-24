// XSelectLevelObject (XSelectLevelObject.cpp of the original): level select.
//
// The four world banners, each with a row of ten level dots (lit up to the level reached in that
// world, for the worlds open) and a "cleared" badge once the world's last level is won; a marker
// on the current level; the status display (level, gold, life) at the top; and a menu of three
// items along the bottom: start the level, pick a level, back. Left / Right choose an item, the
// button takes it, Escape chooses back. While picking, Left / Right move along the levels reached
// and Up / Down between the open worlds (both wrapping round); the button or Escape ends picking.
// Back saves the game and returns to the title. Over the title's scrolling background, with the
// menu music.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "game/XProcessObject.h"
#include <memory>

class JSprite;
class XCamera;

class XSelectLevelObject : public XProcessObject {
public:
    explicit XSelectLevelObject(XApplication* app);
    ~XSelectLevelObject() override;
    void ProcessFrame(unsigned n) override;
    void Render(Device* d, BackBuffer* bb) override;

private:
    std::unique_ptr<XCamera> camera_;                    // +0x90
    std::unique_ptr<JSprite> arrowLeft_, arrowRight_;    // +0x94, +0x98: blinking
    std::unique_ptr<JSprite> arrowUp_, arrowDown_;       // +0x9c, +0xa0: blinking (picking)
    std::unique_ptr<JSprite> items_[3];                  // +0xa4: start, pick a level, back
    std::unique_ptr<JSprite> background_;                // +0xb0: the 120 x 120 tile
    std::unique_ptr<JSprite> unregistered_;              // +0xb4 (drawn only if unregistered)
    std::unique_ptr<JSprite> heading_;                   // +0xb8
    std::unique_ptr<JSprite> worlds_[4];                 // +0xbc: the world banners
    std::unique_ptr<JSprite> dot_;                       // +0xcc: a level, 2 frames (reached, not)
    std::unique_ptr<JSprite> link_;                      // +0xd0: between two dots
    std::unique_ptr<JSprite> cleared_;                   // +0xd4: on a cleared world's banner
    std::unique_ptr<JSprite> panel_;                     // +0xd8
    std::unique_ptr<JSprite> marker_;                    // +0xdc: on the current level
    int choice_ = -1;        // +0xe0: the menu item (-1 while fading in)
    unsigned counter_ = 0;   // +0xe4: ticks since the screen opened
    bool picking_ = false;   // +0xe8: picking a level
};
