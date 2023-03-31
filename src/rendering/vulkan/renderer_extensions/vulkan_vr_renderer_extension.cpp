//
// Created by ozzadar on 2023-03-29.
//

#include "vulkan_vr_renderer_extension.h"

#include <youtube_engine/service_locator.h>

#include <rendering/vulkan/vulkan_initializers.h>
#include <rendering/vulkan/vulkan_utilities.h>
#include <vr/openxr/open_xr_subsystem.h>

namespace OZZ {
    VulkanVRRendererExtension::VulkanVRRendererExtension(VulkanRenderer* renderer) : _renderer(renderer) {
        _vrSubsystem = ServiceLocator::GetVRSubsystem();
        if (!_vrSubsystem || !_vrSubsystem->IsInitialized()) {
            std::cerr << "VR not initialized when creating swapchain!" << std::endl;
            return;
        } else if (_vrSubsystem->GetBackendType() == VRBackend::OpenXR) {
            auto *xr = dynamic_cast<OpenXRSubsystem *>(_vrSubsystem);
            xr->CreateSessionVulkan(_renderer->_instance, _renderer->_physicalDevice, _renderer->_device, _renderer->_graphicsQueueFamily);
        }

        createSwapchain();
        createCommands();
        createRenderPass();
        createFramebuffers();
    }

    VulkanVRRendererExtension::~VulkanVRRendererExtension() {
        if (_vrSubsystem && _vrSubsystem->IsInitialized() && _vrSubsystem->GetBackendType() == VRBackend::OpenXR) {
            auto *xr = dynamic_cast<OpenXRSubsystem *>(_vrSubsystem);
            xr->EndSessionVulkan();
        }
        _renderer->WaitForIdle();

        for (auto& eyeFrames : _frames) {
            for (auto& eyeFrame: eyeFrames) {
                if (eyeFrame.FrameBuffer != VK_NULL_HANDLE) {
                    vkDestroyFramebuffer(_renderer->_device, eyeFrame.FrameBuffer, nullptr);
                    eyeFrame.FrameBuffer = VK_NULL_HANDLE;
                }

                if (eyeFrame.ImageView != VK_NULL_HANDLE) {
                    vkDestroyImageView(_renderer->_device, eyeFrame.ImageView, nullptr);
                    eyeFrame.ImageView = VK_NULL_HANDLE;
                }

                if (eyeFrame.DepthImageView != VK_NULL_HANDLE) {
                    vkDestroyImageView(_renderer->_device, eyeFrame.DepthImageView, nullptr);
                    eyeFrame.DepthImageView = VK_NULL_HANDLE;
                }

                if (eyeFrame.DepthImage != VK_NULL_HANDLE) {
                    vmaDestroyImage(_renderer->_allocator, eyeFrame.DepthImage, eyeFrame.DepthAllocation);
                    eyeFrame.DepthImage = VK_NULL_HANDLE;
                }

                // Destroy command buffers and pools
                if (eyeFrame.MainCommandBuffer != VK_NULL_HANDLE) {
                    vkFreeCommandBuffers(_renderer->_device, eyeFrame.CommandPool, 1, &eyeFrame.MainCommandBuffer);
                    vkDestroyCommandPool(_renderer->_device, eyeFrame.CommandPool, nullptr);

                    eyeFrame.MainCommandBuffer = VK_NULL_HANDLE;
                    eyeFrame.CommandPool = VK_NULL_HANDLE;
                }

                eyeFrame.CameraData.reset();
            }
        }

        vkDestroyRenderPass(_renderer->_device, _renderPass, nullptr);
        _renderPass = VK_NULL_HANDLE;

        _frames.clear();
    }

    void VulkanVRRendererExtension::RenderFrame(SceneParams &sceneParams, const std::vector<RenderableObject> &objects) {
        if (_vrSubsystem->GetBackendType() == VRBackend::OpenXR) {
            auto *xr = dynamic_cast<OpenXRSubsystem *>(_vrSubsystem);

            if (!xr->_ready) return;
        }

        auto eyePoses = beginFrame();
        _renderer->_descriptorSetManager.NextDescriptorFrame();

        if (eyePoses.empty()) return;

        // Render
        uint32_t currentEye { 0 };
        for (auto eye : eyePoses) {

            float angleWidth = tan(eye.FOV.AngleRight) - tan(eye.FOV.AngleLeft);
            float angleHeight = tan(eye.FOV.AngleDown) - tan(eye.FOV.AngleUp);
            const float farDistance = 1000.f;
            const float nearDistance = 0.01f;

            // build projection matrix
            sceneParams.Camera.Projection = glm::mat4{0};
            sceneParams.Camera.Projection[0][0] = 2.0f / angleWidth;
            sceneParams.Camera.Projection[2][0] = (tan(eye.FOV.AngleRight) + tan(eye.FOV.AngleLeft)) / angleWidth;
            sceneParams.Camera.Projection[1][1] = 2.0f / angleHeight;
            sceneParams.Camera.Projection[2][1] = (tan(eye.FOV.AngleUp) + tan(eye.FOV.AngleDown)) / angleHeight;
            sceneParams.Camera.Projection[2][2] = -farDistance / (farDistance - nearDistance);
            sceneParams.Camera.Projection[3][2] = -(farDistance * nearDistance) / (farDistance - nearDistance);
            sceneParams.Camera.Projection[2][3] = -1;
            sceneParams.Camera.View = glm::inverse(
                    glm::translate(glm::mat4(1.0f), sceneParams.EyePosition + eye.Position)
                    * glm::mat4_cast(eye.Orientation * sceneParams.EyeRotation)
            );

            renderEye(currentEye, sceneParams, objects);
            currentEye++;
        }

        endFrameVR(eyePoses);
    }

    VkRenderPass VulkanVRRendererExtension::GetRenderPass() {
        return _renderPass;
    }

    void VulkanVRRendererExtension::createSwapchain() {
        if (_vrSubsystem->GetBackendType() == VRBackend::OpenXR) {
            auto *xr = dynamic_cast<OpenXRSubsystem *>(_vrSubsystem);
            xr->createSwapchains();
            createFrameData();

            size_t eyeIndex { 0 };
            for (auto& eyeBuffers : _frames) {
                size_t bufferDepth { 0 };
                for (auto& frame : eyeBuffers) {
                    // get the swapchain information for this frame
                    auto& swapchain = xr->GetVulkanSwapchain(static_cast<int>(eyeIndex));

                    // create image view
                    VkImageViewCreateInfo imageViewCreateInfo { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
                    imageViewCreateInfo.image = swapchain.Images[bufferDepth].image;
                    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                    imageViewCreateInfo.format = swapchain.Format;
                    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
                    imageViewCreateInfo.subresourceRange.levelCount = 1;
                    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
                    imageViewCreateInfo.subresourceRange.layerCount = 1;

                    VK_CHECK("Failed to create Vulkan VR image view",
                             vkCreateImageView(_renderer->_device, &imageViewCreateInfo, nullptr, &frame.ImageView));

                    // Create depth image
                    VkExtent3D depthImageExtent {
                            .width = swapchain.Width,
                            .height = swapchain.Height,
                            .depth = 1
                    };

                    _depthFormat = VK_FORMAT_D32_SFLOAT;

                    VkImageCreateInfo depthCreateInfo {
                            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                            .imageType = VK_IMAGE_TYPE_2D,
                            .format = _depthFormat,
                            .extent = depthImageExtent,
                            .mipLevels = 1,
                            .arrayLayers = 1,
                            .samples = VK_SAMPLE_COUNT_1_BIT,
                            .tiling = VK_IMAGE_TILING_OPTIMAL,
                            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                    };

                    VmaAllocationCreateInfo depthImageCreateInfo {
                            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
                            .requiredFlags = VkMemoryPropertyFlags {VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT}
                    };

                    vmaCreateImage(_renderer->_allocator, &depthCreateInfo, &depthImageCreateInfo,
                                   &frame.DepthImage, &frame.DepthAllocation, nullptr);

                    VkImageViewCreateInfo depthImageViewCreateInfo {
                            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                            .image = frame.DepthImage,
                            .viewType = VK_IMAGE_VIEW_TYPE_2D,
                            .format = _depthFormat,
                            .subresourceRange = {
                                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                                    .baseMipLevel = 0,
                                    .levelCount = 1,
                                    .baseArrayLayer = 0,
                                    .layerCount = 1,
                            }
                    };

                    VK_CHECK("VulkanRenderer::createSwapchain()::vkCreateImageView",
                             vkCreateImageView(_renderer->_device, &depthImageViewCreateInfo, nullptr, &frame.DepthImageView));

                    bufferDepth++;
                }
                eyeIndex++;
            }
        }
    }

    void VulkanVRRendererExtension::createFrameData() {
        if (_vrSubsystem->GetBackendType() == VRBackend::OpenXR) {
            auto *xr = dynamic_cast<OpenXRSubsystem *>(_vrSubsystem);
            // We need to create the vr frame buffers, which require the frames. We can add the frames here now. To start,
            // we need to know both the eye count, and the bufferDepth of the VR swapchains

            auto bufferDepth = xr->GetVulkanBufferDepth();
            auto eyeCount = OZZ::OpenXRSubsystem::EyeCount;

            for (auto eyeIndex = 0; eyeIndex < eyeCount; eyeIndex++) {
                // for each eye, reserve frame data with enough buffer depth
                _frames.emplace_back(bufferDepth);
            }
        }
    }

    void VulkanVRRendererExtension::createCommands() {
        size_t eyeIndex { 0 };
        for (auto& eyeBuffers : _frames) {
            size_t bufferDepth{0};
            for (auto &frame: eyeBuffers) {
                if (frame.CommandPool == VK_NULL_HANDLE) {
                    VkCommandPoolCreateInfo commandPoolCreateInfo = VulkanInitializers::CommandPoolCreateInfo(
                            _renderer->_graphicsQueueFamily,
                            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
                    VK_CHECK("VulkanRenderer::createCommands()::vkCreateCommandPool",
                             vkCreateCommandPool(_renderer->_device, &commandPoolCreateInfo, nullptr, &frame.CommandPool));
                }

                VkCommandBufferAllocateInfo commandBufferAllocateInfo = VulkanInitializers::CommandBufferAllocateInfo(
                        frame.CommandPool);
                VK_CHECK("VulkanRenderer::createCommands()::vkAllocateCommandBuffers",
                         vkAllocateCommandBuffers(_renderer->_device, &commandBufferAllocateInfo, &frame.MainCommandBuffer));

                bufferDepth++;
            }
            eyeIndex++;
        }
    }

    void VulkanVRRendererExtension::createRenderPass() {
        if (_renderPass != VK_NULL_HANDLE) return;

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = VK_FORMAT_R8G8B8A8_SRGB;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference attachmentRef{};
        attachmentRef.attachment = 0;
        attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment{
                .flags = 0,
                .format = _depthFormat,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };

        VkAttachmentReference depthAttachmentRef{
                .attachment = 1,
                .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &attachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
//
//        VkSubpassDependency depth_dependency = {};
//        depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
//        depth_dependency.dstSubpass = 0;
//        depth_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT  | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
//        depth_dependency.srcAccessMask = 0;
//        depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
//        depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

//        VkSubpassDependency dependencies[] { depth_dependency };
        VkAttachmentDescription attachments[] {colorAttachment, depthAttachment};

        VkRenderPassCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        createInfo.flags = 0;
        createInfo.attachmentCount = 2;
        createInfo.pAttachments = attachments;
        createInfo.subpassCount = 1;
        createInfo.pSubpasses = &subpass;
        createInfo.dependencyCount = 0;
        createInfo.pDependencies = nullptr;

        VkResult result = vkCreateRenderPass(_renderer->_device, &createInfo, nullptr, &_renderPass);

        if (result != VK_SUCCESS)
        {
            cerr << "Failed to create Vulkan render pass: " << result << endl;
        }
    }

    void VulkanVRRendererExtension::createFramebuffers() {
        if (_vrSubsystem->GetBackendType() == VRBackend::OpenXR) {
            auto *xr = dynamic_cast<OpenXRSubsystem *>(_vrSubsystem);

            size_t eyeIndex { 0 };
            for (auto& eyeBuffers : _frames) {
                size_t bufferDepth { 0 };
                for (auto& frame : eyeBuffers) {
                    // get the swapchain information for this frame
                    auto& swapchain = xr->GetVulkanSwapchain(static_cast<int>(eyeIndex));

                    VkImageView attachments[2];
                    attachments[0] = frame.ImageView;
                    attachments[1] = frame.DepthImageView;

                    // create framebuffer
                    VkFramebufferCreateInfo framebufferCreateInfo { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
                    framebufferCreateInfo.renderPass = _renderPass;
                    framebufferCreateInfo.attachmentCount = 2;
                    framebufferCreateInfo.pAttachments = attachments;
                    framebufferCreateInfo.width = swapchain.Width;
                    framebufferCreateInfo.height = swapchain.Height;
                    framebufferCreateInfo.layers = 1;

                    VK_CHECK("Failed to create framebuffer for VR",
                             vkCreateFramebuffer(_renderer->_device, &framebufferCreateInfo, nullptr, &frame.FrameBuffer));

                    bufferDepth++;
                }
                eyeIndex++;
            }
        }
    }

    std::vector<EyePoseInfo> VulkanVRRendererExtension::beginFrame() {
        if (_vrSubsystem->GetBackendType() == VRBackend::OpenXR) {
            auto *xr = dynamic_cast<OpenXRSubsystem *>(_vrSubsystem);

            auto [frameWait, frameState] = xr->WaitForNextVulkanFrame();
            if (!frameState.shouldRender) {
                return {};
            }

            return xr->BeginVulkanFrame(frameWait, frameState);
        }

        return {};
    }

    void VulkanVRRendererExtension::renderEye(uint32_t eyeIndex, const SceneParams &sceneParams,
                                              const vector<RenderableObject> &objects) {
        if (_vrSubsystem->GetBackendType() == VRBackend::OpenXR) {
            auto *xr = dynamic_cast<OpenXRSubsystem *>(_vrSubsystem);

            uint32_t imageIndex = xr->AcquireVulkanSwapchainImage(eyeIndex);
            auto& swapchain = xr->GetVulkanSwapchain(static_cast<int>(eyeIndex));

            auto& eyeFrame = _frames[eyeIndex][imageIndex];

            // Ensure there's a uniform buffer to hold camera data
            if (!eyeFrame.CameraData) {
                eyeFrame.CameraData = _renderer->CreateUniformBuffer();
            }

            // Upload the camera data to the buffer
            // Usually I would avoid casting away the const -- but I did it here to save effort in making overloads
            eyeFrame.CameraData->UploadData(const_cast<int*>(reinterpret_cast<const int*>(&sceneParams.Camera)), sizeof(sceneParams.Camera));

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            auto vkResult = vkBeginCommandBuffer(eyeFrame.MainCommandBuffer, &beginInfo);

            if (vkResult != VK_SUCCESS) {
                std::cout << "Failed to begin command buffer! " << vkResult << std::endl;
                return;
            }

            VkClearValue clearValue{};
            clearValue.color = { { 0.2f, 0.2f, 0.2f, 1.0f } };

            VkClearValue depthClear {
                    .depthStencil = {
                            .depth = 1.f
                    }
            };
            VkClearValue clearValues[] = { clearValue, depthClear };

            VkRenderPassBeginInfo beginRenderPassInfo{};
            beginRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            beginRenderPassInfo.renderPass = _renderPass;
            beginRenderPassInfo.framebuffer = eyeFrame.FrameBuffer;

            beginRenderPassInfo.renderArea = {
                    { 0, 0 },
                    { (uint32_t)swapchain.Width, (uint32_t)swapchain.Height }
            };
            beginRenderPassInfo.clearValueCount = 2;
            beginRenderPassInfo.pClearValues = clearValues;


            vkCmdBeginRenderPass(eyeFrame.MainCommandBuffer, &beginRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            _renderer->renderObjects(eyeFrame.CommandPool, eyeFrame.MainCommandBuffer, eyeFrame.CameraData, objects);

            vkCmdEndRenderPass(eyeFrame.MainCommandBuffer);

            vkResult = vkEndCommandBuffer(eyeFrame.MainCommandBuffer);

            if (vkResult != VK_SUCCESS)
            {
                cerr << "Failed to end Vulkan command buffer: " << vkResult << endl;
                return;
            }

            VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.pWaitDstStageMask = &stageMask;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &eyeFrame.MainCommandBuffer;


            vkResult = vkQueueSubmit(_renderer->_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

            if (vkResult != VK_SUCCESS)
            {
                cerr << "Failed to submit Vulkan command buffer: " << vkResult << endl;
            }

            xr->ReleaseVulkanSwapchainImage(static_cast<int>(eyeIndex));
        }
    }

    void VulkanVRRendererExtension::endFrameVR(const vector<EyePoseInfo> &eyePoses) {
        if (_vrSubsystem->GetBackendType() == VRBackend::OpenXR) {
            auto *xr = dynamic_cast<OpenXRSubsystem *>(_vrSubsystem);
            xr->EndVulkanFrame(eyePoses);
        }
    }
}