#include "multiplatform_window.h"

MultiPlatformWindow::MultiPlatformWindow() {
    _window = nullptr;
}

void MultiPlatformWindow::OpenWindow(WindowData data){
    glfwInit();

    _window = glfwCreateWindow(static_cast<int>(data.width), static_cast<int>(data.height), data.title.c_str(), nullptr, nullptr);
}

bool MultiPlatformWindow::Update(){
    glfwPollEvents();

   
    return glfwWindowShouldClose(_window);
}
