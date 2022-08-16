//
// Created by Paul Mauviel on 2022-02-18.
//

#include "vulkan_shader.h"
#include "vulkan_utilities.h"
#include "vulkan_initializers.h"
#include "vulkan_pipeline_builder.h"
#include "vulkan_types.h"

namespace OZZ {
    VulkanShader::VulkanShader(VulkanRenderer *renderer)  : _renderer(renderer) {}

    VulkanShader::~VulkanShader() {
        cleanPipelineObjects();
    }

    void VulkanShader::Bind() {
        vkCmdBindPipeline(_renderer->_mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
    }

    void VulkanShader::Load(const std::string &&vertexShader, const std::string &&fragmentShader) {
        cleanPipelineObjects();

        _vertexShaderLoc = vertexShader;
        _fragmentShaderLoc = fragmentShader;

        VkShaderModule vertShader;
        if (!VulkanUtilities::LoadShaderModule(vertexShader, _renderer->_device, vertShader)) {
            std::cout << "Failed to load vertex shader module\n";
        }

        VkShaderModule fragShader;
        if (!VulkanUtilities::LoadShaderModule(fragmentShader, _renderer->_device, fragShader)) {
            std::cout << "Failed to load fragment shader module\n";
        }

        auto pipelineLayoutInfo = VulkanInitializers::PipelineLayoutCreateInfo();
        VK_CHECK(vkCreatePipelineLayout(_renderer->_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout));

        VulkanPipelineBuilder pipelineBuilder;
        pipelineBuilder._shaderStages.push_back(
                VulkanInitializers::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShader));
        pipelineBuilder._shaderStages.push_back(
                VulkanInitializers::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShader));

        auto vertexBindings = GetVertexInputDescription();

        pipelineBuilder._vertexInputInfo = VulkanInitializers::PipelineVertexInputStateCreateInfo();
        pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexBindings.Bindings.size();
        pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexBindings.Bindings.data();
        pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexBindings.Attributes.size();
        pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexBindings.Attributes.data();

        pipelineBuilder._inputAssembly = VulkanInitializers::PipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        // build the viewport
        pipelineBuilder._viewport = {
                .x = 0.f,
                .y = 0.f,
                .width = static_cast<float>(_renderer->_windowExtent.width),
                .height = static_cast<float>(_renderer->_windowExtent.height),
                .minDepth = 0.f,
                .maxDepth = 1.f
        };

        pipelineBuilder._scissor = {
                .offset = {0 , 0},
                .extent = _renderer->_windowExtent
        };

        pipelineBuilder._rasterizer = VulkanInitializers::PipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
        pipelineBuilder._multisampling = VulkanInitializers::PipelineMultisampleStateCreateInfo();
        pipelineBuilder._colorBlendAttachment = VulkanInitializers::PipelineColorBlendAttachmentState();
        pipelineBuilder._pipelineLayout = _pipelineLayout;

        _pipeline = pipelineBuilder.BuildPipeline(_renderer->_device, _renderer->_renderPass);

        vkDestroyShaderModule(_renderer->_device, fragShader, nullptr);
        vkDestroyShaderModule(_renderer->_device, vertShader, nullptr);
    }

    void VulkanShader::Rebuild() {
        Load(std::move(_vertexShaderLoc), std::move(_fragmentShaderLoc));
    }

    void VulkanShader::cleanPipelineObjects() {
        if (_pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(_renderer->_device, _pipeline, nullptr);
            _pipeline = VK_NULL_HANDLE;
        }

        if (_pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(_renderer->_device, _pipelineLayout, nullptr);
            _pipelineLayout = VK_NULL_HANDLE;
        }
    }
}