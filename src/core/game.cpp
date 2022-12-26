//
// Created by ozzadar on 2021-05-29.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#include <youtube_engine/core/game.h>
#include <youtube_engine/service_locator.h>
#include <youtube_engine/platform/filesystem.h>

#include <platform/multiplatform_window.h>
#include <platform/sdl_window.h>
#include <platform/configuration_manager.h>
#include <rendering/vulkan/vulkan_renderer.h>


namespace OZZ {
    Game::Game() : Game("New Youtube Engine Game") {}

    Game::Game(std::string windowTitle) : _title(std::move(windowTitle)), _running(false) {}

    Game::~Game() {
        _currentScene.reset();
        shutdownServices();
    }

    void Game::Run(int argc, char **argv) {
        // Set up configuration
        // Set the home folder path
        Filesystem::GameTitle = _title;
        std::cout << "User app directory: " << Filesystem::GetAppUserDataDirectory() << std::endl;

        initializeServices();

        // Create scene
        _currentScene = std::make_unique<Scene>();

        _running = true;

        Init();

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
        ServiceLocator::Provide(new ConfigurationManager());

        auto* configuration = ServiceLocator::GetConfiguration();
        auto& engineConfiguration = configuration->GetEngineConfiguration();

        configuration->ListenForEngineSettingChange(EngineSetting::ResolutionX, Configuration::EngineSettingChangeCallback{
                .Ref = "Game",
                .Func = [this]() {
                    // Update window type
                }
        });

        configuration->ListenForEngineSettingChange(EngineSetting::ResolutionY, Configuration::EngineSettingChangeCallback{
                .Ref = "Game",
                .Func = [this]() {
                    // Update window type
                }
        });

        configuration->ListenForEngineSettingChange(EngineSetting::WindowDisplayMode, Configuration::EngineSettingChangeCallback{
                .Ref = "Game",
                .Func = [this]() {
                    auto* configuration = ServiceLocator::GetConfiguration();
                    auto& engineConfiguration = configuration->GetEngineConfiguration();

                    auto* window = ServiceLocator::GetWindow();

                    if (window) {
                        window->SetWindowDisplayMode(engineConfiguration.WinDisplayMode);
                    }
                }
        });

        // provide input manager
        ServiceLocator::Provide(new InputManager());

        // Provide a window
        switch (engineConfiguration.WinType) {
            case WindowType::SDL:
                ServiceLocator::Provide(new SDLWindow());
                break;
            case WindowType::GLFW:
                ServiceLocator::Provide(new MultiPlatformWindow());
                break;
        }

        // Open the window
        ServiceLocator::GetWindow()->OpenWindow({
                .Title = _title,
                .Width = engineConfiguration.ResX,
                .Height = engineConfiguration.ResY,
                .DisplayMode = engineConfiguration.WinDisplayMode,
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