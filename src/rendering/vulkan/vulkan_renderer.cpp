//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#include "vulkan_renderer.h"

#include <cmath>

#include <youtube_engine/service_locator.h>

#include <VkBootstrap.h>


#include "vulkan_initializers.h"
#include "vulkan_utilities.h"
#include "vulkan_shader.h"
#include "vulkan_buffer.h"


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
        _triangleShader = CreateShader();
        _triangleShader->Load("basic.vert.spv", "basic.frag.spv");
        _triangleShader2 = CreateShader();
        _triangleShader2->Load("basic.vert.spv", "basic.frag.spv");
        createPipelines();

    }

    void VulkanRenderer::Shutdown() {
        vkDeviceWaitIdle(_device);

        if (_triangleShader) {
            _triangleShader.reset();
            _triangleShader = nullptr;
        }
        if (_triangleShader2) {
            _triangleShader2.reset();
            _triangleShader2 = nullptr;
        }

        if (_triangleBuffer) {
            _triangleBuffer.reset();
            _triangleBuffer = nullptr;
        }

        if (_triangleIndexBuffer) {
            _triangleIndexBuffer.reset();
            _triangleIndexBuffer = nullptr;
        }

        if (_triangle2Buffer) {
            _triangle2Buffer.reset();
            _triangle2Buffer = nullptr;
        }

        if (_triangle2IndexBuffer) {
            _triangle2IndexBuffer.reset();
            _triangle2IndexBuffer = nullptr;
        }

        if (_triangleUniformBuffer) {
            _triangleUniformBuffer.reset();
            _triangleUniformBuffer = nullptr;
        }

        cleanupSwapchain();

        vmaDestroyAllocator(_allocator);
        vkDestroyFence(_device, _renderFence, nullptr);
        vkDestroySemaphore(_device, _presentSemaphore, nullptr);
        vkDestroySemaphore(_device, _renderSemaphore, nullptr);

        vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
        vkDestroyCommandPool(_device, _commandPool, nullptr);

        vkDestroyDevice(_device, nullptr);
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
        vkDestroyInstance(_instance, nullptr);
    }

    void VulkanRenderer::RenderFrame() {
        VK_CHECK(vkWaitForFences(_device, 1, &_renderFence, true, 1000000000)); // 1
        VK_CHECK(vkResetFences(_device, 1, &_renderFence));                     // 0

        uint32_t swapchainImageIndex;
        VkResult result = vkAcquireNextImageKHR(_device, _swapchain, 1000000000, _presentSemaphore,
                                                VK_NULL_HANDLE, &swapchainImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapchain();
            return;
        } else {
            VK_CHECK(result);
        }

        VK_CHECK(vkResetCommandBuffer(_mainCommandBuffer, 0));

        VkCommandBuffer cmd = _mainCommandBuffer;

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

        float flashColour = abs(sin((float) _frameNumber / 120.f));

        VkClearValue clearValue{
                .color = {0.f, flashColour, flashColour, 1.f}
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

        renderPassBeginInfo.framebuffer = _framebuffers[swapchainImageIndex];
        // connect clear values
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &clearValue;

        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject uboObject{
                glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                glm::perspective(glm::radians(45.0f), _windowExtent.width / (float) _windowExtent.height, 0.1f, 10.0f)
        };

        uboObject.proj[1][1] *= -1;

        _triangleUniformBuffer->UploadData(uboObject);
//        _triangleUniformBuffer2->UploadData(uboObject);

        vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // DRAW CALLS
        _triangleShader->Bind(reinterpret_cast<uint64_t>(cmd));

        _triangle2Buffer->Bind(reinterpret_cast<uint64_t>(cmd));
        _triangle2IndexBuffer->Bind(reinterpret_cast<uint64_t>(cmd));
        vkCmdDrawIndexed(cmd, _triangle2IndexBuffer->GetCount(), 1, 0, 0, 0);

        _triangleShader2->Bind(reinterpret_cast<uint64_t>(cmd));

        _triangleBuffer->Bind(reinterpret_cast<uint64_t>(cmd));
        _triangleIndexBuffer->Bind(reinterpret_cast<uint64_t>(cmd));
        vkCmdDrawIndexed(cmd, _triangleIndexBuffer->GetCount(), 1, 0, 0, 0);

        vkCmdEndRenderPass(cmd);
        VK_CHECK(vkEndCommandBuffer(cmd));

        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        submit.pWaitDstStageMask = &waitStage;

        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &_presentSemaphore;

        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &_renderSemaphore;

        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &_mainCommandBuffer;

        VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, _renderFence));

        VkPresentInfoKHR presentInfoKhr{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        presentInfoKhr.swapchainCount = 1;
        presentInfoKhr.pSwapchains = &_swapchain;

        presentInfoKhr.waitSemaphoreCount = 1;
        presentInfoKhr.pWaitSemaphores = &_renderSemaphore;

        presentInfoKhr.pImageIndices = &swapchainImageIndex;

        result = vkQueuePresentKHR(_graphicsQueue, &presentInfoKhr);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            recreateSwapchain();
        } else {
            VK_CHECK(result);
        }

        // Remove any expired shaders from the weak references
        erase_if(_shaders, [](auto shader) { return shader.expired(); });

        _frameNumber++;
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
    }

    void VulkanRenderer::cleanupSwapchain() {
        for (auto framebuffer: _framebuffers) {
            vkDestroyFramebuffer(_device, framebuffer, nullptr);
        }

        vkDestroyRenderPass(_device, _renderPass, nullptr);
        vkFreeCommandBuffers(_device, _commandPool, 1, &_mainCommandBuffer);

        vkDestroySwapchainKHR(_device, _swapchain, nullptr);

        for (auto imageView: _swapchainImageViews) {
            vkDestroyImageView(_device, imageView, nullptr);
        }
    }

    void VulkanRenderer::recreateSwapchain() {
        vkDeviceWaitIdle(_device);

        cleanupSwapchain();

        createSwapchain();
        createCommands();
        createDefaultRenderPass();
        createFramebuffers();
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

        auto[width, height] = ServiceLocator::GetWindow()->GetWindowExtents();
        _windowExtent.width = width;
        _windowExtent.height = height;

        vkb::SwapchainBuilder swapchainBuilder{_physicalDevice, _device, _surface};
        vkb::Swapchain vkbSwapchain = swapchainBuilder
                .use_default_format_selection()
                .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)     // Hard VSync
                .set_desired_extent(width, height)
                .set_old_swapchain(VK_NULL_HANDLE)
                .build()
                .value();

        // Store swapchain and all its related images
        _swapchain = vkbSwapchain.swapchain;
        _swapchainImages = vkbSwapchain.get_images().value();
        _swapchainImageViews = vkbSwapchain.get_image_views().value();
        _swapchainImageFormat = vkbSwapchain.image_format;
    }

    void VulkanRenderer::createCommands() {

        if (_commandPool == VK_NULL_HANDLE) {
            VkCommandPoolCreateInfo commandPoolCreateInfo = VulkanInitializers::CommandPoolCreateInfo(
                    _graphicsQueueFamily,
                    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
            VK_CHECK(vkCreateCommandPool(_device, &commandPoolCreateInfo, nullptr, &_commandPool));
        }

        VkCommandBufferAllocateInfo commandBufferAllocateInfo = VulkanInitializers::CommandBufferAllocateInfo(
                _commandPool);
        VK_CHECK(vkAllocateCommandBuffers(_device, &commandBufferAllocateInfo, &_mainCommandBuffer));

    }

    void VulkanRenderer::createDescriptorPools() {
        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        descriptorPoolCreateInfo.flags = 0;
        descriptorPoolCreateInfo.maxSets = 10;
        descriptorPoolCreateInfo.poolSizeCount = POOL_SIZE_COUNT;
        descriptorPoolCreateInfo.pPoolSizes = POOL_SIZES;

        vkCreateDescriptorPool(_device, &descriptorPoolCreateInfo, nullptr, &_descriptorPool);

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

        VkSubpassDescription subpass{
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .colorAttachmentCount = 1,
                .pColorAttachments = &colorAttachmentRef
        };

        VkRenderPassCreateInfo renderPassCreateInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        renderPassCreateInfo.attachmentCount = 1;
        renderPassCreateInfo.pAttachments = &colorAttachment;
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpass;

        VK_CHECK(vkCreateRenderPass(_device, &renderPassCreateInfo, nullptr, &_renderPass));
    }

    void VulkanRenderer::createFramebuffers() {
        VkFramebufferCreateInfo framebufferCreateInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebufferCreateInfo.renderPass = _renderPass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.width = _windowExtent.width;
        framebufferCreateInfo.height = _windowExtent.height;
        framebufferCreateInfo.layers = 1;

        const auto swapchainImageCount = static_cast<uint32_t>(_swapchainImages.size());
        _framebuffers.resize(swapchainImageCount);

        for (uint32_t i = 0; i < swapchainImageCount; i++) {
            framebufferCreateInfo.pAttachments = &_swapchainImageViews[i];
            VK_CHECK(vkCreateFramebuffer(_device, &framebufferCreateInfo, nullptr, &_framebuffers[i]));
        }
    }

    void VulkanRenderer::createSyncStructures() {
        VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_renderFence));

        VkSemaphoreCreateInfo semaphoreCreateInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_presentSemaphore));
        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_renderSemaphore));
    }

    void VulkanRenderer::createPipelines() {

        _triangleBuffer = CreateVertexBuffer();
        _triangleBuffer->UploadData({
                                            Vertex{
                                                    .position = {0.75f, 0.75f, 0.f},
                                                    .color = {1.f, 0.f, 0.f, 1.f},
                                            },
                                            Vertex{
                                                    .position = {-0.75f, 0.75f, 0.f},
                                                    .color = {0.f, 1.f, 0.f, 1.f},
                                            },
                                            Vertex{
                                                    .position = {-0.75f, -0.75f, 0.f},
                                                    .color = {0.f, 0.f, 1.f, 1.f},
                                            },
                                            Vertex{
                                                    .position = {0.75f, -0.75f, 0.f},
                                                    .color = {0.f, 0.f, 1.f, 1.f},
                                            },
                                    });

        _triangleIndexBuffer = CreateIndexBuffer();
        _triangleIndexBuffer->UploadData({0, 1, 2, 2, 3, 0});

        _triangle2Buffer = CreateVertexBuffer();
        _triangle2Buffer->UploadData({
                                             Vertex{
                                                     .position = {1.f, 1.f, 0.f},
                                                     .color = {1.f, 1.f, 1.f, 1.f},
                                             },
                                             Vertex{
                                                     .position = {-1.f, 1.f, 0.f},
                                                     .color = {1.f, 1.f, 1.f, 1.f},
                                             },
                                             Vertex{
                                                     .position = {0.f, -1.f, 0.f},
                                                     .color = {1.f, 1.f, 1.f, 1.f},
                                             }
                                     });

        _triangle2IndexBuffer = CreateIndexBuffer();
        _triangle2IndexBuffer->UploadData({0, 1, 2});

        _triangleUniformBuffer = CreateUniformBuffer();

        UniformBufferObject uboObject{
                glm::rotate(glm::mat4(1.0f), 1.f * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                glm::perspective(glm::radians(45.0f), _windowExtent.width / (float) _windowExtent.height, 0.1f, 10.0f)
        };

        uboObject.proj[1][1] *= -1;

        _triangleUniformBuffer->UploadData(uboObject);

        _triangleShader->AddUniformBuffer(_triangleUniformBuffer);

        auto buffer = CreateUniformBuffer();

        UniformBufferObject uboObject2{
                glm::rotate(glm::mat4(1.0f), 1.f * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                glm::perspective(glm::radians(45.0f), _windowExtent.width / (float) _windowExtent.height, 0.1f, 10.0f)
        };

        uboObject2.proj[1][1] *= -1;

        buffer->UploadData(uboObject2);
        _triangleShader2->AddUniformBuffer(buffer);
    }

}