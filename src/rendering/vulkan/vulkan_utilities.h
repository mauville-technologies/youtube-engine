//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once
#include <iostream>

using namespace std;
#define VK_CHECK(x)                                                             \
    do {                                                                        \
        VkResult err = x;                                                       \
        if (err) {                                                              \
            std::cout << "Detected Vulkan error: " << err << std::endl;         \
            abort();                                                            \
        }                                                                       \
    } while(0)

class VulkanUtilities {

};


