//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#include "vulkan_utilities.h"
#include <fstream>
#include <vector>

namespace OZZ {

    bool VulkanUtilities::LoadShaderModule(const string &filePath, VkDevice device, VkShaderModule &outShaderModule) {
        std::ifstream file(filePath.c_str(), std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            // ERROR LOGGING?
            return false;
        }

        size_t filesize = static_cast<size_t>(file.tellg());
        std::vector<uint32_t> buffer(filesize / sizeof(uint32_t));

        file.seekg(0);
        file.read((char*)buffer.data(), filesize);
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
        return true;
    }
}