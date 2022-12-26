//
// Created by ozzadar on 2022-02-01.
//

#pragma once
#include <filesystem>
#include <string>
#include <iostream>

#ifndef ASSETS_DIR_NAME
#define ASSETS_DIR_NAME "assets"
#endif

#if defined(OZZ_WINDOWS)
#include <windows.h>
#include <KnownFolders.h>
#include <shlobj.h>
#elif defined(OZZ_LINUX)
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
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

        static inline bool CreateDirectory(const Path& path) {
            return std::filesystem::create_directory(path);
        };

        static inline bool DoesFileExist(const Path& path) {
            std::error_code ec;
            auto exists = std::filesystem::exists(path, ec);
            return exists;
        }

        static inline Path GetAppUserDataDirectory() {
            Path homeDir;
#if defined(OZZ_WINDOWS)
            PWSTR path = nullptr;
            auto hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &path);
            if (SUCCEEDED(hr)) {
                homeDir = Path{path};
            }
            CoTaskMemFree(path);
#elif defined(OZZ_LINUX)
            struct passwd *pw = getpwuid(getuid());

            const char *homedir = pw->pw_dir;

            homeDir = {homeDir};
#endif
            return homeDir / GameTitle;
        }

        static std::string GameTitle;
    };
}
