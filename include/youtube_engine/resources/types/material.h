#pragma once
#include <youtube_engine/resources/types/resource.h>
#include <youtube_engine/rendering/shader.h>

namespace OZZ {
    struct Material : public Resource {
    public:
        explicit Material(const Path& path);
        ~Material() override;

    private:
        void load(const Path& path);
        void unload();

    private:
        std::shared_ptr<Shader> _shader;
    };
}