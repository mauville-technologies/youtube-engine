//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <youtube_engine/rendering/types.h>

namespace OZZ {
    struct VertexInputDescription {
        std::vector<VkVertexInputBindingDescription> Bindings {};
        std::vector<VkVertexInputAttributeDescription> Attributes{};

        VkPipelineVertexInputStateCreateFlags Flags = 0;
    };

    inline VertexInputDescription GetVertexInputDescription() {
        VertexInputDescription vertexInputDescription {};

        VkVertexInputBindingDescription binding {};
        binding.binding = 0;
        binding.stride = sizeof(Vertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        vertexInputDescription.Bindings.push_back(binding);

        vertexInputDescription.Attributes.push_back({
           .location = 0,
           .binding = 0,
           .format = VK_FORMAT_R32G32B32_SFLOAT,
           .offset = static_cast<uint32_t>(offsetof(Vertex, position))
        });

        vertexInputDescription.Attributes.push_back({
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = static_cast<uint32_t>(offsetof(Vertex, color))
        });

        vertexInputDescription.Attributes.push_back({
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = static_cast<uint32_t>(offsetof(Vertex, uv))
        });

        vertexInputDescription.Attributes.push_back({
            .location = 3,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = static_cast<uint32_t>(offsetof(Vertex, normal))
        });

        return vertexInputDescription;
    }
}