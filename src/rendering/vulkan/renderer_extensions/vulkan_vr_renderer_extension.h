//
// Created by ozzadar on 2023-03-29.
//

#pragma once
#include <rendering/vulkan/renderer_extensions/renderer_extension.h>
#include "youtube_engine/rendering/renderables.h"

namespace OZZ {
    class VulkanVRRendererExtension : public RendererExtension {
    public:
        VulkanVRRendererExtension();

        ~VulkanVRRendererExtension();

        void RenderFrame(SceneParams &sceneParams, const std::vector <RenderableObject> &objects) override;

        VkRenderPass GetRenderPass() override;
    };
}
