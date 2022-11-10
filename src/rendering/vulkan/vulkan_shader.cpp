//
// Created by ozzadar on 2022-02-01.
//

#include <youtube_engine/service_locator.h>
#include <map>
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
        if (!_vertexShader.empty() && !_fragmentShader.empty())
            Load(std::move(_vertexShader), std::move(_fragmentShader));
    }

    void VulkanShader::Bind() {
        // Bind Uniforms
        for (auto& uniform : _uniformBuffers) {
            auto descriptorSet = dynamic_cast<VulkanUniformBuffer*>(uniform.get())->GetDescriptorSet(&_descriptorSetLayouts[0]);

            vkCmdBindDescriptorSets( _renderer->getCurrentFrame().MainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout,
                                    0, 1, &descriptorSet, 0, nullptr);
        }

        vkCmdBindDescriptorSets(_renderer->getCurrentFrame().MainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout,
                                1, 1, &_texturesDescriptorSet, 0, nullptr);


        // TODO: Figure out how to bind descriptor sets and junk

        vkCmdBindPipeline(_renderer->getCurrentFrame().MainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

        auto [width, height] = ServiceLocator::GetWindow()->GetWindowExtents();
        VkViewport viewport = {0.0, 0.0, static_cast<float>(width), static_cast<float>(height), 0.0, 1.0};
        vkCmdSetViewport(_renderer->getCurrentFrame().MainCommandBuffer, 0, 1, &viewport);

        VkRect2D scissor {
            .offset = {0, 0},
            .extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) },
        };

        vkCmdSetScissor(_renderer->getCurrentFrame().MainCommandBuffer, 0, 1, &scissor);
    }


    void VulkanShader::AddUniformBuffer(std::shared_ptr<UniformBuffer> buffer) {
        _uniformBuffers.push_back(buffer);
        Rebuild();
    }


    void VulkanShader::AddTexture(std::shared_ptr<Texture> texture) {
        _textures.push_back(texture);
        Rebuild();
    }

    void VulkanShader::Load(const std::string&& vertexShader, const std::string&& fragmentShader) {
        _vertexShader = vertexShader;
        _fragmentShader = fragmentShader;

        VkShaderModule fragmentShaderModule;
        ShaderData fragmentShaderData;
        if (!VulkanUtilities::LoadShaderModule(fragmentShader, _renderer->_device, fragmentShaderModule, fragmentShaderData)) {
            std::cout << "Failed to load fragment shader module at: " << _fragmentShader << "\n";
        }

        VkShaderModule vertexShaderModule;
        ShaderData vertexShaderData;
        if (!VulkanUtilities::LoadShaderModule(vertexShader, _renderer->_device, vertexShaderModule, vertexShaderData)) {
            std::cout << "Failed to load vertex shader module at: " << _vertexShader << "\n";
        }

        _data = ShaderData::Merge(fragmentShaderData, vertexShaderData);

        buildDescriptorSets();

        auto pipelineLayoutInfo = VulkanInitializers::PipelineLayoutCreateInfo();

        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(_descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = _descriptorSetLayouts.data();

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
        pipelineBuilder._depthStencil = VulkanInitializers::DepthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

        _pipeline = pipelineBuilder.BuildPipeline(_renderer->_device, _renderer->_renderPass);

        vkDestroyShaderModule(_renderer->_device, fragmentShaderModule, nullptr);
        vkDestroyShaderModule(_renderer->_device, vertexShaderModule, nullptr);
    }

    void VulkanShader::cleanPipeline() {
        _texturesDescriptorSet = VK_NULL_HANDLE;

        if (_pipeline) {
            vkDestroyPipeline(_renderer->_device, _pipeline, nullptr);
        }

        if (_pipelineLayout) {
            vkDestroyPipelineLayout(_renderer->_device, _pipelineLayout, nullptr);
        }

        for (auto descriptorSetLayout : _descriptorSetLayouts) {
            vkDestroyDescriptorSetLayout(_renderer->_device, descriptorSetLayout, nullptr);
        }

        _descriptorSetLayouts.clear();

        for (auto& uniform : _uniformBuffers) {
            dynamic_cast<VulkanUniformBuffer*>(uniform.get())->ResetDescriptorSet();
        }

        for (auto& texture: _textures) {
            dynamic_cast<VulkanTexture*>(texture.get())->ResetDescriptorSet();
        }
    }

    void VulkanShader::buildDescriptorSets() {
        // TODO: Build descriptor sets based on ShaderData

        // First step is to collect the descriptors
        std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> _descriptorSetDescriptions {};

        for (const auto& resource : _data.Resources) {
            if (!_descriptorSetDescriptions.contains(resource.Set)) {
                _descriptorSetDescriptions[resource.Set] = {};
            }

            switch (resource.Type) {
                case ShaderResource::ResourceType::PushConstant:
                    break;
                case ShaderResource::ResourceType::Uniform:
                    _descriptorSetDescriptions[resource.Set].push_back(GetUniformBufferLayoutBinding(resource.Binding));
                    break;
                case ShaderResource::ResourceType::Sampler:
                    _descriptorSetDescriptions[resource.Set].push_back(GetTextureLayoutBinding(resource.Binding));
                    break;
                default:
                    break;
            }
        }


        for (const auto& [key, bindings] : _descriptorSetDescriptions) {
            auto createDescriptorSetLayout = BuildDescriptorSetLayout(bindings);

            VkDescriptorSetLayout currentLayout { VK_NULL_HANDLE };

            VK_CHECK(vkCreateDescriptorSetLayout(_renderer->_device, &createDescriptorSetLayout, nullptr,
                                                 &currentLayout));

            _descriptorSetLayouts.push_back(currentLayout);
        }

        VkDescriptorSetAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocateInfo.descriptorPool = _renderer->_descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &_descriptorSetLayouts[1];
        VK_CHECK(vkAllocateDescriptorSets(_renderer->_device, &allocateInfo, &_texturesDescriptorSet));

        for (size_t i = 0; i < _textures.size(); i++) {
            auto texture = _textures[i];
            dynamic_cast<VulkanTexture *>(texture.get())->WriteToDescriptorSet(_texturesDescriptorSet, static_cast<int>(i));
        }
    }


}
