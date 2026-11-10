#include "ui_draw.h"
#include "../gfx/device.h"
#include "../../third_party/font8x8_basic.h"
#include <algorithm>
#include <cstring>

namespace {
constexpr int kPad = 2;             // transparent border around each glyph
constexpr int kCell = 8 + 2 * kPad;  // atlas cell size
}  // namespace

UIDraw::~UIDraw() { delete font_; }

void UIDraw::EnsureFont() {
    if (font_) return;
    // 16 x 8 glyph atlas. Each 8x8 glyph sits in a 12x12 cell with a 2-pixel transparent border:
    // with antialiasing, pixels on a glyph's edge can be shaded from just outside its quad, and
    // without the border they would pick up the neighbouring glyph (a thin light seam).
    // White glyph pixels, everything else black (colour-keyed out).
    Image img;
    img.width = 16 * kCell;
    img.height = 8 * kCell;
    img.bpp = 32;
    img.pixels.assign((size_t)img.width * img.height, 0xFF000000u);
    for (int ch = 0; ch < 128; ch++) {
        int gx = (ch % 16) * kCell + kPad, gy = (ch / 16) * kCell + kPad;
        for (int row = 0; row < 8; row++) {
            unsigned bits = (unsigned char)font8x8_basic[ch][row];
            for (int col = 0; col < 8; col++)
                if (bits & (1u << col)) img.pixels[(size_t)(gy + row) * img.width + gx + col] = 0xFFFFFFFFu;
        }
    }
    font_ = Texture::Create(img, true, 0x000000, false);
}

void UIDraw::Begin(Device* d) {
    dev_ = d;
    EnsureFont();
    const FrameLayout& L = d->Layout();
    left_ = -L.offsetX;
    right_ = 640.f + L.offsetX;
}

void UIDraw::Quad(std::vector<D3DLVERTEX>& to, float x0, float y0, float x1, float y1, float u0, float v0, float u1,
                  float v1, UIColor c) {
    auto b = [](float f) { return (uint32_t)(std::min(1.f, std::max(0.f, f)) * 255.f + 0.5f); };
    uint32_t col = b(c.a) << 24 | b(c.r) << 16 | b(c.g) << 8 | b(c.b);
    D3DLVERTEX v[4];
    memset(v, 0, sizeof v);
    v[0].x = x0; v[0].y = y0; v[0].tu = u0; v[0].tv = v0;
    v[1].x = x1; v[1].y = y0; v[1].tu = u1; v[1].tv = v0;
    v[2].x = x0; v[2].y = y1; v[2].tu = u0; v[2].tv = v1;
    v[3].x = x1; v[3].y = y1; v[3].tu = u1; v[3].tv = v1;
    for (auto& q : v) q.color = col;
    // two triangles: 0 1 2 and 2 1 3 (the strip DrawUIQuad draws)
    for (int i : {0, 1, 2, 2, 1, 3}) to.push_back(v[i]);
}

void UIDraw::BeginBatch() {
    batching_ = true;
    rects_.clear();
    glyphs_.clear();
}

void UIDraw::EndBatch() {
    batching_ = false;
    if (!dev_) return;
    dev_->DrawUIQuads(rects_, nullptr);
    dev_->DrawUIQuads(glyphs_, font_);
}

void UIDraw::Rect(float x0, float y0, float x1, float y1, UIColor c) {
    if (batching_) {
        Quad(rects_, x0, y0, x1, y1, 0, 0, 0, 0, c);
        return;
    }
    float rgba[4] = {c.r, c.g, c.b, c.a};
    dev_->DrawUIQuad(x0, y0, x1, y1, 0, 0, 0, 0, nullptr, rgba);
}

void UIDraw::Text(float x, float y, const std::string& s, float scale, UIColor c, bool shadow) {
    if (shadow) Text(x + scale, y + scale, s, scale, UIColor{0, 0, 0, c.a * 0.7f}, false);
    float rgba[4] = {c.r, c.g, c.b, c.a};
    float g = 8.f * scale;
    for (unsigned char ch : s) {
        if (ch > 32 && ch < 128) {
            const float aw = 16.f * kCell, ah = 8.f * kCell;
            float u0 = ((ch % 16) * kCell + kPad) / aw, v0 = ((ch / 16) * kCell + kPad) / ah;
            if (batching_) Quad(glyphs_, x, y, x + g, y + g, u0, v0, u0 + 8 / aw, v0 + 8 / ah, c);
            else dev_->DrawUIQuad(x, y, x + g, y + g, u0, v0, u0 + 8 / aw, v0 + 8 / ah, font_, rgba);
        }
        x += g;
    }
}
