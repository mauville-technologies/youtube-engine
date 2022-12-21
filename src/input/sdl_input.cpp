#include <SDL_scancode.h>
#include <SDL_mouse.h>
#include "sdl_input.h"
#include <iostream>

namespace OZZ {

    void SDLInput::UpdateKeyboardState(const Uint8* keyboardState) {
        for (int i = 0; i < SDL_NUM_SCANCODES; i++) {
            InputKey key = sdlKeyToInputKey(i);
            // skip unsupported keys
            if (key == InputKey::Unknown) continue;

            auto value = static_cast<float>(keyboardState[static_cast<Uint8>(i)]);
            _keyboardState[key].value = value;
        }
    }

    InputKey SDLInput::sdlKeyToInputKey(Uint8 key) {
        switch(key) {
            case SDL_SCANCODE_A:
                return InputKey::KeyA;
            case SDL_SCANCODE_B:
                return InputKey::KeyB;
            case SDL_SCANCODE_C:
                return InputKey::KeyC;
            case SDL_SCANCODE_D:
                return InputKey::KeyD;
            case SDL_SCANCODE_E:
                return InputKey::KeyE;
            case SDL_SCANCODE_Q:
                return InputKey::KeyQ;
            case SDL_SCANCODE_S:
                return InputKey::KeyS;
            case SDL_SCANCODE_W:
                return InputKey::KeyW;
            default:
                return InputKey::Unknown;
        }
    }

    void SDLInput::UpdateMouseState(Uint32 buttons, int x, int y) {
        _mouseState[InputKey::MouseButtonLeft].value = static_cast<float>((buttons & SDL_BUTTON_LMASK) != 0);
        _mouseState[InputKey::MouseButtonRight].value = static_cast<float>((buttons & SDL_BUTTON_RMASK) != 0);
        _mouseState[InputKey::MouseButtonMiddle].value = static_cast<float>((buttons & SDL_BUTTON_MMASK) != 0);

        float lastX = _mouseState[InputKey::MouseX].value;
        float lastY = _mouseState[InputKey::MouseY].value;

        _mouseState[InputKey::MouseMoveX].value = static_cast<float>(x) - lastX;
        _mouseState[InputKey::MouseMoveY].value = static_cast<float>(y) - lastY;
        _mouseState[InputKey::MouseX].value = static_cast<float>(x);
        _mouseState[InputKey::MouseY].value = static_cast<float>(y);

    }

    void SDLInput::AddController(SDL_GameController *newJoystick) {
        auto id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(newJoystick));
        _connectedControllers[id] = newJoystick;
    }

    void SDLInput::RemoveController(SDL_JoystickID id) {
        _connectedControllers.erase(id);
    }

    std::unordered_map<InputKey, InputDeviceState> SDLInput::GetGamepadState(int index) {
        std::unordered_map<InputKey, InputDeviceState> state {};

        auto joystickId = static_cast<SDL_JoystickID>(index);

        if (_connectedControllers.find(joystickId) == _connectedControllers.end()) return {};

        auto* controller = _connectedControllers[joystickId];

        // Get Buttons
        for (Uint8 i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++) {
            auto pressed = SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(i));

            auto inputKey = sdlControllerButtonToInputKey(static_cast<SDL_GameControllerButton>(i));

            state[inputKey] = InputDeviceState {
                .value = static_cast<float>(pressed)
            };
        }

        // Get Axes
        for (Uint8 i = 0; i < SDL_CONTROLLER_AXIS_MAX; i++) {
            auto pressed = SDL_GameControllerGetAxis(controller, static_cast<SDL_GameControllerAxis>(i));

            float value = 0.f;

            if (pressed > 0) {
                value = static_cast<float>(pressed) / 32767.0f;
            } else {
                value = static_cast<float>(pressed) / 32768.0f;
            }
            auto inputKey = sdlControllerAxisToInputKey(static_cast<SDL_GameControllerAxis>(i));

            state[inputKey] = InputDeviceState {
                    .value = value
            };
        }

        return state;
    }

    InputKey SDLInput::sdlControllerButtonToInputKey(SDL_GameControllerButton button) {
        switch (button) {
            case SDL_CONTROLLER_BUTTON_A:
                return InputKey::ControllerButtonA;
            case SDL_CONTROLLER_BUTTON_B:
                return InputKey::ControllerButtonB;
            case SDL_CONTROLLER_BUTTON_X:
                return InputKey::ControllerButtonX;
            case SDL_CONTROLLER_BUTTON_Y:
                return InputKey::ControllerButtonY;
            case SDL_CONTROLLER_BUTTON_BACK:
                return InputKey::ControllerButtonSelect;
            case SDL_CONTROLLER_BUTTON_GUIDE:
                return InputKey::ControllerButtonGuide;
            case SDL_CONTROLLER_BUTTON_START:
                return InputKey::ControllerButtonStart;
            case SDL_CONTROLLER_BUTTON_LEFTSTICK:
                return InputKey::ControllerButtonLeftStick;
            case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
                return InputKey::ControllerButtonRightStick;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                return InputKey::ControllerButtonLeftShoulder;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                return InputKey::ControllerButtonRightShoulder;
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                return InputKey::ControllerButtonDpadUp;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                return InputKey::ControllerButtonDpadRight;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                return InputKey::ControllerButtonDpadLeft;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                return InputKey::ControllerButtonDpadRight;
            case SDL_CONTROLLER_BUTTON_MISC1:
                return InputKey::ControllerButtonMisc1;
            case SDL_CONTROLLER_BUTTON_PADDLE1:
                return InputKey::ControllerButtonPaddle1;
            case SDL_CONTROLLER_BUTTON_PADDLE2:
                return InputKey::ControllerButtonPaddle2;
            case SDL_CONTROLLER_BUTTON_PADDLE3:
                return InputKey::ControllerButtonPaddle3;
            case SDL_CONTROLLER_BUTTON_PADDLE4:
                return InputKey::ControllerButtonPaddle4;
            case SDL_CONTROLLER_BUTTON_TOUCHPAD:
                return InputKey::ControllerButtonTouchPad;
            default:
                return InputKey::Unknown;
        }
    }

    InputKey SDLInput::sdlControllerAxisToInputKey(SDL_GameControllerAxis axis) {
        switch (axis) {
            case SDL_CONTROLLER_AXIS_LEFTX:
                return InputKey::ControllerAxisLeftX;
            case SDL_CONTROLLER_AXIS_LEFTY:
                return InputKey::ControllerAxisLeftY;
            case SDL_CONTROLLER_AXIS_RIGHTX:
                return InputKey::ControllerAxisRightX;
            case SDL_CONTROLLER_AXIS_RIGHTY:
                return InputKey::ControllerAxisRightY;
            case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                return InputKey::ControllerAxisTriggerLeft;
            case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                return InputKey::ControllerAxisTriggerRight;
            default:
                return InputKey::Unknown;
        }
    }
}