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
        _currentScene.reset();
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
            auto currentFrameTime { std::chrono::high_resolution_clock::now() };

            auto durDeltaTime = currentFrameTime - _lastFrameTime;
            auto deltaTime = std::chrono::duration<float, std::milli > { durDeltaTime }.count() / 1000.f;
            _lastFrameTime = currentFrameTime;

            // Update game state
            Update(deltaTime);

            // Update physics
            ServiceLocator::GetRenderer()->BeginFrame();

            _currentScene->Draw();

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
//        ServiceLocator::Provide(new MultiPlatformWindow());
        ServiceLocator::Provide(new SDLWindow());

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
        ServiceLocator::Provide(new ResourceManager());
    }

    void Game::shutdownServices() {
        ServiceLocator::ShutdownServices();
    }
}