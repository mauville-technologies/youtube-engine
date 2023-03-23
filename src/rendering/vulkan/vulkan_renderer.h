//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once

#include <youtube_engine/rendering/renderer.h>
#include <vector>
#include <array>
#include <set>
#include <youtube_engine/vr/vr_subsystem.h>

#include "vulkan_includes.h"
#include "vulkan_descriptor_set_manager.h"

namespace OZZ {
    constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    struct VRFrameData {
        VkCommandPool CommandPool { VK_NULL_HANDLE };
        VkCommandBuffer MainCommandBuffer { VK_NULL_HANDLE };

        VkImageView ImageView { VK_NULL_HANDLE };
        VkFramebuffer FrameBuffer { VK_NULL_HANDLE };

        VkImageView DepthImageView { VK_NULL_HANDLE };
        VkImage DepthImage { VK_NULL_HANDLE };
        VmaAllocation DepthAllocation { VK_NULL_HANDLE };
        VkFormat DepthFormat { VK_FORMAT_D32_SFLOAT };

        std::shared_ptr<UniformBuffer> CameraData { nullptr };
    };

    struct FrameData {
        VkSemaphore PresentSemaphore { VK_NULL_HANDLE };
        VkSemaphore RenderSemaphore { VK_NULL_HANDLE };

        VkCommandPool CommandPool { VK_NULL_HANDLE };
        VkCommandBuffer MainCommandBuffer { VK_NULL_HANDLE };

        VkFence RenderFence { VK_NULL_HANDLE };
        uint32_t SwapchainImageIndex { 0 };

        std::shared_ptr<UniformBuffer> CameraData { nullptr };
    };

    struct VulkanQueueFamilyIndices {
        std::optional<uint32_t> GraphicsFamily;

        bool IsComplete() {
            return GraphicsFamily.has_value();
        }
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
        void cleanupSwapchain();
        void cleanResources();

        void recreateSwapchain();

        void createSwapchain();
        void createFrameData();
        void createCommands();
        void createDescriptorPools();
        void createDefaultRenderPass();
        void createVRRenderPass(VkFormat format);
        void createFramebuffers();
        void createSyncStructures();

        void createBufferCommands();
        void createWindowCommands();
        void createVRCommands();

        void createWindowSwapchain();
        void createVRSwapchain();

        void createWindowFramebuffers();
        void createVRFramebuffers();

        void createVRFrameData();

        void beginFrameWindow();
        std::vector<EyePoseInfo> beginFrameVR();

        void renderFrameWindow(const SceneParams& sceneParams, const std::vector<RenderableObject>& objects);
        void renderFrameVR(const std::vector<EyePoseInfo>& eyeInfo, SceneParams& sceneParams, const std::vector<RenderableObject>& objects);
        void renderEye(uint32_t eyeIndex, const SceneParams& sceneParams, const std::vector<RenderableObject>& objects);

        void endFrameWindow();
        void endFrameVR(const std::vector<EyePoseInfo>& eyePoses);

        void renderObjects(VkCommandPool commandPool, VkCommandBuffer commandBuffer, std::shared_ptr<UniformBuffer> cameraBuffer, const std::vector<RenderableObject>& objects);

        VkPhysicalDevice getPhysicalDevice();
        std::tuple<VkDevice, VulkanQueueFamilyIndices> createLogicalDevice(VkPhysicalDevice device, const std::set<std::string>& deviceExtensions);

        FrameData& getCurrentFrame();
        uint32_t getCurrentFrameNumber() const;

        VulkanQueueFamilyIndices getQueueFamilyIndices(VkPhysicalDevice device);

    private:

        bool _initialized { false };
        bool _resetting { false };

        //TODO: TEMPORARY FRAME NUMBER
        uint64_t _frameNumber {0};
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
        VkSurfaceKHR _surface;
        VmaAllocator _allocator;

        /*
         * SWAPCHAIN
         */
        VkSwapchainKHR _swapchain;
        VkFormat _swapchainImageFormat;
        std::vector<VkImage> _swapchainImages;
        std::vector<VkImageView> _swapchainImageViews;

        VkImageView _depthImageView { VK_NULL_HANDLE };
        VkImage _depthImage { VK_NULL_HANDLE };
        VmaAllocation _depthImageAllocation { VK_NULL_HANDLE };
        VkFormat _depthFormat;

        VkExtent2D _windowExtent;

        /*
         * POOLS AND QUEUES
         */
        VkQueue _graphicsQueue;
        uint32_t _graphicsQueueFamily;

        VkCommandPool _bufferCommandPool { VK_NULL_HANDLE };

        /*
         * RENDER PASSES
         */
        VkRenderPass _renderPass;
        VkRenderPass _vrRenderPass { VK_NULL_HANDLE };
        std::vector<VkFramebuffer> _framebuffers {3};

        /*
         * SYNCHRONIZATION OBJECTS
         */

        FrameData _frames[MAX_FRAMES_IN_FLIGHT];
        std::vector<std::vector<VRFrameData>> _vrFrames;

    };
}


