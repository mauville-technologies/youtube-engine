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
        virtual ~VertexBuffer() = default;
        virtual void Bind() = 0;
        virtual void UploadData(const std::vector<Vertex>&) = 0;

        virtual uint64_t GetCount() = 0;
    };

    class IndexBuffer {
    public:
        virtual ~IndexBuffer() = default;
        virtual void Bind() = 0;
        virtual void UploadData(const std::vector<uint32_t>&) = 0;
        virtual uint32_t GetCount() = 0;
    };

    class UniformBuffer {
    public:
        virtual ~UniformBuffer() = default;
        virtual void Bind() = 0;
        virtual void UploadData(int* data, uint32_t size) = 0;
    };
}