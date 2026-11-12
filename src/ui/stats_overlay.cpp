#include "stats_overlay.h"
#include "../gfx/device.h"
#include <algorithm>
#include <cstdio>

namespace {
// In the 480-high space, in the menu's 8x8 font at one size: a line is 10 high, a column 248
// wide (29 characters inside its margins: a label of up to 13 and a value of up to 15).
constexpr float kLine = 10.f;
constexpr float kPad = 8.f;
constexpr float kColumn = 248.f;
constexpr float kGap = 8.f;            // between two columns
constexpr size_t kLabelChars = 13;
constexpr size_t kValueChars = 15;
constexpr size_t kLineChars = 29;      // a value across the whole line
// The graphs: a heading line over each strip
constexpr float kFrameH = 28.f, kBlendH = 16.f, kTicksH = 6.f;
constexpr float kGraphsH = 4.f + 3 * kLine + kFrameH + 3.f + kBlendH + 3.f + kTicksH;
// The look the owner chose from the mockups: a light dark wash (about 45%) so the game shows
// through, a faint border, opaque text outlined in black so it reads over bright scenery.
constexpr UIColor kWash{0.04f, 0.04f, 0.16f, 0.45f};
constexpr UIColor kBorder{0.48f, 0.5f, 0.82f, 0.7f};
constexpr UIColor kHeading{1.0f, 0.84f, 0.37f, 1.0f};
constexpr UIColor kLabel{0.87f, 0.88f, 0.95f, 1.0f};
constexpr UIColor kValue{0.66f, 0.9f, 1.0f, 1.0f};
constexpr UIColor kDim{0.72f, 0.73f, 0.83f, 1.0f};
constexpr UIColor kTrack{0.f, 0.f, 0.f, 0.3f};          // behind each graph
constexpr UIColor kGood{0.5f, 0.88f, 0.63f, 0.95f};     // a frame near the expected time
constexpr UIColor kSpike{1.0f, 0.48f, 0.42f, 1.0f};     // a frame half again as long: a stutter
constexpr UIColor kLineCol{1.0f, 0.84f, 0.37f, 0.9f};   // the expected frame time
constexpr UIColor kBlend{0.66f, 0.9f, 1.0f, 0.95f};
constexpr UIColor kNoTick{0.23f, 0.25f, 0.44f, 0.95f};
constexpr UIColor kTwoTicks{1.0f, 0.84f, 0.37f, 0.95f};

std::string Fit(const std::string& s, size_t n) { return s.size() <= n ? s : s.substr(0, n - 2) + ".."; }
}  // namespace

float StatsOverlay::Height(const std::vector<StatsGroup>& groups) {
    float h = 0;
    for (const StatsGroup& g : groups) h += kLine * (float)(1 + g.rows.size()) + 4.f;
    return h;
}

void StatsOverlay::OutlinedText(float x, float y, const std::string& s, UIColor c) {
    // a black edge up-left, then the text with the font's own shadow down-right
    ui_.Text(x - 1, y - 1, s, 1, UIColor{0, 0, 0, 0.85f}, false);
    ui_.Text(x, y, s, 1, c, true);
}

void StatsOverlay::Panel(float x0, float y0, float x1, float y1) {
    ui_.Rect(x0, y0, x1, y1, kWash);
    ui_.Rect(x0, y0, x1, y0 + 1, kBorder);
    ui_.Rect(x0, y1 - 1, x1, y1, kBorder);
    ui_.Rect(x0, y0 + 1, x0 + 1, y1 - 1, kBorder);
    ui_.Rect(x1 - 1, y0 + 1, x1, y1 - 1, kBorder);
}

void StatsOverlay::Column(float x, float y, const std::vector<StatsGroup>& groups) {
    const float valueX = x + (float)(kLabelChars + 1) * 8.f;
    for (const StatsGroup& g : groups) {
        y += 4.f;
        OutlinedText(x, y, g.title, kHeading);
        y += kLine;
        for (const StatsGroup::Row& r : g.rows) {
            if (r.label.empty()) {
                OutlinedText(x, y, Fit(r.value, kLineChars), r.dim ? kDim : kValue);
            } else {
                OutlinedText(x, y, Fit(r.label, kLabelChars), kLabel);
                OutlinedText(valueX, y, Fit(r.value, kValueChars), r.dim ? kDim : kValue);
            }
            y += kLine;
        }
    }
}

// Three strips, one column a frame, the newest at the right: the frame time (bars from the
// bottom, red when half again as long as expected, a gold line at the expected time), the blend
// each frame was drawn at (an even sawtooth is even motion) and the ticks each frame ran (dark
// none, green one, gold two or more).
void StatsOverlay::Graphs(float x, float y, float width, const StatsGraphs& g) {
    const int cols = std::max(1, (int)width);
    const int n = (int)std::min<size_t>(g.frameMs.size(), (size_t)cols);
    const size_t first = g.frameMs.size() - (size_t)n;
    const float x0 = x + (float)(cols - n);  // (fewer frames than columns: they fill from the right)
    const size_t chars = (size_t)(width / 8.f);
    char b[64];

    y += 4.f;
    if (g.targetMs > 0) snprintf(b, sizeof b, "FRAME TIME  %.2f ms %s", g.targetMs, g.target.c_str());
    else snprintf(b, sizeof b, "FRAME TIME");
    OutlinedText(x, y, Fit(b, chars), kHeading);
    y += kLine;
    ui_.Rect(x, y, x + width, y + kFrameH, kTrack);
    const float ref = g.targetMs > 0 ? g.targetMs : 0.f;
    for (int i = 0; i < n; i++) {
        float ms = g.frameMs[first + (size_t)i];
        if (ms <= 0) continue;
        float h = std::min(1.f, ms / g.scaleMs) * kFrameH;
        bool spike = ref > 0 && ms > ref * 1.5f;
        ui_.Rect(x0 + (float)i, y + kFrameH - h, x0 + (float)i + 1.f, y + kFrameH, spike ? kSpike : kGood);
    }
    if (ref > 0 && ref < g.scaleMs) {
        float ly = y + kFrameH - ref / g.scaleMs * kFrameH;
        ui_.Rect(x, ly, x + width, ly + 1.f, kLineCol);
    }
    y += kFrameH + 3.f;

    OutlinedText(x, y, g.smooth ? "BLEND" : "BLEND  (Smooth motion off)", g.smooth ? kHeading : kDim);
    y += kLine;
    ui_.Rect(x, y, x + width, y + kBlendH, kTrack);
    for (int i = 0; i < n; i++) {
        float a = std::clamp(g.alpha[first + (size_t)i], 0.f, 1.f);
        float h = std::max(1.f, a * kBlendH);
        ui_.Rect(x0 + (float)i, y + kBlendH - h, x0 + (float)i + 1.f, y + kBlendH, g.smooth ? kBlend : kNoTick);
    }
    y += kBlendH + 3.f;

    OutlinedText(x, y, "TICKS / FRAME", kHeading);
    y += kLine;
    ui_.Rect(x, y, x + width, y + kTicksH, kTrack);
    for (int i = 0; i < n; i++) {
        unsigned t = g.ticks[first + (size_t)i];
        ui_.Rect(x0 + (float)i, y, x0 + (float)i + 1.f, y + kTicksH, t == 0 ? kNoTick : t == 1 ? kGood : kTwoTicks);
    }
}

void StatsOverlay::Render(Device* d, float top, float sideTop, bool full, const std::string& header,
                          const std::string& subheader, const std::vector<StatsGroup>& left,
                          const std::vector<StatsGroup>& rightIn, const std::vector<StatsGroup>& extra,
                          const StatsGraphs& graphs) {
    ui_.Begin(d);
    const bool two = ui_.Right() - ui_.Left() >= kTwoColumnView && !rightIn.empty();
    std::vector<StatsGroup> right = rightIn;
    if (two) right.insert(right.end(), extra.begin(), extra.end());
    // Full in one column (4:3 to about 16:10) has no room left under it: its graphs go beside it
    const bool side = full && !two;
    const float x0 = ui_.Left() + 6.f;
    const float width = two ? 2 * kColumn + kGap : kColumn;
    float body = two ? std::max(Height(left), Height(right)) : Height(left) + Height(right);
    if (!side) body += kGraphsH;
    const float height = kPad + 2 * kLine + body + kPad;
    // (kept on screen: with a large HUD size the panel moves up over the HUD's level info rather
    // than run off the bottom)
    top = std::max(4.f, std::min(top, 474.f - height));
    const float x1 = x0 + width, y1 = top + height;

    // (in one batch: the backgrounds, borders and graph bars, then every letter: two draws in all)
    ui_.BeginBatch();
    Panel(x0, top, x1, y1);
    const float tx = x0 + kPad;
    float y = top + kPad;
    OutlinedText(tx, y, header, kHeading);
    OutlinedText(tx, y + kLine, subheader, kDim);
    y += 2 * kLine;
    if (two) {
        Column(tx, y, left);
        Column(tx + kColumn + kGap, y, right);
    } else {
        Column(tx, y, left);
        Column(tx, y + Height(left), right);
    }
    if (side) {
        const float sx0 = x1 + kGap, sx1 = sx0 + kColumn;
        const float sideH = kPad + Height(extra) + kGraphsH + kPad;
        const float sy0 = std::max(4.f, std::min(sideTop, 474.f - sideH));
        Panel(sx0, sy0, sx1, sy0 + sideH);
        Column(sx0 + kPad, sy0 + kPad - 4.f, extra);
        Graphs(sx0 + kPad, sy0 + kPad - 4.f + Height(extra), kColumn - 2 * kPad, graphs);
    } else {
        Graphs(tx, y + body - kGraphsH, width - 2 * kPad, graphs);
    }
    ui_.EndBatch();
}
