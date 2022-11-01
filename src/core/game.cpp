//
// Created by ozzadar on 2021-05-29.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#include <youtube_engine/core/game.h>
#include <youtube_engine/service_locator.h>

#include <platform/multiplatform_window.h>
#include <platform/sdl_window.h>
#include <rendering/vulkan/vulkan_renderer.h>


namespace OZZ {
    Game::Game() : Game("New Youtube Engine Game") {}

    Game::Game(std::string windowTitle) : _title(std::move(windowTitle)), _running(true) {
        // Create scene
        _currentScene = std::make_unique<Scene>();

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

            if (ServiceLocator::GetInputManager()) {
                ServiceLocator::GetInputManager()->processInput();
            }

            // calculate deltaTime

            // Update game state
            Update(0.0f);

            // Update physics
            ServiceLocator::GetRenderer()->BeginFrame();

            Render();
            // Draw
            ServiceLocator::GetRenderer()->EndFrame();
        }

        ServiceLocator::GetRenderer()->WaitForIdle();
        OnExit();
    }

    void Game::initializeServices() {
        // provide input manager
        ServiceLocator::Provide(new InputManager());

        // Provide a window
        ServiceLocator::Provide(new MultiPlatformWindow());
//        ServiceLocator::Provide(new SDLWindow());

        // Open the window
        ServiceLocator::GetWindow()->OpenWindow({
                .title = _title,
                .width = 800,
                .height = 600
        });

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