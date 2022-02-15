//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once

#include <youtube_engine/rendering/renderer.h>
#include <vector>
#include "vulkan_includes.h"

namespace OZZ {
    constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    struct FrameData {
        VkSemaphore PresentSemaphore { VK_NULL_HANDLE };
        VkSemaphore RenderSemaphore { VK_NULL_HANDLE };

        VkCommandPool CommandPool { VK_NULL_HANDLE };
        VkCommandBuffer MainCommandBuffer { VK_NULL_HANDLE };

        VkFence RenderFence { VK_NULL_HANDLE };
        uint32_t SwapchainImageIndex { 0 };
    };

    class VulkanRenderer : public Renderer {
    // Vulkan Objects should have access to the renderer -- so let's make friends
    friend struct VulkanBuffer;
    friend class VulkanVertexBuffer;
    friend class VulkanIndexBuffer;
    friend class VulkanUniformBuffer;
    friend class VulkanShader;
    friend class VulkanTexture;

    public:
        void Init(RendererSettings settings) override;
        void Shutdown() override;
        void BeginFrame() override;
        void EndFrame() override;

        void DrawIndexBuffer(IndexBuffer* buffer) override;

        void WaitForIdle() override;

        std::shared_ptr<Shader> CreateShader() override;
        std::shared_ptr<VertexBuffer> CreateVertexBuffer() override;
        std::shared_ptr<IndexBuffer> CreateIndexBuffer() override;
        std::shared_ptr<UniformBuffer> CreateUniformBuffer() override;
        std::shared_ptr<Texture> CreateTexture() override;

    private:
        void initCore();
        void cleanupSwapchain();

        void recreateSwapchain();
        void rebuildShaders();

        void createSwapchain();
        void createCommands();
        void createDescriptorPools();
        void createDefaultRenderPass();
        void createFramebuffers();
        void createSyncStructures();

        FrameData& getCurrentFrame();

    private:

        //TODO: TEMPORARY FRAME NUMBER
        uint64_t _frameNumber {0};
        bool _recreateFrameBuffer { false };

        RendererSettings _rendererSettings {};

        /*
         * CORE VULKAN
         */
        VkInstance _instance;
        VkDebugUtilsMessengerEXT _debug_messenger;
        VkPhysicalDevice _physicalDevice;   // physical device
        VkDevice _device;                   // logical device
        VkSurfaceKHR _surface;
        VmaAllocator _allocator;

        /*
         * SWAPCHAIN
         */
        VkSwapchainKHR _swapchain;
        VkFormat _swapchainImageFormat;
        std::vector<VkImage> _swapchainImages;
        std::vector<VkImageView> _swapchainImageViews;
        VkExtent2D _windowExtent;

        /*
         * POOLS AND QUEUES
         */
        VkQueue _graphicsQueue;
        uint32_t _graphicsQueueFamily;

        static constexpr uint32_t POOL_SIZE_COUNT { 1 };
        static constexpr VkDescriptorPoolSize POOL_SIZES[] {
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 }
        };

        VkDescriptorPool _descriptorPool { VK_NULL_HANDLE };

        /*
         * RENDER PASSES
         */
        VkRenderPass _renderPass;
        std::vector<VkFramebuffer> _framebuffers {3};

        /*
         * SYNCHRONIZATION OBJECTS
         */

        FrameData _frames[MAX_FRAMES_IN_FLIGHT];

        /*
         * References to the shaders created in the system to be rebuilt when swapchain is recreated
         */
        std::vector<std::weak_ptr<Shader>> _shaders {};
    };
}


