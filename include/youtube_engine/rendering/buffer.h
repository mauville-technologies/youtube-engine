//
// Created by Paul Mauviel on 2022-05-04.
//

#pragma once
#include <youtube_engine/rendering/types.h>
#include <vector>

namespace OZZ {
    class VertexBuffer {
    public:
        virtual ~VertexBuffer() = default;

        virtual void Bind() = 0;
        virtual void UploadData(const std::vector<Vertex>& vertices) = 0;
    private:
    };

    class IndexBuffer {
    public:
        virtual ~IndexBuffer() = default;

        virtual void Bind() = 0;
        virtual void UploadData(const std::vector<uint32_t>& indices) = 0;
        virtual size_t GetCount() = 0;
    private:
    };
}
