#include "glfw_window.h"

CustomWindow::CustomWindow() {
    _window = nullptr;
}

void CustomWindow::OpenWindow(){
    glfwInit();

    _window = glfwCreateWindow(800, 600, "My awesome engine window", nullptr, nullptr);
}

bool CustomWindow::Update(){
    glfwPollEvents();

   
    return glfwWindowShouldClose(_window);
}
