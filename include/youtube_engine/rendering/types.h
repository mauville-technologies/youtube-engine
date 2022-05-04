//
// Created by Paul Mauviel on 2022-05-04.
//

#pragma once
#include <glm/glm.hpp>

namespace OZZ {
    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;
        glm::vec2 uv;
        glm::vec3 normal;
    };
}