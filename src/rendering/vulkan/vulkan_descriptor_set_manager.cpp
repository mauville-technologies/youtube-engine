//
// Created by ozzadar on 2022-11-29.
//

#include "vulkan_descriptor_set_manager.h"
#include "vulkan_utilities.h"

namespace OZZ {
    VulkanDescriptorSetManager::VulkanDescriptorSetManager(VkDevice* device) :
            _device{device}, _descriptorCache(MAX_DESCRIPTOR_FRAMES), _descriptorPools(MAX_DESCRIPTOR_FRAMES) {

        for (auto i = 0; i < MAX_DESCRIPTOR_FRAMES; i++) {
            _descriptorCache.emplace_back();

            VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
            descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            descriptorPoolCreateInfo.maxSets = 2000;
            descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(POOL_SIZES.size());
            descriptorPoolCreateInfo.pPoolSizes = POOL_SIZES.data();

            VK_CHECK("VulkanDescriptorSetManager::Constructor", vkCreateDescriptorPool(*_device, &descriptorPoolCreateInfo, nullptr, &_descriptorPools[i]));
        }
    }

    VkDescriptorSet VulkanDescriptorSetManager::GetDescriptorSet(VkDescriptorSetLayout layout) {
        auto frameCache = _descriptorCache[_currentDescriptorFrame];
        auto pipelineCache = frameCache[layout];
        VkDescriptorSet descriptorSet{VK_NULL_HANDLE};

        VkDescriptorSetAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocateInfo.descriptorPool = _descriptorPools[_currentDescriptorFrame];
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &layout;
        VK_CHECK("VulkanDescriptorSetManager::GetDescriptorSet", vkAllocateDescriptorSets(*_device, &allocateInfo, &descriptorSet));

        pipelineCache.push_back(descriptorSet);


        return descriptorSet;
    }

    void VulkanDescriptorSetManager::NextDescriptorFrame() {
        _currentDescriptorFrame++;

        if (_currentDescriptorFrame >= MAX_DESCRIPTOR_FRAMES) {
            _currentDescriptorFrame = 0;
        }

        auto nextDescriptorFrame = _currentDescriptorFrame + 1;

        if (nextDescriptorFrame >= MAX_DESCRIPTOR_FRAMES) {
            nextDescriptorFrame = 0;
        }

        // We assume that by the time we get back around the descriptor pools; they won't be in flight anymore.
        vkResetDescriptorPool(*_device, _descriptorPools[nextDescriptorFrame], 0);
        _descriptorCache[nextDescriptorFrame].clear();
    }

    VulkanDescriptorSetManager::~VulkanDescriptorSetManager() {
//        Shutdown();
    }

    void VulkanDescriptorSetManager::Shutdown() {
        for (auto &descriptorPool : _descriptorPools) {
            vkResetDescriptorPool(*_device, descriptorPool, 0);
            vkDestroyDescriptorPool(*_device, descriptorPool, nullptr);
        }

        _descriptorCache.clear();
        _descriptorPools.clear();
    }
}