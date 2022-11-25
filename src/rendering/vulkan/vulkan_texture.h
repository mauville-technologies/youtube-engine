//
// Created by ozzadar on 2022-02-11.
//

#pragma once

#include <youtube_engine/rendering/texture.h>
#include "vulkan_renderer.h"

namespace OZZ {
    class VulkanTexture : public Texture {
        friend class VulkanRenderer;
    public:
        explicit VulkanTexture(VulkanRenderer* renderer);
        ~VulkanTexture() override;

        void WriteToDescriptorSet(VkDescriptorSet descriptorSet, int dstBinding);
        void ResetDescriptorSet();

        void Bind() override;
        void BindSamplerSettings() override;

        void UploadData(const ImageData &data) override;

        [[nodiscard]] std::pair<uint32_t, uint32_t> GetSize() const override;

        [[nodiscard]] int *GetHandle() const override;

    private:
        VulkanRenderer* _renderer { nullptr };

        uint32_t _width { 0 };
        uint32_t _height { 0 };


        VkImage _image { VK_NULL_HANDLE };
        VmaAllocation _allocation { VK_NULL_HANDLE };
        VkImageView _imageView { VK_NULL_HANDLE };
        VkSampler _sampler { VK_NULL_HANDLE };
    };
}


