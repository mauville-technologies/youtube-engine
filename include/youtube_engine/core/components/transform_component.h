//
// Created by ozzadar on 2022-10-24.
//

#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace OZZ {
    class TransformComponent {

    public:
        TransformComponent() = default;
        ~TransformComponent() = default;

        glm::mat4 GetTransform();

        glm::quat Rotation {glm::vec3(0.0, 0.0, 0.0) };
        glm::vec3 Translation {0.f };
        glm::vec3 Scale {1.f };
    };
}
