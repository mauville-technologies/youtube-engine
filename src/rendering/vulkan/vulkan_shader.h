//
// Created by Paul Mauviel on 2022-02-18.
//

#pragma once
#include <youtube_engine/rendering/shader.h>
#include "vulkan_renderer.h"

namespace OZZ {
    class VulkanShader : public Shader {
    public:
        explicit VulkanShader(VulkanRenderer* renderer);
        ~VulkanShader() override;

        void Bind() override;
        void Load(const std::string &&vertexShader, const std::string &&fragmentShader) override;
        void Rebuild();

    private:

        void cleanPipelineObjects();
    private:
        VulkanRenderer* _renderer { nullptr };

        // Vulkan handles
        VkPipelineLayout _pipelineLayout { VK_NULL_HANDLE };
        VkPipeline _pipeline { VK_NULL_HANDLE };

        /*
         * FILE LOCATIONS FOR REBUILDING
         */
        std::string _vertexShaderLoc;
        std::string _fragmentShaderLoc;
    };
}


