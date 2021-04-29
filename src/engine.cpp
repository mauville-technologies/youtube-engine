#include <youtube_engine/engine.h>
#include <iostream>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <youtube_engine/service_locator.h>

#include "platform/glfw_window.h"

void YoutubeEngine::Init() {
    std::cout << "Initializing window!" << std::endl;

    ServiceLocator::Provide(new CustomWindow());
}