// JSoundList / JSound (the J library): the game's sound effects, each a WAVE resource with a
// fixed number of copies so it can overlap itself. XApplication owns the list; the logo and the
// game play through it.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include <cstdint>
#include <map>
#include <vector>

class JSoundList {
public:
    // JSoundList::PlayStereo -> JSound::SetVolume, SetPanFactor (clamped to 0..1 and -3..3),
    // PlayStereo: beyond one unit of pan the sound fades out, at three it is silent
    void PlayStereo(uint16_t waveId, float pan, float volume);
    // JSoundList::PlayMono: centred
    void PlayMono(uint16_t waveId, float volume) { PlayStereo(waveId, 0.0f, volume); }

private:
    struct Sound {
        std::vector<int> buffers;  // the copies' audio buffers
        int next = 0;
    };
    std::map<uint16_t, Sound> sounds_;
};
