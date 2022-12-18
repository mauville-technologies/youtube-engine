//
// Created by ozzadar on 2021-08-12.
//

#pragma once
#include <unordered_map>
#include <youtube_engine/input/input_key.h>
#include <youtube_engine/input/input_devices.h>
#include <GLFW/glfw3.h>

namespace OZZ {
    class MultiplatformInput {
    public:

        std::unordered_map<InputKey, InputDeviceState> GetKeyboardState(int index) { return _keyboardState; }
        std::unordered_map<InputKey, InputDeviceState> GetMouseState(int index) { return _mouseState; }
        std::unordered_map<InputKey, InputDeviceState> GetGamepadState(const GLFWgamepadstate& state);

        void UpdateKeyboardState(GLFWwindow* window);
        void UpdateMouseState(GLFWwindow* window);

    private:
        static InputKey multiplatformKeyToInputKey(int key);
        static InputKey multiplatformMouseButtonToInputKey(int button);
    private:
        std::unordered_map<InputKey, InputDeviceState> _keyboardState {};
        std::unordered_map<InputKey, InputDeviceState> _mouseState {};

    };
}


