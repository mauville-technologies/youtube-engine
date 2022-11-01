//
// Created by ozzadar on 2022-10-25.
//

#pragma once
namespace OZZ {

    struct Resource {
    public:
        using GUID = std::string;

        enum class Type {
            UNDEFINED,
            MESH
        };

        explicit Resource(Type type) : _type(type) {

        }

    private:
        Type _type;
        GUID _guid;
    };

}

