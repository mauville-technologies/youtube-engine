//
// Created by ozzadar on 2023-03-29.
//

#include "vulkan_vr_renderer_extension.h"

namespace OZZ {
    VulkanVRRendererExtension::VulkanVRRendererExtension() {
    }

    VulkanVRRendererExtension::~VulkanVRRendererExtension() {

    }

    void VulkanVRRendererExtension::RenderFrame(SceneParams &sceneParams, const std::vector<RenderableObject> &objects) {

    }

    VkRenderPass VulkanVRRendererExtension::GetRenderPass() {
        return VK_NULL_HANDLE;
    }
}