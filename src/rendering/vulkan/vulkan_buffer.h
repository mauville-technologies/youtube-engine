//
// Created by ozzadar on 2022-02-02.
//

#pragma once
#include <youtube_engine/rendering/buffer.h>
#include <vk_mem_alloc.h>
#include <memory>
#include "vulkan_renderer.h"

namespace OZZ {

    struct VulkanBuffer {
        VulkanBuffer(VmaAllocator* allocator, uint64_t bufferSize, VkBufferUsageFlags bufferUsage, VmaMemoryUsage vmaUsage);
        ~VulkanBuffer();

        void UploadData(int* data, uint64_t bufferSize);

        VkBuffer Buffer { nullptr };
        VmaAllocation Allocation { nullptr };

        static void CopyBuffer(VkDevice* device, VkCommandPool* commandPool, VkQueue* queue,
                               VulkanBuffer* srcBuffer, VulkanBuffer *dstBuffer, VkDeviceSize size);
    private:
        VmaAllocator* _allocator { nullptr };
    };

    class VulkanVertexBuffer : public VertexBuffer {
    public:
        explicit VulkanVertexBuffer(VulkanRenderer* renderer);
        ~VulkanVertexBuffer();


        void UploadData(const std::vector<Vertex>& vertices) override;
        void Bind(uint64_t commandHandle) override;
        uint64_t GetCount() override { return _count; };

    private:
        VulkanRenderer* _renderer;

        uint64_t _bufferSize;
        std::shared_ptr<VulkanBuffer> _buffer { nullptr };

        uint64_t _count = 0;
    };

    class VulkanIndexBuffer : public IndexBuffer {
    public:
        VulkanIndexBuffer(VmaAllocator* allocator);
        ~VulkanIndexBuffer();
        void Bind(uint64_t commandHandle) override;

        void UploadData(const std::vector<uint32_t> &vector) override;

        uint64_t GetCount() override { return _count; };

    private:
        VmaAllocator* _allocator;

        uint64_t _bufferSize;
        std::shared_ptr<VulkanBuffer> _buffer { nullptr };

        uint64_t _count = 0;
    };
}

