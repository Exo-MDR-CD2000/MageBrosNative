// JDInput (the J library's DirectInput wrapper) as the menus use it: the keyboard and two
// gamepads read once a frame (UpdateInputState), with "pressed" meaning held now and not at the
// last update. Also the menu helpers CheckUpPressed, CheckDownPressed, CheckLeftPressed,
// CheckRightPressed and CheckButtonPressed (XApplication.cpp of the original). The game reads
// its heroes' input itself, each tick.
//
// Part of the reconstructed game code (src/game, LICENSE part 2).
#pragma once
#include <cstdint>

class JDInput {
public:
    void UpdateInputState();
    bool GetKeyboardState(int dik) const { return (keys_[dik] & 0x80) != 0; }
    bool GetKeyboardPressed(int dik) const { return (keys_[dik] & 0x80) && !(prevKeys_[dik] & 0x80); }
    bool GetJoyButtonPressed(int pad, int button) const;
    // GetJoyDirPressed: 0 up, 1 right, 2 down, 3 left (the stick past 800 of 1000)
    bool GetJoyDirPressed(int pad, int dir) const;

private:
    static constexpr int kPads = 2;
    struct Pad {
        bool present = false;
        bool button[2] = {};
        bool dir[4] = {};
    };
    uint8_t keys_[256] = {}, prevKeys_[256] = {};
    Pad pads_[kPads], prevPads_[kPads];
};

// Up arrow or W, or either gamepad pushed up (unless the joystick is disabled: never, here)
bool CheckUpPressed(const JDInput& in);
// Down arrow or S, or either gamepad pushed down
bool CheckDownPressed(const JDInput& in);
// Left arrow or A, or either gamepad pushed left
bool CheckLeftPressed(const JDInput& in);
// Right arrow or D, or either gamepad pushed right
bool CheckRightPressed(const JDInput& in);
// Enter (main or keypad), either Shift or either Ctrl, or button 1 or 2 of either gamepad
bool CheckButtonPressed(const JDInput& in);
