#pragma once

#include <youtube_engine/platform/window.h>
#include <GLFW/glfw3.h>

class MultiPlatformWindow : public Window {
public:
    MultiPlatformWindow();
    virtual void OpenWindow() override;
    virtual bool Update() override;
private:
    GLFWwindow* _window;
};