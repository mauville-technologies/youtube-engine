//
// Created by ozzadar on 2022-10-25.
//

#pragma once
#include <youtube_engine/resources/types/resource.h>

namespace OZZ {
    struct Mesh_R : public Resource {
    public:
        Mesh_R(const Path& path) : Resource(path, Resource::Type::MESH) {
            load(path);
        }

        ~Mesh_R() {
            // Unload
            unload();
        }
    private:
        void load(const Path& path) {

        }

        void unload() {

        }
    };
}