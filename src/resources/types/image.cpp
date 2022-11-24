//
// Created by ozzadar on 2022-11-11.
//

#include <youtube_engine/resources/types/image.h>
#include <youtube_engine/service_locator.h>

namespace OZZ {
    Image::Image(const Path &path) : Resource(path, Resource::Type::IMAGE) {
        load(Filesystem::GetAssetPath() / path);
    }

    Image::Image(const Path &path, const ImageData& data) : Resource(path, Resource::Type::IMAGE) {
        _texture = ServiceLocator::GetRenderer()->CreateTexture();
        _texture->UploadData(data);
    }

    Image::~Image() {
        unload();
    }

    void Image::load(const Path &path) {
        _texture = ServiceLocator::GetRenderer()->CreateTexture();
        _texture->UploadData(ImageData(path, true));
    }

    void Image::unload() {
        _texture.reset();
    }

}