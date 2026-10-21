// Decoding of the bitmap resources (Windows DIBs: 4-, 8- and 24-bit, bottom-up or top-down).
#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>

struct Image {
    int width = 0, height = 0;
    int bpp = 0;                    // bit depth of the source DIB
    std::vector<uint32_t> pixels;   // top-down, 0xAARRGGBB, alpha = 0xFF
    // Native (mods): a PNG with real transparency uses its alpha instead of a colour key, and a
    // replacement image is laid out at the original's size (so an HD image just looks sharper).
    bool hasAlpha = false;
    int logicalWidth = 0, logicalHeight = 0;  // 0 = width / height
    int LogicalWidth() const { return logicalWidth ? logicalWidth : width; }
    int LogicalHeight() const { return logicalHeight ? logicalHeight : height; }
};

bool DecodeDIB(const uint8_t* data, size_t size, Image* out);

// A .bmp file (BITMAPFILEHEADER + the DIB) for the resource, unchanged; used by asset export.
std::vector<uint8_t> DIBToBmpFile(const uint8_t* data, size_t size);
