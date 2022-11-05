//
// Created by ozzadar on 2022-02-01.
//

#pragma once
#include <filesystem>
#include <string>

#ifndef ASSETS_DIR_NAME
#define ASSETS_DIR_NAME "assets"
#endif

namespace OZZ {
    using Path = std::filesystem::path;

    class Filesystem {
    private:
    public:
        static inline Path GetRootPath() {
            return std::filesystem::current_path();
        }

        static inline Path GetShaderPath() {
            return GetAssetPath() /= "shaders";
        }

        static inline Path GetAssetPath() {
            return GetRootPath() /= ASSETS_DIR_NAME;
        }

        static inline bool DoesFileExist(const Path& path) {
            return std::filesystem::exists(path);
        }
    };
}
