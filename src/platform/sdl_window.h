//
// Created by ozzadar on 2021-09-11.
//

#pragma once
#include <youtube_engine/platform/window.h>
#include <SDL3/SDL.h>
#include <input/sdl_input.h>

namespace OZZ {
    class SDLWindow : public Window {
    public:
        SDLWindow() = default;
        void OpenWindow(WindowData data) override;
        bool Update() override;

        std::pair<int, int> GetWindowExtents() override;

        void SetWindowDisplayMode(WindowDisplayMode displayMode) override;

        void RequestDrawSurface(std::unordered_map<SurfaceArgs, int*> args) override;
        void RegisterWindowResizedCallback(std::function<void()> function) override {
            _windowResizedCallback = function;
        }

        std::unordered_map<InputKey, InputDeviceState> getGamepadState(int joystickId);

    private:
        SDL_Window* _window = nullptr;
        std::function<void()> _windowResizedCallback;

        SDLInput _input {};
        bool wasWindowed { false };
        int previousPosX { 0 };
        int previousPosY { 0 };
    };
}


