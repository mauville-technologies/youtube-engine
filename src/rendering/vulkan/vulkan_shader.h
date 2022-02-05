//
// Created by ozzadar on 2022-02-01.
//

#pragma once
#include <youtube_engine/rendering/shader.h>
#include <vulkan/vulkan.h>
namespace OZZ {
    class VulkanRenderer;

    class VulkanShader : public Shader {
    public:
        VulkanShader(VulkanRenderer* renderer);

        void Rebuild();

        void Bind(uint64_t commandHandle) override;
        void Load(const std::string&& vertexShader, const std::string&& fragmentShader) override;

        ~VulkanShader() override;
    private:
        VulkanRenderer* _renderer;
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


