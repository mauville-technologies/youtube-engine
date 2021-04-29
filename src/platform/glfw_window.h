#pragma once

#include <youtube_engine/platform/window.h>
#include <GLFW/glfw3.h>

class CustomWindow : public Window {
public:
    CustomWindow();
    virtual void OpenWindow() override;
    virtual bool Update() override;
private:
    GLFWwindow* _window;
};