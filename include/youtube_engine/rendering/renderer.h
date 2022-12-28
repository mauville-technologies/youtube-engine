//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once
#include <youtube_engine/rendering/shader.h>
#include <youtube_engine/rendering/buffer.h>
#include <youtube_engine/rendering/texture.h>
#include <youtube_engine/rendering/renderables.h>

#include <string>
#include <memory>

namespace OZZ {
    struct RendererSettings {
        std::string ApplicationName;
    };

    class Renderer {
        friend class Game;
        friend class ServiceLocator;
    public:
        virtual void Init() = 0;
        virtual void Shutdown() = 0;

        virtual void BeginFrame() = 0;
        virtual void RenderFrame(const SceneParams& sceneParams, const std::vector<RenderableObject>& objects) = 0;
        virtual void EndFrame() = 0;

        virtual ~Renderer() = default;

        virtual void WaitForIdle() = 0;

        virtual std::shared_ptr<Shader> CreateShader() = 0;
        virtual std::shared_ptr<VertexBuffer> CreateVertexBuffer() = 0;
        virtual std::shared_ptr<IndexBuffer> CreateIndexBuffer() = 0;
        virtual std::shared_ptr<UniformBuffer> CreateUniformBuffer() = 0;
        virtual std::shared_ptr<Texture> CreateTexture() = 0;

    private:
        virtual void Reset() = 0;
        virtual void Reset(RendererSettings) = 0;
    };
}