//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once

#include <vector>
#include <array>
#include <set>

#include <youtube_engine/rendering/renderer.h>
#include <rendering/vulkan/renderer_extensions/renderer_extension.h>

#include <youtube_engine/vr/vr_subsystem.h>

#include "vulkan_includes.h"
#include "vulkan_descriptor_set_manager.h"

namespace OZZ {
    constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;



    struct VulkanQueueFamilyIndices {
        std::optional<uint32_t> GraphicsFamily;

        bool IsComplete() {
            return GraphicsFamily.has_value();
        }
    };

    class VulkanRenderer : public Renderer {
    // Vulkan Objects should have access to the renderer -- so let's make friends
    friend class VulkanWindowRendererExtension;
    friend class VulkanVRRendererExtension;
    friend struct VulkanBuffer;
    friend class VulkanVertexBuffer;
    friend class VulkanIndexBuffer;
    friend class VulkanUniformBuffer;
    friend class VulkanShader;
    friend class VulkanTexture;

    public:
        void Reset() override;
        void Reset(RendererSettings) override;

        void RenderFrame(SceneParams& sceneParams, const std::vector<RenderableObject>& objects) override;

        void WaitForIdle() override;

        std::shared_ptr<Shader> CreateShader() override;
        std::shared_ptr<VertexBuffer> CreateVertexBuffer() override;
        std::shared_ptr<IndexBuffer> CreateIndexBuffer() override;
        std::shared_ptr<UniformBuffer> CreateUniformBuffer() override;
        std::shared_ptr<Texture> CreateTexture() override;

    private:
        void Init() override;
        void Shutdown() override;

        void initCore();
        void cleanResources();

        void createBufferCommands();

        void renderObjects(VkCommandPool commandPool, VkCommandBuffer commandBuffer, std::shared_ptr<UniformBuffer> cameraBuffer, const std::vector<RenderableObject>& objects);

        VkRenderPass GetActiveRenderPass();

        VkPhysicalDevice getPhysicalDevice();
        std::tuple<VkDevice, VulkanQueueFamilyIndices> createLogicalDevice(VkPhysicalDevice device, const std::set<std::string>& deviceExtensions);

        VulkanQueueFamilyIndices getQueueFamilyIndices(VkPhysicalDevice device);

    private:

        bool _initialized { false };
        bool _resetting { false };

        //TODO: TEMPORARY FRAME NUMBER
        bool _recreateFrameBuffer { false };

        RendererSettings _rendererSettings {};

        VulkanDescriptorSetManager _descriptorSetManager;

        /*
         * CORE VULKAN
         */
        VkInstance _instance;
        VkDebugUtilsMessengerEXT _debug_messenger;
        VkPhysicalDevice _physicalDevice;   // physical device
        VkDevice _device;                   // logical device
        VmaAllocator _allocator;

        /*
         * POOLS AND QUEUES
         */
        VkQueue _graphicsQueue;
        uint32_t _graphicsQueueFamily;

        VkCommandPool _bufferCommandPool { VK_NULL_HANDLE };

        std::unique_ptr<RendererExtension> _rendererExtension { nullptr };
    };
}


