#include "XGameScene.h"
#include "game/JSoundList.h"
#include "game/XApplication.h"
#include "game/XParticle.h"
#include "game/XShop.h"
#include "game/XStatusDisplay.h"
#include "audio/midi.h"
#include "framework/d3dutil.h"
#include "gfx/device.h"
#include "input/input.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
// XGameObject::XGameObject: the start positions, side by side across the level in a two-player
// game
const D3DVECTOR kStartOnePlayer(100.0f, 0.0f, 60.0f);
const D3DVECTOR kStartTwoPlayerA(60.0f, 0.0f, 60.0f);
const D3DVECTOR kStartTwoPlayerB(140.0f, 0.0f, 60.0f);

// XGameObject::ProcessFrame: the fades last until tick 254; after a level ends the screen waits
// 200 ticks (cleared) or 300 (lost) before fading out
constexpr unsigned kFadeTicks = 254;
constexpr unsigned kClearedWait = 200;
constexpr unsigned kLostWait = 300;
// processType: 0 playing (after the fade: back to level select), 1 next level, 2 next world,
// 3 the ending, 4 lost
enum { PLAYING = 0, NEXT_LEVEL = 1, NEXT_WORLD = 2, TO_ENDING = 3, LOST = 4 };

// DirectInput codes not named in input.h
enum { DIK_2_ = 0x03, DIK_LBRACKET_ = 0x1A, DIK_RBRACKET_ = 0x1B };

bool Alive(const XHero& h) {
    XHero::LiveState s = h.GetLiveState();
    return s != XHero::NON && s != XHero::DEAD && s != XHero::COMPLETED;
}
}  // namespace

// XHeroBase: a 35-unit shadow under each hero
XGameScene::XGameScene(XApplication* app)
    : XProcessObject(app),
      heroShadowA_(std::make_unique<XShadow>(35.0f)),
      heroShadowB_(std::make_unique<XShadow>(35.0f)) {}
XGameScene::~XGameScene() = default;

void XGameScene::Start() {
    shop_ = std::make_unique<XShop>(app_, &camera_);
    app_->StatusDisplay().InitForGame();
    InitLevel();
}

bool XGameScene::InitLevel() {
    const XLevelStatus& ls = app_->LevelStatus();
    if (!level_.Load(ls.currentWorld, ls.currentLevel)) return false;
    app_->MarkLevelStart();  // (native: what the options menu's "Level select" goes back to)
    // objects not reconstructed yet keep their coloured markers
    level_.FilterMarkers([](const XLevelView::Object& o) { return !XLevelLoop::IsReconstructed(o.code); });
    // XGameObject::XGameObject makes both heroes; XGameObject::InitLevel resets hero B only in a
    // two-player game
    bool onePlayer = app_->OnePlayer();
    heroA_ = std::make_unique<XHero>(true);
    heroA_->SetStatus(&app_->HeroStatus(true));
    heroB_ = std::make_unique<XHero>(false);
    heroB_->SetStatus(&app_->HeroStatus(false));
    if (onePlayer) {
        heroA_->SetStartPos(kStartOnePlayer);
    } else {
        heroA_->SetStartPos(kStartTwoPlayerA);
        heroB_->SetStartPos(kStartTwoPlayerB);
    }
    heroA_->Reset();
    if (!onePlayer) heroB_->Reset();
    loop_.Load(level_, heroA_.get(), heroB_.get(), onePlayer, [this](uint16_t id, float z) { PlaySound(id, z); });
    XStatusDisplay& sd = app_->StatusDisplay();
    loop_.SetLostGoldHandler([&sd](bool heroA, int amount) { sd.ShowLostGoldMessage(heroA, amount); });
    loop_.SetMonoSoundHandler([this](uint16_t id, float volume) { app_->Sounds().PlayMono(id, volume); });
    loop_.SetStopMusicHandler([this] { app_->StopMusic(); });  // JMidiList::Stop
    loop_.SetShakeHandler([this](float magnitude) { camera_.StartShaking(magnitude); });
    loop_.SetShopHandler([this](XHero* hero) { shop_->Start(hero); });
    shop_->Reset();
    camera_.ResetForGame(level_.Depth());
    sd.InitForGame();  // XStatusDisplay::Reset: the counter and the lost-gold messages
    levelChanged_ = true;
    app_->PlayMusic(level_.MusicTrack());  // JMidiList::Play: Music06 with a boss, else the world's
    StartFadeIn(PLAYING);
    return true;
}

void XGameScene::TestPlaceHero(float x, float z, bool heroA) {
    XHero* h = heroA ? heroA_.get() : heroB_.get();
    if (!h || (!heroA && app_->OnePlayer())) return;
    h->SetStartPos(D3DVECTOR(x, 0.0f, z));
    h->Reset();  // drops in from above, as at a level start
}

void XGameScene::UpdateInput() {
    uint8_t keys[256];
    Input::GetKeyboardState(keys);
    auto down = [&](int dik) { return (keys[dik] & 0x80) != 0; };
    auto pressed = [&](int dik) { return down(dik) && !(prevKeys_[dik] & 0x80); };
    // one player's keys and gamepad (pad 0 for hero A, pad 1 for hero B): the arrows, fire and
    // jump (each with a second key), buttons 1 and 2 and the stick
    auto read = [&](XHeroInput& in, int right, int left, int up, int dn, int fire1, int fire2, int jump1,
                    int jump2, int padIndex) {
        DIJOYSTATE js{};
        bool pad = Input::NumJoysticks() > padIndex && Input::GetJoystickState(padIndex, &js);
        bool b0 = pad && (js.rgbButtons[0] & 0x80), b1 = pad && (js.rgbButtons[1] & 0x80);
        uint8_t* prev = prevButtons_[padIndex];
        in.keyRight = down(right);
        in.keyLeft = down(left);
        in.keyUp = down(up);
        in.keyDown = down(dn);
        in.button1Pressed = pressed(fire1) || pressed(fire2) || (b0 && !prev[0]);
        in.button1Down = down(fire1) || down(fire2) || b0;
        in.button2Pressed = pressed(jump1) || pressed(jump2) || (b1 && !prev[1]);
        in.button2Down = down(jump1) || down(jump2) || b1;
        in.keyControlUsed = in.keyRight || in.keyLeft || in.keyUp || in.keyDown;
        if (in.keyControlUsed || !pad) {
            in.valueX = in.valueY = 0;  // (and when the joystick is disabled in the settings)
        } else {
            in.valueX = js.lX / 1000.0f;
            in.valueY = js.lY / 1000.0f;
        }
        prev[0] = b0;
        prev[1] = b1;
    };
    // XHeroA::_UpdateInput: the arrows; fire Right Ctrl or [; jump Right Shift or ]
    XHeroInput& a = inputA_;
    read(a, DIK_RIGHT, DIK_LEFT, DIK_UP, DIK_DOWN, DIK_RCONTROL, DIK_LBRACKET_, DIK_RSHIFT, DIK_RBRACKET_, 0);
    if (app_->OnePlayer()) {  // plus player 2's Left Ctrl and Left Shift in a one-player game
        a.button1Pressed = a.button1Pressed || pressed(DIK_LCONTROL);
        a.button1Down = a.button1Down || down(DIK_LCONTROL);
        a.button2Pressed = a.button2Pressed || pressed(DIK_LSHIFT);
        a.button2Down = a.button2Down || down(DIK_LSHIFT);
    }
    // XHeroB::_UpdateInput: D A W S; fire Left Ctrl or 1; jump Left Shift or 2
    read(inputB_, DIK_D, DIK_A, DIK_W, DIK_S, DIK_LCONTROL, DIK_1, DIK_LSHIFT, DIK_2_, 1);
    std::copy(keys, keys + 256, prevKeys_);
}

// Native (widescreen): how much wider than the original's 4:3 the 3D view is, from the window's
// shape as the renderer lays it out this frame (the width of the 480-high picture over 640), so
// any window size or monitor counts, resized or not. 1 at 4:3 and in Stretch, 4/3 at 16:9.
static float ViewWidthFactor() { return std::max(1.0f, GetDevice().Layout().virtualW / 640.0f); }

void XGameScene::PlaySound(uint16_t waveId, float z) {
    // XApplication::PlaySound: panned by the Z offset from the camera, silent beyond 480 units.
    // Native: the 240 (and so the 480) grows with the view's width, so a sound at the edge of a
    // wide screen is placed and faded as one at the edge of the original's 4:3 screen (exactly
    // the original at 4:3).
    float pan = (z - camera_.LookAt().z) / (240.0f * ViewWidthFactor());
    if (!(pan > -2.0f && pan < 2.0f)) return;
    app_->Sounds().PlayStereo(waveId, pan, 1.0f);
}

void XGameScene::ProcessFrame(unsigned n) {
    if (state_ == NON || !heroA_) return;
    // the menus' input, read once a frame (Escape here, and the shop's keys)
    JDInput& in = app_->GetDInput();
    in.UpdateInputState();
    tick_ += n;

    if (state_ == FADEIN) {
        ProcessFadeIn();
        if (tick_ >= kFadeTicks) StartProcess(PLAYING);
    } else if (state_ == FADEOUT) {
        ProcessFadeOut();
        if (tick_ >= kFadeTicks) {
            XLevelStatus& ls = app_->LevelStatus();
            if (processType_ == PLAYING) {  // (a lost level fades out with type 0)
                app_->SetState(XApplication::SELECT_LEVEL);
                state_ = NON;
            } else if (processType_ == NEXT_LEVEL) {
                ls.IncreaseLevel();
                InitLevel();
            } else if (processType_ == NEXT_WORLD) {
                ls.IncreaseWorld();
                app_->SetState(XApplication::SELECT_LEVEL);
                state_ = NON;
            } else if (processType_ == TO_ENDING) {
                ls.IncreaseWorld();
                app_->SetState(XApplication::ENDING);
                state_ = NON;
            }
            app_->DoSaveData();
            return;
        }
    } else if (state_ == PROCESS) {
        if (processType_ == PLAYING) {
            // one player clears the level by warping and loses it by dying; two clear it when one
            // has warped and the other has warped or died, and lose it when both have died
            const XLevelStatus& ls = app_->LevelStatus();
            int next = !ls.IsLastLevel() ? NEXT_LEVEL : !ls.IsLastWorld() ? NEXT_WORLD : TO_ENDING;
            XHero::LiveState a = heroA_->GetLiveState(), b = heroB_->GetLiveState();
            if (app_->OnePlayer()) {
                if (a == XHero::COMPLETED) StartProcess(next);
                else if (a == XHero::DEAD) StartProcess(LOST);
            } else {
                if ((a == XHero::COMPLETED && (b == XHero::COMPLETED || b == XHero::DEAD)) ||
                    (a == XHero::DEAD && b == XHero::COMPLETED))
                    StartProcess(next);
                else if (a == XHero::DEAD && b == XHero::DEAD)
                    StartProcess(LOST);
            }
            // Escape makes each hero still playing give up the level (even with the shop open:
            // the shop takes Escape as "exit" as well)
            if (in.GetKeyboardPressed(DIK_ESCAPE)) {
                for (XHero* h : {heroA_.get(), heroB_.get()})
                    if (h->GetLiveState() == XHero::NORMAL) h->StartDying();
            }
        } else if (processType_ == LOST) {
            if (tick_ > kLostWait) StartFadeOut(PLAYING);
        } else if (tick_ > kClearedWait) {
            StartFadeOut(processType_);
        }
    }
    if (shop_->GetState() == XShop::NON) {
        for (unsigned i = 0; i < n; i++) Tick();
    } else {
        // the shop pauses the level: only the status display and the camera run, then the shop.
        // (The heroes' keys are still read, so a key held over from the shop doesn't count as
        // pressed afresh when play resumes, as the original's once-a-frame JDInput.)
        for (unsigned i = 0; i < n; i++) {
            UpdateInput();
            app_->StatusDisplay().ProcessFrame();
            camera_.ProcessFrame(heroA_->GetPos().z, Alive(*heroA_), heroB_->GetPos().z, Alive(*heroB_));
        }
        shop_->ProcessFrame(n);
    }
}

bool XGameScene::PlayerHasControl() const {
    if (state_ != PROCESS || processType_ != PLAYING || !heroA_) return false;
    XShop::STATE shop = shop_->GetState();
    if (shop != XShop::NON && shop != XShop::PROCESS) return false;
    return heroA_->GetLiveState() == XHero::NORMAL || heroB_->GetLiveState() == XHero::NORMAL;
}

void XGameScene::Tick() {
    UpdateInput();
    app_->StatusDisplay().ProcessFrame();  // the status display first
    loop_.ProcessBeforeHero(camera_.LookAt().z);
    // hero A, then hero B, each with the other (the hero-on-hero collision and carrying)
    heroA_->ProcessFrame(inputA_, level_.Collision(), loop_, heroB_.get());
    heroB_->ProcessFrame(inputB_, level_.Collision(), loop_, heroA_.get());
    loop_.ProcessAfterHero();
    // XCamera::ProcessFrame always runs in the game (its tick and the shake); it follows the
    // heroes that are alive
    camera_.ProcessFrame(heroA_->GetPos().z, Alive(*heroA_), heroB_->GetPos().z, Alive(*heroB_));
}

void XGameScene::SetDeviceState(Device* d) {
    // a 45-degree lens (aspect 0.75, near 10, far 1000), one white directional light from
    // (-1, -1, 0.5) at full strength (specular 1, unused: specular lighting stays off) and a
    // dim 0x4c4c4c ambient. The game never changes them afterwards; only the view moves.
    D3DMATRIX proj;
    D3DUtil_SetProjectionMatrix(proj, 0.785398f, 0.75f, 10.0f, 1000.0f);
    d->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &proj);
    D3DLIGHT7 light;
    D3DUtil_InitLight(light, D3DLIGHT_DIRECTIONAL, 1.0f, 1.0f, -1.0f, -1.0f, 0.5f);
    d->SetLight(0, &light);
    d->LightEnable(0, 1);
    d->SetRenderState(D3DRENDERSTATE_AMBIENT, 0xff4c4c4c);
}

void XGameScene::Render(Device* d, BackBuffer* bb) {
    if (state_ == NON || !heroA_) return;
    camera_.DoSetTransform(d);
    // native: the draw range grows with the 3D view (ViewWidthFactor): 400 at 4:3, 533 at 16:9
    loop_.SetVisibleRange(400.0f * ViewWidthFactor());
    // XGameObject::Render: the map, the heroes (with their shadows), the level's objects, then the
    // particles and the shots
    level_.Render(d, false);
    d->SetRenderState(D3DRENDERSTATE_LIGHTING, 1);
    // hero A, then hero B; XHeroBase::Render ends with the shadow under the hero (not while gone,
    // dead or clear)
    auto renderHero = [&](XHero* h, XShadow& shadow) {
        if (!h) return;
        h->Render(d);
        if (Alive(*h)) {
            shadow.SetPos(h->GetPos());
            shadow.Render(d, level_.Collision());
        }
    };
    renderHero(heroA_.get(), *heroShadowA_);
    renderHero(heroB_.get(), *heroShadowB_);
    loop_.RenderObjects(d, camera_.View());
    loop_.RenderEffects(d, camera_.View());
    D3DMATRIX identity;
    D3DUtil_SetIdentityMatrix(identity);
    d->SetTransform(D3DTRANSFORMSTATE_WORLD, &identity);

    // then the status display, blitted over the scene, and the fade over everything
    XStatusDisplay::Info info;
    info.world = level_.World();
    info.level = level_.Level();
    info.bossLevel = loop_.BossLevel();
    info.bossHealth = loop_.BossHealth();
    info.litTorches = loop_.LitTorches();
    info.numTorches = loop_.NumTorches();
    info.onePlayer = app_->OnePlayer();
    info.statusA = &app_->HeroStatus(true);
    info.statusB = &app_->HeroStatus(false);
    info.heroADead = heroA_->GetLiveState() == XHero::DEAD;
    info.heroBDead = heroB_->GetLiveState() == XHero::DEAD;
    if (shop_->GetState() != XShop::NON) shop_->RenderDim(d);  // native: widescreen only
    app_->StatusDisplay().Render(bb, info);
    if (shop_->GetState() != XShop::NON) shop_->Render(d, bb);
    RenderFade(bb);
}
