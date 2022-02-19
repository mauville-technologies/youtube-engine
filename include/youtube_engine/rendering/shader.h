//
// Created by Paul Mauviel on 2022-02-18.
//

#pragma once
#include <string>
namespace OZZ {
    class Shader {
    public:
        virtual ~Shader() = default;

        virtual void Bind() = 0;
        virtual void Load(const std::string&& vertexShader, const std::string&& fragmentShader) = 0;
    };
}