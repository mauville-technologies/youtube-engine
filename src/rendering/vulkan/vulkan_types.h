//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once
#include <youtube_engine/rendering/types.h>
#include <cstddef>
#include "vulkan_includes.h"

namespace OZZ {
    struct VertexInputDescription {
        std::vector<VkVertexInputBindingDescription> bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;

        VkPipelineVertexInputStateCreateFlags flags = 0;
    };

    inline VertexInputDescription GetVertexDescription() {
        VertexInputDescription description{};

        // 1 vertex buffer binding
        VkVertexInputBindingDescription mainBinding {};
        mainBinding.binding = 0;
        mainBinding.stride = sizeof(Vertex);
        mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        description.bindings.push_back(mainBinding);


        // Vertex attributes
        // Position
        description.attributes.push_back({
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = static_cast<uint32_t>(offsetof(Vertex, position))
        });

        // Color
        description.attributes.push_back({
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = static_cast<uint32_t>(offsetof(Vertex, color))
        });

        // uv
        description.attributes.push_back({
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = static_cast<uint32_t>(offsetof(Vertex, uv))
        });

        // normal
        description.attributes.push_back({
            .location = 3,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = static_cast<uint32_t>(offsetof(Vertex, normal))
        });

        return description;
    }


    /*
     * UNIFORM BUFFER THINGS
     * TODO: This needs to be abstracted
     */
    inline VkDescriptorSetLayoutBinding GetUniformBufferLayoutBinding(int binding) {
        VkDescriptorSetLayoutBinding uboLayoutBinding {};
        uboLayoutBinding.binding = binding;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1; // could be an array?
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;  // Likely for textures

        return uboLayoutBinding;
    }

    inline VkDescriptorSetLayoutBinding GetTextureLayoutBinding(int binding) {
        VkDescriptorSetLayoutBinding textureLayoutBinding {};
        textureLayoutBinding.binding = binding;
        textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureLayoutBinding.descriptorCount = 1; // could be an array?
        textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        textureLayoutBinding.pImmutableSamplers = nullptr;  // Likely for textures

        return textureLayoutBinding;
    }

    inline VkDescriptorSetLayoutCreateInfo BuildDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings) {
        VkDescriptorSetLayoutCreateInfo vkDescriptorSetLayoutCreateInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        vkDescriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        vkDescriptorSetLayoutCreateInfo.pBindings = bindings.data();

        return vkDescriptorSetLayoutCreateInfo;
    };

    inline VkFormat ColorTypeToVulkanFormatType(ColorType colorType) {
        switch(colorType) {
            case ColorType::FLOAT:
                return VK_FORMAT_R32G32B32_SFLOAT;
            case ColorType::UNSIGNED_CHAR4:
                return VK_FORMAT_R8G8B8A8_SRGB;
            default:
                return VK_FORMAT_R8G8B8_SRGB;
        }
    }
}