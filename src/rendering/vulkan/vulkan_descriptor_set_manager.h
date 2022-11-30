//
// Created by ozzadar on 2022-11-29.
//

#pragma once

#include "vulkan_includes.h"

#include <unordered_map>
#include <vector>
#include <array>

namespace OZZ {
    class VulkanDescriptorSetManager {
    public:
        VulkanDescriptorSetManager() = default;
        explicit VulkanDescriptorSetManager(VkDevice* device);

        ~VulkanDescriptorSetManager();

        VkDescriptorSet GetDescriptorSet(VkDescriptorSetLayout layout);
        void NextDescriptorFrame();

        void Shutdown();
    private:
        static constexpr std::array<VkDescriptorPoolSize, 2> POOL_SIZES {
                VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 }
        };

        static constexpr uint8_t MAX_DESCRIPTOR_FRAMES = 3;

        uint8_t _currentDescriptorFrame { 0 };
        VkDevice* _device { VK_NULL_HANDLE };
        std::vector<VkDescriptorPool> _descriptorPools;
        std::vector<std::unordered_map<VkDescriptorSetLayout, std::vector<VkDescriptorSet>>> _descriptorCache;
    };

}

