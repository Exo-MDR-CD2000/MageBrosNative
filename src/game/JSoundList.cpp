#include "JSoundList.h"
#include "audio/audio.h"
#include "core/assets.h"
#include "core/wave.h"
#include <algorithm>

namespace {
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
}  // namespace

void JSoundList::PlayStereo(uint16_t waveId, float pan, float volume) {
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
