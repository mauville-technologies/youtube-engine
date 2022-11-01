//
// Created by ozzadar on 2022-10-25.
//

#pragma once
#include <resources/types/resource.h>

namespace OZZ {
    struct MeshResource : public Resource {
    public:
        MeshResource() : Resource(Resource::Type::MESH) {}
    };
}