//
// Created by ozzadar on 2021-05-29.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#include "youtube_engine/platform/game.h"
#include <youtube_engine/service_locator.h>
#include "multiplatform_window.h"
#include "rendering/vulkan/vulkan_renderer.h"

namespace OZZ {
    Game::Game() : Game("New Youtube Engine Game") {}

    Game::Game(std::string windowTitle) : _title(std::move(windowTitle)), _running(true) {
        initializeServices();
    }

    Game::~Game() {
        shutdownServices();
    }

    void Game::Run() {


        // run the application
        while (_running) {
            // Update the window
            if (ServiceLocator::GetWindow()->Update()) {
                _running = false;
                continue;
            }

            // calculate deltaTime

            // Update game state
            Update(0.0f);

            // Update physics

            // Draw
            ServiceLocator::GetRenderer()->RenderFrame();
        }
    }

    void Game::initializeServices() {
        // Provide a window
        ServiceLocator::Provide(new MultiPlatformWindow());
        // Open the window
        ServiceLocator::GetWindow()->OpenWindow({
                                                        .title = _title,
                                                        .width = 800,
                                                        .height = 600
                                                });
        // Initialize input system

        // initialize the renderer
        RendererSettings settings {
            .ApplicationName = _title
        };

        ServiceLocator::Provide(new VulkanRenderer(), settings);
    }

    void Game::shutdownServices() {
        ServiceLocator::ShutdownServices();
    }
}