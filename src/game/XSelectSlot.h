// XSelectSlot (XSelectSlot.cpp of the original): the save-slot screen.
//
// The 8 slots of the game mode chosen on the title (one or two players), each showing its number
// and, if it holds a save, the date it was saved and the world and level reached; below them
// "back". Up / Down choose (wrapping round), the button takes the slot (an empty one starts a new
// game there) and goes to level select, or goes back to the title; Escape goes back. Delete on a
// slot with a save asks whether to delete it: Left / Right choose yes or no (no at first), the
// button answers, Escape answers no. Over the title's scrolling background, with the menu music.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "game/XProcessObject.h"
#include "game/XSaveData.h"
#include <memory>

class JNumber;
class JObject3DRect;
class JSprite;
class XCamera;

class XSelectSlot : public XProcessObject {
public:
    explicit XSelectSlot(XApplication* app);
    ~XSelectSlot() override;
    void ProcessFrame(unsigned n) override;
    void Render(Device* d, BackBuffer* bb) override;

private:
    static constexpr int kSlots = XSaveData::kSlots;
    static constexpr int kBack = kSlots;  // the cursor on "back"

    void LoadSlots();   // _LoadSlots
    void DeleteSlot();  // _DeleteSlot

    std::unique_ptr<XCamera> camera_;                    // +0x90
    std::unique_ptr<JSprite> cursorLeft_, cursorRight_;  // +0x94, +0x98: the blinking arrows
    std::unique_ptr<JSprite> background_;                // +0x9c: the 120 x 120 tile
    std::unique_ptr<JSprite> unregistered_;              // +0xa0 (drawn only if unregistered)
    std::unique_ptr<JSprite> help_;                      // +0xa4: the keys, at the bottom
    std::unique_ptr<JSprite> headingOne_, headingTwo_;   // +0xa8, +0xac: one player / two players
    std::unique_ptr<JSprite> noData_, date_;             // +0xb0, +0xb4: the labels on a slot
    std::unique_ptr<JSprite> worldLabel_, dash_;         // +0xb8, +0xbc
    std::unique_ptr<JSprite> month_;                     // +0xc0: 12 frames, January to December
    std::unique_ptr<JSprite> slotNumber_;                // +0xc4: 8 frames, slots 1 to 8
    std::unique_ptr<JSprite> slotBar_[kSlots];           // +0xc8: 2 frames, chosen and not
    std::unique_ptr<JSprite> back_;                      // +0xe8: 2 frames
    std::unique_ptr<JNumber> day_, world_, level_;       // +0xec, +0xf0, +0xf4
    std::unique_ptr<JSprite> question_, yes_, no_;       // +0xf8, +0xfc, +0x100: deleting
    std::unique_ptr<JObject3DRect> questionBar_;         // +0x104: the dark bar behind the question
    int cursor_ = 0;           // +0x108: 0..7 a slot, 8 back
    int deleteChoice_ = 0;     // +0x10c: 0 yes, 1 no
    unsigned counter_ = 0;     // +0x110: ticks since the screen opened
    bool confirmDelete_ = false;  // +0x114: the delete question is open
    bool empty_[kSlots] = {};     // +0x118: the slot holds no save
    XSaveData slots_[kSlots];     // +0x138
};
