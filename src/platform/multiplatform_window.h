#pragma once
#define GLFW_INCLUDE_VULKAN

#include <youtube_engine/platform/window.h>
#include <GLFW/glfw3.h>
#include <input/multiplatform_input.h>

namespace OZZ {
    class MultiPlatformWindow : public Window {
    public:
        MultiPlatformWindow() = default;

        void OpenWindow(WindowData data) override;
        bool Update() override;

        std::pair<int, int> GetWindowExtents() override;
        void RequestDrawSurface(std::unordered_map<SurfaceArgs, std::any> args) override;

    private:
        std::unordered_map<InputKey, InputDeviceState> getGamepadState(int joystickId);
        MultiplatformInput _input {};
        GLFWwindow *_window = nullptr;
    };
}