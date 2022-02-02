//
// Created by ozzadar on 2022-02-01.
//

#pragma once
#include <string>

namespace OZZ {
    class Shader {
    public:
        virtual void Bind(uint64_t commandHandle) = 0;
        virtual void Load(const std::string&& vertexShader, const std::string&& fragmentShader) = 0;

        virtual ~Shader() = default;
        //TODO: Figure out uniforms
    };
}
