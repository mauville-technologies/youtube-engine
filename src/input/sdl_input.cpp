#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_mouse.h>
#include "sdl_input.h"

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
            case SDL_SCANCODE_F:
                return InputKey::KeyF;
            case SDL_SCANCODE_G:
                return InputKey::KeyG;
            case SDL_SCANCODE_H:
                return InputKey::KeyH;
            case SDL_SCANCODE_I:
                return InputKey::KeyI;
            case SDL_SCANCODE_J:
                return InputKey::KeyJ;
            case SDL_SCANCODE_K:
                return InputKey::KeyK;
            case SDL_SCANCODE_L:
                return InputKey::KeyL;
            case SDL_SCANCODE_M:
                return InputKey::KeyM;
            case SDL_SCANCODE_N:
                return InputKey::KeyN;
            case SDL_SCANCODE_O:
                return InputKey::KeyO;
            case SDL_SCANCODE_P:
                return InputKey::KeyP;
            case SDL_SCANCODE_Q:
                return InputKey::KeyQ;
            case SDL_SCANCODE_R:
                return InputKey::KeyR;
            case SDL_SCANCODE_S:
                return InputKey::KeyS;
            case SDL_SCANCODE_T:
                return InputKey::KeyT;
            case SDL_SCANCODE_U:
                return InputKey::KeyU;
            case SDL_SCANCODE_V:
                return InputKey::KeyV;
            case SDL_SCANCODE_W:
                return InputKey::KeyW;
            case SDL_SCANCODE_X:
                return InputKey::KeyX;
            case SDL_SCANCODE_Y:
                return InputKey::KeyY;
            case SDL_SCANCODE_Z:
                return InputKey::KeyZ;
            case SDL_SCANCODE_LEFTBRACKET:
                return InputKey::KeyBracketL;
            case SDL_SCANCODE_RIGHTBRACKET:
                return InputKey::KeyBracketR;
            case SDL_SCANCODE_BACKSLASH:
                return InputKey::KeyBackslash;
            case SDL_SCANCODE_SEMICOLON:
                return InputKey::KeySemicolon;
            case SDL_SCANCODE_APOSTROPHE:
                return InputKey::KeyApostrophe;
            case SDL_SCANCODE_COMMA:
                return InputKey::KeyComma;
            case SDL_SCANCODE_PERIOD:
                return InputKey::KeyPeriod;
            case SDL_SCANCODE_SLASH:
                return InputKey::KeyForwardSlash;
            case SDL_SCANCODE_GRAVE:
                return InputKey::KeyTilde;
            case SDL_SCANCODE_ESCAPE:
                return InputKey::KeyEscape;
            case SDL_SCANCODE_F1:
                return InputKey::KeyF1;
            case SDL_SCANCODE_F2:
                return InputKey::KeyF2;
            case SDL_SCANCODE_F3:
                return InputKey::KeyF3;
            case SDL_SCANCODE_F4:
                return InputKey::KeyF4;
            case SDL_SCANCODE_F5:
                return InputKey::KeyF5;
            case SDL_SCANCODE_F6:
                return InputKey::KeyF6;
            case SDL_SCANCODE_F7:
                return InputKey::KeyF7;
            case SDL_SCANCODE_F8:
                return InputKey::KeyF8;
            case SDL_SCANCODE_F9:
                return InputKey::KeyF9;
            case SDL_SCANCODE_F10:
                return InputKey::KeyF10;
            case SDL_SCANCODE_F11:
                return InputKey::KeyF11;
            case SDL_SCANCODE_F12:
                return InputKey::KeyF12;
            case SDL_SCANCODE_PRINTSCREEN:
                return InputKey::KeyPrintScreen;
            case SDL_SCANCODE_SCROLLLOCK:
                return InputKey::KeyScrollLock;
            case SDL_SCANCODE_PAUSE:
                return InputKey::KeyPauseBreak;
            case SDL_SCANCODE_INSERT:
                return InputKey::KeyInsert;
            case SDL_SCANCODE_HOME:
                return InputKey::KeyHome;
            case SDL_SCANCODE_PAGEUP:
                return InputKey::KeyPageUp;
            case SDL_SCANCODE_PAGEDOWN:
                return InputKey::KeyPageDown;
            case SDL_SCANCODE_END:
                return InputKey::KeyEnd;
            case SDL_SCANCODE_DELETE:
                return InputKey::KeyDelete;
            case SDL_SCANCODE_BACKSPACE:
                return InputKey::KeyBackspace;
            case SDL_SCANCODE_UP:
                return InputKey::KeyArrowUp;
            case SDL_SCANCODE_LEFT:
                return InputKey::KeyArrowLeft;
            case SDL_SCANCODE_RIGHT:
                return InputKey::KeyArrowRight;
            case SDL_SCANCODE_DOWN:
                return InputKey::KeyArrowDown;
            case SDL_SCANCODE_TAB:
                return InputKey::KeyTab;
            case SDL_SCANCODE_CAPSLOCK:
                return InputKey::KeyCapsLock;
            case SDL_SCANCODE_LSHIFT:
                return InputKey::KeyShiftLeft;
            case SDL_SCANCODE_RSHIFT:
                return InputKey::KeyShiftRight;
            case SDL_SCANCODE_RETURN:
                return InputKey::KeyEnter;
            case SDL_SCANCODE_LCTRL:
                return InputKey::KeyCtrlLeft;
            case SDL_SCANCODE_RCTRL:
                return InputKey::KeyCtrlRight;
            case SDL_SCANCODE_LALT:
                return InputKey::KeyAltLeft;
            case SDL_SCANCODE_RALT:
                return InputKey::KeyAltRight;
            case SDL_SCANCODE_NUMLOCKCLEAR:
                return InputKey::KeyNumlock;
            case SDL_SCANCODE_KP_DIVIDE:
                return InputKey::KeyNumpadDivide;
            case SDL_SCANCODE_KP_MULTIPLY:
                return InputKey::KeyNumpadMultiply;
            case SDL_SCANCODE_KP_MINUS:
                return InputKey::KeyNumpadMinus;
            case SDL_SCANCODE_KP_PLUS:
                return InputKey::KeyNumpadPlus;
            case SDL_SCANCODE_KP_ENTER:
                return InputKey::KeyNumpadReturn;
            case SDL_SCANCODE_KP_PERIOD:
                return InputKey::KeyNumpadPeriod;
            case SDL_SCANCODE_KP_1:
                return InputKey::KeyNumpad1;
            case SDL_SCANCODE_KP_2:
                return InputKey::KeyNumpad2;
            case SDL_SCANCODE_KP_3:
                return InputKey::KeyNumpad3;
            case SDL_SCANCODE_KP_4:
                return InputKey::KeyNumpad4;
            case SDL_SCANCODE_KP_5:
                return InputKey::KeyNumpad5;
            case SDL_SCANCODE_KP_6:
                return InputKey::KeyNumpad6;
            case SDL_SCANCODE_KP_7:
                return InputKey::KeyNumpad7;
            case SDL_SCANCODE_KP_8:
                return InputKey::KeyNumpad8;
            case SDL_SCANCODE_KP_9:
                return InputKey::KeyNumpad9;
            case SDL_SCANCODE_KP_0:
                return InputKey::KeyNumpad0;
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

            float value;

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