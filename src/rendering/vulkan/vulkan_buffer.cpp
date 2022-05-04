//
// Created by Paul Mauviel on 2022-05-04.
//

#include "vulkan_buffer.h"
#include <iostream>

namespace OZZ {
    /*
     *      BUFFER STRUCT
     */

    VulkanBuffer::VulkanBuffer(VmaAllocator *allocator, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage)
    : _allocator {allocator}, _usageFlags(usageFlags), _memoryUsage(memoryUsage) {}

    VulkanBuffer::~VulkanBuffer() {
        clearBuffer();
    }

    void VulkanBuffer::UploadData(int *data, size_t bufferSize) {
        if (_bufferSize != bufferSize || _buffer == VK_NULL_HANDLE) {
            createBuffer(bufferSize);
        }

        // Upload data
        void *tempData;

        vmaMapMemory(*_allocator, _allocation, &tempData);
        memcpy(tempData, data, bufferSize);
        vmaUnmapMemory(*_allocator, _allocation);
    }

    void VulkanBuffer::createBuffer(size_t bufferSize) {
        clearBuffer();
        _bufferSize = bufferSize;

        VkBufferCreateInfo vkBufferCreateInfo { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        vkBufferCreateInfo.size = _bufferSize;
        vkBufferCreateInfo.usage = _usageFlags;

        VmaAllocationCreateInfo vmaAllocationCreateInfo {};
        vmaAllocationCreateInfo.usage = _memoryUsage;

        if (vmaCreateBuffer(*_allocator, &vkBufferCreateInfo, &vmaAllocationCreateInfo, &_buffer, &_allocation, nullptr) != VK_SUCCESS) {
            std::cerr << "Failed to allocate buffer!" << std::endl;
        }
    }

    void VulkanBuffer::clearBuffer() {
        if (_buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(*_allocator, _buffer, _allocation);
            _buffer = VK_NULL_HANDLE;
            _allocation = VK_NULL_HANDLE;
        }
    }

    /*
     *      VERTEX BUFFER
     */

    VulkanVertexBuffer::VulkanVertexBuffer(VulkanRenderer *renderer) : _renderer(renderer) {
        _buffer = std::make_shared<VulkanBuffer>(&_renderer->_allocator,
                                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    }

    VulkanVertexBuffer::~VulkanVertexBuffer() {
        if (_buffer) {
            _buffer.reset();
            _buffer = nullptr;
        }
    }

    void VulkanVertexBuffer::Bind() {
        if (_buffer && _buffer->IsAllocated()) {
            VkDeviceSize offset {0};

            VkBuffer buffers[1] {_buffer->GetBuffer()};
            vkCmdBindVertexBuffers(_renderer->_mainCommandBuffer, 0, 1,
                                   buffers, &offset);
        }
    }

    void VulkanVertexBuffer::UploadData(const std::vector<Vertex> &vertices) {
        auto bufferSize = vertices.size() * sizeof(Vertex);
        _buffer->UploadData((int*)vertices.data(), bufferSize);
    }

    /*
     *      INDEX BUFFER
     */

    VulkanIndexBuffer::VulkanIndexBuffer(VulkanRenderer *renderer) : _renderer(renderer) {
        _buffer = std::make_shared<VulkanBuffer>(&_renderer->_allocator,
                                                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    }

    VulkanIndexBuffer::~VulkanIndexBuffer() {
        if (_buffer) {
            _buffer.reset();
            _buffer = nullptr;
        }
    }

    void VulkanIndexBuffer::Bind() {
        if (_buffer && _buffer->IsAllocated()) {
            VkDeviceSize offset { 0 };
            vkCmdBindIndexBuffer(_renderer->_mainCommandBuffer, _buffer->GetBuffer(), offset,
                                 VK_INDEX_TYPE_UINT32);
        }
    }

    void VulkanIndexBuffer::UploadData(const std::vector<uint32_t> &indices) {
        auto bufferSize = indices.size() * sizeof(uint32_t);
        _count = indices.size();
        _buffer->UploadData((int*)indices.data(), bufferSize);
    }

    size_t VulkanIndexBuffer::GetCount() {
        return _count;
    }
}