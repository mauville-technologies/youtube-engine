//
// Created by ozzadar on 2022-02-01.
//

#pragma once
#include <youtube_engine/rendering/shader.h>
#include <youtube_engine/rendering/buffer.h>
#include <youtube_engine/rendering/texture.h>
#include "vulkan_includes.h"

namespace OZZ {
    class VulkanRenderer;

    class VulkanShader : public Shader {
    public:
        explicit VulkanShader(VulkanRenderer* renderer);

        void Rebuild();

        void Bind() override;
        void Load(const std::string&& vertexShader, const std::string&& fragmentShader) override;
        void AddUniformBuffer(std::shared_ptr<UniformBuffer> buffer) override;
        void AddTexture(std::shared_ptr<Texture> texture) override;

        ~VulkanShader() override;
    private:
        void cleanPipeline();
        void buildDescriptorSets();

    private:
        VulkanRenderer* _renderer;
        /*
         * PIPELINES
         */
        VkDescriptorSetLayout _descriptorSetLayout { VK_NULL_HANDLE };

        VkDescriptorSetLayout _textureSetLayout { VK_NULL_HANDLE };
        VkDescriptorSet _texturesDescriptorSet { VK_NULL_HANDLE };

        VkPipelineLayout _pipelineLayout{ VK_NULL_HANDLE };
        VkPipeline _pipeline { VK_NULL_HANDLE };

        /*
         * FILE LOCATIONS FOR REBUILDING
         */
        std::string _vertexShader;
        std::string _fragmentShader;

        std::vector<std::shared_ptr<UniformBuffer>> _uniformBuffers {};
        std::vector<std::shared_ptr<Texture>> _textures {};
    };


}


