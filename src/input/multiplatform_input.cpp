//
// Created by ozzadar on 2021-08-12.
//

#include "multiplatform_input.h"

namespace OZZ {

    void MultiplatformInput::UpdateKeyboardState(GLFWwindow* window) {
        for (int key = GLFW_KEY_UNKNOWN; key < GLFW_KEY_LAST; key++) {
            InputKey iKey = multiplatformKeyToInputKey(key);
            if (iKey == InputKey::Unknown) continue;

            auto state = glfwGetKey(window, key);

            _keyboardState[iKey].value = static_cast<float>(state);
        }
    }

    void MultiplatformInput::UpdateMouseState(GLFWwindow* window) {

        // Set buttons
        for (int button = GLFW_MOUSE_BUTTON_1; button < GLFW_MOUSE_BUTTON_LAST; button++) {
            InputKey iKey = multiplatformMouseButtonToInputKey(button);
            if (iKey == InputKey::Unknown) continue;

            auto state = glfwGetMouseButton(window, button);
            _mouseState[iKey].value = static_cast<float>(state);
        }

        // Set movement
        float lastX = _mouseState[InputKey::MouseX].value;
        float lastY = _mouseState[InputKey::MouseY].value;

        double x, y;
        glfwGetCursorPos(window, &x, &y);

        _mouseState[InputKey::MouseMoveX].value = static_cast<float>(x) - lastX;
        _mouseState[InputKey::MouseMoveY].value = static_cast<float>(y) - lastY;
        _mouseState[InputKey::MouseX].value = static_cast<float>(x);
        _mouseState[InputKey::MouseY].value = static_cast<float>(y);
    }

    InputKey MultiplatformInput::multiplatformKeyToInputKey(int key) {
        switch (key) {
            case GLFW_KEY_A:
                return InputKey::KeyA;
            case GLFW_KEY_B:
                return InputKey::KeyB;
            case GLFW_KEY_C:
                return InputKey::KeyC;
            case GLFW_KEY_D:
                return InputKey::KeyD;
            case GLFW_KEY_E:
                return InputKey::KeyE;
            case GLFW_KEY_S:
                return InputKey::KeyS;
            case GLFW_KEY_W:
                return InputKey::KeyW;
            default:
                return InputKey::Unknown;
        }
    }

    InputKey MultiplatformInput::multiplatformMouseButtonToInputKey(int button) {
        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                return InputKey::MouseButtonLeft;
            case GLFW_MOUSE_BUTTON_RIGHT:
                return InputKey::MouseButtonRight;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                return InputKey::MouseButtonMiddle;
            default:
                return InputKey::Unknown;
        }
    }

    std::unordered_map<InputKey, InputDeviceState> MultiplatformInput::GetGamepadState(const GLFWgamepadstate &state) {
        std::unordered_map<InputKey, InputDeviceState> gamepadState {};

        // Get the buttons
        for (int i = 0; i <= GLFW_GAMEPAD_BUTTON_LAST; i++) {
            int buttonState = state.buttons[i];
            float value = buttonState == GLFW_PRESS ? 1.f : 0.f;

            switch (i) {
                case GLFW_GAMEPAD_BUTTON_B:
                    gamepadState[InputKey::ControllerButtonB].value = value;
                    break;
                case GLFW_GAMEPAD_BUTTON_A:
                    gamepadState[InputKey::ControllerButtonA].value = value;
                    break;
                case GLFW_GAMEPAD_BUTTON_X:
                    gamepadState[InputKey::ControllerButtonX].value = value;
                    break;
                case GLFW_GAMEPAD_BUTTON_Y:
                    gamepadState[InputKey::ControllerButtonY].value = value;
                    break;
                case GLFW_GAMEPAD_BUTTON_LEFT_BUMPER:
                    gamepadState[InputKey::ControllerButtonLeftShoulder].value = value;
                    break;
                case GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER:
                    gamepadState[InputKey::ControllerButtonRightShoulder].value = value;
                    break;
                case GLFW_GAMEPAD_BUTTON_BACK:
                    gamepadState[InputKey::ControllerButtonSelect].value = value;
                    break;
                case GLFW_GAMEPAD_BUTTON_START:
                    gamepadState[InputKey::ControllerButtonStart].value = value;
                    break;
                case GLFW_GAMEPAD_BUTTON_LEFT_THUMB:
                    gamepadState[InputKey::ControllerButtonLeftStick].value = value;
                    break;
                case GLFW_GAMEPAD_BUTTON_RIGHT_THUMB:
                    gamepadState[InputKey::ControllerButtonRightStick].value = value;
                    break;
                case GLFW_GAMEPAD_BUTTON_DPAD_UP:
                    gamepadState[InputKey::ControllerButtonDpadUp].value = value;
                    break;
                case GLFW_GAMEPAD_BUTTON_DPAD_RIGHT:
                    gamepadState[InputKey::ControllerButtonDpadRight].value = value;
                    break;
                case GLFW_GAMEPAD_BUTTON_DPAD_DOWN:
                    gamepadState[InputKey::ControllerButtonDpadDown].value = value;
                    break;
                case GLFW_GAMEPAD_BUTTON_DPAD_LEFT:
                    gamepadState[InputKey::ControllerButtonDpadLeft].value = value;
                    break;
                case GLFW_GAMEPAD_BUTTON_GUIDE:
                default:
                    break;
            }
        }

        // get the axes
        for (int i = 0; i <= GLFW_GAMEPAD_AXIS_LAST; i++) {
            float value = state.axes[i];

            switch (i) {
                case GLFW_GAMEPAD_AXIS_LEFT_X:
                    gamepadState[InputKey::ControllerAxisLeftX].value = value;
                    break;
                case GLFW_GAMEPAD_AXIS_LEFT_Y:
                    gamepadState[InputKey::ControllerAxisLeftY].value = -value;
                    break;
                case GLFW_GAMEPAD_AXIS_RIGHT_X:
                    gamepadState[InputKey::ControllerAxisRightX].value = value;
                    break;
                case GLFW_GAMEPAD_AXIS_RIGHT_Y:
                    gamepadState[InputKey::ControllerAxisRightY].value = -value;
                    break;
                case GLFW_GAMEPAD_AXIS_LEFT_TRIGGER:
                    gamepadState[InputKey::ControllerAxisTriggerLeft].value = value;
                    break;
                case GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER:
                    gamepadState[InputKey::ControllerAxisTriggerRight].value = value;
                    break;
                default:
                    break;
            }
        }

        return gamepadState;
    }

}