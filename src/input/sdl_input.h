#pragma once
#include <unordered_map>
#include <youtube_engine/input/input_key.h>
#include <youtube_engine/input/input_devices.h>
#include <SDL_types.h>
#include <SDL_gamecontroller.h>

namespace OZZ {
    class SDLInput {
    public:
        std::unordered_map<InputKey, InputDeviceState> GetKeyboardState(int) { return _keyboardState; }
        std::unordered_map<InputKey, InputDeviceState> GetMouseState(int) { return _mouseState; }
        std::unordered_map<InputKey, InputDeviceState> GetGamepadState(int index);

        void UpdateKeyboardState(const Uint8* keyboardState);
        void UpdateMouseState(Uint32 buttons, int x, int y);

        void AddController(SDL_GameController* newJoystick);
        void RemoveController(SDL_JoystickID id);

    private:
        static InputKey sdlKeyToInputKey(Uint8 key);
        static InputKey sdlControllerButtonToInputKey(SDL_GameControllerButton button);
        static InputKey sdlControllerAxisToInputKey(SDL_GameControllerAxis axis);
    private:
        std::unordered_map<InputKey, InputDeviceState> _keyboardState {};
        std::unordered_map<InputKey, InputDeviceState> _mouseState {};

        std::unordered_map<SDL_JoystickID, SDL_GameController*> _connectedControllers {};
    };
}


