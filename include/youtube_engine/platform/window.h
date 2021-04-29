#pragma once

class Window {
public:
    virtual void OpenWindow() = 0;
    virtual bool Update() = 0;
};