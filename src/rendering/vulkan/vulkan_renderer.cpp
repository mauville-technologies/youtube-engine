//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#include "vulkan_renderer.h"

#include <cmath>
#include <chrono>
#include <VkBootstrap.h>

#include <youtube_engine/service_locator.h>
#include "vulkan_initializers.h"
#include "vulkan_utilities.h"
#include "vulkan_shader.h"
#include "vulkan_buffer.h"
#include "vulkan_texture.h"

#include <glm/gtc/matrix_transform.hpp>

namespace OZZ {
    void VulkanRenderer::Init(RendererSettings settings) {
        _rendererSettings = settings;

        initCore();
        createSwapchain();
        createCommands();
        createDescriptorPools();

        createDefaultRenderPass();
        createFramebuffers();
        createSyncStructures();
    }

    void VulkanRenderer::Shutdown() {
        WaitForIdle();

        cleanupSwapchain();
        vkDestroySwapchainKHR(_device, _swapchain, nullptr);

        vmaDestroyAllocator(_allocator);

        for (auto& frame : _frames) {
            vkDestroySemaphore(_device, frame.PresentSemaphore, nullptr);
            vkDestroySemaphore(_device, frame.RenderSemaphore, nullptr);
            vkDestroyFence(_device, frame.RenderFence, nullptr);

            vkDestroyCommandPool(_device, frame.CommandPool, nullptr);
        }

        vkDestroyDevice(_device, nullptr);
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
        vkDestroyInstance(_instance, nullptr);
    }

    void VulkanRenderer::BeginFrame() {
        VK_CHECK(vkWaitForFences(_device, 1, &getCurrentFrame().RenderFence, true, 1000000000)); // 1
        VK_CHECK(vkResetFences(_device, 1, &getCurrentFrame().RenderFence));                     // 0

        VkResult result = vkAcquireNextImageKHR(_device, _swapchain, 1000000000, getCurrentFrame().PresentSemaphore,
                                                VK_NULL_HANDLE, &getCurrentFrame().SwapchainImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapchain();
            return;
        }

        VK_CHECK(vkResetCommandBuffer(getCurrentFrame().MainCommandBuffer, 0));

        VkCommandBuffer cmd = getCurrentFrame().MainCommandBuffer;

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

        float flashColour = abs(sin((float) _frameNumber / 120.f));

        VkClearValue clearValue{
                .color = {0.f, flashColour, flashColour, 1.f}
        };

        VkClearValue depthClear {
            .depthStencil = {
                    .depth = 1.f
            }
        };

        VkRenderPassBeginInfo renderPassBeginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        renderPassBeginInfo.renderPass = _renderPass;
        renderPassBeginInfo.renderArea = {
                .offset = {
                        .x = 0,
                        .y = 0
                },
                .extent = _windowExtent
        };

        VkClearValue clearValues[] = { clearValue, depthClear };
        renderPassBeginInfo.framebuffer = _framebuffers[getCurrentFrame().SwapchainImageIndex];
        // connect clear values
        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanRenderer::EndFrame() {
        auto cmd = getCurrentFrame().MainCommandBuffer;
        vkCmdEndRenderPass(cmd);
        VK_CHECK(vkEndCommandBuffer(cmd));

        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        submit.pWaitDstStageMask = &waitStage;

        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &getCurrentFrame().PresentSemaphore;

        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &getCurrentFrame().RenderSemaphore;

        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &getCurrentFrame().MainCommandBuffer;

        vkQueueSubmit(_graphicsQueue, 1, &submit, getCurrentFrame().RenderFence);

        VkPresentInfoKHR presentInfoKhr{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        presentInfoKhr.swapchainCount = 1;
        presentInfoKhr.pSwapchains = &_swapchain;

        presentInfoKhr.waitSemaphoreCount = 1;
        presentInfoKhr.pWaitSemaphores = &getCurrentFrame().RenderSemaphore;

        presentInfoKhr.pImageIndices = &getCurrentFrame().SwapchainImageIndex;

        auto result = vkQueuePresentKHR(_graphicsQueue, &presentInfoKhr);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _recreateFrameBuffer) {
            _recreateFrameBuffer = false;
            recreateSwapchain();
        } else {
            VK_CHECK(result);
        }

        // Remove any expired shaders from the weak references
        erase_if(_shaders, [](auto shader) { return shader.expired(); });

        _frameNumber++;
    }


    void VulkanRenderer::DrawIndexBuffer(IndexBuffer *buffer) {
        vkCmdDrawIndexed(getCurrentFrame().MainCommandBuffer, buffer->GetCount(), 1, 0, 0, 0);
    }


    void VulkanRenderer::WaitForIdle() {
        vkDeviceWaitIdle(_device);
    }

    std::shared_ptr<Shader> VulkanRenderer::CreateShader() {
        auto newShader = std::make_shared<VulkanShader>(this);
        _shaders.push_back(newShader);
        return newShader;
    }


    std::shared_ptr<VertexBuffer> VulkanRenderer::CreateVertexBuffer() {
        return std::make_shared<VulkanVertexBuffer>(this);
    }

    std::shared_ptr<IndexBuffer> VulkanRenderer::CreateIndexBuffer() {
        return std::make_shared<VulkanIndexBuffer>(this);
    }

    std::shared_ptr<UniformBuffer> VulkanRenderer::CreateUniformBuffer() {
        return std::make_shared<VulkanUniformBuffer>(this);
    }

    std::shared_ptr<Texture> VulkanRenderer::CreateTexture() {
        return std::make_shared<VulkanTexture>(this);
    }

    /*
     * PRIVATE
     */

    void VulkanRenderer::initCore() {

        std::vector<std::string> extensions {};

#if __APPLE__
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        extensions.emplace_back("VK_MVK_macos_surface");
    #endif
#endif
        // Initialize the instance
        vkb::InstanceBuilder builder;
        builder.set_app_name(_rendererSettings.ApplicationName.c_str())
                .request_validation_layers(true)
                .require_api_version(1, 1, 0)
                .use_default_debug_messenger();

        for (auto& extension : extensions) {
            builder.enable_extension(extension.c_str());
        }

        auto builderInstance = builder.build();

        vkb::Instance vkb_inst = builderInstance.value();

        _instance = vkb_inst.instance;
        _debug_messenger = vkb_inst.debug_messenger;

        // request vulkan surface
        std::unordered_map<SurfaceArgs, int*> surfaceArgs{
                {SurfaceArgs::INSTANCE,    (int*)_instance},
                {SurfaceArgs::OUT_SURFACE, (int*)&_surface}
        };

        ServiceLocator::GetWindow()->RequestDrawSurface(surfaceArgs);

        std::vector<std::string> deviceExtensions {};

#if __APPLE__
    #if TARGET_OS_MAC
        deviceExtensions.emplace_back("VK_KHR_portability_subset");
    #endif
#endif

        // Select physical device
        vkb::PhysicalDeviceSelector selector{vkb_inst};

        for (auto& extension : deviceExtensions) {
            selector.add_required_extension(extension.c_str());
        }

        vkb::PhysicalDevice vkbPhysicalDevice{
                selector
                        .set_minimum_version(1, 1)
                        .set_surface(_surface)
                        .select()
                        .value()
        };

        // create the vulkan device
        vkb::DeviceBuilder deviceBuilder{vkbPhysicalDevice};
        vkb::Device vkbDevice{deviceBuilder.build().value()};

        _device = vkbDevice.device;
        _physicalDevice = vkbPhysicalDevice.physical_device;

        _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

        VmaAllocatorCreateInfo allocatorCreateInfo{};
        allocatorCreateInfo.physicalDevice = _physicalDevice;
        allocatorCreateInfo.device = _device;
        allocatorCreateInfo.instance = _instance;
        vmaCreateAllocator(&allocatorCreateInfo, &_allocator);

        ServiceLocator::GetWindow()->RegisterWindowResizedCallback([this](){
            _recreateFrameBuffer = true;
        });
    }

    void VulkanRenderer::cleanupSwapchain() {
        for (auto framebuffer: _framebuffers) {
            vkDestroyFramebuffer(_device, framebuffer, nullptr);
        }

        for (auto& frame : _frames) {
            vkFreeCommandBuffers(_device, frame.CommandPool, 1, &frame.MainCommandBuffer);
        }

        vkDestroyRenderPass(_device, _renderPass, nullptr);

        for (auto imageView: _swapchainImageViews) {
            vkDestroyImageView(_device, imageView, nullptr);
        }

        vkDestroyImageView(_device, _depthImageView, nullptr);
        vmaDestroyImage(_allocator, _depthImage, _depthImageAllocation);

        vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
    }

    void VulkanRenderer::recreateSwapchain() {
        vkDeviceWaitIdle(_device);
        cleanupSwapchain();

        createSwapchain();
        createCommands();
        createDefaultRenderPass();
        createFramebuffers();
        createDescriptorPools();

        rebuildShaders();
        //TODO: RESET SHADERS!
    }

    void VulkanRenderer::rebuildShaders() {
        for (const auto &shader: _shaders) {
            if (auto shaderPtr = shader.lock()) {
                reinterpret_pointer_cast<VulkanShader>(shaderPtr)->Rebuild();
            }
        }
    }

    void VulkanRenderer::createSwapchain() {

        VkSwapchainKHR oldSwapchain = _swapchain;

        auto[width, height] = ServiceLocator::GetWindow()->GetWindowExtents();
        _windowExtent.width = width;
        _windowExtent.height = height;

        vkb::SwapchainBuilder swapchainBuilder{_physicalDevice, _device, _surface};

        vkb::Swapchain vkbSwapchain = swapchainBuilder
                .use_default_format_selection()
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
            vkDestroySwapchainKHR(_device, oldSwapchain, nullptr);
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

        vmaCreateImage(_allocator, &depthCreateInfo, &depthImageCreateInfo,
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

        VK_CHECK(vkCreateImageView(_device, &depthImageViewCreateInfo, nullptr, &_depthImageView));
    }

    void VulkanRenderer::createCommands() {

        for (auto& frame : _frames) {
            if (frame.CommandPool == VK_NULL_HANDLE) {
                VkCommandPoolCreateInfo commandPoolCreateInfo = VulkanInitializers::CommandPoolCreateInfo(
                        _graphicsQueueFamily,
                        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
                VK_CHECK(vkCreateCommandPool(_device, &commandPoolCreateInfo, nullptr, &frame.CommandPool));
            }

            VkCommandBufferAllocateInfo commandBufferAllocateInfo = VulkanInitializers::CommandBufferAllocateInfo(
                    frame.CommandPool);
            VK_CHECK(vkAllocateCommandBuffers(_device, &commandBufferAllocateInfo, &frame.MainCommandBuffer));
        }
    }

    void VulkanRenderer::createDescriptorPools() {
        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        descriptorPoolCreateInfo.flags = 0;
        descriptorPoolCreateInfo.maxSets = 10;
        descriptorPoolCreateInfo.poolSizeCount = POOL_SIZES.size();
        descriptorPoolCreateInfo.pPoolSizes = POOL_SIZES.data();

        VK_CHECK(vkCreateDescriptorPool(_device, &descriptorPoolCreateInfo, nullptr, &_descriptorPool));

    }

    void VulkanRenderer::createDefaultRenderPass() {
        VkAttachmentDescription colorAttachment{
                .format = _swapchainImageFormat,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
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

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        //dependency from outside to the subpass, making this subpass dependent on the previous renderpasses
        VkSubpassDependency depth_dependency = {};
        depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        depth_dependency.dstSubpass = 0;
        depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        depth_dependency.srcAccessMask = 0;
        depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkSubpassDependency dependencies[] { dependency, depth_dependency };

        VkAttachmentDescription attachments[] {colorAttachment, depthAttachment};
        VkRenderPassCreateInfo renderPassCreateInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        renderPassCreateInfo.attachmentCount = 2;
        renderPassCreateInfo.pAttachments = attachments;
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpass;
        renderPassCreateInfo.dependencyCount = 2;
        renderPassCreateInfo.pDependencies = dependencies;

        VK_CHECK(vkCreateRenderPass(_device, &renderPassCreateInfo, nullptr, &_renderPass));
    }

    void VulkanRenderer::createFramebuffers() {
        VkFramebufferCreateInfo framebufferCreateInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebufferCreateInfo.renderPass = _renderPass;
        framebufferCreateInfo.attachmentCount = 2;
        framebufferCreateInfo.width = _windowExtent.width;
        framebufferCreateInfo.height = _windowExtent.height;
        framebufferCreateInfo.layers = 1;

        const auto swapchainImageCount = static_cast<uint32_t>(_swapchainImages.size());
        _framebuffers.resize(swapchainImageCount);


        for (uint32_t i = 0; i < swapchainImageCount; i++) {
            VkImageView attachments[2];
            attachments[0] = _swapchainImageViews[i];
            attachments[1] = _depthImageView;

            framebufferCreateInfo.pAttachments = attachments;
            VK_CHECK(vkCreateFramebuffer(_device, &framebufferCreateInfo, nullptr, &_framebuffers[i]));
        }
    }

    void VulkanRenderer::createSyncStructures() {
        VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkSemaphoreCreateInfo semaphoreCreateInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

        for (auto& frame : _frames) {
            VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &frame.RenderFence));

            VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &frame.PresentSemaphore));
            VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &frame.RenderSemaphore));
        }
    }

    FrameData &VulkanRenderer::getCurrentFrame() {
        return _frames[_frameNumber % MAX_FRAMES_IN_FLIGHT];
    }
}