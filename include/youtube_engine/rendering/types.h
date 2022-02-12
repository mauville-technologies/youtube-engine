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

    struct UniformBufferObject {
        glm::mat4 model {1.f };
        glm::mat4 view { 1.f };
        glm::mat4 proj { 1.f };
    };

    enum class ColorType {
        FLOAT,
        UNSIGNED_CHAR3,
        UNSIGNED_CHAR4,
    };

}