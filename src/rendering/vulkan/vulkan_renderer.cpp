//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#include "vulkan_renderer.h"

#include <cmath>
#include <VkBootstrap.h>

#include <youtube_engine/service_locator.h>
#include "vulkan_initializers.h"
#include "vulkan_utilities.h"
#include "vulkan_shader.h"
#include "vulkan_buffer.h"
#include "vulkan_texture.h"

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
        vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);

        vkDestroySwapchainKHR(_device, _swapchain, nullptr);

        for (auto& frame : _frames) {
            frame.CameraData.reset();
            vkDestroySemaphore(_device, frame.PresentSemaphore, nullptr);
            vkDestroySemaphore(_device, frame.RenderSemaphore, nullptr);
            vkDestroyFence(_device, frame.RenderFence, nullptr);

            vkDestroyCommandPool(_device, frame.CommandPool, nullptr);
        }

        vmaDestroyAllocator(_allocator);

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

        if (result == VK_ERROR_OUT_OF_DATE_KHR ) {
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

    void VulkanRenderer::RenderFrame(const SceneParams &sceneParams, const vector<RenderableObject> &objects) {
        auto& currentFrame = getCurrentFrame();
        auto currentFrameNumber = getCurrentFrameNumber();

        // Ensure there's a uniform buffer to hold camera data
        if (!currentFrame.CameraData) {
            currentFrame.CameraData = CreateUniformBuffer();
        }

        // Upload the camera data to the buffer
        // Usually I would avoid casting away the const -- but I did it here to save effort in making overloads
        currentFrame.CameraData->UploadData(const_cast<int*>(reinterpret_cast<const int*>(&sceneParams.Camera)), sizeof(sceneParams.Camera));

        // Render all the objects
        std::string lastMaterial {};
        for (auto& object : objects) {
            auto mesh = object.Mesh.lock();

            if (mesh) {
                for (auto &submesh: mesh->GetSubmeshes()) {
                    auto material = submesh.GetMaterial().lock();
                    if (!material) {
                        std::cout << "Submesh doesn't have a material assigned!" << std::endl;
                        continue;
                    }

                    auto shader = material->GetShader().lock();
                    if (!shader) {
                        std::cout << "Material doesn't have a shader assigned!" << std::endl;
                        continue;
                    }

                    std::vector<VkWriteDescriptorSet> writeSets {};

                    auto shaderData = shader->GetShaderData();

                    if (lastMaterial != material->GetID()) {
                        lastMaterial = material->GetID();

                        //TODO: Update descriptor set with camera data
                        if (shaderData.Resources.contains(ResourceName::CameraData)) {
                            auto cameraData = shaderData.Resources[ResourceName::CameraData];

                            auto *buffer = dynamic_cast<VulkanUniformBuffer *>(currentFrame.CameraData.get());
                            auto descriptorSet = dynamic_cast<VulkanShader *>(shader.get())->GetDescriptorSet(currentFrameNumber,
                                    cameraData.Set);

                            VkDescriptorBufferInfo descriptorBufferInfo{};
                            descriptorBufferInfo.buffer = buffer->_buffer->Buffer;
                            descriptorBufferInfo.offset = 0;
                            descriptorBufferInfo.range = buffer->_bufferSize;

                            auto writeSet = VulkanUtilities::WriteDescriptorSetUniformBuffer(descriptorSet,
                                                                                             cameraData.Binding,
                                                                                             &descriptorBufferInfo);
                            writeSets.push_back(writeSet);
                        }
                    }

                    // Bind all textures
                    for (int i = (int)ResourceName::Diffuse0; i < (int)ResourceName::EndTextures; i++) {
                        if (shader->GetShaderData().Resources.contains((ResourceName)i)) {
                            auto textureData = shader->GetShaderData().Resources.at((ResourceName)i);
                            auto descriptorSet = dynamic_cast<VulkanShader*>(shader.get())->GetDescriptorSet(currentFrameNumber, textureData.Set);

                            // Get texture
                            auto texture = submesh.GetTexture((ResourceName)i).lock();
                            if (!texture) {
                                continue;
                            }

                            auto vulkanTexture = texture->GetTexture().lock();
                            if (!vulkanTexture) {
                                continue;
                            }

                            auto renderTexture = dynamic_cast<VulkanTexture*>(vulkanTexture.get());
                            VkDescriptorImageInfo imageBufferInfo {
                                    .sampler = renderTexture->_sampler,
                                    .imageView = renderTexture->_imageView,
                                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                            };

                            writeSets.push_back(VulkanUtilities::WriteDescriptorSetTexture(descriptorSet, textureData.Binding, &imageBufferInfo));
                        }
                    }

                    // Upload model matrix
                    auto modelBuffer = mesh->GetModelBuffer().lock();

                    if (!modelBuffer) {
                        std::cout << "Mesh model buffer is not valid!" << std::endl;
                        continue;
                    }

                    modelBuffer->UploadData(const_cast<int *>(reinterpret_cast<const int*>(&object.Transform)), sizeof(ModelObject));

                    if (shader->GetShaderData().Resources.contains(ResourceName::ModelData)) {
                        auto modelData = shader->GetShaderData().Resources.at(ResourceName::ModelData);
                        auto descriptorSet = dynamic_cast<VulkanShader *>(shader.get())->GetDescriptorSet(currentFrameNumber,
                                                                                                          modelData.Set);
                        auto *buffer = dynamic_cast<VulkanUniformBuffer *>(mesh->GetModelBuffer().lock().get());

                        VkDescriptorBufferInfo descriptorBufferInfo{};
                        descriptorBufferInfo.buffer = buffer->_buffer->Buffer;
                        descriptorBufferInfo.offset = 0;
                        descriptorBufferInfo.range = buffer->_bufferSize;

                        writeSets.push_back(
                                VulkanUtilities::WriteDescriptorSetUniformBuffer(descriptorSet, modelData.Binding,
                                                                                 &descriptorBufferInfo));
                    }

                    if (!writeSets.empty()) {
                        vkUpdateDescriptorSets(_device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0,
                                               nullptr);
                    }

                    // Bind and draw the things
                    shader->Bind();

                    submesh._indexBuffer->Bind();
                    submesh._vertexBuffer->Bind();
                    vkCmdDrawIndexed(currentFrame.MainCommandBuffer, submesh._indexBuffer->GetCount(), 1, 0, 0, 0);
                }
            }
        }
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

        _frameNumber++;
    }

    void VulkanRenderer::WaitForIdle() {
        vkDeviceWaitIdle(_device);
    }

    std::shared_ptr<Shader> VulkanRenderer::CreateShader() {
        return std::make_shared<VulkanShader>(this);
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

    }

    void VulkanRenderer::recreateSwapchain() {
        auto [width, height] = ServiceLocator::GetWindow()->GetWindowExtents();

        if (width == 0 || height == 0) {
            // if the framebuffer size is zero, the window is minimized and we should pause the renderer.
            _recreateFrameBuffer = true;
            return;
        }

        vkDeviceWaitIdle(_device);
        cleanupSwapchain();

        createSwapchain();
        createCommands();
        createDefaultRenderPass();
        createFramebuffers();
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
        descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(POOL_SIZES.size());
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
        return _frames[getCurrentFrameNumber()];
    }

    uint32_t VulkanRenderer::getCurrentFrameNumber() {
        return _frameNumber % MAX_FRAMES_IN_FLIGHT;
    };

}