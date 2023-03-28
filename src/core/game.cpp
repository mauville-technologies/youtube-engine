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
#include "youtube_engine/vr/vr_subsystem.h"
#include "vr/openxr/open_xr_subsystem.h"


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

        auto* configuration = ServiceLocator::GetConfiguration();
        auto& engineConfiguration = configuration->GetEngineConfiguration();
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

            if (engineConfiguration.VR) {
                auto* vr = ServiceLocator::GetVRSubsystem();

                if (vr && vr->IsInitialized()) {
                    vr->Update();
                }
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

            if (!_rendererResetRequested) {
                // Update physics
                _currentScene->Draw();
            } else {
                std::cout << "Renderer resetting!" << std::endl;
                if (ServiceLocator::GetVRSubsystem()) {
                    ServiceLocator::GetVRSubsystem()->Reset();
                }
                ServiceLocator::GetRenderer()->Reset();
                std::cout << "Renderer Has Reset" << std::endl;
                _rendererResetRequested = false;
            }
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

        if (engineConfiguration.VR) {
            VRSettings vrSettings{
                    .ApplicationName = _title,
                    .Renderer = engineConfiguration.Renderer
            };

            // Start VR System
            ServiceLocator::Provide(new OpenXRSubsystem(), vrSettings);
        }

        switch (engineConfiguration.Renderer) {
            case RendererAPI::Vulkan: {
                // initialize the renderer
                RendererSettings settings {
                        .ApplicationName = _title,
                        .VR = engineConfiguration.VR
                };

                ServiceLocator::Provide(new VulkanRenderer(), settings);
            }
        }

        ServiceLocator::Provide(new ResourceManager());
    }

    void Game::shutdownServices() {
        ServiceLocator::ShutdownServices();
    }

    void Game::ResetRenderer() {
        if (!_rendererResetRequested)
            _rendererResetRequested = true;
    }
}