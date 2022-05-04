//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once
#include <string>
#include <youtube_engine/rendering/shader.h>
#include <youtube_engine/rendering/buffer.h>
#include <memory>

namespace OZZ {
    struct RendererSettings {
        std::string ApplicationName;
    };

    class Renderer {
    public:
        virtual ~Renderer() = default;
        virtual void Init(RendererSettings) = 0;
        virtual void Shutdown() = 0;
        virtual void RenderFrame() = 0;

        virtual std::shared_ptr<Shader> CreateShader() = 0;
        virtual std::shared_ptr<VertexBuffer> CreateVertexBuffer() = 0;
        virtual std::shared_ptr<IndexBuffer> CreateIndexBuffer() = 0;
    };
}