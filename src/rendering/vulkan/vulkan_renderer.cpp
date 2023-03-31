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
#include <rendering/vulkan/renderer_extensions/vulkan_vr_renderer_extension.h>
#include <rendering/vulkan/renderer_extensions/vulkan_window_renderer_extension.h>

#include "vulkan_initializers.h"
#include "vulkan_shader.h"
#include "vulkan_buffer.h"
#include "vulkan_texture.h"
#include "vulkan_utilities.h"

namespace OZZ {
    void VulkanRenderer::Init() {
        if (_initialized) return;
        initCore() ;
        createBufferCommands();

        if (_rendererSettings.VR) {
            _rendererExtension = std::make_unique<VulkanVRRendererExtension>(this);
        } else {
            _rendererExtension = std::make_unique<VulkanWindowRendererExtension>(this);
        }

        _initialized = true;
    }

    void VulkanRenderer::Shutdown() {
        if (!_initialized) return;
        WaitForIdle();
        cleanResources();

        vkDestroyDevice(_device, nullptr);
        _device = VK_NULL_HANDLE;
        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
        vkDestroyInstance(_instance, nullptr);
        _instance = VK_NULL_HANDLE;
        _initialized = false;
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
        _rendererExtension->RenderFrame(sceneParams, objects);
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
                vulkanVersion = static_cast<uint32_t>(graphicsRequirements.minApiVersionSupported);
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

    void VulkanRenderer::cleanResources() {
        _descriptorSetManager.Shutdown();

        _rendererExtension.reset();

        if (_bufferCommandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(_device, _bufferCommandPool, nullptr);
            _bufferCommandPool = VK_NULL_HANDLE;
        }

        auto* resourceManager = ServiceLocator::GetResourceManager();
        // Clear Resources
        if (resourceManager) {
            resourceManager->ClearGPUResourcesForReset();
        }

        vmaDestroyAllocator(_allocator);
        _allocator = VK_NULL_HANDLE;
    }

    void VulkanRenderer::createBufferCommands() {
        if (_bufferCommandPool == VK_NULL_HANDLE) {
            VkCommandPoolCreateInfo commandPoolCreateInfo = VulkanInitializers::CommandPoolCreateInfo(
                    _graphicsQueueFamily,
                    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
            VK_CHECK("VulkanRenderer::createCommands()::vkCreateCommandPool", vkCreateCommandPool(_device, &commandPoolCreateInfo, nullptr, &_bufferCommandPool));
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
        createInfo.enabledExtensionCount = static_cast<uint32_t>(dExtensions.size());
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

    VkRenderPass VulkanRenderer::GetActiveRenderPass() {
        if (_rendererExtension) return _rendererExtension->GetRenderPass();

        return VK_NULL_HANDLE;
    }
}