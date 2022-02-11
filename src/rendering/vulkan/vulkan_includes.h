//
// Created by ozzadar on 2022-02-11.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once

#include <vulkan/vulkan.h>

// This file is riddled with compiler warnings that we want to suppress. So we move our vulkan includes into here.

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <vk_mem_alloc.h>
#pragma clang diagnostic pop