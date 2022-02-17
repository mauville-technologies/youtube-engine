//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once
#include "vulkan_includes.h"

namespace OZZ {
    class VulkanInitializers {
    public:
        static VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flag = 0);
        static VkCommandBufferAllocateInfo  CommandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1,
                VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

        // RENDER PASS INITIALIZERS

        static VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
        static VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo();
        static VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology);
        static VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo(VkPolygonMode polygonMode);
        static VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo();
        static VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState();
        static VkPipelineDepthStencilStateCreateInfo DepthStencilCreateInfo(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp);
        static VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo();
    };
}


