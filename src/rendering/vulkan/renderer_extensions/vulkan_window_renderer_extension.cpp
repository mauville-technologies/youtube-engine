//
// Created by ozzadar on 2023-03-29.
//

#include "vulkan_window_renderer_extension.h"
#include "rendering/vulkan/vulkan_initializers.h"
#include <rendering/vulkan/vulkan_utilities.h>
#include <youtube_engine/service_locator.h>
#include <VkBootstrap.h>

namespace OZZ {
    VulkanWindowRendererExtension::VulkanWindowRendererExtension(VulkanRenderer* renderer) : _renderer(renderer) {
        init();
    }

    VulkanWindowRendererExtension::~VulkanWindowRendererExtension() {
        shutdown();
    }

    void VulkanWindowRendererExtension::init() {
        createSwapchain();
        createCommands();
        createRenderPass();
        createFramebuffers();
        createSyncStructures();
    }

    void VulkanWindowRendererExtension::shutdown() {
        _renderer->WaitForIdle();
        for (auto& framebuffer: _framebuffers) {
            vkDestroyFramebuffer(_renderer->_device, framebuffer, nullptr);
            framebuffer = VK_NULL_HANDLE;
        }

        for (auto& frame : _frames) {
            vkDestroySemaphore(_renderer->_device, frame.PresentSemaphore, nullptr);
            vkDestroySemaphore(_renderer->_device, frame.RenderSemaphore, nullptr);
            vkDestroyFence(_renderer->_device, frame.RenderFence, nullptr);
            vkDestroyCommandPool(_renderer->_device, frame.CommandPool, nullptr);
            frame.CameraData.reset();
            frame.CameraData = nullptr;

            frame.PresentSemaphore = VK_NULL_HANDLE;
            frame.RenderSemaphore = VK_NULL_HANDLE;
            frame.RenderFence = VK_NULL_HANDLE;
            frame.CommandPool = VK_NULL_HANDLE;
            frame.MainCommandBuffer = VK_NULL_HANDLE;
            frame.SwapchainImageIndex = 0;
        }

        for (auto& imageView: _swapchainImageViews) {
            vkDestroyImageView(_renderer->_device, imageView, nullptr);
            imageView = VK_NULL_HANDLE;
        }

        vkDestroyRenderPass(_renderer->_device, _renderPass, nullptr);
        _renderPass = VK_NULL_HANDLE;

        vkDestroyImageView(_renderer->_device, _depthImageView, nullptr);
        _depthImageView = VK_NULL_HANDLE;

        vmaDestroyImage(_renderer->_allocator, _depthImage, _depthImageAllocation);
        _depthImage = VK_NULL_HANDLE;
        _depthImageAllocation = VK_NULL_HANDLE;

        vkDestroySwapchainKHR(_renderer->_device, _swapchain, nullptr);
        _swapchain = VK_NULL_HANDLE;

        vkDestroySurfaceKHR(_renderer->_instance, _surface, nullptr);
        _frameNumber = 0;
    }

    void VulkanWindowRendererExtension::createSwapchain() {
        auto oldSwapchain = _swapchain;

        auto[width, height] = ServiceLocator::GetWindow()->GetWindowExtents();
        _windowExtent.width = width;
        _windowExtent.height = height;

        // request vulkan surface
        if (_surface == VK_NULL_HANDLE) {
            std::unordered_map<SurfaceArgs, int *> surfaceArgs{
                    {SurfaceArgs::INSTANCE,    (int *) _renderer->_instance},
                    {SurfaceArgs::OUT_SURFACE, (int *) &_surface}
            };
            ServiceLocator::GetWindow()->RequestDrawSurface(surfaceArgs);
        }

        vkb::SwapchainBuilder swapchainBuilder{_renderer->_physicalDevice, _renderer->_device, _surface};

        vkb::Swapchain vkbSwapchain = swapchainBuilder
                .set_desired_format({
                        .format = VK_FORMAT_R8G8B8A8_SRGB,
                        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
                })
                .set_desired_present_mode(VK_PRESENT_MODE_FIFO_RELAXED_KHR)     // Hard VSync
                .set_desired_extent(width, height)
                .set_old_swapchain(oldSwapchain)
                .build()
                .value();

        // Store swapchain and all its related images
        _swapchain = vkbSwapchain.swapchain;
        _swapchainImages = vkbSwapchain.get_images().value();
        _swapchainImageViews = vkbSwapchain.get_image_views().value();
        _swapchainImageFormat = vkbSwapchain.image_format;

        if (oldSwapchain) {
            vkDestroySwapchainKHR(_renderer->_device, oldSwapchain, nullptr);
            oldSwapchain = VK_NULL_HANDLE;
        }

        // Create depth image
        VkExtent3D depthImageExtent {
                .width = _windowExtent.width,
                .height = _windowExtent.height,
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
                       &_depthImage, &_depthImageAllocation, nullptr);

        VkImageViewCreateInfo depthImageViewCreateInfo {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = _depthImage,
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
                 vkCreateImageView(_renderer->_device, &depthImageViewCreateInfo, nullptr, &_depthImageView));
    }

    void VulkanWindowRendererExtension::createCommands() {
        for (auto& frame : _frames) {
            if (frame.CommandPool == VK_NULL_HANDLE) {
                VkCommandPoolCreateInfo commandPoolCreateInfo = VulkanInitializers::CommandPoolCreateInfo(
                        _renderer->_graphicsQueueFamily,
                        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
                VK_CHECK("VulkanRenderer::createCommands()::vkCreateCommandPool", vkCreateCommandPool(_renderer->_device, &commandPoolCreateInfo, nullptr, &frame.CommandPool));
            }

            VkCommandBufferAllocateInfo commandBufferAllocateInfo = VulkanInitializers::CommandBufferAllocateInfo(
                    frame.CommandPool);
            VK_CHECK("VulkanRenderer::createCommands()::vkAllocateCommandBuffers", vkAllocateCommandBuffers(_renderer->_device, &commandBufferAllocateInfo, &frame.MainCommandBuffer));
        }
    }

    void VulkanWindowRendererExtension::createRenderPass() {
        VkAttachmentDescription colorAttachment{
                .format = _swapchainImageFormat,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout =  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        };

        VkAttachmentReference colorAttachmentRef{
                .attachment = 0,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };

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

        VkSubpassDescription subpass{
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .colorAttachmentCount = 1,
                .pColorAttachments = &colorAttachmentRef,
                .pDepthStencilAttachment = &depthAttachmentRef
        };

        VkSubpassDependency depth_dependency = {};
        depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        depth_dependency.dstSubpass = 0;
        depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        depth_dependency.srcAccessMask = 0;
        depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkSubpassDependency dependencies[] { depth_dependency };

        VkAttachmentDescription attachments[] {colorAttachment, depthAttachment};
        VkRenderPassCreateInfo renderPassCreateInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        renderPassCreateInfo.attachmentCount = 2;
        renderPassCreateInfo.pAttachments = attachments;
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpass;
        renderPassCreateInfo.dependencyCount = 1;
        renderPassCreateInfo.pDependencies = dependencies;

        VK_CHECK("VulkanRenderer::createWindowRenderPass()::vkCreateRenderPass", vkCreateRenderPass(_renderer->_device, &renderPassCreateInfo, nullptr, &_renderPass));
    }

    void VulkanWindowRendererExtension::createFramebuffers() {
        VkFramebufferCreateInfo framebufferCreateInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebufferCreateInfo.renderPass = _renderPass;
        framebufferCreateInfo.attachmentCount = 2;
        framebufferCreateInfo.width = _windowExtent.width;
        framebufferCreateInfo.height = _windowExtent.height;
        framebufferCreateInfo.layers = 1;

        _framebuffers.resize(_swapchainImageViews.size());

        for (uint32_t i = 0; i < _swapchainImageViews.size(); i++) {
            VkImageView attachments[2];
            attachments[0] = _swapchainImageViews[i];
            attachments[1] = _depthImageView;

            framebufferCreateInfo.pAttachments = attachments;
            VK_CHECK("VulkanRenderer::createFramebuffers()::vkCreateFramebuffer", vkCreateFramebuffer(_renderer->_device, &framebufferCreateInfo, nullptr, &_framebuffers[i]));
        }
    }

    void VulkanWindowRendererExtension::createSyncStructures() {
        VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkSemaphoreCreateInfo semaphoreCreateInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

        for (auto& frame : _frames) {
            VK_CHECK("VulkanRenderer::createSyncStructures()::vkCreateFence", vkCreateFence(_renderer->_device, &fenceCreateInfo, nullptr, &frame.RenderFence));

            VK_CHECK("VulkanRenderer::createSyncStructures()::vkCreateSemaphore1", vkCreateSemaphore(_renderer->_device, &semaphoreCreateInfo, nullptr, &frame.PresentSemaphore));
            VK_CHECK("VulkanRenderer::createSyncStructures()::vkCreateSemaphore2", vkCreateSemaphore(_renderer->_device, &semaphoreCreateInfo, nullptr, &frame.RenderSemaphore));
        }
    }

    VulkanWindowRendererExtension::FrameData &VulkanWindowRendererExtension::getCurrentFrame() {
        return _frames[getCurrentFrameNumber()];
    }

    uint32_t VulkanWindowRendererExtension::getCurrentFrameNumber() const {
        return _frameNumber % MAX_FRAMES_IN_FLIGHT;
    }

    void VulkanWindowRendererExtension::RenderFrame(SceneParams &sceneParams, const vector<RenderableObject> &objects) {
        auto& frame = getCurrentFrame();

        beginFrame(frame);
        renderFrame(frame, sceneParams, objects);
        endFrame(frame);

        _frameNumber++;
    }

    void VulkanWindowRendererExtension::beginFrame(FrameData& frame) {
        VK_CHECK("VulkanRenderer::BeginFrame()::vkWaitForFences", vkWaitForFences(_renderer->_device, 1, &frame.RenderFence, true, 1000000000)); // 1
        VK_CHECK("VulkanRenderer::BeginFrame()::vkResetFences", vkResetFences(_renderer->_device, 1, &frame.RenderFence));                     // 0

        VkResult result = vkAcquireNextImageKHR(_renderer->_device, _swapchain, 1000000000, frame.PresentSemaphore,
                                                VK_NULL_HANDLE, &frame.SwapchainImageIndex);

        _renderer->_descriptorSetManager.NextDescriptorFrame();

        if (result == VK_ERROR_OUT_OF_DATE_KHR ) {
            recreate();
            return;
        }

        VK_CHECK("VulkanRenderer::BeginFrame()::vkResetCommandBuffer", vkResetCommandBuffer(frame.MainCommandBuffer, 0));

        VkCommandBuffer cmd = frame.MainCommandBuffer;

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK("VulkanRenderer::BeginFrame()::vkBeginCommandBuffer", vkBeginCommandBuffer(cmd, &beginInfo));

        float flashColour = abs(sin((float) _frameNumber / 120.f));

        VkClearValue clearValue{
                .color = {0.f, flashColour, flashColour, 1.f}
        };

        VkClearValue depthClear {
                .depthStencil = {
                        .depth = 1.f
                }
        };
        VkClearValue clearValues[] = { clearValue, depthClear };

        VkRenderPassBeginInfo renderPassBeginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        renderPassBeginInfo.renderPass = _renderPass;
        renderPassBeginInfo.renderArea = {
                .offset = {
                        .x = 0,
                        .y = 0
                },
                .extent = _windowExtent
        };

        renderPassBeginInfo.framebuffer = _framebuffers[frame.SwapchainImageIndex];
        // connect clear values
        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanWindowRendererExtension::renderFrame(FrameData& frame, const SceneParams& sceneParams, const std::vector<RenderableObject>& objects) {
        // Ensure there's a uniform buffer to hold camera data
        if (!frame.CameraData) {
            frame.CameraData = _renderer->CreateUniformBuffer();
        }

        // Upload the camera data to the buffer
        // Usually I would avoid casting away the const -- but I did it here to save effort in making overloads
        frame.CameraData->UploadData(const_cast<int*>(reinterpret_cast<const int*>(&sceneParams.Camera)), sizeof(sceneParams.Camera));

        _renderer->renderObjects(frame.CommandPool, frame.MainCommandBuffer, frame.CameraData, objects);
    }

    void VulkanWindowRendererExtension::endFrame(FrameData& frame) {
        auto cmd = frame.MainCommandBuffer;
        vkCmdEndRenderPass(cmd);
        VK_CHECK("VulkanRenderer::EndFrame()::vkEndCommandBuffer", vkEndCommandBuffer(cmd));

        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        submit.pWaitDstStageMask = &waitStage;

        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &frame.PresentSemaphore;

        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &frame.RenderSemaphore;

        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &frame.MainCommandBuffer;

        vkQueueSubmit(_renderer->_graphicsQueue, 1, &submit, frame.RenderFence);

        VkPresentInfoKHR presentInfoKhr{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        presentInfoKhr.swapchainCount = 1;
        presentInfoKhr.pSwapchains = &_swapchain;

        presentInfoKhr.waitSemaphoreCount = 1;
        presentInfoKhr.pWaitSemaphores = &frame.RenderSemaphore;

        presentInfoKhr.pImageIndices = &frame.SwapchainImageIndex;

        auto result = vkQueuePresentKHR(_renderer->_graphicsQueue, &presentInfoKhr);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _renderer->_recreateFrameBuffer) {
            _renderer->_recreateFrameBuffer = false;
            recreate();
        } else {
            VK_CHECK("VulkanRenderer::EndFrame()::vkQueuePresentKHR", result);
        }
    }

    void VulkanWindowRendererExtension::recreate() {
        auto [width, height] = ServiceLocator::GetWindow()->GetWindowExtents();

        if (width == 0 || height == 0) {
            // if the framebuffer size is zero, the window is minimized and we should pause the renderer.
            _renderer->_recreateFrameBuffer = true;
            return;
        }

        _renderer->WaitForIdle();
        cleanSwapchain();

        createSwapchain();
        createCommands();
        createRenderPass();
        createFramebuffers();
    }

    void VulkanWindowRendererExtension::cleanSwapchain() {
        for (auto& framebuffer: _framebuffers) {
            vkDestroyFramebuffer(_renderer->_device, framebuffer, nullptr);
            framebuffer = VK_NULL_HANDLE;
        }

        for (auto& frame : _frames) {
            vkFreeCommandBuffers(_renderer->_device, frame.CommandPool, 1, &frame.MainCommandBuffer);
            frame.MainCommandBuffer = VK_NULL_HANDLE;
        }

        if (_renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(_renderer->_device, _renderPass, nullptr);
            _renderPass = VK_NULL_HANDLE;
        }

        for (auto& imageView: _swapchainImageViews) {
            vkDestroyImageView(_renderer->_device, imageView, nullptr);
            imageView = VK_NULL_HANDLE;
        }

        vkDestroyImageView(_renderer->_device, _depthImageView, nullptr);
        _depthImageView = VK_NULL_HANDLE;
        vmaDestroyImage(_renderer->_allocator, _depthImage, _depthImageAllocation);
        _depthImage = VK_NULL_HANDLE;
    }

    VkRenderPass VulkanWindowRendererExtension::GetRenderPass() {
        return _renderPass;
    }
}