// The original's frames-per-second display (XFPSDisplay): an "FPS" label and a three-digit
// number in the bottom right, drawn by XApplication::Render after the running screen when the
// setting is on (RegInfo_GetDisplayFPS in the original; [video] show_fps here).
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include "game/JSprite.h"

class BackBuffer;

class XFPSDisplay {
public:
    // XFPSDisplay::XFPSDisplay: the label (bitmap 0xbf) and the digits (0xbb, ten frames), shown
    // with leading zeros, three of them
    XFPSDisplay();
    // XFPSDisplay::Render: the label at (552, 454), the number 4 pixels after it
    void Render(BackBuffer* bb, int fps);

private:
    JSprite label_;   // +0x08
    JNumber number_;  // +0x0c
};
