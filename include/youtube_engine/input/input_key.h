//
// Created by ozzadar on 2021-08-12.
//

#pragma once

namespace OZZ {
    enum class InputKey {
        UNKNOWN,
        KEY_A,
        KEY_B,
        KEY_C,
        KEY_D,
        KEY_E,
        KEY_S,
        KEY_W,
        GAMEPAD_L_THUMB_X,
        GAMEPAD_L_THUMB_Y,
        GAMEPAD_R_THUMB_X,
        GAMEPAD_R_THUMB_Y,
        GAMEPAD_R_TRIGGER,
        GAMEPAD_L_TRIGGER,
        GAMEPAD_Y,
        GAMEPAD_X,
        GAMEPAD_B,
        GAMEPAD_A,
        GAMEPAD_START,
        GAMEPAD_SELECT,
        GAMEPAD_BUMPER_R,
        GAMEPAD_BUMPER_L,
        GAMEPAD_L3,
        GAMEPAD_R3,
        GAMEPAD_DPAD_UP,
        GAMEPAD_DPAD_RIGHT,
        GAMEPAD_DPAD_LEFT,
        GAMEPAD_DPAD_DOWN,
        MOUSE_POS_X,
        MOUSE_POS_Y,
        MOUSE_MOVE_X,
        MOUSE_MOVE_Y,
        MOUSE_RIGHT,
        MOUSE_LEFT,
        MOUSE_MIDDLE
    };

    enum class InputSource {
        KEYBOARD,
        MOUSE,
        GAMEPAD,
        UNKNOWN
    };

    struct InputAction {
        std::string actionName { "" };
        float scale { 1.f };
    };

    InputSource GetInputSourceFromKey(InputKey key);
}