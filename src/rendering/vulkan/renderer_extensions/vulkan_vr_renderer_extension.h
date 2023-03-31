//
// Created by ozzadar on 2023-03-29.
//

#pragma once
#include <rendering/vulkan/renderer_extensions/renderer_extension.h>
#include <youtube_engine/rendering/renderables.h>
#include <rendering/vulkan/vulkan_renderer.h>
namespace OZZ {
    class VulkanVRRendererExtension : public RendererExtension {
    public:
        struct FrameData {
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
    public:
        VulkanVRRendererExtension(VulkanRenderer* renderer);

        ~VulkanVRRendererExtension() override;

        void RenderFrame(SceneParams &sceneParams, const std::vector <RenderableObject> &objects) override;

        VkRenderPass GetRenderPass() override;

    private:
        void createSwapchain();
        void createFrameData();
        void createCommands();
        void createRenderPass();
        void createFramebuffers();

        std::vector<EyePoseInfo> beginFrame();
        void renderEye(uint32_t eyeIndex, const SceneParams &sceneParams,
                       const std::vector<RenderableObject>& objects);
        void endFrameVR(const std::vector<EyePoseInfo>& eyePoses);

    private:
        VulkanRenderer* _renderer;
        VirtualRealitySubsystem* _vrSubsystem;

        VkFormat _depthFormat;
        std::vector<std::vector<FrameData>> _frames;

        VkRenderPass _renderPass { VK_NULL_HANDLE };

    };
}
