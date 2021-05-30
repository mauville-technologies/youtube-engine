#include "glfw_window.h"

MultiPlatformWindow::MultiPlatformWindow() {
    _window = nullptr;
}

void MultiPlatformWindow::OpenWindow(){
    glfwInit();

    _window = glfwCreateWindow(800, 600, "My awesome engine window", nullptr, nullptr);
}

bool MultiPlatformWindow::Update(){
    glfwPollEvents();

   
    return glfwWindowShouldClose(_window);
}
