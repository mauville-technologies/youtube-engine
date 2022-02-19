//
// Created by Paul Mauviel on 2022-02-18.
//

#pragma once

#include <filesystem>

namespace OZZ {
    using Path = std::filesystem::path;

    class FileSystem {
    private:
    public:
        static inline Path GetRootPath() {
            return std::filesystem::current_path();
        }

        static inline Path GetShaderPath() {
            return GetRootPath() /= "shaders";
        }
    };
}