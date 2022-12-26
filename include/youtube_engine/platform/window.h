#pragma once

#include <string>
#include <unordered_map>
#include <any>
#include <tuple>
#include <functional>

namespace OZZ {
    enum class WindowType {
        SDL,
        GLFW
    };

    enum class SurfaceArgs {
        INSTANCE,
        ALLOCATORS,
        OUT_SURFACE
    };

    struct WindowData {
        std::string title;
        uint32_t width, height;
    };

    class Window {
    public:
        virtual void OpenWindow(WindowData data) = 0;
        virtual bool Update() = 0;
        virtual ~Window() = default;

        virtual std::pair<int, int> GetWindowExtents() = 0;
        virtual float GetAspectRatio() {
            auto [width, height] = GetWindowExtents();
            return static_cast<float>(width) / static_cast<float>(height);
        }

        virtual void RequestDrawSurface(std::unordered_map<SurfaceArgs, int*>) = 0;
        virtual void RegisterWindowResizedCallback(std::function<void()>) = 0;
    };
}