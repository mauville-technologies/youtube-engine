//
// Created by ozzadar on 2022-10-25.
//

#pragma once
#include <youtube_engine/platform/filesystem.h>

namespace OZZ {

    struct Resource {
        friend class ResourceManager;
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
        virtual void ClearGPUResource() = 0;
        virtual void RecreateGPUResource() = 0;
    private:
        GUID _guid;
        Type _type;
    };
}

