// RIFF WAVE resources. Every sound in the game is uncompressed PCM (8-bit mono, 22050 Hz), but
// 16-bit and stereo are accepted too so replacement sounds can use them.
#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>

struct WaveData {
    int channels = 0, sampleRate = 0, bitsPerSample = 0;
    std::vector<float> samples;  // interleaved, -1..1
    size_t FrameCount() const { return channels ? samples.size() / channels : 0; }
};

bool DecodeWave(const uint8_t* data, size_t size, WaveData* out);
