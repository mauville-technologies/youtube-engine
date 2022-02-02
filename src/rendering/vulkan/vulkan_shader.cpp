//
// Created by ozzadar on 2022-02-01.
//

#include "vulkan_shader.h"
#include "vulkan_utilities.h"
#include "vulkan_initializers.h"
#include "vulkan_pipeline_builder.h"

namespace OZZ {
    VulkanShader::VulkanShader(VkRenderPass* renderPass, VkDevice* device, VkExtent2D* windowExtent) :
        _device{ device }, _renderPass { renderPass }, _windowExtent{ windowExtent } {}


    VulkanShader::~VulkanShader() {
        std::cout << "Destroying Shader" << std::endl;
        vkDestroyPipeline(*_device, _pipeline, nullptr);
        vkDestroyPipelineLayout(*_device, _pipelineLayout, nullptr);

        _device = nullptr;
        _windowExtent = nullptr;
        _renderPass = nullptr;
    }

    void VulkanShader::Bind(uint64_t commandHandle) {
        vkCmdBindPipeline(VkCommandBuffer(commandHandle), VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
    }

    void VulkanShader::Load(const std::string&& vertexShader, const std::string&& fragmentShader) {
        VkShaderModule fragmentShaderModule;
        if (!VulkanUtilities::LoadShaderModule(fragmentShader, *_device, fragmentShaderModule)) {
            std::cout << "Failed to load triangle fragment shader module\n";
        } else {
            std::cout << "Successfully loaded triangle fragment shader module\n";
        }

        VkShaderModule vertexShaderModule;
        if (!VulkanUtilities::LoadShaderModule(vertexShader, *_device, vertexShaderModule)) {
            std::cout << "Failed to load triangle vertex shader module\n";
        } else {
            std::cout << "Successfully loaded triangle vertex shader module\n";
        }

        auto pipelineLayoutInfo = VulkanInitializers::PipelineLayoutCreateInfo();
        VK_CHECK(vkCreatePipelineLayout(*_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout));

        /*
         * TEMPORARY PIPELINE BUILDING
         */

        VulkanPipelineBuilder pipelineBuilder;
        pipelineBuilder._shaderStages.push_back(
                VulkanInitializers::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule));
        pipelineBuilder._shaderStages.push_back(
                VulkanInitializers::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule));

        pipelineBuilder._vertexInputInfo = VulkanInitializers::PipelineVertexInputStateCreateInfo();
        pipelineBuilder._inputAssembly = VulkanInitializers::PipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        // build the viewport
        pipelineBuilder._viewport = {
                .x = 0.f,
                .y = 0.f,
                .width = static_cast<float>(_windowExtent->width),
                .height = static_cast<float>(_windowExtent->height),
                .minDepth = 0.f,
                .maxDepth = 1.f
        };

        pipelineBuilder._scissor = {
                .offset = {0 , 0},
                .extent = *_windowExtent
        };

        pipelineBuilder._rasterizer = VulkanInitializers::PipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
        pipelineBuilder._multisampling = VulkanInitializers::PipelineMultisampleStateCreateInfo();
        pipelineBuilder._colorBlendAttachment = VulkanInitializers::PipelineColorBlendAttachmentState();
        pipelineBuilder._pipelineLayout = _pipelineLayout;

        _pipeline = pipelineBuilder.BuildPipeline(*_device, *_renderPass);

        vkDestroyShaderModule(*_device, fragmentShaderModule, nullptr);
        vkDestroyShaderModule(*_device, vertexShaderModule, nullptr);
    }


}
