#include "XGameScene.h"
#include "game/XParticle.h"
#include "game/XStatusDisplay.h"
#include "audio/audio.h"
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
// XGameObject::XGameObject: the one-player start position
const D3DVECTOR kStartOnePlayer(100.0f, 0.0f, 60.0f);

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
enum { DIK_LBRACKET_ = 0x1A, DIK_RBRACKET_ = 0x1B };
}  // namespace

XGameScene::XGameScene() : heroShadow_(std::make_unique<XShadow>(35.0f)) {}  // XHeroBase: a 35-unit shadow
XGameScene::~XGameScene() = default;

bool XGameScene::Load(int world, int level) {
    if (!level_.Load(world, level)) return false;
    // objects not reconstructed yet keep their coloured markers
    level_.FilterMarkers([](const XLevelView::Object& o) { return !XLevelLoop::IsReconstructed(o.code); });
    hero_ = std::make_unique<XHero>();
    hero_->SetStatus(&status_);
    hero_->SetStartPos(kStartOnePlayer);
    hero_->Reset();
    loop_.Load(level_, hero_.get(), &status_, onePlayer_, [this](uint16_t id, float z) { PlaySound(id, z); });
    if (!statusDisplay_) statusDisplay_ = std::make_unique<XStatusDisplay>();
    statusDisplay_->InitForGame();
    loop_.SetLostGoldHandler([this](bool heroA, int amount) { statusDisplay_->ShowLostGoldMessage(heroA, amount); });
    camera_.ResetForGame(level_.Depth());
    processTicks_ = 0;
    return true;
}

void XGameScene::TestPlaceHero(float x, float z) {
    if (!hero_) return;
    hero_->SetStartPos(D3DVECTOR(x, 0.0f, z));
    hero_->Reset();  // drops in from above, as at a level start
}

void XGameScene::UpdateInput() {
    uint8_t keys[256];
    Input::GetKeyboardState(keys);
    auto down = [&](int dik) { return (keys[dik] & 0x80) != 0; };
    auto pressed = [&](int dik) { return down(dik) && !(prevKeys_[dik] & 0x80); };
    DIJOYSTATE js{};
    bool pad = Input::NumJoysticks() > 0 && Input::GetJoystickState(0, &js);
    bool b0 = pad && (js.rgbButtons[0] & 0x80), b1 = pad && (js.rgbButtons[1] & 0x80);

    XHeroInput& in = input_;
    in.keyRight = down(DIK_RIGHT);
    in.keyLeft = down(DIK_LEFT);
    in.keyUp = down(DIK_UP);
    in.keyDown = down(DIK_DOWN);
    // fire: Right Ctrl, [ or button 1 (plus Left Ctrl in a one-player game)
    in.button1Pressed = pressed(DIK_RCONTROL) || pressed(DIK_LBRACKET_) || (b0 && !prevButtons_[0]);
    in.button1Down = down(DIK_RCONTROL) || down(DIK_LBRACKET_) || b0;
    if (onePlayer_) {
        in.button1Pressed = in.button1Pressed || pressed(DIK_LCONTROL);
        in.button1Down = in.button1Down || down(DIK_LCONTROL);
    }
    // jump: Right Shift, ] or button 2 (plus Left Shift in a one-player game)
    in.button2Pressed = pressed(DIK_RSHIFT) || pressed(DIK_RBRACKET_) || (b1 && !prevButtons_[1]);
    in.button2Down = down(DIK_RSHIFT) || down(DIK_RBRACKET_) || b1;
    if (onePlayer_) {
        in.button2Pressed = in.button2Pressed || pressed(DIK_LSHIFT);
        in.button2Down = in.button2Down || down(DIK_LSHIFT);
    }
    in.keyControlUsed = in.keyRight || in.keyLeft || in.keyUp || in.keyDown;
    if (in.keyControlUsed || !pad) {
        in.valueX = in.valueY = 0;  // (and when the joystick is disabled in the settings)
    } else {
        in.valueX = js.lX / 1000.0f;
        in.valueY = js.lY / 1000.0f;
    }
    // XGameObject::ProcessFrame: Escape gives up the level
    if (pressed(DIK_ESCAPE) && hero_ && hero_->GetLiveState() == XHero::NORMAL) hero_->StartDying();
    std::copy(keys, keys + 256, prevKeys_);
    prevButtons_[0] = b0;
    prevButtons_[1] = b1;
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
    if (!hero_) return;
    UpdateInput();
    statusDisplay_->ProcessFrame();  // XGameObject::ProcessFrame: the status display first
    loop_.ProcessBeforeHero(camera_.LookAt().z);
    hero_->ProcessFrame(input_, level_.Collision(), loop_);
    loop_.ProcessAfterHero();
    XHero::LiveState s = hero_->GetLiveState();
    if (s != XHero::DEAD && s != XHero::COMPLETED) camera_.ProcessFrame(hero_->GetPos().z);

    if (s == XHero::COMPLETED && ++processTicks_ > 200) {
        // the next level of the world; after the last one, the next world (level select later)
        int w = level_.World(), l = level_.Level() + 1;
        if (l >= GameFiles::kLevelsPerWorld) {
            l = 0;
            w = (w + 1) % GameFiles::kWorldCount;
        }
        Load(w, l);
        levelChanged_ = true;
    } else if (s == XHero::DEAD && ++processTicks_ > 300) {
        Load(level_.World(), level_.Level());  // (level select later)
        levelChanged_ = true;
    }
}

void XGameScene::Render(Device* d) {
    camera_.DoSetTransform(d);
    // XGameObject::Render: the map, the heroes (with their shadows), the level's objects, then the
    // particles and the shots
    level_.Render(d, false);
    d->SetRenderState(D3DRENDERSTATE_LIGHTING, 1);
    if (hero_) {
        hero_->Render(d);
        // XHeroBase::Render ends with the shadow under the hero (not while gone, dead or clear)
        XHero::LiveState s = hero_->GetLiveState();
        if (s != XHero::NON && s != XHero::DEAD && s != XHero::COMPLETED) {
            heroShadow_->SetPos(hero_->GetPos());
            heroShadow_->Render(d, level_.Collision());
        }
    }
    loop_.RenderObjects(d);
    loop_.RenderEffects(d, camera_.View());
    D3DMATRIX identity;
    D3DUtil_SetIdentityMatrix(identity);
    d->SetTransform(D3DTRANSFORMSTATE_WORLD, &identity);
}

void XGameScene::RenderStatus(BackBuffer* bb) {
    if (!statusDisplay_ || !hero_) return;
    XStatusDisplay::Info info;
    info.world = level_.World();
    info.level = level_.Level();
    info.bossLevel = loop_.BossLevel();
    info.bossHealth = 0;  // (the bosses come with stage 5)
    info.litTorches = loop_.LitTorches();
    info.numTorches = loop_.NumTorches();
    info.onePlayer = onePlayer_;
    info.statusA = &status_;
    info.heroADead = hero_->GetLiveState() == XHero::DEAD;
    statusDisplay_->Render(bb, info);
}
