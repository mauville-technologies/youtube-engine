//
// Created by ozzadar on 2022-02-01.
//

#pragma once
#include <youtube_engine/rendering/shader.h>
#include <vulkan/vulkan.h>
namespace OZZ {
    class VulkanShader : public Shader {
    public:
        VulkanShader(VkRenderPass* renderPass, VkDevice* device, VkExtent2D* windowExtent);

        void Rebuild();

        void Bind(uint64_t commandHandle) override;
        void Load(const std::string&& vertexShader, const std::string&& fragmentShader) override;

        ~VulkanShader() override;
    private:
        /*
         *  Handles
         */
        VkDevice* _device { nullptr };
        VkRenderPass* _renderPass { nullptr };

        VkExtent2D* _windowExtent { nullptr };
        /*
         * PIPELINES
         */
        VkPipelineLayout _pipelineLayout{ VK_NULL_HANDLE };
        VkPipeline _pipeline { VK_NULL_HANDLE };

        /*
         * FILE LOCATIONS FOR REBUILDING
         */
        std::string _vertexShader;
        std::string _fragmentShader;
    };
}


