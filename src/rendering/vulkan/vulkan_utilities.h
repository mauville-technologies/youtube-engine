//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once
#include <iostream>
#include <vector>

#include "vulkan_includes.h"

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
        static std::vector<uint32_t> LoadShaderModule(const std::string& filePath, VkDevice device, VkShaderModule &outShaderModule);
    };
}


