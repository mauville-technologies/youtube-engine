//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once
#include <youtube_engine/rendering/shader.h>

#include <iostream>
#include <vector>
#include <spirv_glsl.hpp>
#include "vulkan_includes.h"
#include "vulkan_texture.h"
#include "vulkan_buffer.h"

using namespace std;
#define VK_CHECK(x)                                                             \
    do {                                                                        \
        VkResult err = x;                                                       \
        if (err) {                                                              \
            std::cout << "Detected Vulkan error: " << err << std::endl;         \
            abort();                                                            \
        }                                                                       \
    } while(0)

namespace OZZ {
    class VulkanUtilities {
    public:
        static bool LoadShaderModule(const std::string& filePath, VkDevice device, VkShaderModule &outShaderModule, ShaderData& outShaderData);
        static ShaderResource BuildShaderResource(ResourceType type, spirv_cross::Resource res, const spirv_cross::CompilerGLSL& shader);
        static ShaderData LoadShaderData(const spirv_cross::CompilerGLSL& shader);
        static VkWriteDescriptorSet WriteDescriptorSetTexture(VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorImageInfo* texture);
        static VkWriteDescriptorSet WriteDescriptorSetUniformBuffer(VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorBufferInfo* descriptorBufferInfo);
    };
}


