#pragma once
#include <memory>
#include <utility>
#include <youtube_engine/platform/window.h>
#include <youtube_engine/rendering/renderer.h>
#include <youtube_engine/input/input_manager.h>
#include <youtube_engine/resources/resource_manager.h>
#include <youtube_engine/platform/configuration.h>
#include <youtube_engine/vr/vr_subsystem.h>

namespace OZZ {
    class ServiceLocator {
    public:
        static inline Window* GetWindow() { return _window.get(); }
        static inline Renderer* GetRenderer() { return _renderer.get(); }
        static inline InputManager* GetInputManager() { return _inputManager.get(); }
        static inline ResourceManager* GetResourceManager() { return _resourceManager.get(); }
        static inline Configuration* GetConfiguration() { return _configuration.get(); }
        static inline VirtualRealitySubsystem* GetVRSubsystem() { return _vrSubsystem.get(); }

        static inline void Provide(Window *window) {
            if (_window != nullptr) return;
            _window = std::unique_ptr<Window>(window);
        }

        static inline void Provide(VirtualRealitySubsystem* vrSubsystem, VRSettings settings) {
            if (_vrSubsystem != nullptr) return;

            _vrSubsystem = std::unique_ptr<VirtualRealitySubsystem>(vrSubsystem);
            _vrSubsystem->Init(std::move(settings));
        }

        static inline void Provide(Renderer* renderer, RendererSettings settings) {
            if (_renderer != nullptr) return;

            _renderer = std::unique_ptr<Renderer>(renderer);
            _renderer->Reset(std::move(settings));
        }

        static inline void Provide(InputManager* inputManager) {
            if (_inputManager != nullptr) return;
            _inputManager = std::unique_ptr<InputManager>(inputManager);
        }

        static inline void Provide(ResourceManager* resourceManager) {
            if (_resourceManager != nullptr) return;
            _resourceManager = std::unique_ptr<ResourceManager>(resourceManager);
        }

        static inline void Provide(Configuration* configurationManager) {
            if (_configuration != nullptr) return;

            _configuration = std::unique_ptr<Configuration>(configurationManager);
            _configuration->Init();
        }

        static inline void ShutdownServices() {
            // ensure we shut down services in the correct order
            // usually opposite order of initialized.
            shutdownResourceManager();
            shutdownInputManager();
            shutdownRenderer();
            shutdownVRSubsystem();
            shutdownWindow();
            shutdownConfiguration();
        }

    private:

        static inline std::unique_ptr<Window> _window = nullptr;
        static inline std::unique_ptr<Renderer> _renderer = nullptr;
        static inline std::unique_ptr<InputManager> _inputManager = nullptr;
        static inline std::unique_ptr<ResourceManager> _resourceManager = nullptr;
        static inline std::unique_ptr<VirtualRealitySubsystem> _vrSubsystem = nullptr;
        static inline std::unique_ptr<Configuration> _configuration = nullptr;

        static inline void shutdownWindow() {
            _window.reset();
            _window = nullptr;
        }

        static inline void shutdownVRSubsystem() {
            if (!_vrSubsystem) return;
            if (_renderer) {
                _renderer->WaitForIdle();
            }

            _vrSubsystem->Shutdown();
            _vrSubsystem = nullptr;
        }

        static inline void shutdownRenderer() {
            if (!_renderer) return;

            _renderer->Shutdown();
            _renderer = nullptr;
        }

        static inline void shutdownInputManager() {
            if (!_inputManager) return;

            _inputManager.reset();
        }

        static inline void shutdownResourceManager() {
            if (!_resourceManager) return;

            _resourceManager.reset();
        }

        static inline void shutdownConfiguration() {
            if (!_configuration) return;
            _configuration.reset();
        }
    };
}