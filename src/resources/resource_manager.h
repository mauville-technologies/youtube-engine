//
// Created by ozzadar on 2022-10-24.
//

#pragma once
#include <unordered_map>

#include <resources/types/resource.h>
#include <resources/types/mesh_resource.h>
#include <string>
namespace OZZ {

    class ResourceManager {
    public:
        Resource::GUID CreateMesh() {

        }

    private:
        std::unordered_map<Resource::GUID, Resource> _resources;
    };
}
