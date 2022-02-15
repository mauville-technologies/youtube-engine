//
// Created by ozzadar on 2022-02-02.
//

#include "vulkan_buffer.h"
#include "vulkan_utilities.h"
#include "vulkan_renderer.h"

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

    void VulkanBuffer::CopyBuffer(VkDevice* device, VkCommandPool* commandPool, VkQueue* queue,
                                  VulkanBuffer *srcBuffer, VulkanBuffer *dstBuffer, VkDeviceSize size) {
        // Create the command buffer
        VkCommandBufferAllocateInfo allocateInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandPool = *commandPool;

        allocateInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(*device, &allocateInfo, &commandBuffer);

        // Record the command buffer
        VkCommandBufferBeginInfo commandBufferBeginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

        VkBufferCopy copyRegion {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = size
        };

        vkCmdCopyBuffer(commandBuffer, srcBuffer->Buffer, dstBuffer->Buffer, 1, &copyRegion);
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(*queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(*queue);

        vkFreeCommandBuffers(*device, *commandPool, 1, &commandBuffer);
    }

    void VulkanBuffer::CopyBufferToImage(VkCommandBuffer cmd, VkImageLayout dstImageLayout, VulkanBuffer *srcBuffer, VkImage *dstImage, VkExtent3D imageExtent) {
        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = imageExtent;

        vkCmdCopyBufferToImage(cmd, srcBuffer->Buffer, *dstImage, dstImageLayout, 1, &copyRegion);
    }


    /*
     *
     * VERTEX BUFFER
     *
     */
    VulkanVertexBuffer::VulkanVertexBuffer(VulkanRenderer* renderer) :
        _renderer(renderer), _bufferSize { 0 } {}

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


            _buffer = std::make_shared<VulkanBuffer>(
                    &_renderer->_allocator, _bufferSize,
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VMA_MEMORY_USAGE_GPU_ONLY);

        }

        auto stagingBuffer = std::make_shared<VulkanBuffer>(
                &_renderer->_allocator, _bufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VMA_MEMORY_USAGE_CPU_ONLY);
        stagingBuffer->UploadData((int*)vertices.data(), _bufferSize);

        auto& frame = _renderer->getCurrentFrame();
        VulkanBuffer::CopyBuffer(&_renderer->_device, &frame.CommandPool, &_renderer->_graphicsQueue,
                                 stagingBuffer.get(), _buffer.get(), _bufferSize);
    }

    void VulkanVertexBuffer::Bind() {
        if (_buffer) {
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(_renderer->getCurrentFrame().MainCommandBuffer, 0, 1, &_buffer->Buffer, &offset);
        }
    }


    /*
     *
     * INDEX BUFFER
     *
     */

    VulkanIndexBuffer::VulkanIndexBuffer(VulkanRenderer* renderer) : _renderer { renderer }, _bufferSize { 0 } {}

    VulkanIndexBuffer::~VulkanIndexBuffer() {
        _buffer.reset();
        _buffer = nullptr;
    }

    void VulkanIndexBuffer::Bind() {
        if (_buffer) {
            VkDeviceSize offset = 0;
            vkCmdBindIndexBuffer(_renderer->getCurrentFrame().MainCommandBuffer, _buffer->Buffer, offset, VK_INDEX_TYPE_UINT32);
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
            _buffer = std::make_shared<VulkanBuffer>(
                    &_renderer->_allocator, _bufferSize,
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    VMA_MEMORY_USAGE_GPU_ONLY);
        }

        auto stagingBuffer = std::make_shared<VulkanBuffer>(
                &_renderer->_allocator, _bufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VMA_MEMORY_USAGE_CPU_ONLY);
        stagingBuffer->UploadData((int*)indices.data(), _bufferSize);

        auto& frame = _renderer->getCurrentFrame();
        VulkanBuffer::CopyBuffer(&_renderer->_device, &frame.CommandPool, &_renderer->_graphicsQueue,
                                 stagingBuffer.get(), _buffer.get(), _bufferSize);
    }

    /*
     *
     * UNIFORM BUFFER
     *
     */

    VulkanUniformBuffer::VulkanUniformBuffer(VulkanRenderer *renderer) : _renderer { renderer }, _bufferSize { 0 } {}

    VulkanUniformBuffer::~VulkanUniformBuffer() {
        _buffer.reset();
        _buffer = nullptr;
    }

    VkDescriptorSet VulkanUniformBuffer::GetDescriptorSet(VkDescriptorSetLayout* descriptorSetLayout) {
        if (!_descriptorSet) {
            if (!_buffer) {
                // Give it default data if no data was explicitely given yet
                UploadData({});
            }

            VkDescriptorSetAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            allocateInfo.descriptorPool = _renderer->_descriptorPool;
            allocateInfo.descriptorSetCount = 1;
            allocateInfo.pSetLayouts = descriptorSetLayout;

            VK_CHECK(vkAllocateDescriptorSets(_renderer->_device, &allocateInfo, &_descriptorSet));

            if (_buffer) {
                VkDescriptorBufferInfo descriptorBufferInfo{};
                descriptorBufferInfo.buffer = _buffer->Buffer;
                descriptorBufferInfo.offset = 0;
                descriptorBufferInfo.range = sizeof(UniformBufferObject);

                VkWriteDescriptorSet descriptorSetWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                descriptorSetWrite.dstSet = _descriptorSet;
                descriptorSetWrite.dstBinding = 0;
                descriptorSetWrite.dstArrayElement = 0;
                descriptorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorSetWrite.descriptorCount = 1;
                descriptorSetWrite.pBufferInfo = &descriptorBufferInfo;
                descriptorSetWrite.pImageInfo = nullptr;
                descriptorSetWrite.pTexelBufferView = nullptr;

                vkUpdateDescriptorSets(_renderer->_device, 1, &descriptorSetWrite, 0, nullptr);
            }
        }

        return _descriptorSet;
    }

    void VulkanUniformBuffer::Bind() {}

    void VulkanUniformBuffer::UploadData(const UniformBufferObject &object) {
        uint64_t newBufferSize { sizeof(object) };

        // If the buffer size changed, we need to recreate it
        if (_bufferSize != newBufferSize) {
            // if the buffer exists, reset it (clear old buffer)
            if (_buffer) _buffer.reset();

            _bufferSize = newBufferSize;
            _buffer = std::make_shared<VulkanBuffer>(
                    &_renderer->_allocator, _bufferSize,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VMA_MEMORY_USAGE_CPU_TO_GPU);
        }

        _buffer->UploadData((int*)&object, _bufferSize);
    }

    void VulkanUniformBuffer::ResetDescriptorSet() {
        _descriptorSet = VK_NULL_HANDLE;
    }


}
