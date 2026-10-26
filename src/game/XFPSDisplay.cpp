#include "XFPSDisplay.h"
#include "core/config.h"
#include "gfx/device.h"

namespace {
constexpr uint16_t kBitmapLabel = 0xbf, kBitmapDigits = 0xbb;
}  // namespace

XFPSDisplay::XFPSDisplay() : label_(kBitmapLabel, 1), number_(kBitmapDigits) {
    number_.SetDisplayZero(true);
    number_.SetNumDigits(3);
}

void XFPSDisplay::Render(BackBuffer* bb, int fps) {
    // native: on every screen the HUD settings move it out to the bottom-right corner (HUD position
    // Edges, in widescreen) and resize it from that corner (HUD size); the original's at 100%
    Device& d = GetDevice();
    d.BeginHudScale(GetConfig().hudScale, 640.0f, 480.0f, (float)d.HudEdgeShift());
    int x = 552, y = 454;
    label_.SetPosition(x, y);
    label_.Draw(bb);
    x += label_.GetWidth() + 4;
    number_.SetPosition(x, y);
    number_.SetNumber(fps);  // CD3DApplication::GetFPS
    number_.Draw(bb);
    d.EndHudScale();
}
