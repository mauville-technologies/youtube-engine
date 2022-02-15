//
// Created by ozzadar on 2022-02-11.
//
#include <youtube_engine/rendering/images.h>
#include <stb_image.h>
#include <iostream>


namespace OZZ {

    ImageData::ImageData(const Path &filePath, bool flipVertical) {
        Path texturePath = Filesystem::GetAssetPath() /= filePath;
        stbi_set_flip_vertically_on_load(flipVertical);


        auto image = stbi_load(texturePath.string().c_str(), &_width, &_height, &_channels, STBI_rgb_alpha);

        if (!image) {
            std::cout << "Error loading image at: " << texturePath << std::endl;
            _valid = false;
            return;
        }

        _channels = 4;
        _data.resize(_width * _height * _channels);

        memcpy(_data.data(), image, _width * _height * _channels);

        stbi_image_free(image);

        updateColorType();
    }

    ImageData::ImageData(uint32_t width, uint32_t height, glm::vec4 color) { // NOLINT
        _width = static_cast<int>(width);
        _height = static_cast<int>(height);
        _channels = 4;
        updateColorType();

        _data.resize(_width * _height * _channels);

        for (uint32_t i = 0; i < width * height * 4; i ++) {
            auto colorByte = static_cast<unsigned char>(color[i % 4] * 255.f);
            _data[i] = colorByte;
        }
    }

    ImageData::~ImageData() {
        _data.clear();
    }

    uint32_t ImageData::GetDataSize() const {
        return _width * _height * _channels;
    }

    const unsigned char *ImageData::GetData() const {
        return _data.data();
    }

    void ImageData::updateColorType() {
        switch(_channels) {
            case 4:
                _colorType = ColorType::UNSIGNED_CHAR4;
                break;
            default:
                _colorType = ColorType::UNSIGNED_CHAR3;
        }
    }


}
