//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once
#include <string>
#include <memory>
#include <youtube_engine/rendering/shader.h>
#include <youtube_engine/rendering/buffer.h>
#include <youtube_engine/rendering/texture.h>

namespace OZZ {
    struct RendererSettings {
        std::string ApplicationName;
    };

    class Renderer {
    public:
        virtual void Init(RendererSettings) = 0;
        virtual void Shutdown() = 0;

        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        virtual ~Renderer() = default;
        virtual void DrawIndexBuffer(IndexBuffer* buffer) = 0;

        virtual void WaitForIdle() = 0;

        virtual std::shared_ptr<Shader> CreateShader() = 0;
        virtual std::shared_ptr<VertexBuffer> CreateVertexBuffer() = 0;
        virtual std::shared_ptr<IndexBuffer> CreateIndexBuffer() = 0;
        virtual std::shared_ptr<UniformBuffer> CreateUniformBuffer() = 0;
        virtual std::shared_ptr<Texture> CreateTexture() = 0;

    };
}