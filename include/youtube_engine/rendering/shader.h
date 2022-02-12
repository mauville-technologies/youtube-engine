//
// Created by ozzadar on 2022-02-01.
//

#pragma once
#include <string>
#include <memory>
#include <youtube_engine/rendering/buffer.h>
#include <youtube_engine/rendering/texture.h>

namespace OZZ {
    class Shader {
    public:
        virtual void Bind(uint64_t commandHandle) = 0;
        virtual void Load(const std::string&& vertexShader, const std::string&& fragmentShader) = 0;
        virtual void AddUniformBuffer(std::shared_ptr<UniformBuffer> buffer) = 0;
        virtual void AddTexture(std::shared_ptr<Texture> texture) = 0;

        virtual ~Shader() = default;
        //TODO: Figure out uniforms
    };
}
