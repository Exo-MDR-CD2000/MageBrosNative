#include "XGameScene.h"
#include "game/XParticle.h"
#include "game/XStatusDisplay.h"
#include "audio/audio.h"
#include "audio/midi.h"
#include "core/assets.h"
#include "core/game_files.h"
#include "core/wave.h"
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

// XApplication::InitDeviceObjects: how many copies of each sound JSoundList creates (each play
// takes the next copy, so a sound can overlap itself that many times); unlisted sounds get one
struct SoundCopies {
    uint16_t id;
    int copies;
};
const SoundCopies kSoundCopies[] = {
    {0x269, 1}, {0x26a, 1}, {0x266, 2}, {0x2ad, 2}, {0x263, 2}, {0x268, 2}, {0x26e, 2}, {0x27e, 2}, {0x272, 1},
    {0x270, 4}, {0x27d, 2}, {0x26b, 2}, {0x2b4, 2}, {0x27b, 1}, {0x278, 4}, {0x279, 4}, {0x262, 6}, {0x2ae, 1},
    {0x267, 1}, {0xea, 1},  {0x273, 1}, {0x26c, 2}, {0x276, 1}, {0x277, 1}, {0x27a, 1}, {0x27c, 1}, {0x2a2, 1},
    {0x2a4, 1}, {0x2a3, 1}, {0x2a1, 1}, {0x2a5, 4}, {0x2a6, 1}, {0x2a7, 1}, {0x2a8, 1}, {0x2a9, 1}, {0x2aa, 1},
    {0x2ab, 1}, {0x2b3, 1}, {0xec, 1},  {0xed, 1},  {0x282, 1}, {0x284, 1}, {0x283, 1}, {0x2d0, 1},
};

// DirectInput codes not named in input.h
enum { DIK_2_ = 0x03, DIK_LBRACKET_ = 0x1A, DIK_RBRACKET_ = 0x1B };

bool Alive(const XHero& h) {
    XHero::LiveState s = h.GetLiveState();
    return s != XHero::NON && s != XHero::DEAD && s != XHero::COMPLETED;
}
}  // namespace

// XHeroBase: a 35-unit shadow under each hero
XGameScene::XGameScene()
    : heroShadowA_(std::make_unique<XShadow>(35.0f)), heroShadowB_(std::make_unique<XShadow>(35.0f)) {}
XGameScene::~XGameScene() = default;

bool XGameScene::Load(int world, int level) {
    if (!level_.Load(world, level)) return false;
    // objects not reconstructed yet keep their coloured markers
    level_.FilterMarkers([](const XLevelView::Object& o) { return !XLevelLoop::IsReconstructed(o.code); });
    // XGameObject::XGameObject makes both heroes; XGameObject::InitLevel resets hero B only in a
    // two-player game
    heroA_ = std::make_unique<XHero>(true);
    heroA_->SetStatus(&statusA_);
    heroB_ = std::make_unique<XHero>(false);
    heroB_->SetStatus(&statusB_);
    if (onePlayer_) {
        heroA_->SetStartPos(kStartOnePlayer);
    } else {
        heroA_->SetStartPos(kStartTwoPlayerA);
        heroB_->SetStartPos(kStartTwoPlayerB);
    }
    heroA_->Reset();
    if (!onePlayer_) heroB_->Reset();
    loop_.Load(level_, heroA_.get(), heroB_.get(), onePlayer_, [this](uint16_t id, float z) { PlaySound(id, z); });
    if (!statusDisplay_) statusDisplay_ = std::make_unique<XStatusDisplay>();
    statusDisplay_->InitForGame();
    loop_.SetLostGoldHandler([this](bool heroA, int amount) { statusDisplay_->ShowLostGoldMessage(heroA, amount); });
    loop_.SetMonoSoundHandler([this](uint16_t id, float volume) { PlayStereo(id, 0.0f, volume); });  // JSoundList::PlayMono
    loop_.SetStopMusicHandler([] { Midi::Close(); });  // JMidiList::Stop: "close all"
    loop_.SetShakeHandler([this](float magnitude) { camera_.StartShaking(magnitude); });
    camera_.ResetForGame(level_.Depth());
    processTicks_ = 0;
    return true;
}

void XGameScene::TestPlaceHero(float x, float z, bool heroA) {
    XHero* h = heroA ? heroA_.get() : heroB_.get();
    if (!h || (!heroA && onePlayer_)) return;
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
    if (onePlayer_) {  // plus player 2's Left Ctrl and Left Shift in a one-player game
        a.button1Pressed = a.button1Pressed || pressed(DIK_LCONTROL);
        a.button1Down = a.button1Down || down(DIK_LCONTROL);
        a.button2Pressed = a.button2Pressed || pressed(DIK_LSHIFT);
        a.button2Down = a.button2Down || down(DIK_LSHIFT);
    }
    // XHeroB::_UpdateInput: D A W S; fire Left Ctrl or 1; jump Left Shift or 2
    read(inputB_, DIK_D, DIK_A, DIK_W, DIK_S, DIK_LCONTROL, DIK_1, DIK_LSHIFT, DIK_2_, 1);
    // XGameObject::ProcessFrame: Escape makes each hero still playing give up the level
    if (pressed(DIK_ESCAPE)) {
        for (XHero* h : {heroA_.get(), heroB_.get()})
            if (h && h->GetLiveState() == XHero::NORMAL) h->StartDying();
    }
    std::copy(keys, keys + 256, prevKeys_);
}

void XGameScene::PlaySound(uint16_t waveId, float z) {
    // XApplication::PlaySound: panned by the Z offset from the camera, silent beyond 480 units
    float pan = (z - camera_.LookAt().z) / 240.0f;
    if (!(pan > -2.0f && pan < 2.0f)) return;
    PlayStereo(waveId, pan, 1.0f);
}

void XGameScene::PlayStereo(uint16_t waveId, float pan, float volume) {
    // JSoundList::PlayStereo -> JSound::SetVolume, SetPanFactor (clamped to 0..1 and -3..3), PlayStereo
    auto it = sounds_.find(waveId);
    if (it == sounds_.end()) {
        Sound snd;
        int copies = 1;
        for (const auto& e : kSoundCopies)
            if (e.id == waveId) copies = e.copies;
        if (const auto* w = Assets::Wave(waveId)) {
            WaveData wd;
            if (DecodeWave(w->data(), w->size(), &wd))
                for (int i = 0; i < copies; i++) {
                    int h = Audio::CreateBufferF32(wd.samples.data(), wd.FrameCount(), wd.channels, wd.sampleRate);
                    if (h >= 0) snd.buffers.push_back(h);
                }
        }
        it = sounds_.emplace(waveId, std::move(snd)).first;
    }
    Sound& snd = it->second;
    if (snd.buffers.empty()) return;
    volume = std::min(1.0f, std::max(0.0f, volume));
    pan = std::min(3.0f, std::max(-3.0f, pan));
    if (volume <= 0.0f || pan <= -3.0f || pan >= 3.0f) return;
    int h = snd.buffers[snd.next];
    Audio::Stop(h);
    Audio::SetPosition(h, 0);
    // beyond one unit of pan the sound fades: (3 - |pan|) / 2, as a linear share of 100 dB
    float fade = 1.0f;
    if (pan < -1.0f) fade = (pan + 3.0f) * 0.5f;
    else if (pan > 1.0f) fade = (-pan + 3.0f) * 0.5f;
    Audio::SetVolume(h, (long)(volume * fade * 10000.0f) - 10000);
    // two units of pan take 100 dB off the other side
    long dsPan = 0;
    if (pan < 0.0f) dsPan = (long)(std::min(-pan, 2.0f) * 0.5f * -10000.0f);
    else if (pan > 0.0f) dsPan = (long)(std::min(pan, 2.0f) * 0.5f * 10000.0f);
    Audio::SetPan(h, dsPan);
    Audio::Play(h, false);
    snd.next = (snd.next + 1) % (int)snd.buffers.size();  // the next copy, so sounds can overlap
}

void XGameScene::ProcessFrame() {
    if (!heroA_) return;
    UpdateInput();
    statusDisplay_->ProcessFrame();  // XGameObject::ProcessFrame: the status display first
    loop_.ProcessBeforeHero(camera_.LookAt().z);
    // hero A, then hero B (each gets the other for the hero-on-hero checks: chunk 17)
    heroA_->ProcessFrame(inputA_, level_.Collision(), loop_);
    heroB_->ProcessFrame(inputB_, level_.Collision(), loop_);
    loop_.ProcessAfterHero();
    // XCamera::ProcessFrame always runs in the game (its tick and the shake); it follows the
    // heroes that are alive
    camera_.ProcessFrame(heroA_->GetPos().z, Alive(*heroA_), heroB_->GetPos().z, Alive(*heroB_));

    // XGameObject::ProcessFrame: one player clears the level by warping and loses it by dying;
    // two clear it when one has warped and the other has warped or died, and lose it when both
    // have died
    XHero::LiveState a = heroA_->GetLiveState(), b = heroB_->GetLiveState();
    bool cleared, lost;
    if (onePlayer_) {
        cleared = a == XHero::COMPLETED;
        lost = a == XHero::DEAD;
    } else {
        cleared = (a == XHero::COMPLETED && (b == XHero::COMPLETED || b == XHero::DEAD)) ||
                  (a == XHero::DEAD && b == XHero::COMPLETED);
        lost = a == XHero::DEAD && b == XHero::DEAD;
    }
    if (cleared && ++processTicks_ > 200) {
        // the next level of the world; after the last one, the next world (level select later)
        int w = level_.World(), l = level_.Level() + 1;
        if (l >= GameFiles::kLevelsPerWorld) {
            l = 0;
            w = (w + 1) % GameFiles::kWorldCount;
        }
        Load(w, l);
        levelChanged_ = true;
    } else if (lost && ++processTicks_ > 300) {
        Load(level_.World(), level_.Level());  // (level select later)
        levelChanged_ = true;
    }
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

void XGameScene::Render(Device* d) {
    camera_.DoSetTransform(d);
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
}

void XGameScene::RenderStatus(BackBuffer* bb) {
    if (!statusDisplay_ || !heroA_) return;
    XStatusDisplay::Info info;
    info.world = level_.World();
    info.level = level_.Level();
    info.bossLevel = loop_.BossLevel();
    info.bossHealth = loop_.BossHealth();
    info.litTorches = loop_.LitTorches();
    info.numTorches = loop_.NumTorches();
    info.onePlayer = onePlayer_;
    info.statusA = &statusA_;
    info.statusB = &statusB_;
    info.heroADead = heroA_->GetLiveState() == XHero::DEAD;
    info.heroBDead = heroB_->GetLiveState() == XHero::DEAD;
    statusDisplay_->Render(bb, info);
}
