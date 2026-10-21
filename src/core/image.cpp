#include "image.h"

namespace {
uint16_t rd16(const uint8_t* p) { return (uint16_t)(p[0] | p[1] << 8); }
uint32_t rd32(const uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

// Size of the header plus colour table, i.e. the offset of the pixel bits within the DIB.
bool DibLayout(const uint8_t* d, size_t size, uint32_t* bitsOffset) {
    if (size < 40) return false;
    uint32_t hdr = rd32(d);
    if (hdr < 40 || hdr > size) return false;
    int bpp = rd16(d + 14);
    uint32_t compression = rd32(d + 16);
    uint32_t clrUsed = rd32(d + 32);
    uint32_t colors = bpp <= 8 ? (clrUsed ? clrUsed : 1u << bpp) : 0;
    uint32_t masks = (compression == 3 && hdr == 40) ? 12 : 0;  // BI_BITFIELDS
    *bitsOffset = hdr + masks + colors * 4;
    return *bitsOffset <= size;
}
}  // namespace

bool DecodeDIB(const uint8_t* d, size_t size, Image* out) {
    uint32_t bitsOff;
    if (!DibLayout(d, size, &bitsOff)) return false;
    int32_t w = (int32_t)rd32(d + 4), h = (int32_t)rd32(d + 8);
    int bpp = rd16(d + 14);
    uint32_t compression = rd32(d + 16);
    if (compression != 0 || w <= 0 || h == 0 || w > 16384) return false;
    if (bpp != 4 && bpp != 8 && bpp != 24 && bpp != 32) return false;
    bool topDown = h < 0;
    if (h < 0) h = -h;
    if (h > 16384) return false;

    const uint8_t* pal = d + rd32(d);
    uint32_t palCount = bpp <= 8 ? (bitsOff - rd32(d)) / 4 : 0;
    size_t stride = (((size_t)w * bpp + 31) / 32) * 4;
    if (bitsOff + stride * (size_t)h > size) return false;

    out->width = w;
    out->height = h;
    out->bpp = bpp;
    out->pixels.assign((size_t)w * h, 0xFF000000u);
    auto palColor = [&](uint32_t i) -> uint32_t {
        if (i >= palCount) return 0xFF000000u;
        const uint8_t* p = pal + i * 4;  // RGBQUAD: B G R x
        return 0xFF000000u | (uint32_t)p[2] << 16 | (uint32_t)p[1] << 8 | p[0];
    };
    for (int y = 0; y < h; y++) {
        const uint8_t* row = d + bitsOff + stride * (size_t)(topDown ? y : h - 1 - y);
        uint32_t* dst = &out->pixels[(size_t)y * w];
        for (int x = 0; x < w; x++) {
            switch (bpp) {
                case 4: dst[x] = palColor((row[x / 2] >> ((x & 1) ? 0 : 4)) & 15); break;
                case 8: dst[x] = palColor(row[x]); break;
                case 24: {
                    const uint8_t* p = row + x * 3;
                    dst[x] = 0xFF000000u | (uint32_t)p[2] << 16 | (uint32_t)p[1] << 8 | p[0];
                    break;
                }
                case 32: {
                    const uint8_t* p = row + x * 4;
                    dst[x] = 0xFF000000u | (uint32_t)p[2] << 16 | (uint32_t)p[1] << 8 | p[0];
                    break;
                }
            }
        }
    }
    return true;
}

std::vector<uint8_t> DIBToBmpFile(const uint8_t* d, size_t size) {
    std::vector<uint8_t> f;
    uint32_t bitsOff;
    if (!DibLayout(d, size, &bitsOff)) return f;
    uint32_t total = 14 + (uint32_t)size;
    auto put16 = [&](uint32_t v) { f.push_back(v & 0xFF); f.push_back((v >> 8) & 0xFF); };
    auto put32 = [&](uint32_t v) { put16(v & 0xFFFF); put16(v >> 16); };
    f.push_back('B');
    f.push_back('M');
    put32(total);
    put32(0);
    put32(14 + bitsOff);
    f.insert(f.end(), d, d + size);
    return f;
}
