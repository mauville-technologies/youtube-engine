#include "multiplatform_window.h"
#include <stdexcept>
#include <iostream>
#include <youtube_engine/service_locator.h>

namespace OZZ {
    void MultiPlatformWindow::OpenWindow(WindowData data) {
        std::cout << "Opening GLFW window" << std::endl;
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        _window = glfwCreateWindow(static_cast<int>(data.width), static_cast<int>(data.height), data.title.c_str(),
                                   nullptr, nullptr);

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

        glfwSetKeyCallback(_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            // Get the input
            auto* input = static_cast<MultiplatformInput*>(glfwGetWindowUserPointer(window));

            if (input) {
                // set the new value for key
                float value = 0.f;

                switch (action) {
                    case GLFW_PRESS:
                    case GLFW_REPEAT:
                        value = 1.f;
                        break;
                    default:
                        value = 0.f;
                }

                input->UpdateKeyboardState(window);
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