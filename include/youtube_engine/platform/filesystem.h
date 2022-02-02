//
// Created by ozzadar on 2022-02-01.
//

#pragma once
#include <filesystem>

namespace OZZ {
    using Path = std::filesystem::path;

    class Filesystem {
    private:
    public:
        static inline Path GetRootPath() {
            return std::filesystem::current_path();
        }

        static inline Path GetShaderPath() {
            return GetRootPath() /= "shaders";
        }

        static inline Path GetAssetPath() {
            return GetRootPath() /= "assets";
        }
    };
}
