//
// Created by ozzadar on 2021-09-11.
//

#include "sdl_window.h"

#include <youtube_engine/service_locator.h>
#include <rendering/vulkan/vulkan_includes.h>
#include <SDL3/SDL_vulkan.h>
#include <iostream>

namespace OZZ {

    void SDLWindow::OpenWindow(WindowData data) {
        std::cout << "Opening SDL window" << std::endl;

        SDL_Init(SDL_INIT_EVERYTHING);

        auto width = data.width;
        auto height = data.height;

        _window = SDL_CreateWindow(data.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width,
                                   height, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

        auto* inputManager = ServiceLocator::GetInputManager();

        inputManager->RegisterDevice(InputDevice {
                .Source = InputSource::Keyboard,
                .Index = 0,
                .StateFunc = std::bind(&SDLInput::GetKeyboardState, &_input, std::placeholders::_1)
        });

        inputManager->RegisterDevice(InputDevice {
                .Source = InputSource::Mouse,
                .Index = 0,
                .StateFunc = std::bind(&SDLInput::GetMouseState, &_input, std::placeholders::_1)
        });
    }

    bool SDLWindow::Update() {
        auto* inputManager = ServiceLocator::GetInputManager();

        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {

            // Do stuff with events
            if (event.type == SDL_QUIT) return true;

            if (event.type == SDL_WINDOWEVENT_RESIZED) {
                if (_windowResizedCallback) {
                    _windowResizedCallback();
                }
            }

            if (event.type == SDL_CONTROLLERDEVICEADDED) {
                std::cout << "Controller connected" << std::endl;
                if (inputManager) {
                    SDL_GameController *pad = SDL_GameControllerOpen(event.cdevice.which);
                    _input.AddController(pad);

                    auto id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad));
                    inputManager->RegisterDevice(InputDevice{
                            .Source = InputSource::Controller,
                            .Index = static_cast<int>(id),
                            .StateFunc = std::bind(&SDLWindow::getGamepadState, this, std::placeholders::_1)
                    });
                }
            }

            if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
                std::cout << "Controller disconnected" << std::endl;
                auto id = event.cdevice.which;
                _input.RemoveController(id);

                if (inputManager) {
                    inputManager->RemoveDevice(InputSource::Controller, static_cast<int>(id));
                }
            }
        }

        auto keyState = SDL_GetKeyboardState(nullptr);
        _input.UpdateKeyboardState(keyState);

        int mouseX, mouseY;

        auto buttons = SDL_GetMouseState(&mouseX, &mouseY);
        _input.UpdateMouseState(buttons, mouseX, mouseY);

        return false;
    }

    std::pair<int, int> SDLWindow::GetWindowExtents() {
        int width, height;
        SDL_Vulkan_GetDrawableSize(_window, &width, &height);

        return {width, height};
    }

    void SDLWindow::RequestDrawSurface(std::unordered_map<SurfaceArgs, int*> args) {
        // Extract what we need
        try {
            auto vkInstance = reinterpret_cast<VkInstance>(args[SurfaceArgs::INSTANCE]);
            auto *outSurface = reinterpret_cast<VkSurfaceKHR*>(args[SurfaceArgs::OUT_SURFACE]);

            if (vkInstance == VK_NULL_HANDLE) {
                throw std::runtime_error("Must provide an instance!");
            }

            if (SDL_Vulkan_CreateSurface(_window, vkInstance, outSurface) != SDL_TRUE) {
                throw std::runtime_error("Failed to create window surface!");
            }
        } catch (std::bad_any_cast& e) {
            std::cout << "Failed to cast window surface arguments: " << e.what() << std::endl;
        }
    }

    std::unordered_map<InputKey, InputDeviceState> SDLWindow::getGamepadState(int joystickId) {
        return _input.GetGamepadState(joystickId);
    }
}