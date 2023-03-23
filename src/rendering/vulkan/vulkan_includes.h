//
// Created by ozzadar on 2022-02-11.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once

#include <vulkan/vulkan.h>
#define XR_USE_GRAPHICS_API_VULKAN

#ifdef __clang__
    // This file is riddled with compiler warnings that we want to suppress. So we move our vulkan includes into here.
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Weverything"
#endif

#include <vk_mem_alloc.h>

#ifdef __clang__
    #pragma clang diagnostic pop
#endif