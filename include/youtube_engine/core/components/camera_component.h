//
// Created by ozzadar on 2022-12-16.
//

#pragma once

#include <glm/glm.hpp>

namespace OZZ {
    class CameraComponent {
    public:
        CameraComponent() = default;
        ~CameraComponent() = default;

        [[nodiscard]] bool IsActive() const { return _active; }
        void SetActive(bool active) { _active = active; }

        void SetIsPerspective(bool isPerspective) { _isPerspective = isPerspective; }
        [[nodiscard]] bool IsPerspective() const { return _isPerspective; }

        [[nodiscard]] glm::mat4 GetProjectionMatrix() const;

        static glm::mat4 GetViewMatrix(const glm::mat4& tMat);
        static glm::vec3 GetForwardVector(const glm::mat4& tMat);
        static glm::vec3 GetRightVector(const glm::mat4& tMat);
        static glm::vec3 GetUpVector(const glm::mat4& tMat);

        float FieldOfView {45.f };
        float Near {0.001f };
        float Far {1000.f };

    private:
        bool _isPerspective { true };
        bool _active { false };
    };
}
