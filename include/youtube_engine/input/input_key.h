//
// Created by ozzadar on 2021-08-12.
//

#pragma once
#include <string>

namespace OZZ {
    enum class InputKey {
        Unknown,

        KeyA,
        KeyB,
        KeyC,
        KeyD,
        KeyE,
        KeyQ,
        KeyS,
        KeyW,
        KeyMax,

        ControllerButtonA,
        ControllerButtonB,
        ControllerButtonX,
        ControllerButtonY,
        ControllerButtonSelect,
        ControllerButtonGuide,
        ControllerButtonStart,
        ControllerButtonLeftStick,
        ControllerButtonRightStick,
        ControllerButtonLeftShoulder,
        ControllerButtonRightShoulder,
        ControllerButtonDpadUp,
        ControllerButtonDpadDown,
        ControllerButtonDpadLeft,
        ControllerButtonDpadRight,
        ControllerButtonMisc1,
        ControllerButtonPaddle1,
        ControllerButtonPaddle2,
        ControllerButtonPaddle3,
        ControllerButtonPaddle4,
        ControllerButtonTouchPad,
        ControllerAxisLeftX,
        ControllerAxisLeftY,
        ControllerAxisRightX,
        ControllerAxisRightY,
        ControllerAxisTriggerLeft,
        ControllerAxisTriggerRight,
        ControllerMax,

        MouseX,
        MouseY,
        MouseMoveX,
        MouseMoveY,
        MouseButtonRight,
        MouseButtonLeft,
        MouseButtonMiddle,
        MouseMax
    };

    enum class InputSource {
        Unknown,
        Keyboard,
        Controller,
        Mouse,
    };

    struct InputAction {
        std::string ActionName;
        float Scale { 1.f };
    };

    InputSource GetInputSourceFromKey(InputKey key);
}