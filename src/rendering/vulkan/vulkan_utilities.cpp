//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#include "vulkan_utilities.h"
#include <youtube_engine/platform/filesystem.h>
#include <spirv_glsl.hpp>

#include <fstream>
#include <vector>

namespace OZZ {
    std::vector<uint32_t> VulkanUtilities::LoadShaderModule(const string &shaderPath, VkDevice device, VkShaderModule &outShaderModule) {
        std::ifstream file(shaderPath, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            // ERROR LOGGING?
            return {};
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
            return {};
        }
        outShaderModule = shaderModule;

        spirv_cross::CompilerGLSL glsl(buffer);
        spirv_cross::ShaderResources resources = glsl.get_shader_resources();

        for (auto& uniformBuffer : resources.uniform_buffers) {
            unsigned set = glsl.get_decoration(uniformBuffer.id, spv::DecorationDescriptorSet);
            unsigned binding = glsl.get_decoration(uniformBuffer.id, spv::DecorationBinding);
            std::cout << "There was a uniform buffer called " << uniformBuffer.name << " at (set, binding) -- (" << set << ", " << binding << ")" << std::endl;

            // Get base type
            const spirv_cross::SPIRType &baseType = glsl.get_type(uniformBuffer.base_type_id);

            if (baseType.basetype == spirv_cross::SPIRType::Struct) {
                std::cout << "This is a struct\n";

                auto size = glsl.get_declared_struct_size(baseType);
                std::cout << "It has size " << size << "\n";
                for (auto &memberTypeId : baseType.member_types) {
                    auto memberType = glsl.get_type(memberTypeId);
                    auto memberBaseType = memberType.basetype;
                    if (memberBaseType == spirv_cross::SPIRType::Float) {
                        if (memberType.vecsize != 1) {
                            std::cout << "Vector or matrix with " << memberType.vecsize << " vectors and " << memberType.columns << " columns\n";
                        }
                    }
                }
            }
        }
        return buffer;
    }
}