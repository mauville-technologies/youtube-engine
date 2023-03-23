//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#include "vulkan_renderer.h"

#include <cmath>
#include <map>
#include <VkBootstrap.h>

#include <youtube_engine/service_locator.h>
#include <vr/openxr/open_xr_subsystem.h>

#include "vulkan_initializers.h"
#include "vulkan_shader.h"
#include "vulkan_buffer.h"
#include "vulkan_texture.h"
#include "vulkan_utilities.h"


namespace OZZ {
    void VulkanRenderer::Init() {
        if (_initialized) return;
        initCore() ;

        if (_rendererSettings.VR) {
            auto* vr = ServiceLocator::GetVRSubsystem();
            if (!vr || !vr->IsInitialized()) {
                std::cerr << "VR not initialized on renderer start up, disabling VR." << std::endl;
                _rendererSettings.VR = false;
            } else if (vr->GetBackendType() == VRBackend::OpenXR) {
                auto *xr = dynamic_cast<OpenXRSubsystem *>(vr);
                xr->CreateSessionVulkan(_instance, _physicalDevice, _device, _graphicsQueueFamily);
            }
        }

        createSwapchain();
        std::cout << "Done Creating Swapchains" << std::endl;
        createCommands();
        std::cout << "Done Creating Commands" << std::endl;
        createDescriptorPools();
        std::cout << "Done Descriptor Pools" << std::endl;

//        createDefaultRenderPass();
        createVRRenderPass(VK_FORMAT_R8G8B8A8_SRGB);
        std::cout << "Done Creating Render pass" << std::endl;

        createFramebuffers();
        std::cout << "Done Creating Frame buffers" << std::endl;

//        createSyncStructures();

        _initialized = true;
    }

    void VulkanRenderer::Shutdown() {
        if (!_initialized) return;
        WaitForIdle();

        if (_rendererSettings.VR) {
            auto* vr = ServiceLocator::GetVRSubsystem();
            if (vr && vr->IsInitialized() && vr->GetBackendType() == VRBackend::OpenXR) {
                auto *xr = dynamic_cast<OpenXRSubsystem *>(vr);
                xr->EndSessionVulkan();
            }
        }

        cleanResources();

        vkDestroyDevice(_device, nullptr);
        _device = VK_NULL_HANDLE;
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        _surface = VK_NULL_HANDLE;
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
        vkDestroyInstance(_instance, nullptr);
        _instance = VK_NULL_HANDLE;
        _initialized = false;
        _frameNumber = 0;
    }

    void VulkanRenderer::Reset() {
        Reset(_rendererSettings);
    }

    void VulkanRenderer::Reset(RendererSettings settings) {
        _resetting = true;
        _rendererSettings = settings;

        Shutdown();
        Init();
        auto* resourceManager = ServiceLocator::GetResourceManager();

        if (resourceManager) {
            resourceManager->RecreateGPUResourcesAfterReset();
        }
        _resetting = false;
    }

    void VulkanRenderer::RenderFrame(SceneParams &sceneParams, const vector<RenderableObject> &objects) {
//        beginFrameWindow();
//            if (!_initialized || _resetting) return;
//            renderFrameWindow(sceneParams, objects);
//        endFrameWindow();
        _frameNumber++;

        if (_rendererSettings.VR) {
            auto* vr = ServiceLocator::GetVRSubsystem();
            if (!vr || !vr->IsInitialized()) {
            } else if (vr->GetBackendType() == VRBackend::OpenXR) {
                auto *xr = dynamic_cast<OpenXRSubsystem *>(vr);

                if (!xr->_ready) return;
            }

            auto eyePoses = beginFrameVR();

            if (eyePoses.empty()) return;
//            if (!_initialized || _resetting) return;
            renderFrameVR(eyePoses, sceneParams, objects);

            endFrameVR(eyePoses);
        }

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
        uint32_t vulkanVersion = VK_MAKE_VERSION(1, 2, 0);

        std::set<std::string> instanceExtensions {};

        instanceExtensions.insert("VK_KHR_get_physical_device_properties2");
        instanceExtensions.insert("VK_EXT_debug_report");

        if (_rendererSettings.VR) {
            auto* vr = ServiceLocator::GetVRSubsystem();
            if (!vr || !vr->IsInitialized()) {
                std::cerr << "VR not initialized on renderer start up, disabling VR." << std::endl;
                _rendererSettings.VR = false;
            } else if (vr->GetBackendType() == VRBackend::OpenXR) {
                auto* xr = dynamic_cast<OpenXRSubsystem *>(vr);
                for (auto& extension : xr->GetVulkanInstanceExtensions()) {
                    instanceExtensions.insert(extension);
                }

                auto graphicsRequirements = xr->GetVulkanGraphicsRequirements();
                vulkanVersion = graphicsRequirements.minApiVersionSupported;
            }
        }

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
                .require_api_version(vulkanVersion)
                .use_default_debug_messenger();

        for (auto& extension : instanceExtensions) {
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

        _physicalDevice = getPhysicalDevice();

        std::set<std::string> deviceExtensions {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
        };

        if (_rendererSettings.VR) {
            auto* vr = ServiceLocator::GetVRSubsystem();
            if (!vr || !vr->IsInitialized()) {
                std::cerr << "VR not initialized on renderer start up, disabling VR." << std::endl;
                _rendererSettings.VR = false;
            }

            if (vr->GetBackendType() == VRBackend::OpenXR) {
                for (auto& extension : dynamic_cast<OpenXRSubsystem *>(vr)->GetVulkanDeviceExtensions()) {
                    deviceExtensions.insert(extension);
                }
            }
        }

        deviceExtensions.insert("VK_KHR_push_descriptor");

#if __APPLE__
    #if TARGET_OS_MAC
        deviceExtensions.emplace_back("VK_KHR_portability_subset");
    #endif
#endif

        auto [device, queueIndices] = createLogicalDevice(_physicalDevice, deviceExtensions);

        if (device == VK_NULL_HANDLE) {
            std::cerr << "Failed to acquire Vulkan logical device" << std::endl;
            return;
        }

        _device = device;
        _graphicsQueueFamily = queueIndices.GraphicsFamily.value();
        vkGetDeviceQueue(_device, _graphicsQueueFamily, 0, &_graphicsQueue);

        VmaAllocatorCreateInfo allocatorCreateInfo{};
        allocatorCreateInfo.physicalDevice = _physicalDevice;
        allocatorCreateInfo.device = _device;
        allocatorCreateInfo.instance = _instance;
        vmaCreateAllocator(&allocatorCreateInfo, &_allocator);

        ServiceLocator::GetWindow()->RegisterWindowResizedCallback([this](){
            _recreateFrameBuffer = true;
        });

        _descriptorSetManager = VulkanDescriptorSetManager { &_device };
    }

    void VulkanRenderer::cleanupSwapchain() {
        // Clean VR swapchain
        for (auto& eyeFrames : _vrFrames) {
            for (auto& eyeFrame: eyeFrames) {

                if (eyeFrame.FrameBuffer != VK_NULL_HANDLE) {
                    vkDestroyFramebuffer(_device, eyeFrame.FrameBuffer, nullptr);
                    eyeFrame.FrameBuffer = VK_NULL_HANDLE;
                }

                if (eyeFrame.ImageView != VK_NULL_HANDLE) {
                    vkDestroyImageView(_device, eyeFrame.ImageView, nullptr);
                    eyeFrame.ImageView = VK_NULL_HANDLE;
                }

                if (eyeFrame.DepthImageView != VK_NULL_HANDLE) {
                    vkDestroyImageView(_device, eyeFrame.DepthImageView, nullptr);
                    eyeFrame.DepthImageView = VK_NULL_HANDLE;
                }

                // Destroy command buffers and pools
                if (eyeFrame.MainCommandBuffer != VK_NULL_HANDLE) {
                    vkFreeCommandBuffers(_device, eyeFrame.CommandPool, 1, &eyeFrame.MainCommandBuffer);
                    vkDestroyCommandPool(_device, eyeFrame.CommandPool, nullptr);

                    eyeFrame.MainCommandBuffer = VK_NULL_HANDLE;
                    eyeFrame.CommandPool = VK_NULL_HANDLE;
                }

                if (eyeFrame.DepthImage != VK_NULL_HANDLE) {
                    vmaDestroyImage(_allocator, eyeFrame.DepthImage, eyeFrame.DepthAllocation);
                    eyeFrame.DepthImage = VK_NULL_HANDLE;
                }
            }
        }

        for (auto& framebuffer: _framebuffers) {
            vkDestroyFramebuffer(_device, framebuffer, nullptr);
            framebuffer = VK_NULL_HANDLE;
        }

        for (auto& frame : _frames) {
            vkFreeCommandBuffers(_device, frame.CommandPool, 1, &frame.MainCommandBuffer);
            frame.MainCommandBuffer = VK_NULL_HANDLE;
        }

        if (_vrRenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(_device, _vrRenderPass, nullptr);
            _vrRenderPass = VK_NULL_HANDLE;
        }

        if (_renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(_device, _renderPass, nullptr);
            _renderPass = VK_NULL_HANDLE;
        }

        for (auto& imageView: _swapchainImageViews) {
            vkDestroyImageView(_device, imageView, nullptr);
            imageView = VK_NULL_HANDLE;
        }

        vkDestroyImageView(_device, _depthImageView, nullptr);
        _depthImageView = VK_NULL_HANDLE;
        vmaDestroyImage(_allocator, _depthImage, _depthImageAllocation);
        _depthImage = VK_NULL_HANDLE;
    }

    void VulkanRenderer::cleanResources() {
        cleanupSwapchain();

        _descriptorSetManager.Shutdown();

        vkDestroySwapchainKHR(_device, _swapchain, nullptr);
        _swapchain = VK_NULL_HANDLE;

        if (_rendererSettings.VR) {
            for (auto &eye : _vrFrames) {
                for (auto &frame : eye) {
                    frame.CameraData.reset();
                }
            }
        }

        for (auto& frame : _frames) {
            frame.CameraData.reset();
            vkDestroySemaphore(_device, frame.PresentSemaphore, nullptr);
            frame.PresentSemaphore = VK_NULL_HANDLE;
            vkDestroySemaphore(_device, frame.RenderSemaphore, nullptr);
            frame.RenderSemaphore = VK_NULL_HANDLE;
            vkDestroyFence(_device, frame.RenderFence, nullptr);
            frame.RenderFence = VK_NULL_HANDLE;
            vkDestroyCommandPool(_device, frame.CommandPool, nullptr);
            frame.CommandPool = VK_NULL_HANDLE;
        }

        vkDestroyCommandPool(_device, _bufferCommandPool, nullptr);

        auto* resourceManager = ServiceLocator::GetResourceManager();
        // Clear Resources
        if (resourceManager) {
            resourceManager->ClearGPUResourcesForReset();
        }

        vmaDestroyAllocator(_allocator);
        _allocator = VK_NULL_HANDLE;
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
//        createWindowSwapchain();

        if (_rendererSettings.VR) {
            createVRSwapchain();
        }
    }


    void VulkanRenderer::createFrameData() {
        if (_rendererSettings.VR) {
            createVRFrameData();
        }
    }

    void VulkanRenderer::createCommands() {
        createBufferCommands();
//        createWindowCommands();

        if (_rendererSettings.VR) {
            createVRCommands();
        }
    }

    void VulkanRenderer::createDescriptorPools() {}

    void VulkanRenderer::createDefaultRenderPass() {
        VkAttachmentDescription colorAttachment{
                .format = _swapchainImageFormat,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
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

        VK_CHECK("VulkanRenderer::createDefaultRenderPass()::vkCreateRenderPass", vkCreateRenderPass(_device, &renderPassCreateInfo, nullptr, &_renderPass));
    }

    void VulkanRenderer::createVRRenderPass(VkFormat format) {
        if (_vrRenderPass != VK_NULL_HANDLE) return;

        VkAttachmentDescription attachment{};
        attachment.format = VK_FORMAT_R8G8B8A8_SRGB;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference attachmentRef{};
        attachmentRef.attachment = 0;
        attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &attachmentRef;

        VkRenderPassCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        createInfo.flags = 0;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = &attachment;
        createInfo.subpassCount = 1;
        createInfo.pSubpasses = &subpass;

        VkResult result = vkCreateRenderPass(_device, &createInfo, nullptr, &_vrRenderPass);

        if (result != VK_SUCCESS)
        {
            cerr << "Failed to create Vulkan render pass: " << result << endl;
        }
    }

    void VulkanRenderer::createFramebuffers() {
//        createWindowFramebuffers();

        if (_rendererSettings.VR) {
            createVRFramebuffers();
        }
    }

    void VulkanRenderer::createSyncStructures() {
        VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkSemaphoreCreateInfo semaphoreCreateInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

        for (auto& frame : _frames) {
            VK_CHECK("VulkanRenderer::createSyncStructures()::vkCreateFence", vkCreateFence(_device, &fenceCreateInfo, nullptr, &frame.RenderFence));

            VK_CHECK("VulkanRenderer::createSyncStructures()::vkCreateSemaphore1", vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &frame.PresentSemaphore));
            VK_CHECK("VulkanRenderer::createSyncStructures()::vkCreateSemaphore2", vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &frame.RenderSemaphore));
        }
    }

    void VulkanRenderer::createWindowSwapchain() {

        VkSwapchainKHR oldSwapchain = _swapchain;

        auto[width, height] = ServiceLocator::GetWindow()->GetWindowExtents();
        _windowExtent.width = width;
        _windowExtent.height = height;

        vkb::SwapchainBuilder swapchainBuilder{_physicalDevice, _device, _surface};

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

        VK_CHECK("VulkanRenderer::createSwapchain()::vkCreateImageView", vkCreateImageView(_device, &depthImageViewCreateInfo, nullptr, &_depthImageView));
    }

    void VulkanRenderer::createVRSwapchain() {
        if (_rendererSettings.VR) {
            auto *vr = ServiceLocator::GetVRSubsystem();
            if (!vr || !vr->IsInitialized()) {
                std::cerr << "VR not initialized on renderer start up, disabling VR." << std::endl;
                _rendererSettings.VR = false;
            } else if (vr->GetBackendType() == VRBackend::OpenXR) {
                auto *xr = dynamic_cast<OpenXRSubsystem *>(vr);
                xr->createSwapchains();
                createVRFrameData();

                size_t eyeIndex { 0 };
                for (auto& eyeBuffers : _vrFrames) {
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

                        VK_CHECK("Failed to create Vulkan VR image view", vkCreateImageView(_device, &imageViewCreateInfo, nullptr, &frame.ImageView));

//                        // Create depth image
//                        VkExtent3D depthImageExtent {
//                                .width = swapchain.Width,
//                                .height = swapchain.Height,
//                                .depth = 1
//                        };
//
//                        _depthFormat = VK_FORMAT_D32_SFLOAT;
//
//                        VkImageCreateInfo depthCreateInfo {
//                                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
//                                .imageType = VK_IMAGE_TYPE_2D,
//                                .format = _depthFormat,
//                                .extent = depthImageExtent,
//                                .mipLevels = 1,
//                                .arrayLayers = 1,
//                                .samples = VK_SAMPLE_COUNT_1_BIT,
//                                .tiling = VK_IMAGE_TILING_OPTIMAL,
//                                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
//                        };
//
//                        VmaAllocationCreateInfo depthImageCreateInfo {
//                                .usage = VMA_MEMORY_USAGE_GPU_ONLY,
//                                .requiredFlags = VkMemoryPropertyFlags {VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT}
//                        };
//
//                        vmaCreateImage(_allocator, &depthCreateInfo, &depthImageCreateInfo,
//                                       &frame.DepthImage, &frame.DepthAllocation, nullptr);
//
//                        VkImageViewCreateInfo depthImageViewCreateInfo {
//                                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
//                                .image = frame.DepthImage,
//                                .viewType = VK_IMAGE_VIEW_TYPE_2D,
//                                .format = _depthFormat,
//                                .subresourceRange = {
//                                        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
//                                        .baseMipLevel = 0,
//                                        .levelCount = 1,
//                                        .baseArrayLayer = 0,
//                                        .layerCount = 1,
//                                }
//                        };
//
//                        VK_CHECK("VulkanRenderer::createSwapchain()::vkCreateImageView", vkCreateImageView(_device, &depthImageViewCreateInfo, nullptr, &frame.DepthImageView));

                        bufferDepth++;
                    }
                    eyeIndex++;
                }
            }
        }
    }

    void VulkanRenderer::createBufferCommands() {
        if (_bufferCommandPool == VK_NULL_HANDLE) {
            VkCommandPoolCreateInfo commandPoolCreateInfo = VulkanInitializers::CommandPoolCreateInfo(
                    _graphicsQueueFamily,
                    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
            VK_CHECK("VulkanRenderer::createCommands()::vkCreateCommandPool", vkCreateCommandPool(_device, &commandPoolCreateInfo, nullptr, &_bufferCommandPool));
        }
    }

    void VulkanRenderer::createWindowCommands() {
        for (auto& frame : _frames) {
            if (frame.CommandPool == VK_NULL_HANDLE) {
                VkCommandPoolCreateInfo commandPoolCreateInfo = VulkanInitializers::CommandPoolCreateInfo(
                        _graphicsQueueFamily,
                        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
                VK_CHECK("VulkanRenderer::createCommands()::vkCreateCommandPool", vkCreateCommandPool(_device, &commandPoolCreateInfo, nullptr, &frame.CommandPool));
            }

            VkCommandBufferAllocateInfo commandBufferAllocateInfo = VulkanInitializers::CommandBufferAllocateInfo(
                    frame.CommandPool);
            VK_CHECK("VulkanRenderer::createCommands()::vkAllocateCommandBuffers", vkAllocateCommandBuffers(_device, &commandBufferAllocateInfo, &frame.MainCommandBuffer));
        }
    }

    void VulkanRenderer::createVRCommands() {
        size_t eyeIndex { 0 };
        for (auto& eyeBuffers : _vrFrames) {
            size_t bufferDepth{0};
            for (auto &frame: eyeBuffers) {
                if (frame.CommandPool == VK_NULL_HANDLE) {
                    VkCommandPoolCreateInfo commandPoolCreateInfo = VulkanInitializers::CommandPoolCreateInfo(
                            _graphicsQueueFamily,
                            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
                    VK_CHECK("VulkanRenderer::createCommands()::vkCreateCommandPool", vkCreateCommandPool(_device, &commandPoolCreateInfo, nullptr, &frame.CommandPool));
                }

                VkCommandBufferAllocateInfo commandBufferAllocateInfo = VulkanInitializers::CommandBufferAllocateInfo(
                        frame.CommandPool);
                VK_CHECK("VulkanRenderer::createCommands()::vkAllocateCommandBuffers", vkAllocateCommandBuffers(_device, &commandBufferAllocateInfo, &frame.MainCommandBuffer));

                bufferDepth++;
            }
            eyeIndex++;
        }
    }

    void VulkanRenderer::createWindowFramebuffers() {
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
            VK_CHECK("VulkanRenderer::createFramebuffers()::vkCreateFramebuffer", vkCreateFramebuffer(_device, &framebufferCreateInfo, nullptr, &_framebuffers[i]));
        }
    }

    void VulkanRenderer::createVRFramebuffers() {
        if (_rendererSettings.VR) {
            auto *vr = ServiceLocator::GetVRSubsystem();
            if (!vr || !vr->IsInitialized()) {
                std::cerr << "VR not initialized on renderer start up, disabling VR." << std::endl;
                _rendererSettings.VR = false;
            } else if (vr->GetBackendType() == VRBackend::OpenXR) {
                auto *xr = dynamic_cast<OpenXRSubsystem *>(vr);

                size_t eyeIndex { 0 };
                for (auto& eyeBuffers : _vrFrames) {
                    size_t bufferDepth { 0 };
                    for (auto& frame : eyeBuffers) {
                        // get the swapchain information for this frame
                        auto& swapchain = xr->GetVulkanSwapchain(static_cast<int>(eyeIndex));

                        VkImageView attachments[2];
                        attachments[0] = frame.ImageView;
                        attachments[1] = frame.DepthImageView;

                        // create framebuffer
                        VkFramebufferCreateInfo framebufferCreateInfo { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
                        framebufferCreateInfo.renderPass = _vrRenderPass;
                        framebufferCreateInfo.attachmentCount = 1;
                        framebufferCreateInfo.pAttachments = &frame.ImageView;
                        framebufferCreateInfo.width = swapchain.Width;
                        framebufferCreateInfo.height = swapchain.Height;
                        framebufferCreateInfo.layers = 1;

                        VK_CHECK("Failed to create framebuffer for VR", vkCreateFramebuffer(_device, &framebufferCreateInfo, nullptr, &frame.FrameBuffer));

                        bufferDepth++;
                    }
                    eyeIndex++;
                }


            }
        }
    }

    void VulkanRenderer::createVRFrameData() {
        if (_rendererSettings.VR) {
            auto *vr = ServiceLocator::GetVRSubsystem();
            if (!vr || !vr->IsInitialized()) {
                std::cerr << "VR not initialized on renderer start up, disabling VR." << std::endl;
                _rendererSettings.VR = false;
            } else if (vr->GetBackendType() == VRBackend::OpenXR) {
                auto *xr = dynamic_cast<OpenXRSubsystem *>(vr);
                // We need to create the vr frame buffers, which require the frames. We can add the frames here now. To start,
                // we need to know both the eye count, and the bufferDepth of the VR swapchains

                auto bufferDepth = xr->GetVulkanBufferDepth();
                auto eyeCount = OZZ::OpenXRSubsystem::EyeCount;

                for (auto eyeIndex = 0; eyeIndex < eyeCount; eyeIndex++) {
                    // for each eye, reserve frame data with enough buffer depth
                    _vrFrames.emplace_back(bufferDepth);
                }
            }
        }
    }

    void VulkanRenderer::beginFrameWindow() {
        VK_CHECK("VulkanRenderer::BeginFrame()::vkWaitForFences", vkWaitForFences(_device, 1, &getCurrentFrame().RenderFence, true, 1000000000)); // 1
        VK_CHECK("VulkanRenderer::BeginFrame()::vkResetFences", vkResetFences(_device, 1, &getCurrentFrame().RenderFence));                     // 0

        _descriptorSetManager.NextDescriptorFrame();

        VkResult result = vkAcquireNextImageKHR(_device, _swapchain, 1000000000, getCurrentFrame().PresentSemaphore,
                                                VK_NULL_HANDLE, &getCurrentFrame().SwapchainImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR ) {
            recreateSwapchain();
            return;
        }

        VK_CHECK("VulkanRenderer::BeginFrame()::vkResetCommandBuffer", vkResetCommandBuffer(getCurrentFrame().MainCommandBuffer, 0));

        VkCommandBuffer cmd = getCurrentFrame().MainCommandBuffer;

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

        renderPassBeginInfo.framebuffer = _framebuffers[getCurrentFrame().SwapchainImageIndex];
        // connect clear values
        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    std::vector<EyePoseInfo> VulkanRenderer::beginFrameVR() {
        if (_rendererSettings.VR) {
            auto* vr = ServiceLocator::GetVRSubsystem();
            if (!vr || !vr->IsInitialized()) {
                std::cerr << "VR not initialized on renderer start up, disabling VR." << std::endl;
                _rendererSettings.VR = false;
                return {};
            } else if (vr->GetBackendType() == VRBackend::OpenXR) {
                _descriptorSetManager.NextDescriptorFrame();

                auto *xr = dynamic_cast<OpenXRSubsystem *>(vr);

                auto [frameWait, frameState] = xr->WaitForNextVulkanFrame();
                if (!frameState.shouldRender) {
                    return {};
                }

                return xr->BeginVulkanFrame(frameWait, frameState);
            }
        }

        return {};
    }

    void VulkanRenderer::renderFrameWindow(const SceneParams& sceneParams, const std::vector<RenderableObject>& objects) {
        auto& currentFrame = getCurrentFrame();

        // Ensure there's a uniform buffer to hold camera data
        if (!currentFrame.CameraData) {
            currentFrame.CameraData = CreateUniformBuffer();
        }

        // Upload the camera data to the buffer
        // Usually I would avoid casting away the const -- but I did it here to save effort in making overloads
        currentFrame.CameraData->UploadData(const_cast<int*>(reinterpret_cast<const int*>(&sceneParams.Camera)), sizeof(sceneParams.Camera));

        renderObjects(currentFrame.CommandPool, currentFrame.MainCommandBuffer, currentFrame.CameraData, objects);
    }

    void VulkanRenderer::renderFrameVR(const std::vector<EyePoseInfo>& eyeInfo, SceneParams& sceneParams, const std::vector<RenderableObject>& objects) {
        uint32_t currentEye { 0 };
        for (auto eye : eyeInfo) {

            float angleWidth = tan(eye.FOV.AngleRight) - tan(eye.FOV.AngleLeft);
            float angleHeight = tan(eye.FOV.AngleDown) - tan(eye.FOV.AngleUp);
            const float farDistance = 0.01;
            const float nearDistance = 1000;

            // build projection matrix
            sceneParams.Camera.Projection = glm::mat4{1.f};
            sceneParams.Camera.Projection[0][0] = 2.0f / angleWidth;
            sceneParams.Camera.Projection[2][0] = (tan(eye.FOV.AngleRight) + tan(eye.FOV.AngleLeft)) / angleWidth;
            sceneParams.Camera.Projection[1][1] = 2.0f / angleHeight;
            sceneParams.Camera.Projection[2][1] = (tan(eye.FOV.AngleUp) + tan(eye.FOV.AngleDown)) / angleHeight;
            sceneParams.Camera.Projection[2][2] = -farDistance / (farDistance - nearDistance);
            sceneParams.Camera.Projection[3][2] = -(farDistance * nearDistance) / (farDistance - nearDistance);
            sceneParams.Camera.Projection[2][3] = -1;

            sceneParams.Camera.View = glm::inverse(
                glm::translate(glm::mat4(1.0f), eye.Position)
                * glm::mat4_cast(eye.Orientation)
            );

            renderEye(currentEye, sceneParams, objects);
            currentEye++;
        }
    }

    void VulkanRenderer::renderEye(uint32_t eyeIndex, const SceneParams &sceneParams,
                                   const vector<RenderableObject> &objects) {
        if (_rendererSettings.VR) {
            auto* vr = ServiceLocator::GetVRSubsystem();
            if (!vr || !vr->IsInitialized()) {
                std::cerr << "VR not initialized on renderer start up, disabling VR." << std::endl;
                _rendererSettings.VR = false;
                return;
            } else if (vr->GetBackendType() == VRBackend::OpenXR) {
                auto *xr = dynamic_cast<OpenXRSubsystem *>(vr);

                uint32_t imageIndex = xr->AcquireVulkanSwapchainImage(eyeIndex);
                auto& swapchain = xr->GetVulkanSwapchain(static_cast<int>(eyeIndex));

                auto& vrFrame = _vrFrames[eyeIndex][imageIndex];

                // Ensure there's a uniform buffer to hold camera data
                if (!vrFrame.CameraData) {
                    vrFrame.CameraData = CreateUniformBuffer();
                }

                // Upload the camera data to the buffer
                // Usually I would avoid casting away the const -- but I did it here to save effort in making overloads
                vrFrame.CameraData->UploadData(const_cast<int*>(reinterpret_cast<const int*>(&sceneParams.Camera)), sizeof(sceneParams.Camera));

                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

                auto vkResult = vkBeginCommandBuffer(vrFrame.MainCommandBuffer, &beginInfo);

                if (vkResult != VK_SUCCESS) {
                    std::cout << "Failed to begin command buffer! " << vkResult << std::endl;
                    return;
                }

                VkClearValue clearValue{};
                clearValue.color = { { 0.2f, 0.2f, 0.2f, 1.0f } };

                VkRenderPassBeginInfo beginRenderPassInfo{};
                beginRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                beginRenderPassInfo.renderPass = _vrRenderPass;
                beginRenderPassInfo.framebuffer = vrFrame.FrameBuffer;
                beginRenderPassInfo.renderArea = {
                        { 0, 0 },
                        { (uint32_t)swapchain.Width, (uint32_t)swapchain.Height }
                };
                beginRenderPassInfo.clearValueCount = 1;
                beginRenderPassInfo.pClearValues = &clearValue;


                vkCmdBeginRenderPass(vrFrame.MainCommandBuffer, &beginRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                renderObjects(vrFrame.CommandPool, vrFrame.MainCommandBuffer, vrFrame.CameraData, objects);

                vkCmdEndRenderPass(vrFrame.MainCommandBuffer);

                vkResult = vkEndCommandBuffer(vrFrame.MainCommandBuffer);

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
                submitInfo.pCommandBuffers = &vrFrame.MainCommandBuffer;


                vkResult = vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

                if (vkResult != VK_SUCCESS)
                {
                    cerr << "Failed to submit Vulkan command buffer: " << vkResult << endl;
                }

                xr->ReleaseVulkanSwapchainImage(static_cast<int>(eyeIndex));
            }
        }
    }

    void VulkanRenderer::endFrameWindow() {
        auto cmd = getCurrentFrame().MainCommandBuffer;
        vkCmdEndRenderPass(cmd);
        VK_CHECK("VulkanRenderer::EndFrame()::vkEndCommandBuffer", vkEndCommandBuffer(cmd));

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
            VK_CHECK("VulkanRenderer::EndFrame()::vkQueuePresentKHR", result);
        }
    }

    void VulkanRenderer::endFrameVR(const std::vector<EyePoseInfo>& eyePoses) {
        if (_rendererSettings.VR) {
            auto *vr = ServiceLocator::GetVRSubsystem();
            if (!vr || !vr->IsInitialized()) {
                std::cerr << "VR not initialized on renderer start up, disabling VR." << std::endl;
                _rendererSettings.VR = false;
                return;
            } else if (vr->GetBackendType() == VRBackend::OpenXR) {
                auto *xr = dynamic_cast<OpenXRSubsystem *>(vr);
                xr->EndVulkanFrame(eyePoses);
            }
        }
    }


    void VulkanRenderer::renderObjects(VkCommandPool commandPool, VkCommandBuffer commandBuffer, std::shared_ptr<UniformBuffer> cameraBuffer, const std::vector<RenderableObject>& objects) {
// Render all the objects
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

                    std::map<int, VkDescriptorSet> descriptorSets {};
                    auto shaderData = shader->GetShaderData();

                    for (auto& [resourceName, resource] : shaderData.Resources) {
                        if (!descriptorSets.contains(resource.Set)) {
                            auto descriptorSetLayout = dynamic_cast<VulkanShader *>(shader.get())->GetDescriptorSetLayout(resource.Set);
                            auto descriptorSet = _descriptorSetManager.GetDescriptorSet(descriptorSetLayout);

                            descriptorSets[resource.Set] = descriptorSet;
                        }
                    }


                    if (shaderData.Resources.contains(ResourceName::CameraData)) {
                        auto cameraData = shaderData.Resources[ResourceName::CameraData];
                        auto *buffer = dynamic_cast<VulkanUniformBuffer *>(cameraBuffer.get());

                        VkDescriptorBufferInfo descriptorBufferInfo{};
                        descriptorBufferInfo.buffer = buffer->_buffer->Buffer;
                        descriptorBufferInfo.offset = 0;
                        descriptorBufferInfo.range = buffer->_bufferSize;

                        auto writeSet = VulkanUtilities::WriteDescriptorSetUniformBuffer(descriptorSets[cameraData.Set],
                                                                                         cameraData.Binding,
                                                                                         &descriptorBufferInfo);
                        writeSets.push_back(writeSet);
                    }

                    // Bind all textures
                    for (int i = (int)ResourceName::Diffuse0; i < (int)ResourceName::EndTextures; i++) {
                        if (shader->GetShaderData().Resources.contains((ResourceName)i)) {
                            auto textureData = shader->GetShaderData().Resources.at((ResourceName)i);

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

                            writeSets.push_back(VulkanUtilities::WriteDescriptorSetTexture(
                                    descriptorSets[textureData.Set], textureData.Binding, &imageBufferInfo));
                        }
                    }

                    if (!writeSets.empty()) {
                        vkUpdateDescriptorSets(_device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0,
                                               nullptr);
                    }

                    // Bind and draw the things
                    shader->Bind(commandBuffer);

                    for (auto& [set, descriptorSetCollection] : descriptorSets) {
                        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                dynamic_cast<VulkanShader *>(shader.get())->GetPipelineLayout(),
                                                set, 1,
                                                &descriptorSetCollection, 0,
                                                nullptr);
                    }

                    submesh._indexBuffer->Bind(commandBuffer);
                    submesh._vertexBuffer->Bind(commandBuffer);

                    vkCmdPushConstants(commandBuffer,
                                       dynamic_cast<VulkanShader *>(shader.get())->GetPipelineLayout(),
                                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelObject), &object.Transform);

                    vkCmdDrawIndexed(commandBuffer, submesh._indexBuffer->GetCount(), 1, 0, 0, 0);
                }
            }
        }
    }

    VkPhysicalDevice VulkanRenderer::getPhysicalDevice() {
        VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};

        if (_rendererSettings.VR) {
            auto* vr = ServiceLocator::GetVRSubsystem();
            if (!vr || !vr->IsInitialized()) {
                std::cerr << "VR not initialized on renderer start up, disabling VR." << std::endl;
                _rendererSettings.VR = false;
            } else if (vr->GetBackendType() == VRBackend::OpenXR) {
                auto *xr = dynamic_cast<OpenXRSubsystem *>(vr);
                physicalDevice = xr->GetVulkanPhysicalDevice(_instance);
            }
        } else {
            uint32_t deviceCount = 0;

            vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
            if (deviceCount == 0) {
                std::cerr << "Failed to find GPUS with Vulkan support!" << std::endl;
                return VK_NULL_HANDLE;
            }

            std::vector<VkPhysicalDevice> devices(deviceCount);
            vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

            auto isDeviceSuitable = [this](VkPhysicalDevice device) {
                VkPhysicalDeviceProperties deviceProperties;
                VkPhysicalDeviceFeatures deviceFeatures;
                vkGetPhysicalDeviceProperties(device, &deviceProperties);
                vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

                auto queueFamilies = getQueueFamilyIndices(device);
                return queueFamilies.IsComplete();
            };

            for (const auto &device: devices) {
                if (isDeviceSuitable(device)) {
                    physicalDevice = device;
                    break;
                }
            }

            if (physicalDevice == VK_NULL_HANDLE) {
                std::cerr << "Failed to find GPU with required extensions!" << std::endl;
            }
        }
        return physicalDevice;
    }

    std::tuple<VkDevice, VulkanQueueFamilyIndices> VulkanRenderer::createLogicalDevice(VkPhysicalDevice device, const std::set<std::string>& deviceExtensions) {
        VkDevice logicalDevice { VK_NULL_HANDLE };
        auto queueFamilies = getQueueFamilyIndices(device);

        float queuePriority = 1.f;
        VkDeviceQueueCreateInfo queueCreateInfo { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
        queueCreateInfo.queueFamilyIndex = queueFamilies.GraphicsFamily.value();
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkDeviceCreateInfo createInfo { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;

        std::vector<const char*> dExtensions {};

        for (const auto& extension: deviceExtensions) {
            dExtensions.emplace_back(extension.c_str());
        }
        createInfo.enabledExtensionCount = dExtensions.size();
        createInfo.ppEnabledExtensionNames = dExtensions.data();

        createInfo.enabledLayerCount = 0;

        if (vkCreateDevice(device, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
            std::cerr << "Failed to create vulkan logical device!" << std::endl;
            return {VK_NULL_HANDLE, {}};
        }

        return {logicalDevice, queueFamilies};
    }

    VulkanQueueFamilyIndices VulkanRenderer::getQueueFamilyIndices(VkPhysicalDevice device) {
        VulkanQueueFamilyIndices indices {};

        uint32_t queueFamilyCount { 0 };
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i { 0 };
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.GraphicsFamily = i;
            }

            if (indices.IsComplete()) break;
            i++;
        }
        return indices;
    }

    FrameData &VulkanRenderer::getCurrentFrame() {
        return _frames[getCurrentFrameNumber()];
    }

    uint32_t VulkanRenderer::getCurrentFrameNumber() const {
        return _frameNumber % MAX_FRAMES_IN_FLIGHT;
    }
}