//
// Created by ozzadar on 2022-02-11.
//

#pragma once
#include <youtube_engine/rendering/images.h>
#include <tuple>
namespace OZZ {
    class Texture {
    public:
        enum class Slot {
            UNKNOWN,
            DIFFUSE0,
            DIFFUSE1,
            DIFFUSE2,
            DIFFUSE3,
        };

    public:
        virtual ~Texture() = default;

        virtual void Bind() = 0;

        virtual void BindSamplerSettings() = 0;

        virtual void UploadData(const ImageData &data) = 0;

        [[nodiscard]] virtual std::pair<uint32_t, uint32_t> GetSize() const = 0;

        [[nodiscard]] virtual int *GetHandle() const = 0;
    };
}
