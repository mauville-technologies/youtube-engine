//
// Created by ozzadar on 2022-10-24.
//

#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <numbers>
#include <algorithm>

namespace OZZ {
    enum class MoveDirection {
        Forward,
        Backward,
        Left,
        Right,
        Up,
        Down
    };

    class TransformComponent {

    public:
        TransformComponent() { recalculateTransform(); }
        ~TransformComponent() = default;

        glm::mat4& GetTransform() { return _transform; }

        const glm::vec3& GetPosition() { return _translation; }
        void SetPosition(const glm::vec3 position) {
            _translation = position;
            recalculateTransform();
        }

        const glm::vec3& GetScale() { return _scale; }
        void SetScale(const glm::vec3 scale) {
            _scale = scale;
            recalculateTransform();
        }

        glm::vec3 GetRotation() { return _rotation; }

        void SetRotation(const glm::vec3 rotation) {
            _rotation = rotation;
            recalculateTransform();
        }

        void Translate(MoveDirection direction, float amount) {
            auto forwardVector = glm::vec3{_transform[2]};
            auto upVector = glm::vec3{_transform[1]};
            auto rightVector = glm::vec3{_transform[0]};

            switch (direction) {
                case MoveDirection::Forward:
                    _translation += forwardVector * amount;
                    break;
                case MoveDirection::Backward:
                    _translation -= forwardVector * amount;
                    break;
                case MoveDirection::Left:
                    _translation -= rightVector * amount;
                    break;
                case MoveDirection::Right:
                    _translation += rightVector * amount;
                    break;
                case MoveDirection::Up:
                    _translation += upVector * amount;
                    break;
                case MoveDirection::Down:
                    _translation -= upVector * amount;
                    break;
                default:
                    break;
            }
            recalculateTransform();
        }

        void RotateBy(float xAmount, float yAmount = 0.f, float zAmount = 0.f, bool constrainPitch = false) {
            _rotation.x += xAmount;
            _rotation.y += yAmount;
            _rotation.z += zAmount;

            if (constrainPitch) {
                _rotation.y = std::clamp(_rotation.y, -89.f, 89.f);
            }
            recalculateTransform();
        }

    private:
        glm::vec3 _rotation { 0.f };
        glm::vec3 _translation {0.f };
        glm::vec3 _scale {1.f };
        glm::mat4 _transform {};

        void recalculateTransform() {
            glm::mat4 translation = glm::translate(glm::mat4{1.f}, _translation);
            glm::mat4 scale = glm::scale(glm::mat4{1.f}, _scale);
            glm::mat4 rotation = glm::yawPitchRoll(glm::radians(_rotation.x), glm::radians(_rotation.y), glm::radians(_rotation.z));
            _transform =  translation * rotation * scale;
        }
    };
}
