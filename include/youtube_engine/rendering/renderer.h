//
// Created by ozzadar on 2021-06-16.
// Copyright (c) 2021 Mauville Technologies Incorporated. All rights reserved.
//

#pragma once
#include <string>

namespace OZZ {
    struct RendererSettings {
        std::string ApplicationName;
    };

    class Renderer {
    public:
        virtual void Init(RendererSettings) = 0;
        virtual void Shutdown() = 0;
        virtual void RenderFrame() = 0;
    };
}