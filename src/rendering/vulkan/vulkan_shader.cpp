//
// Created by ozzadar on 2022-02-01.
//

#include "vulkan_shader.h"
#include "vulkan_utilities.h"
#include "vulkan_initializers.h"
#include "vulkan_pipeline_builder.h"
#include "vulkan_types.h"
#include "vulkan_renderer.h"
#include "vulkan_buffer.h"
#include "vulkan_texture.h"

namespace OZZ {
    VulkanShader::VulkanShader(VulkanRenderer* renderer) :
        _renderer( renderer ) {}


    VulkanShader::~VulkanShader() {
        cleanPipeline();
    }

    void VulkanShader::Rebuild() {
        cleanPipeline();
        Load(std::move(_vertexShader), std::move(_fragmentShader));
    }

    void VulkanShader::Bind(uint64_t commandHandle) {
        // Bind Uniforms
        for (auto& uniform : _uniformBuffers) {
            auto descriptorSet = dynamic_cast<VulkanUniformBuffer*>(uniform.get())->GetDescriptorSet(&_descriptorSetLayout);

            vkCmdBindDescriptorSets((VkCommandBuffer) commandHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout,
                                    0, 1, &descriptorSet, 0, nullptr);
        }

        for (auto& texture : _textures) {
            auto descriptorSet = dynamic_cast<VulkanTexture*>(texture.get())->GetDescriptorSet(&_textureSetLayout);

            vkCmdBindDescriptorSets((VkCommandBuffer) commandHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout,
                                    1, 1, &descriptorSet, 0, nullptr);
        }



        vkCmdBindPipeline(VkCommandBuffer(commandHandle), VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
    }


    void VulkanShader::AddUniformBuffer(std::shared_ptr<UniformBuffer> buffer) {
        _uniformBuffers.push_back(buffer);
    }


    void VulkanShader::AddTexture(std::shared_ptr<Texture> texture) {
        _textures.push_back(texture);
    }

    void VulkanShader::Load(const std::string&& vertexShader, const std::string&& fragmentShader) {
        _vertexShader = vertexShader;
        _fragmentShader = fragmentShader;

        VkShaderModule fragmentShaderModule;
        if (!VulkanUtilities::LoadShaderModule(fragmentShader, _renderer->_device, fragmentShaderModule)) {
            std::cout << "Failed to load fragment shader module at: " << _fragmentShader << "\n";
        }

        VkShaderModule vertexShaderModule;
        if (!VulkanUtilities::LoadShaderModule(vertexShader, _renderer->_device, vertexShaderModule)) {
            std::cout << "Failed to load vertex shader module at: " << _vertexShader << "\n";
        }

        buildDescriptorSets();

        auto pipelineLayoutInfo = VulkanInitializers::PipelineLayoutCreateInfo();

        VkDescriptorSetLayout layouts[] = { _descriptorSetLayout, _textureSetLayout };
        pipelineLayoutInfo.setLayoutCount = 2;
        pipelineLayoutInfo.pSetLayouts = layouts;

        // descriptor set layout goes here
        // therefore they probably belong to the shader

        VK_CHECK(vkCreatePipelineLayout(_renderer->_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout));

        /*
         * TEMPORARY PIPELINE BUILDING
         */

        VertexInputDescription vertexInputDescription = GetVertexDescription();

        VulkanPipelineBuilder pipelineBuilder;

        pipelineBuilder._shaderStages.push_back(
                VulkanInitializers::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule));
        pipelineBuilder._shaderStages.push_back(
                VulkanInitializers::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule));


        // Specify vertex attributes
        pipelineBuilder._vertexInputInfo = VulkanInitializers::PipelineVertexInputStateCreateInfo();
        pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexInputDescription.attributes.data();
        pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputDescription.attributes.size());

        pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexInputDescription.bindings.data();
        pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputDescription.bindings.size());

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

        vkDestroyShaderModule(_renderer->_device, fragmentShaderModule, nullptr);
        vkDestroyShaderModule(_renderer->_device, vertexShaderModule, nullptr);
    }

    void VulkanShader::cleanPipeline() {
        if (_pipeline) {
            vkDestroyPipeline(_renderer->_device, _pipeline, nullptr);
        }

        if (_pipelineLayout) {
            vkDestroyPipelineLayout(_renderer->_device, _pipelineLayout, nullptr);
        }

        if (_descriptorSetLayout) {
            vkDestroyDescriptorSetLayout(_renderer->_device, _descriptorSetLayout, nullptr);
        }

        if (_textureSetLayout) {
            vkDestroyDescriptorSetLayout(_renderer->_device, _textureSetLayout, nullptr);
        }

        for (auto& uniform : _uniformBuffers) {
            dynamic_cast<VulkanUniformBuffer*>(uniform.get())->ResetDescriptorSet();
        }

    }

    void VulkanShader::buildDescriptorSets() {
        std::vector<VkDescriptorSetLayoutBinding> bindings {GetUniformBufferLayoutBinding(0)};
        auto createDescriptorSetLayout = BuildDescriptorSetLayout(bindings);

        VK_CHECK(vkCreateDescriptorSetLayout(_renderer->_device, &createDescriptorSetLayout, nullptr, &_descriptorSetLayout));

        bindings = { GetTextureLayoutBinding(0)};
        createDescriptorSetLayout = BuildDescriptorSetLayout(bindings);
        VK_CHECK(vkCreateDescriptorSetLayout(_renderer->_device, &createDescriptorSetLayout, nullptr, &_textureSetLayout));
    }


}
