//
// Created by ozzadar on 2022-02-02.
//

#pragma once

#include <glm/glm.hpp>

namespace OZZ {
    struct Vertex {
        glm::vec3 position;
        glm::vec4 color {1.f, 1.f, 1.f, 1.f};
        glm::vec2 uv;
        glm::vec3 normal {0.f, 0.f, 0.f };
    };
}