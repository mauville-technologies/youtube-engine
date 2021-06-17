#pragma once
#define GLFW_INCLUDE_VULKAN

#include <youtube_engine/platform/window.h>
#include <GLFW/glfw3.h>

namespace OZZ {
    class MultiPlatformWindow : public Window {
    public:
        MultiPlatformWindow();

        void OpenWindow(WindowData data) override;
        bool Update() override;

        std::pair<int, int> GetWindowExtents() override;
        void RequestDrawSurface(std::unordered_map<SurfaceArgs, std::any> args) override;

    private:
        GLFWwindow *_window;
    };
}