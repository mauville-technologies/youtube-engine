//
// Created by ozzadar on 2021-09-11.
//

#include "sdl_window.h"

#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <iostream>

namespace OZZ {

    void SDLWindow::OpenWindow(WindowData data) {
        SDL_Init(SDL_INIT_EVERYTHING);

        auto width = data.width;
        auto height = data.height;

        _window = SDL_CreateWindow(data.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width,
                                   height, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    }

    bool SDLWindow::Update() {
        SDL_Event event;
        SDL_PollEvent(&event);

        // Do stuff with events
        if (event.type == SDL_QUIT) return true;

        if (event.type == SDL_WINDOWEVENT) {
            switch(event.window.event) {
                case SDL_WINDOWEVENT_RESIZED:{
                    if (_windowResizedCallback) {
                        _windowResizedCallback();
                    }
                    break;
                }

            }
        }
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
}