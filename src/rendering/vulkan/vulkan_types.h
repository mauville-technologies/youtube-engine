//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once
#include <youtube_engine/rendering/types.h>
#include <vulkan/vulkan.h>
#include <cstddef>

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
}