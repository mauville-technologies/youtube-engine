//
// Created by ozzadar on 2022-02-11.
//

#include "vulkan_texture.h"
#include "vulkan_buffer.h"
#include "vulkan_types.h"
#include "vulkan_utilities.h"

#include <iostream>

namespace OZZ {

    VulkanTexture::VulkanTexture(VulkanRenderer *renderer) : _renderer(renderer) {}

    VulkanTexture::~VulkanTexture() {
        vkDestroySampler(_renderer->_device, _sampler, nullptr);
        vmaDestroyImage(_renderer->_allocator, _image, _allocation);
        vkDestroyImageView(_renderer->_device, _imageView, nullptr);
    }


    void VulkanTexture::WriteToDescriptorSet(VkDescriptorSet descriptorSet, int dstBinding) {
        if (!_image) {
            // Give it default data if no data was explicitely given yet
            UploadData(ImageData(1,1, {1.f, 0.f, 1.f, 1.f}));
        }

        VkDescriptorImageInfo imageBufferInfo {
                .sampler = _sampler,
                .imageView = _imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        VkWriteDescriptorSet descriptorSetWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descriptorSetWrite.dstSet = descriptorSet;
        descriptorSetWrite.dstBinding = dstBinding;
        descriptorSetWrite.dstArrayElement = 0;
        descriptorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorSetWrite.descriptorCount = 1;
        descriptorSetWrite.pImageInfo = &imageBufferInfo;

        vkUpdateDescriptorSets(_renderer->_device, 1, &descriptorSetWrite, 0, nullptr);
    }

    void VulkanTexture::ResetDescriptorSet() {}

    void VulkanTexture::BindSamplerSettings() {}

    void VulkanTexture::UploadData(const ImageData &data) {
        auto [width, height] = data.GetSize();
        VkExtent3D imageExtent {
                .width = width,
                .height = height,
                .depth = 1
        };
        if (width != _width || height != _height) {
            // TODO: Clean up old data + trigger a shader rebuild
            VkFilter filters = VK_FILTER_NEAREST;
            VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;

            VkSamplerCreateInfo samplerCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                    .magFilter = filters,
                    .minFilter = filters,
                    .addressModeU = samplerAddressMode,
                    .addressModeV = samplerAddressMode,
                    .addressModeW = samplerAddressMode
            };

            // Create the sampler
            vkCreateSampler(_renderer->_device, &samplerCreateInfo, nullptr, &_sampler);

            // if size has changed, re-create the texture with appropriate size
            VkFormat format = ColorTypeToVulkanFormatType(data.GetColorType());
            VkImageCreateInfo img_info {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                    .imageType = VK_IMAGE_TYPE_2D,
                    .format = format,
                    .extent = imageExtent,
                    .mipLevels = 1,
                    .arrayLayers = 1,
                    .samples = VK_SAMPLE_COUNT_1_BIT,
                    .tiling = VK_IMAGE_TILING_OPTIMAL,
                    .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
            };

            VmaAllocationCreateInfo img_allocinfo {
                .usage = VMA_MEMORY_USAGE_GPU_ONLY
            };

            if (vmaCreateImage(_renderer->_allocator, &img_info, &img_allocinfo, &_image, &_allocation, nullptr) != VK_SUCCESS) {
                std::cout << "Error allocating image" << std::endl;
            }

            _width = width;
            _height = height;

            // We then create the image view
            VkImageViewCreateInfo info = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = _image,
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = format,
                    .subresourceRange = {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel = 0,
                            .levelCount = 1,
                            .baseArrayLayer = 0,
                            .layerCount = 1,
                    }
            };

            vkCreateImageView(_renderer->_device, &info, nullptr, &_imageView);
        }

        // upload the pixels to the correct spot
        auto size = data.GetDataSize();

        auto stagingBuffer = std::make_shared<VulkanBuffer>(
                &_renderer->_allocator, size,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VMA_MEMORY_USAGE_CPU_ONLY);

        stagingBuffer->UploadData((int*)data.GetData(), size);

        VkImageSubresourceRange range {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        };

        VkImageMemoryBarrier imageMemoryBarrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,     // This may not necessarily be true if the texture already exists
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .image = _image,
            .subresourceRange = range,
        };

        VkCommandBufferAllocateInfo allocateInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandPool = _renderer->_bufferCommandPool;
        allocateInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(_renderer->_device, &allocateInfo, &commandBuffer);

        // Record the command buffer
        VkCommandBufferBeginInfo commandBufferBeginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &imageMemoryBarrier);

        VulkanBuffer::CopyBufferToImage(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                        stagingBuffer.get(), &_image, imageExtent);

        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &imageMemoryBarrier);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(_renderer->_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(_renderer->_graphicsQueue);

        vkFreeCommandBuffers(_renderer->_device, _renderer->_bufferCommandPool, 1, &commandBuffer);
    }

    std::pair<uint32_t, uint32_t> VulkanTexture::GetSize() const {
        return {_width, _height};
    }

    int *VulkanTexture::GetHandle() const {
        return nullptr;
    }

}