//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#include "vulkan_utilities.h"
#include <youtube_engine/platform/filesystem.h>

#include <fstream>
#include <vector>

namespace OZZ {
    bool VulkanUtilities::LoadShaderModule(const string &shaderPath, VkDevice device, VkShaderModule &outShaderModule, ShaderData& outShaderData) {
        std::ifstream file(shaderPath, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            // ERROR LOGGING?
            return false;
        }

        size_t filesize = static_cast<size_t>(file.tellg());
        std::vector<uint32_t> buffer(filesize / sizeof(uint32_t));

        file.seekg(0);
        file.read((char*)buffer.data(), static_cast<std::streamsize>(filesize));
        file.close();

        VkShaderModuleCreateInfo shaderModuleCreateInfo { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        shaderModuleCreateInfo.codeSize = buffer.size() * sizeof(uint32_t);
        shaderModuleCreateInfo.pCode = buffer.data();

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            // log a problem?
            return false;
        }
        outShaderModule = shaderModule;

        spirv_cross::CompilerGLSL glsl(buffer);

        outShaderData = LoadShaderData(glsl);
        return true;
    }

    ShaderData VulkanUtilities::LoadShaderData(const spirv_cross::CompilerGLSL &shader) {
        ShaderData data {};
        // Populate ShaderData
        spirv_cross::ShaderResources resources = shader.get_shader_resources();

        // Push Constants
        for (const auto& res : resources.push_constant_buffers) {

            auto resource = BuildShaderResource(ResourceType::PushConstant, res, shader);
            data.Resources[ResourceNameFromString(resource.Name)] = resource;
        }

        // Uniforms
        for (const auto& res : resources.uniform_buffers) {
            auto resource = BuildShaderResource(ResourceType::Uniform, res, shader);
            data.Resources[ResourceNameFromString(resource.Name)] = resource;

        }

        // Textures
        for (const auto& res : resources.sampled_images) {
            auto resource = BuildShaderResource(ResourceType::Sampler, res, shader);
            data.Resources[ResourceNameFromString(resource.Name)] = resource;
        }

        return data;
    }

    ShaderResource VulkanUtilities::BuildShaderResource(ResourceType type, spirv_cross::Resource res,
                                                        const spirv_cross::CompilerGLSL &shader) {
        const auto& buffer_type = shader.get_type(res.base_type_id);

        ShaderResource resource {
            .Name = res.name,
            .Type = type,
        };

        resource.Set = shader.get_decoration(res.id, spv::DecorationDescriptorSet);
        resource.Binding = shader.get_decoration(res.id, spv::DecorationBinding);

        if (type != ResourceType::Sampler) {
            resource.Size = shader.get_declared_struct_size(buffer_type);

            auto memberRanges = shader.get_active_buffer_ranges(res.id);
            // List members
            int i{0};
            for (const auto &range: memberRanges) {
                ShaderResource::Member member{
                        .Name = shader.get_member_name(res.base_type_id, range.index),
                        .Size = range.range,
                        .Offset = range.offset,
                };

                // Derive the type
                auto memberType = shader.get_type(buffer_type.member_types[i]);
                member.Type = static_cast<const MemberType>(memberType.basetype);

                // Vectors and Matrices are special, check for them
                if (member.Type == MemberType::Float) {
                    if (memberType.vecsize != 1) {
                        bool isMatrix = memberType.columns == memberType.vecsize;

                        switch (memberType.vecsize) {
                            case 2: {
                                member.Type = isMatrix
                                            ? MemberType::Mat2
                                            : MemberType::Vec2;
                                break;
                            }
                            case 3: {
                                member.Type = isMatrix
                                              ? MemberType::Mat3
                                              : MemberType::Vec3;
                                break;
                            }
                            case 4: {
                                member.Type = isMatrix
                                              ? MemberType::Mat4
                                              : MemberType::Vec4;
                                break;
                            }
                        }
                    }
                }

                resource.Members.push_back(member);
                i++;
            }
        }
        return resource;
    }

    VkWriteDescriptorSet
    VulkanUtilities::WriteDescriptorSetTexture(VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorImageInfo* imageBufferInfo) {

        VkWriteDescriptorSet descriptorSetWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descriptorSetWrite.dstSet = descriptorSet;
        descriptorSetWrite.dstBinding = binding;
        descriptorSetWrite.dstArrayElement = 0;
        descriptorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorSetWrite.descriptorCount = 1;
        descriptorSetWrite.pImageInfo = imageBufferInfo;

        return descriptorSetWrite;
    }

    VkWriteDescriptorSet
    VulkanUtilities::WriteDescriptorSetUniformBuffer(VkDescriptorSet descriptorSet, uint32_t binding,
                                                     VkDescriptorBufferInfo* descriptorBufferInfo) {

        VkWriteDescriptorSet descriptorSetWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descriptorSetWrite.dstSet = descriptorSet;
        descriptorSetWrite.dstBinding = binding;
        descriptorSetWrite.dstArrayElement = 0;
        descriptorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorSetWrite.descriptorCount = 1;

        // THIS IS THE CHANGE
        descriptorSetWrite.pBufferInfo = descriptorBufferInfo;
        descriptorSetWrite.pImageInfo = nullptr;
        descriptorSetWrite.pTexelBufferView = nullptr;
        return descriptorSetWrite;
    }
}