//
// Created by ozzadar on 2022-10-25.
//

#pragma once
#include <youtube_engine/platform/filesystem.h>

namespace OZZ {

    struct Resource {
    public:
        using GUID = std::string;

        enum class Type {
            UNDEFINED,
            MESH,
            SHADER,
            IMAGE
        };

        explicit Resource(const Path& path, Type type) : _guid(path.string()), _type(type) {}
        virtual ~Resource() = default;

        [[nodiscard]] GUID GetID() const { return _guid; }
    private:
        GUID _guid;
        Type _type;
    };
}

