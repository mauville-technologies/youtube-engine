//
// Created by ozzadar on 2022-10-24.
//

#pragma once
#include <youtube_engine/resources/types/mesh.h>

#include <memory>

namespace OZZ {
    class MeshComponent {
    public:
        MeshComponent() = default;
        explicit MeshComponent(std::shared_ptr<Mesh>&& mesh);

        ~MeshComponent();

        std::weak_ptr<Mesh> GetMesh();
        std::weak_ptr<Mesh> SetMesh(std::shared_ptr<Mesh>&& mesh);
        [[nodiscard]] std::weak_ptr<UniformBuffer> GetModelBuffer() const { return _modelBuffer; }

    private:
        std::shared_ptr<Mesh> _mesh { nullptr };
        std::shared_ptr<UniformBuffer> _modelBuffer;

    };
}

