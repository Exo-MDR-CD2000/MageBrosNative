#include "wave.h"
#include <cstring>

namespace {
uint16_t rd16(const uint8_t* p) { return (uint16_t)(p[0] | p[1] << 8); }
uint32_t rd32(const uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
}  // namespace

bool DecodeWave(const uint8_t* d, size_t size, WaveData* out) {
    if (size < 12 || std::memcmp(d, "RIFF", 4) != 0 || std::memcmp(d + 8, "WAVE", 4) != 0) return false;
    const uint8_t* fmt = nullptr;
    const uint8_t* data = nullptr;
    size_t dataSize = 0;
    size_t o = 12;
    while (o + 8 <= size) {
        uint32_t len = rd32(d + o + 4);
        const uint8_t* body = d + o + 8;
        size_t avail = size - (o + 8);
        if (!std::memcmp(d + o, "fmt ", 4) && len >= 16 && len <= avail) fmt = body;
        if (!std::memcmp(d + o, "data", 4)) {
            data = body;
            dataSize = len <= avail ? len : avail;  // tolerate a data chunk that claims too much
        }
        o += 8 + (size_t)len + (len & 1);
    }
    if (!fmt || !data) return false;
    if (rd16(fmt) != 1) return false;  // PCM only
    out->channels = rd16(fmt + 2);
    out->sampleRate = (int)rd32(fmt + 4);
    out->bitsPerSample = rd16(fmt + 14);
    if (out->channels < 1 || out->channels > 2 || out->sampleRate <= 0) return false;
    out->samples.clear();
    if (out->bitsPerSample == 8) {
        out->samples.reserve(dataSize);
        for (size_t i = 0; i < dataSize; i++) out->samples.push_back((data[i] - 128) / 128.0f);
    } else if (out->bitsPerSample == 16) {
        out->samples.reserve(dataSize / 2);
        for (size_t i = 0; i + 1 < dataSize; i += 2)
            out->samples.push_back((int16_t)rd16(data + i) / 32768.0f);
    } else {
        return false;
    }
    out->samples.resize(out->samples.size() - out->samples.size() % out->channels);
    return true;
}
