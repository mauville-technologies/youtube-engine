//
// Created by ozzadar on 2022-11-11.
//

#include <youtube_engine/resources/types/image.h>
#include <youtube_engine/service_locator.h>

namespace OZZ {
    Image::Image(const Path &path) : Resource(path, Resource::Type::IMAGE) {
        _path = Filesystem::GetAssetPath() / path;
        load(_path);
    }

    Image::Image(const Path &path, ImageData* data) : Resource(path, Resource::Type::IMAGE) {
        _texture = ServiceLocator::GetRenderer()->CreateTexture();
        _texture->UploadData(*data);
        _image = std::unique_ptr<ImageData>(data);
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

    void Image::ClearGPUResource() {
        unload();
    }

    void Image::RecreateGPUResource() {
        if (_image) {
            _texture = ServiceLocator::GetRenderer()->CreateTexture();
            _texture->UploadData(*_image);
            return;
        }
        if (!_path.empty()) {
            load(_path);
        }
    }

}