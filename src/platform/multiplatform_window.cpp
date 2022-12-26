#include "multiplatform_window.h"
#include <stdexcept>
#include <iostream>
#include <youtube_engine/service_locator.h>

namespace OZZ {
    void MultiPlatformWindow::OpenWindow(WindowData data) {
        std::cout << "Opening GLFW window" << std::endl;
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        GLFWmonitor* monitor { nullptr };
        int width = static_cast<int>(data.Width);
        int height = static_cast<int>(data.Height);

        switch(data.DisplayMode) {
            case WindowDisplayMode::Fullscreen: {
                monitor = glfwGetPrimaryMonitor();
                auto *mode = glfwGetVideoMode(monitor);
                width = mode->width;
                height = mode->height;
                break;
            }
            case WindowDisplayMode::BorderlessWindowed: {
                auto *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
                width = mode->width;
                height = mode->height;

                glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
                glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
                break;
            }
            case WindowDisplayMode::Windowed:
                glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
                glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
                monitor = nullptr;
        }

        _window = glfwCreateWindow(width, height, data.Title.c_str(),
                                   monitor, nullptr);

        glfwSetWindowUserPointer(_window, &_input);
        for (int i = 0; i <= GLFW_JOYSTICK_LAST; i++) {
            if (glfwJoystickPresent(i)) {
                std::cout << "Joystick " << i << " is present\n";
                // Register connected devices
                auto* inputManager = ServiceLocator::GetInputManager();

                if (inputManager) {
                    _input.AddController(i);
                    inputManager->RegisterDevice(InputDevice{
                            .Source = InputSource::Controller,
                            .Index = i,
                            .StateFunc = std::bind(&MultiPlatformWindow::getGamepadState, this,
                                                   std::placeholders::_1)
                    });
                }
            }
        }

        // Register some callbacks
        glfwSetJoystickCallback([](int joystickId, int event) {
            auto* inputManager = ServiceLocator::GetInputManager();
            if (inputManager) {
                auto *input = dynamic_cast<MultiPlatformWindow *>(ServiceLocator::GetWindow());
                if (input) {
                    if (event == GLFW_CONNECTED && glfwJoystickIsGamepad(joystickId)) {
                        input->_input.AddController(joystickId);
                        inputManager->RegisterDevice(InputDevice{
                                .Source = InputSource::Controller,
                                .Index = joystickId,
                                .StateFunc = std::bind(&MultiPlatformWindow::getGamepadState, input,
                                                       std::placeholders::_1)
                        });
                        std::cout << "Connected" << "\n";
                    }
                    else if (event == GLFW_DISCONNECTED) {
                        // The joystick was disconnected
                        input->_input.RemoveController(joystickId);
                        inputManager->RemoveDevice(InputSource::Controller, joystickId);
                        std::cout << "Disconnected" << "\n";
                    }
                }
            }
        });

        glfwSetMouseButtonCallback(_window, [](GLFWwindow* window, int button, int action, int mods) {
            // Get the input
            auto* input = static_cast<MultiplatformInput*>(glfwGetWindowUserPointer(window));

            if (input) {
                input->UpdateMouseState(window);
            }
        });

        glfwSetFramebufferSizeCallback(_window, [](GLFWwindow* window, int width, int height) {
            auto *multiplatWindow = dynamic_cast<MultiPlatformWindow *>(ServiceLocator::GetWindow());

            multiplatWindow->_resizeCallback();
        });

        // Register input devices
        auto* inputManager = ServiceLocator::GetInputManager();

        inputManager->RegisterDevice(InputDevice {
            .Source = InputSource::Keyboard,
            .Index = 0,
            .StateFunc = std::bind(&MultiplatformInput::GetKeyboardState, &_input, std::placeholders::_1)
        });

        inputManager->RegisterDevice(InputDevice {
            .Source = InputSource::Mouse,
            .Index = 0,
            .StateFunc = std::bind(&MultiplatformInput::GetMouseState, &_input, std::placeholders::_1)
        });

    }

    bool MultiPlatformWindow::Update() {
        glfwPollEvents();

        // Update keyboard state
        _input.UpdateKeyboardState(_window);
        _input.UpdateMouseState(_window);

        return glfwWindowShouldClose(_window);
    }

    std::pair<int, int> MultiPlatformWindow::GetWindowExtents() {
        int width, height;
        glfwGetFramebufferSize(_window, &width, &height);

        return { width, height };
    }

    void MultiPlatformWindow::SetWindowDisplayMode(WindowDisplayMode displayMode) {
        GLFWmonitor* monitor { nullptr };
        int width {};
        int height {};
        int x = 0, y = 0;
        int refreshRate = 0;
        switch(displayMode) {
            case WindowDisplayMode::Fullscreen: {
                monitor = glfwGetPrimaryMonitor();
                auto *mode = glfwGetVideoMode(monitor);
                width = mode->width;
                height = mode->height;

                auto *currentMon = glfwGetWindowMonitor(_window);
                if (currentMon == nullptr) {
                    int prevX, prevY;
                    glfwGetWindowPos(_window, &prevX, &prevY);

                    if (prevX != 0 && prevY != 0) {
                        previousPosX = prevX;
                        previousPosY = prevY;
                    }
                }
                break;
            }
            case WindowDisplayMode::BorderlessWindowed: {
                auto *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
                width = mode->width;
                height = mode->height;
                refreshRate = mode->refreshRate;
                glfwSetWindowAttrib(_window, GLFW_DECORATED, GLFW_FALSE);

                auto *currentMon = glfwGetWindowMonitor(_window);
                if (currentMon == nullptr) {
                    int prevX, prevY;
                    glfwGetWindowPos(_window, &prevX, &prevY);

                    if (prevX != 0 && prevY != 0) {
                        previousPosX = prevX;
                        previousPosY = prevY;
                    }
                }
                break;
            }
            case WindowDisplayMode::Windowed:
                auto* configuration = ServiceLocator::GetConfiguration();

                auto& engineConfiguration = configuration->GetEngineConfiguration();
                width = static_cast<int>(engineConfiguration.ResX);
                height = static_cast<int>(engineConfiguration.ResY);

                monitor = nullptr;
                x = previousPosX == 0 ? 25 : previousPosX;
                y = previousPosY == 0 ? 25 : previousPosY;

                glfwSetWindowAttrib(_window, GLFW_DECORATED, GLFW_TRUE);
                glfwSetWindowAttrib(_window, GLFW_RESIZABLE, GLFW_TRUE);
        }

        glfwSetWindowMonitor(_window, monitor, x, y, width, height, refreshRate);

    }

    void MultiPlatformWindow::RequestDrawSurface(std::unordered_map<SurfaceArgs, int*> args) {

        // Extract what we need
        try {
            auto vkInstance = reinterpret_cast<VkInstance>(args[SurfaceArgs::INSTANCE]);
            auto *allocationCallbacks = args[SurfaceArgs::ALLOCATORS] ?
                    reinterpret_cast<VkAllocationCallbacks *>(args[SurfaceArgs::ALLOCATORS]): nullptr;
            auto *outSurface = reinterpret_cast<VkSurfaceKHR*>(args[SurfaceArgs::OUT_SURFACE]);

            if (vkInstance == VK_NULL_HANDLE) {
                throw std::runtime_error("Must provide an instance!");
            }

            if (glfwCreateWindowSurface(vkInstance, _window, allocationCallbacks, outSurface) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create window surface!");
            }
        } catch (std::bad_any_cast& e) {
            std::cout << "Failed to cast window surface arguments: " << e.what() << std::endl;
        }
    }

    std::unordered_map<InputKey, InputDeviceState> MultiPlatformWindow::getGamepadState(int joystickId) {
        GLFWgamepadstate state;
        if (glfwGetGamepadState(joystickId, &state)) {
            return _input.GetGamepadState(state);
        }

        return std::unordered_map<InputKey, InputDeviceState>{};
    }
}