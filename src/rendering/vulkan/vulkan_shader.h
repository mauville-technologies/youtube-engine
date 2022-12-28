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

        void FreeResources() override;
        void RecreateResources() override;

        void Rebuild();

        void Bind() override;
        void Load(const std::string&& vertexShader, const std::string&& fragmentShader) override;

        [[nodiscard]] VkPipelineLayout GetPipelineLayout() { return _pipelineLayout; }
        [[nodiscard]] VkDescriptorSetLayout GetDescriptorSetLayout(uint32_t index);

        ~VulkanShader() override;
    private:
        void cleanPipeline();
        void buildDescriptorSets();

    private:
        VulkanRenderer* _renderer;
        /*
         * PIPELINES
         */
        std::vector<VkDescriptorSetLayout> _descriptorSetLayouts {};
        VkPushConstantRange _pushConstants {};

        VkPipelineLayout _pipelineLayout{ VK_NULL_HANDLE };
        VkPipeline _pipeline { VK_NULL_HANDLE };

        /*
         * FILE LOCATIONS FOR REBUILDING
         */
        std::string _vertexShader;
        std::string _fragmentShader;
    };


}


