// XShop (XShop.cpp of the original): the shop screen, opened by the shop building
// (XStaticObj_Shop) for the hero standing at its door.
//
// Four items for gold: the heal potion (20, one more life, up to 5), the magic potion (100, one
// more magic level, up to 7) and two wands that depend on the world (World 1: the second single
// wand 100, the double wand 200; World 2: double 200, second double 300; World 3: second double
// 300, triple 400; World 4: triple 400, second triple 500). An item is greyed out when the hero
// can't pay for it, is at the most life or magic level, or already holds that wand. The items
// are a 2 x 2 grid with "exit" below the right column; the cursor starts on exit. The board on
// the left shows the shopper's magic level and wand. Each player shops with their own keys and
// gamepad (player 1: the arrows, Right Shift or Right Ctrl; player 2: W A S D, Left Shift or
// Left Ctrl), and buys with their own gold. Escape jumps to exit and leaves (and, as in the
// original, the level's own Escape check makes the hero give up the level too).
//
// While the shop is open (fading in, open or fading out, 40 ticks each fade) the level is paused:
// XGameObject::ProcessFrame runs only the status display and the camera, then the shop.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include <memory>

class BackBuffer;
class Device;
class JNumber;
class JObject3DRect;
class JSprite;
class XApplication;
class XCamera;
class XHero;

class XShop {
public:
    enum STATE { NON = 0, FADEIN = 1, FADEOUT = 2, PROCESS = 3 };  // XShop::STATE

    XShop(XApplication* app, const XCamera* camera);  // `camera` is XGameObject's
    ~XShop();

    STATE GetState() const { return state_; }
    void Reset();                   // the items of the level status's world; closed
    void Start(XHero* hero);        // opens the shop for that hero
    void ProcessFrame(unsigned n);  // the fades, then the input and buying
    void Render(Device* d, BackBuffer* bb);
    // Native (widescreen): the level dimmed across the whole view while the shop is open, drawn
    // before the status display (which stays bright). Only when the view is wider than 4:3:
    // the original has no dim.
    void RenderDim(Device* d) const;

private:
    // XShopItemType
    enum ItemType {
        SHOPITEM_SINGLE_WAND = 0,
        SHOPITEM_DOUBLE_WAND = 1,
        SHOPITEM_TRIPLE_WAND = 2,
        SHOPITEM_SINGLE_WAND_X = 3,
        SHOPITEM_DOUBLE_WAND_X = 4,
        SHOPITEM_TRIPLE_WAND_X = 5,
        SHOPITEM_HEAL_POTION = 6,
        SHOPITEM_MAGIC_POTION = 7,
    };
    struct XShopItem {
        ItemType type = SHOPITEM_SINGLE_WAND;
        JSprite* pSprite = nullptr;
        int price = 0;
        int enabled = 0;
    };
    static constexpr int kItems = 4;
    static constexpr int kExit = kItems;  // the cursor on "exit"

    void _UpdateInput();        // the shopper's keys and gamepad
    void _CheckEnabledItems();  // grey out what the shopper can't buy
    float _SlideIn() const;     // how far in: 0..1 over each fade, 1 while open

    XApplication* app_;
    const XCamera* camera_;
    unsigned localCounter_ = 0;
    STATE state_ = NON;
    XHero* pHero_ = nullptr;
    XShopItem shopItem_[kItems];
    std::unique_ptr<JSprite> bmpHealPotion_, bmpMagicPotion_, bmpDoubleWand_, bmpTripleWand_;
    std::unique_ptr<JSprite> bmpSingleWandX_, bmpDoubleWandX_, bmpTripleWandX_;
    std::unique_ptr<JSprite> bmpShopBoard_, bmpShopKeeper_, bmpExit_;
    std::unique_ptr<JSprite> bmpMageIconA_, bmpMageIconB_;
    std::unique_ptr<JSprite> cursor_, cursorSmall_;  // around an item / around exit (two frames: blink)
    std::unique_ptr<JSprite> textWandA_, textWandB_, textMagicLevelA_, textMagicLevelB_;
    std::unique_ptr<JSprite> textWandNames_;  // six frames, one per wand type
    std::unique_ptr<JNumber> numMagicLevel_;
    std::unique_ptr<JObject3DRect> back_;  // the dark panel behind the shop, 300 x 140
    int cursorPos_ = kExit;
    int keyRightPressed_ = 0, keyLeftPressed_ = 0, keyUpPressed_ = 0, keyDownPressed_ = 0;
    int keyButtonPressed_ = 0;
};
