#include "JDInput.h"
#include "input/input.h"
#include <algorithm>

namespace {
// DirectInput codes not named in input.h
enum { DIK_RETURN_ = 0x1C, DIK_NUMPADENTER_ = 0x9C };
}  // namespace

void JDInput::UpdateInputState() {
    std::copy(keys_, keys_ + 256, prevKeys_);
    Input::GetKeyboardState(keys_);
    for (int i = 0; i < kPads; i++) {
        prevPads_[i] = pads_[i];
        Pad& p = pads_[i];
        DIJOYSTATE js{};
        p.present = Input::NumJoysticks() > i && Input::GetJoystickState(i, &js);
        if (!p.present) {
            p = Pad();
            continue;
        }
        p.button[0] = (js.rgbButtons[0] & 0x80) != 0;
        p.button[1] = (js.rgbButtons[1] & 0x80) != 0;
        // JDInput::GetJoyDirState: beyond 800 either way on each axis
        p.dir[0] = js.lY < -800;
        p.dir[1] = js.lX > 800;
        p.dir[2] = js.lY > 800;
        p.dir[3] = js.lX < -800;
    }
}

bool JDInput::GetJoyButtonPressed(int pad, int button) const {
    if (pad < 0 || pad >= kPads || !pads_[pad].present) return false;
    return pads_[pad].button[button] && !prevPads_[pad].button[button];
}

bool JDInput::GetJoyDirPressed(int pad, int dir) const {
    if (pad < 0 || pad >= kPads || !pads_[pad].present) return false;
    return pads_[pad].dir[dir] && !prevPads_[pad].dir[dir];
}

bool CheckUpPressed(const JDInput& in) {
    return in.GetKeyboardPressed(DIK_UP) || in.GetKeyboardPressed(DIK_W) || in.GetJoyDirPressed(0, 0) ||
           in.GetJoyDirPressed(1, 0);
}

bool CheckDownPressed(const JDInput& in) {
    return in.GetKeyboardPressed(DIK_DOWN) || in.GetKeyboardPressed(DIK_S) || in.GetJoyDirPressed(0, 2) ||
           in.GetJoyDirPressed(1, 2);
}

bool CheckLeftPressed(const JDInput& in) {
    return in.GetKeyboardPressed(DIK_LEFT) || in.GetKeyboardPressed(DIK_A) || in.GetJoyDirPressed(0, 3) ||
           in.GetJoyDirPressed(1, 3);
}

bool CheckRightPressed(const JDInput& in) {
    return in.GetKeyboardPressed(DIK_RIGHT) || in.GetKeyboardPressed(DIK_D) || in.GetJoyDirPressed(0, 1) ||
           in.GetJoyDirPressed(1, 1);
}

bool CheckButtonPressed(const JDInput& in) {
    return in.GetKeyboardPressed(DIK_NUMPADENTER_) || in.GetKeyboardPressed(DIK_RETURN_) ||
           in.GetKeyboardPressed(DIK_LSHIFT) || in.GetKeyboardPressed(DIK_RSHIFT) ||
           in.GetKeyboardPressed(DIK_LCONTROL) || in.GetKeyboardPressed(DIK_RCONTROL) ||
           in.GetJoyButtonPressed(0, 0) || in.GetJoyButtonPressed(0, 1) || in.GetJoyButtonPressed(1, 0) ||
           in.GetJoyButtonPressed(1, 1);
}
