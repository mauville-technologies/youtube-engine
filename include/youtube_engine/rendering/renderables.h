//
// Created by ozzadar on 2022-11-24.
//

#pragma once
#include <memory>

#include <youtube_engine/resources/types/mesh.h>
#include <youtube_engine/resources/types/material.h>
#include <glm/glm.hpp>

namespace OZZ {
    struct RenderableObject {
        std::weak_ptr<Mesh> Mesh;
        std::weak_ptr<UniformBuffer> ModelBuffer;
        glm::mat4 Transform;
    };

    struct ModelObject {
        glm::mat4 model;
    };

    struct CameraObject {
        glm::mat4 View;
        glm::mat4 Projection;
    };

    struct SceneParams {
        CameraObject Camera;
    };
}
