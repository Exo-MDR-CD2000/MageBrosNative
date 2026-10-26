#include "XShop.h"
#include "game/JDInput.h"
#include "game/JObject3D.h"
#include "game/JSoundList.h"
#include "game/JSprite.h"
#include "game/XApplication.h"
#include "game/XCamera.h"
#include "game/XHero.h"
#include "gfx/device.h"
#include "input/input.h"

namespace {
// bitmaps: the items (two frames: on, greyed out) are not colour-keyed, nor is "exit"
const uint16_t kBitmapHealPotion = 0x212, kBitmapMagicPotion = 0x214, kBitmapDoubleWand = 0x210;
const uint16_t kBitmapTripleWand = 0x216, kBitmapSingleWandX = 0x215, kBitmapDoubleWandX = 0x211;
const uint16_t kBitmapTripleWandX = 0x217, kBitmapShopBoard = 0x221, kBitmapShopKeeper = 0x213;
const uint16_t kBitmapExit = 0x219, kBitmapMageIconA = 0x1fd, kBitmapMageIconB = 0x1fe;
const uint16_t kBitmapCursor = 0x21b, kBitmapCursorSmall = 0x21a;
const uint16_t kBitmapTextWandA = 0x21e, kBitmapTextWandB = 0x21f;
const uint16_t kBitmapTextMagicLevelA = 0x220, kBitmapTextMagicLevelB = 0x21d, kBitmapTextWandNames = 0x21c;
const uint16_t kBitmapDigits = 0xba;
const uint16_t kTextureBack = 0x218;
// sounds
const uint16_t kSoundMove = 0xec, kSoundDecide = 0xed;
constexpr unsigned kFadeTicks = 40;
// DirectInput codes not named in input.h
enum { DIK_2_ = 0x03, DIK_LBRACKET_ = 0x1A, DIK_RBRACKET_ = 0x1B };
}  // namespace

// XShop::XShop
XShop::XShop(XApplication* app, const XCamera* camera) : app_(app), camera_(camera) {
    for (uint16_t id : {kBitmapHealPotion, kBitmapMagicPotion, kBitmapDoubleWand, kBitmapTripleWand,
                        kBitmapSingleWandX, kBitmapDoubleWandX, kBitmapTripleWandX})
        JSurfaceList::Create(id, false);
    JSurfaceList::Create(kBitmapShopBoard, true);
    JSurfaceList::Create(kBitmapShopKeeper, true);
    JSurfaceList::Create(kBitmapExit, false);
    for (uint16_t id : {kBitmapCursor, kBitmapCursorSmall, kBitmapTextWandA, kBitmapTextWandB,
                        kBitmapTextMagicLevelA, kBitmapTextMagicLevelB, kBitmapTextWandNames})
        JSurfaceList::Create(id, true);
    bmpHealPotion_ = std::make_unique<JSprite>(kBitmapHealPotion, 2);
    bmpMagicPotion_ = std::make_unique<JSprite>(kBitmapMagicPotion, 2);
    bmpDoubleWand_ = std::make_unique<JSprite>(kBitmapDoubleWand, 2);
    bmpTripleWand_ = std::make_unique<JSprite>(kBitmapTripleWand, 2);
    bmpSingleWandX_ = std::make_unique<JSprite>(kBitmapSingleWandX, 2);
    bmpDoubleWandX_ = std::make_unique<JSprite>(kBitmapDoubleWandX, 2);
    bmpTripleWandX_ = std::make_unique<JSprite>(kBitmapTripleWandX, 2);
    bmpShopBoard_ = std::make_unique<JSprite>(kBitmapShopBoard, 1);
    bmpShopKeeper_ = std::make_unique<JSprite>(kBitmapShopKeeper, 1);
    bmpExit_ = std::make_unique<JSprite>(kBitmapExit, 1);
    bmpMageIconA_ = std::make_unique<JSprite>(kBitmapMageIconA, 1);
    bmpMageIconB_ = std::make_unique<JSprite>(kBitmapMageIconB, 1);
    cursor_ = std::make_unique<JSprite>(kBitmapCursor, 2);
    cursorSmall_ = std::make_unique<JSprite>(kBitmapCursorSmall, 2);
    textWandA_ = std::make_unique<JSprite>(kBitmapTextWandA, 1);
    textWandB_ = std::make_unique<JSprite>(kBitmapTextWandB, 1);
    textMagicLevelA_ = std::make_unique<JSprite>(kBitmapTextMagicLevelA, 1);
    textMagicLevelB_ = std::make_unique<JSprite>(kBitmapTextMagicLevelB, 1);
    textWandNames_ = std::make_unique<JSprite>(kBitmapTextWandNames, 6);
    numMagicLevel_ = std::make_unique<JNumber>(kBitmapDigits);
    numMagicLevel_->SetNumDigits(1);
    back_ = std::make_unique<JObject3DRect>(JObject3DRect::Particle(300.0f, 140.0f));
    back_->SetTexture(kTextureBack);
    back_->SetAlphaState(JObject3DRect::ALPHA_TRANSPARENT);
    back_->SetAlphaFactor(0.75f);
}

XShop::~XShop() = default;

// XShop::Reset: the potions, then the world's two wands
void XShop::Reset() {
    shopItem_[0] = {SHOPITEM_HEAL_POTION, bmpHealPotion_.get(), 20, 0};
    shopItem_[1] = {SHOPITEM_MAGIC_POTION, bmpMagicPotion_.get(), 100, 0};
    switch (app_->LevelStatus().currentWorld) {
    case 0:
        shopItem_[2] = {SHOPITEM_SINGLE_WAND_X, bmpSingleWandX_.get(), 100, 0};
        shopItem_[3] = {SHOPITEM_DOUBLE_WAND, bmpDoubleWand_.get(), 200, 0};
        break;
    case 1:
        shopItem_[2] = {SHOPITEM_DOUBLE_WAND, bmpDoubleWand_.get(), 200, 0};
        shopItem_[3] = {SHOPITEM_DOUBLE_WAND_X, bmpDoubleWandX_.get(), 300, 0};
        break;
    case 2:
        shopItem_[2] = {SHOPITEM_DOUBLE_WAND_X, bmpDoubleWandX_.get(), 300, 0};
        shopItem_[3] = {SHOPITEM_TRIPLE_WAND, bmpTripleWand_.get(), 400, 0};
        break;
    case 3:
        shopItem_[2] = {SHOPITEM_TRIPLE_WAND, bmpTripleWand_.get(), 400, 0};
        shopItem_[3] = {SHOPITEM_TRIPLE_WAND_X, bmpTripleWandX_.get(), 500, 0};
        break;
    }
    pHero_ = nullptr;
    localCounter_ = 0;
    state_ = NON;
}

// XShop::Start: the panel 220 across and 150 up at the camera's look-at Z, every item on, then
// greyed out by what the hero has; the cursor on exit
void XShop::Start(XHero* hero) {
    back_->SetWorldPos(D3DVECTOR(220.0f, 150.0f, camera_->LookAt().z));
    pHero_ = hero;
    for (XShopItem& item : shopItem_) item.enabled = 1;
    _CheckEnabledItems();
    cursorPos_ = kExit;
    localCounter_ = 0;
    state_ = FADEIN;
}

// XShop::_CheckEnabledItems: off when too dear, the heal potion at 5 lives, the magic potion at
// magic level 7, a wand the hero already holds. (Only ever turns items off: Start turns them on.)
void XShop::_CheckEnabledItems() {
    const XHeroStatus& s = app_->HeroStatus(pHero_->IsHeroA());
    for (XShopItem& item : shopItem_)
        if (s.gold < item.price) item.enabled = 0;
    if (s.life >= 5) shopItem_[0].enabled = 0;
    if (s.magicLevel >= 7) shopItem_[1].enabled = 0;
    if (s.wandType == shopItem_[2].type) shopItem_[2].enabled = 0;
    if (s.wandType == shopItem_[3].type) shopItem_[3].enabled = 0;
}

// XShop::_UpdateInput: the shopper's own keys and gamepad, pressed this frame. Player 1: the
// arrows and gamepad 1, the button Right Shift, Right Ctrl or gamepad button 1 or 2; player 2:
// D A W S and gamepad 2, Left Shift, Left Ctrl or its buttons. (The joystick is used unless
// RegInfo_GetDisableJoystick says otherwise: always, here.)
void XShop::_UpdateInput() {
    const JDInput& in = app_->GetDInput();
    // native: the button also takes every other key that fires or jumps for the shopper in the
    // level (player 1's [ and ], and in a one-player game Left Ctrl and Left Shift; player 2's 1
    // and 2), and Enter, as the title and level select do. The original read only the two keys
    // below, so the keys a player used in the level could do nothing in the shop. Remapped keys
    // count, as they stand in for these codes.
    bool extra = in.GetKeyboardPressed(DIK_RETURN) || in.GetKeyboardPressed(DIK_NUMPADENTER);
    if (pHero_->IsHeroA()) {
        extra = extra || in.GetKeyboardPressed(DIK_LBRACKET_) || in.GetKeyboardPressed(DIK_RBRACKET_);
        if (app_->OnePlayer())
            extra = extra || in.GetKeyboardPressed(DIK_LCONTROL) || in.GetKeyboardPressed(DIK_LSHIFT);
    } else {
        extra = extra || in.GetKeyboardPressed(DIK_1) || in.GetKeyboardPressed(DIK_2_);
    }
    if (pHero_->IsHeroA()) {
        keyRightPressed_ = in.GetKeyboardPressed(DIK_RIGHT) || in.GetJoyDirPressed(0, 1);
        keyLeftPressed_ = in.GetKeyboardPressed(DIK_LEFT) || in.GetJoyDirPressed(0, 3);
        keyUpPressed_ = in.GetKeyboardPressed(DIK_UP) || in.GetJoyDirPressed(0, 0);
        keyDownPressed_ = in.GetKeyboardPressed(DIK_DOWN) || in.GetJoyDirPressed(0, 2);
        keyButtonPressed_ = in.GetKeyboardPressed(DIK_RSHIFT) || in.GetKeyboardPressed(DIK_RCONTROL) ||
                            in.GetJoyButtonPressed(0, 0) || in.GetJoyButtonPressed(0, 1) || extra;
    } else {
        keyRightPressed_ = in.GetKeyboardPressed(DIK_D) || in.GetJoyDirPressed(1, 1);
        keyLeftPressed_ = in.GetKeyboardPressed(DIK_A) || in.GetJoyDirPressed(1, 3);
        keyUpPressed_ = in.GetKeyboardPressed(DIK_W) || in.GetJoyDirPressed(1, 0);
        keyDownPressed_ = in.GetKeyboardPressed(DIK_S) || in.GetJoyDirPressed(1, 2);
        keyButtonPressed_ = in.GetKeyboardPressed(DIK_LSHIFT) || in.GetKeyboardPressed(DIK_LCONTROL) ||
                            in.GetJoyButtonPressed(1, 0) || in.GetJoyButtonPressed(1, 1) || extra;
    }
}

// XShop::ProcessFrame. The cursor: items 0 1 on top, 2 3 below, exit (4) under 3. One move a
// frame, in the order left, right, up, down, button; each direction plays the move sound even
// when the cursor can't go that way. Escape puts the cursor on exit and presses the button.
void XShop::ProcessFrame(unsigned n) {
    if (state_ == NON) return;
    localCounter_ += n;
    if (state_ == FADEIN) {
        if (localCounter_ >= kFadeTicks) {
            localCounter_ = 0;
            state_ = PROCESS;
        }
    } else if (state_ == FADEOUT) {
        if (localCounter_ >= kFadeTicks) {
            localCounter_ = 0;
            state_ = NON;
        }
    } else if (state_ == PROCESS) {
        _UpdateInput();
        if (app_->GetDInput().GetKeyboardPressed(DIK_ESCAPE)) {
            cursorPos_ = kExit;
            keyButtonPressed_ = 1;
        }
        JSoundList& sounds = app_->Sounds();
        if (keyLeftPressed_) {
            sounds.PlayMono(kSoundMove, 1.0f);
            if (cursorPos_ == 1) cursorPos_ = 0;
            else if (cursorPos_ == 3) cursorPos_ = 2;
        } else if (keyRightPressed_) {
            sounds.PlayMono(kSoundMove, 1.0f);
            if (cursorPos_ == 0) cursorPos_ = 1;
            else if (cursorPos_ == 2) cursorPos_ = 3;
        } else if (keyUpPressed_) {
            sounds.PlayMono(kSoundMove, 1.0f);
            if (cursorPos_ == 2) cursorPos_ = 0;
            else if (cursorPos_ == 3) cursorPos_ = 1;
            else if (cursorPos_ == kExit) cursorPos_ = 3;
        } else if (keyDownPressed_) {
            sounds.PlayMono(kSoundMove, 1.0f);
            if (cursorPos_ == 0) cursorPos_ = 2;
            else if (cursorPos_ == 1) cursorPos_ = 3;
            else if (cursorPos_ == 2 || cursorPos_ == 3) cursorPos_ = kExit;
        } else if (keyButtonPressed_) {
            if (cursorPos_ == kExit) {
                sounds.PlayMono(kSoundDecide, 1.0f);
                localCounter_ = 0;
                state_ = FADEOUT;
            } else if (shopItem_[cursorPos_].enabled) {
                // buy: one more life, one more magic level, or the wand; then the gold
                sounds.PlayMono(kSoundDecide, 1.0f);
                XHeroStatus& s = app_->HeroStatus(pHero_->IsHeroA());
                const XShopItem& item = shopItem_[cursorPos_];
                if (item.type == SHOPITEM_HEAL_POTION) s.life++;
                else if (item.type == SHOPITEM_MAGIC_POTION) s.magicLevel++;
                else s.wandType = (int8_t)item.type;
                pHero_->UpdateMagicLevel();
                pHero_->DecreaseGold(item.price);
                _CheckEnabledItems();
            }
        }
    }
}

// XShop::Render: the panel, the board with the shopper's mage, magic level and wand sliding in
// from the left, the items and exit from the right, the shopkeeper rising from the bottom; the
// blinking cursor while open
float XShop::_SlideIn() const {
    float t = 1.0f;
    if (state_ == FADEIN) t = localCounter_ / 40.0f;
    else if (state_ == FADEOUT) t = (40 - (int)localCounter_) / 40.0f;
    if (t < 0.0f) t = 0.0f;
    else if (t > 1.0f) t = 1.0f;
    return t;
}

void XShop::RenderDim(Device* d) const {
    if (state_ == NON) return;
    int viewLeft, viewRight;
    d->ViewXRange(&viewLeft, &viewRight);
    if (viewLeft >= 0) return;  // 4:3 or Stretch: as the original
    const float black[4] = {0.0f, 0.0f, 0.0f, 0.4f * _SlideIn()};
    d->DrawUIQuad((float)viewLeft, 0.0f, (float)viewRight, 480.0f, 0, 0, 0, 0, nullptr, black);
}

void XShop::Render(Device* d, BackBuffer* bb) {
    if (state_ == NON) return;
    float t = _SlideIn();  // how far in: 0..1 over each fade
    const XHeroStatus& s = app_->HeroStatus(pHero_->IsHeroA());
    // native (widescreen): the board and the items slide in from just past the view's edges (the
    // original's x -280 and 640 at 4:3), so they don't pop up in a wide view's sides
    int viewLeft, viewRight;
    d->ViewXRange(&viewLeft, &viewRight);

    back_->SetWorldPos(D3DVECTOR(220.0f, 150.0f, camera_->LookAt().z));
    back_->SetAlphaFactor(t * 0.75f);
    back_->Render2(d, camera_->View());

    // the board, then under it the mage, "magic level" and its number, and below that "wand" and
    // the wand's name
    const int left = 50 - (int)((1.0f - t) * (float)(330 - viewLeft)), top = 152;
    bmpShopBoard_->SetPosition(left, top);
    bmpShopBoard_->Draw(bb);
    bool heroA = pHero_->IsHeroA();
    JSprite* icon = heroA ? bmpMageIconA_.get() : bmpMageIconB_.get();
    JSprite* textMagicLevel = heroA ? textMagicLevelA_.get() : textMagicLevelB_.get();
    JSprite* textWand = heroA ? textWandA_.get() : textWandB_.get();
    int x = left, y = top + bmpShopBoard_->GetHeight() + 20;
    icon->SetPosition(x, y);
    icon->Draw(bb);
    x = left + 42;
    textMagicLevel->SetPosition(x, y);
    textMagicLevel->Draw(bb);
    x += textMagicLevel->GetWidth() + 4;
    numMagicLevel_->SetPosition(x, y);
    numMagicLevel_->SetNumber(s.magicLevel + 1);
    numMagicLevel_->Draw(bb);
    x = left + 42;
    y += textMagicLevel->GetHeight() + 6;
    textWand->SetPosition(x, y);
    textWand->Draw(bb);
    x += textWand->GetWidth() + 4;
    textWandNames_->SetPosition(x, y);
    textWandNames_->SetCurrentFrame(s.wandType);
    textWandNames_->Draw(bb);

    // the items, 107 across and 124 down, greyed out (frame 1) when off; exit under the right
    // column
    const int right = viewRight - (int)(t * (float)(viewRight - 380)), itemTop = 147;
    const int itemX[kItems] = {right, right + 107, right, right + 107};
    const int itemY[kItems] = {itemTop, itemTop, itemTop + 124, itemTop + 124};
    for (int i = 0; i < kItems; i++) {
        JSprite* sprite = shopItem_[i].pSprite;
        sprite->SetPosition(itemX[i], itemY[i]);
        sprite->SetCurrentFrame(shopItem_[i].enabled ? 0 : 1);
        sprite->Draw(bb);
    }
    bmpExit_->SetPosition(right + 107, itemTop + 248);
    bmpExit_->Draw(bb);

    // the cursor, 11 up and to the left of the item (or exit), blinking every 20 ticks
    if (state_ == PROCESS) {
        int blink = localCounter_ % 40 < 20 ? 0 : 1;
        if (cursorPos_ >= 0 && cursorPos_ <= 3) {
            const JSprite* item = shopItem_[cursorPos_].pSprite;
            cursor_->SetPosition(item->GetPosIntX() - 11, item->GetPosIntY() - 11);
            cursor_->SetCurrentFrame(blink);
            cursor_->Draw(bb);
        } else if (cursorPos_ == kExit) {
            cursorSmall_->SetPosition(bmpExit_->GetPosIntX() - 11, bmpExit_->GetPosIntY() - 11);
            cursorSmall_->SetCurrentFrame(blink);
            cursorSmall_->Draw(bb);
        }
    }

    // the shopkeeper, standing on the bottom edge
    bmpShopKeeper_->SetPosition(100, 480 - (int)(bmpShopKeeper_->GetHeight() * t));
    bmpShopKeeper_->Draw(bb);
}
