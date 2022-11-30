//
// Created by ozzadar on 2022-10-24.
//

#include <youtube_engine/core/components/mesh_component.h>
#include <youtube_engine/service_locator.h>
#include <youtube_engine/rendering/types.h>

#include <iostream>
namespace OZZ {
    MeshComponent::MeshComponent(std::shared_ptr<Mesh> &&mesh) : _mesh {mesh} {
        _modelBuffer = ServiceLocator::GetRenderer()->CreateUniformBuffer();

        ModelObject mod {
                .model = glm::mat4{1.f}
        };

        _modelBuffer->UploadData(reinterpret_cast<int*>(&mod), sizeof(ModelObject));
    }

    MeshComponent::~MeshComponent() {
        _mesh.reset();
        _mesh = nullptr;
    }

    std::weak_ptr<Mesh> MeshComponent::GetMesh() {
        return _mesh;
    }

    std::weak_ptr<Mesh> MeshComponent::SetMesh(std::shared_ptr<Mesh> &&mesh) {
        _mesh = mesh;
        return _mesh;
    }
}