#pragma once
#include <youtube_engine/resources/types/resource.h>
#include <youtube_engine/rendering/shader.h>

namespace OZZ {
    struct Material : public Resource {
    public:
        explicit Material(const Path& path);
        ~Material() override;

        [[nodiscard]] std::weak_ptr<Shader> GetShader() const { return _shader; }
    private:
        void load(const Path& path);
        void unload();

        void ClearGPUResource() override;
        void RecreateGPUResource() override;

    private:
        std::shared_ptr<Shader> _shader;
    };
}