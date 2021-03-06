//
// Created by ozzadar on 2021-07-11.
//

#pragma once
#include "vulkan_includes.h"
#include <vector>

namespace OZZ {
    class VulkanPipelineBuilder {
    public:
        std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
        VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
        VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
        VkViewport _viewport;
        VkRect2D _scissor;

        VkPipelineRasterizationStateCreateInfo _rasterizer;
        VkPipelineColorBlendAttachmentState _colorBlendAttachment;
        VkPipelineMultisampleStateCreateInfo _multisampling;
        VkPipelineLayout _pipelineLayout;
        VkPipelineDepthStencilStateCreateInfo _depthStencil;

        VkPipeline BuildPipeline(VkDevice device, VkRenderPass pass);
    };
}


