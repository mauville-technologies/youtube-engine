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
        glm::quat _rotation { glm::vec3(0.0, 0.0, 0.0) };
    private:
        glm::vec3 _translation { 0.f };
        glm::vec3 _scale { 1.f };
    };
}
