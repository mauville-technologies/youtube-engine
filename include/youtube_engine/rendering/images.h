//
// Created by ozzadar on 2022-02-11.
//

#pragma once
#include <youtube_engine/platform/filesystem.h>
#include <youtube_engine/rendering/types.h>

#include <tuple>
#include <vector>
#include <glm/glm.hpp>

namespace OZZ {
    class ImageData {
    public:
        explicit ImageData(const Path& filePath, bool flipVertical = false);
        ImageData(char* fileData, uint32_t fileLength, bool flipVertical = false);
        explicit ImageData(uint32_t width, uint32_t height, glm::vec4 color);

        ~ImageData();

        [[nodiscard]] inline bool IsValid() const{ return _valid; }
        [[nodiscard]] inline uint32_t GetChannels() const { return _channels; }
        [[nodiscard]] inline ColorType GetColorType() const { return _colorType; }
        [[nodiscard]] inline std::pair<uint32_t, uint32_t> GetSize() const { return {_width, _height}; }
        [[nodiscard]] uint32_t GetDataSize() const;
        [[nodiscard]] const unsigned char* GetData() const;

    private:
        void updateColorType();
    private:
        bool _valid { false };

        int _width { 0 };
        int _height { 0 };
        int _channels { 0 };

        ColorType _colorType { ColorType::UNSIGNED_CHAR4 };
        std::vector<unsigned char> _data;
    };
}