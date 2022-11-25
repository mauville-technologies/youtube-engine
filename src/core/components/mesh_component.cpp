//
// Created by ozzadar on 2022-10-24.
//

#include <youtube_engine/core/components/mesh_component.h>
#include <iostream>
namespace OZZ {
    MeshComponent::MeshComponent(std::shared_ptr<Mesh> &&mesh) : _mesh {mesh} {}

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