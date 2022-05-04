//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once

#include <youtube_engine/rendering/renderer.h>
#include <vulkan/vulkan.h>
#include <vector>
#include "vulkan_memory_allocator.h"

namespace OZZ {
    class VulkanRenderer : public Renderer {
        friend class VulkanShader;
        friend class VulkanIndexBuffer;
        friend class VulkanVertexBuffer;

        /*
         * FUNCTIONS
         */
    public:
        void Init(RendererSettings settings) override;
        void Shutdown() override;
        void RenderFrame() override;

        std::shared_ptr<Shader> CreateShader() override;
        std::shared_ptr<VertexBuffer> CreateVertexBuffer() override;
        std::shared_ptr<IndexBuffer> CreateIndexBuffer() override;

    private:
        void initCore();
        void createSwapchain();
        void createCommands();
        void createDefaultRenderPass();
        void createFramebuffers();
        void createSyncStructures();

        // FUNCTIONS TO BE MOVED OUT OF RENDERER
        void setupScene();


        /*
         * MEMBERS
         */
    private:
        //TODO: TEMPORARY FRAME NUMBER
        uint64_t _frameNumber {0};

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
         * COMMAND POOLS AND QUEUES
         */
        VkQueue _graphicsQueue;
        uint32_t _graphicsQueueFamily;

        VkCommandPool _commandPool;
        VkCommandBuffer _mainCommandBuffer;

        /*
         * RENDER PASSES
         */
        VkRenderPass _renderPass;
        std::vector<VkFramebuffer> _framebuffers {3};

        /*
         * SYNCHRONIZATION OBJECTS
         */
        VkSemaphore _presentSemaphore, _renderSemaphore;
        VkFence _renderFence;

        /*
         *  TEMPORARY RUNTIME GAME OBJECTS
         */
        std::shared_ptr<Shader> _triangleShader { nullptr };
        std::shared_ptr<Shader> _triangleShader2 { nullptr };

        std::shared_ptr<VertexBuffer> _triangle1VertexBuffer { nullptr };
        std::shared_ptr<IndexBuffer> _triangle1IndexBuffer { nullptr };
        std::shared_ptr<VertexBuffer> _triangle2VertexBuffer { nullptr };
        std::shared_ptr<IndexBuffer> _triangle2IndexBuffer { nullptr };
    };
}


