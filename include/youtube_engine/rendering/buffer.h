//
// Created by ozzadar on 2022-02-02.
//
#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <cassert>

#include <youtube_engine/rendering/types.h>

namespace OZZ {

    class VertexBuffer {
    public:
        ~VertexBuffer() = default;
        virtual void Bind(uint64_t commandHandle) = 0;
        virtual void UploadData(const std::vector<Vertex>&) = 0;

        virtual uint64_t GetCount() = 0;
    };

    class IndexBuffer {
    public:
        ~IndexBuffer() = default;
        virtual void Bind(uint64_t commandHandle) = 0;
        virtual void UploadData(const std::vector<uint32_t>&) = 0;
        virtual uint64_t GetCount() = 0;
    };

    class UniformBuffer {
    public:
        ~UniformBuffer() = default;
        virtual void Bind(uint64_t commandHandle) = 0;
        virtual void UploadData(const UniformBufferObject&) = 0;
    };
}