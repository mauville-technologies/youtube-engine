//
// Created by ozzadar on 2022-12-16.
//

#include <youtube_engine/core/components/camera_component.h>
#include <youtube_engine/service_locator.h>
#include <glm/ext/matrix_clip_space.hpp>

namespace OZZ {
    glm::mat4 CameraComponent::GetViewMatrix(const glm::mat4 &tMat) {
        return glm::inverse(tMat);
    }

    glm::vec3 CameraComponent::GetForwardVector(const glm::mat4 &tMat) {
        return glm::normalize(glm::vec3(GetViewMatrix(tMat)[2])) * glm::vec3(1, 1, -1);
    }

    glm::vec3 CameraComponent::GetRightVector(const glm::mat4 &tMat) {
        return glm::normalize(glm::vec3(GetViewMatrix(tMat)[0]));
    }

    glm::vec3 CameraComponent::GetUpVector(const glm::mat4 &tMat) {
        return glm::normalize(glm::vec3(GetViewMatrix(tMat)[1]));
    }

    glm::mat4 CameraComponent::GetProjectionMatrix() const {
        auto aspect = ServiceLocator::GetWindow()->GetAspectRatio();

        return _isPerspective ?
               glm::perspective(glm::radians(FieldOfView), aspect, Near, Far) :
               glm::ortho(-aspect, aspect, -1.f, 1.f, Near, Far);
    }
} // OZZ