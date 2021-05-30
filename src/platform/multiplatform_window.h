#pragma once

#include <youtube_engine/platform/window.h>
#include <GLFW/glfw3.h>

class MultiPlatformWindow : public Window {
public:
    MultiPlatformWindow();
    void OpenWindow(WindowData data) override;
    bool Update() override;
private:
    GLFWwindow* _window;
};