//
// Created by ozzadar on 2022-02-02.
//

#include "vulkan_buffer.h"
#include "vulkan_utilities.h"

namespace OZZ {
    VulkanBuffer::VulkanBuffer(VmaAllocator* allocator, uint64_t bufferSize, VkBufferUsageFlags bufferUsage, VmaMemoryUsage vmaUsage) : _allocator(allocator){
        VkBufferCreateInfo bufferCreateInfo { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferCreateInfo.size = bufferSize;
        bufferCreateInfo.usage = bufferUsage;

        VmaAllocationCreateInfo vmaAllocationCreateInfo {};
        vmaAllocationCreateInfo.usage = vmaUsage;

        // allocate the buffer
        VK_CHECK(vmaCreateBuffer(*_allocator, &bufferCreateInfo, &vmaAllocationCreateInfo,
                     &Buffer,
                     &Allocation,
                     nullptr));
    }

    VulkanBuffer::~VulkanBuffer() {
        vmaDestroyBuffer(*_allocator, Buffer, Allocation);
    }

    void VulkanBuffer::UploadData(int *data, uint64_t bufferSize) {
        void* tempData;

        vmaMapMemory(*_allocator, Allocation, &tempData);
        memcpy(tempData, data, bufferSize);
        vmaUnmapMemory(*_allocator, Allocation);
    }


    /*
     *
     * VERTEX BUFFER
     *
     */
    VulkanVertexBuffer::VulkanVertexBuffer(VmaAllocator *allocator) : _allocator{ allocator }, _bufferSize { 0 } {}

    VulkanVertexBuffer::~VulkanVertexBuffer() {
        _buffer.reset();
        _buffer = nullptr;
    }

    void VulkanVertexBuffer::UploadData(const vector<Vertex> &vertices) {
        uint64_t newBufferSize { vertices.size() * sizeof(Vertex) };

        // If the buffer size changed, we need to recreate it
        if (_bufferSize != newBufferSize) {
            // if the buffer exists, reset it (clear old buffer)
            if (_buffer) _buffer.reset();

            _bufferSize = newBufferSize;
            _count = static_cast<uint64_t>(vertices.size());
            _buffer = std::make_shared<VulkanBuffer>(_allocator, _bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        }

        _buffer->UploadData((int*)vertices.data(), _bufferSize);
    }

    void VulkanVertexBuffer::Bind(uint64_t commandHandle) {
        if (_buffer) {
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(VkCommandBuffer(commandHandle), 0, 1, &_buffer->Buffer, &offset);
        }
    }


    /*
     *
     * INDEX BUFFER
     *
     */

    VulkanIndexBuffer::VulkanIndexBuffer(VmaAllocator* allocator) : _allocator { allocator }, _bufferSize { 0 } {}

    VulkanIndexBuffer::~VulkanIndexBuffer() {
        _buffer.reset();
        _buffer = nullptr;
    }

    void VulkanIndexBuffer::Bind(uint64_t commandHandle) {
        if (_buffer) {
            VkDeviceSize offset = 0;
            vkCmdBindIndexBuffer(VkCommandBuffer(commandHandle), _buffer->Buffer, offset, VK_INDEX_TYPE_UINT32);
        }
    }

    void VulkanIndexBuffer::UploadData(const vector<uint32_t> &indices) {
        uint64_t newBufferSize { indices.size() * sizeof(uint32_t) };

        // If the buffer size changed, we need to recreate it
        if (_bufferSize != newBufferSize) {
            // if the buffer exists, reset it (clear old buffer)
            if (_buffer) _buffer.reset();

            _bufferSize = newBufferSize;
            _count = static_cast<uint64_t>(indices.size());
            _buffer = std::make_shared<VulkanBuffer>(_allocator, _bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        }

        _buffer->UploadData((int*)indices.data(), _bufferSize);
    }
}
