//
// Created by ozzadar on 2022-10-24.
//

#include <youtube_engine/core/components/transform_component.h>

namespace OZZ {

    glm::mat4 TransformComponent::GetTransform() {
        glm::mat4 translation = glm::translate(glm::mat4{1.f}, _translation);
        glm::mat4 scale = glm::scale(glm::mat4{1.f}, _scale);
        glm::mat4 rotation = toMat4(_rotation);
        return translation * rotation * scale;
    }
}