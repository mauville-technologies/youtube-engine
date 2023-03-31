//
// Created by ozzadar on 2023-03-29.
//

#pragma once
#include <rendering/vulkan/renderer_extensions/renderer_extension.h>
#include <rendering/vulkan/vulkan_includes.h>

#include <rendering/vulkan/vulkan_renderer.h>

namespace OZZ {
    constexpr uint8_t MAX_FRAMES = 2;

    class VulkanWindowRendererExtension : public RendererExtension {
        struct FrameData {
            VkSemaphore PresentSemaphore { VK_NULL_HANDLE };
            VkSemaphore RenderSemaphore { VK_NULL_HANDLE };

            VkCommandPool CommandPool { VK_NULL_HANDLE };
            VkCommandBuffer MainCommandBuffer { VK_NULL_HANDLE };

            VkFence RenderFence { VK_NULL_HANDLE };
            uint32_t SwapchainImageIndex { 0 };

            std::shared_ptr<UniformBuffer> CameraData { nullptr };
        };

    public:
        explicit VulkanWindowRendererExtension(VulkanRenderer* renderer);
        ~VulkanWindowRendererExtension() override;

        void RenderFrame(SceneParams &sceneParams, const std::vector <RenderableObject> &objects) override;

        VkRenderPass GetRenderPass() override;

    private:
        void init();
        void shutdown();

        void createSwapchain();
        void createCommands();
        void createRenderPass();
        void createFramebuffers();
        void createSyncStructures();

        void beginFrame(FrameData& frame);
        void renderFrame(FrameData& frame, const SceneParams& sceneParams, const std::vector<RenderableObject>& objects);
        void endFrame(FrameData& frame);

        void recreate();
        void cleanSwapchain();

        FrameData& getCurrentFrame();
        [[nodiscard]] uint32_t getCurrentFrameNumber() const;
    private:
        VulkanRenderer* _renderer { nullptr };
        VkExtent2D _windowExtent;

        VkRenderPass _renderPass { VK_NULL_HANDLE };
        VkSurfaceKHR _surface { VK_NULL_HANDLE };

        VkSwapchainKHR _swapchain { VK_NULL_HANDLE };
        VkFormat _swapchainImageFormat;
        std::vector<VkImage> _swapchainImages;
        std::vector<VkImageView> _swapchainImageViews;
        std::vector<VkFramebuffer> _framebuffers;

        VkImageView _depthImageView { VK_NULL_HANDLE };
        VkImage _depthImage { VK_NULL_HANDLE };
        VmaAllocation _depthImageAllocation { VK_NULL_HANDLE };
        VkFormat _depthFormat;

        uint64_t _frameNumber { 0 };
        std::vector<FrameData> _frames { MAX_FRAMES };
    };
}
