//
// Created by ozzadar on 2023-03-29.
//

#pragma once
#include <youtube_engine/rendering/renderables.h>
#include <rendering/vulkan/vulkan_includes.h>

namespace OZZ {
    class RendererExtension {
    public:
        virtual ~RendererExtension() = default;
        virtual void RenderFrame(SceneParams& sceneParams, const std::vector<RenderableObject>& objects) = 0;
        virtual VkRenderPass GetRenderPass() = 0;
    };
}