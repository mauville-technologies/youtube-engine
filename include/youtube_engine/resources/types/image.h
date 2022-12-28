//
// Created by ozzadar on 2022-11-11.
//

#pragma once
#include <youtube_engine/resources/types/resource.h>
#include <youtube_engine/rendering/texture.h>
#include <youtube_engine/rendering/images.h>

namespace OZZ {

    class Image : public Resource {
    public:
        explicit Image(const Path& path);
        explicit Image(const Path& path, ImageData* data);

        ~Image() override;

        std::weak_ptr<Texture> GetTexture() { return _texture; }
    private:
        void load(const Path& path);
        void unload();

        void ClearGPUResource() override;
        void RecreateGPUResource() override;

    private:
        Path _path;
        std::shared_ptr<Texture> _texture { nullptr };
        std::unique_ptr<ImageData> _image { nullptr };
    };

}

