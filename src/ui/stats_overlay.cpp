#include "stats_overlay.h"
#include "../gfx/device.h"
#include <algorithm>

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
// The look the owner chose from the mockups: a light dark wash (about 45%) so the game shows
// through, a faint border, opaque text outlined in black so it reads over bright scenery.
constexpr UIColor kWash{0.04f, 0.04f, 0.16f, 0.45f};
constexpr UIColor kBorder{0.48f, 0.5f, 0.82f, 0.7f};
constexpr UIColor kHeading{1.0f, 0.84f, 0.37f, 1.0f};
constexpr UIColor kLabel{0.87f, 0.88f, 0.95f, 1.0f};
constexpr UIColor kValue{0.66f, 0.9f, 1.0f, 1.0f};
constexpr UIColor kDim{0.72f, 0.73f, 0.83f, 1.0f};

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

void StatsOverlay::Render(Device* d, float top, const std::string& header, const std::string& subheader,
                          const std::vector<StatsGroup>& left, const std::vector<StatsGroup>& right) {
    ui_.Begin(d);
    const bool two = ui_.Right() - ui_.Left() >= kTwoColumnView && !right.empty();
    const float x0 = ui_.Left() + 6.f;
    const float width = two ? 2 * kColumn + kGap : kColumn;
    float body;
    if (two) {
        body = std::max(Height(left), Height(right));
    } else {
        body = Height(left) + Height(right);
    }
    const float height = kPad + 2 * kLine + body + kPad;
    // (kept on screen: with a large HUD size, Full in one column moves up over the HUD's level
    // info rather than run off the bottom)
    top = std::max(4.f, std::min(top, 474.f - height));
    const float x1 = x0 + width, y1 = top + height;

    // (in one batch: the background and border, then every letter, two draws in all)
    ui_.BeginBatch();
    ui_.Rect(x0, top, x1, y1, kWash);
    ui_.Rect(x0, top, x1, top + 1, kBorder);
    ui_.Rect(x0, y1 - 1, x1, y1, kBorder);
    ui_.Rect(x0, top + 1, x0 + 1, y1 - 1, kBorder);
    ui_.Rect(x1 - 1, top + 1, x1, y1 - 1, kBorder);

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
    ui_.EndBatch();
}
