//
// Created by ozzadar on 2022-11-01.
//

#include <youtube_engine/platform/filesystem.h>
#include <youtube_engine/resources/types/material.h>
#include <youtube_engine/service_locator.h>

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace OZZ {
    Material::Material(const Path &path) : Resource(path, Resource::Type::SHADER) {
        load(path);
    }

    Material::~Material() {
        unload();
    }

    void Material::load(const Path &path) {
        std::cout << "Loading material: " << path.string() << std::endl;

        auto materialPath = Filesystem::GetAssetPath() / path;
        std::ifstream f(materialPath);

        assert(Filesystem::DoesFileExist(materialPath) && (std::string("Cannot load material at path: ") + materialPath.string()).c_str());

        auto renderer = ServiceLocator::GetRenderer();
        _shader = renderer->CreateShader();

        using json = nlohmann::json;
        auto data = json::parse(f);

        auto shaders = data["shaders"].get<std::vector<json>>();

        Path vertexPath {Filesystem::GetShaderPath()};
        Path fragmentPath {Filesystem::GetShaderPath()};

        for (auto shader : shaders) {
            std::cout << "Shader stage: " << shader["stage"].get<std::string>() << std::endl;
            auto shaderStage = shader["stage"].get<std::string>();
            auto shaderName = shader["shader"].get<std::string>();

            if (shaderStage == "VERTEX") {
                vertexPath /= (shaderName + std::string(".vert.spv"));
                assert(Filesystem::DoesFileExist(vertexPath) && (std::string("Cannot load vertex shader module at path: ") + vertexPath.string()).c_str());
            } else if (shaderStage == "FRAGMENT") {
                fragmentPath /= (shaderName + std::string(".frag.spv"));
                assert(Filesystem::DoesFileExist(fragmentPath) && (std::string("Cannot load material at path: ") + fragmentPath.string()).c_str());
            } else {
                assert(false && "Unknown Shader Stage!");
            }
        }

        // Load the shaders
        _shader->Load(vertexPath.string(), fragmentPath.string());

        // TODO: Add material validation here to warn if material is missing data

        // Set the default values
    }

    void Material::unload() {

    }

    void Material::ClearGPUResource() {
        _shader->FreeResources();
    }

    void Material::RecreateGPUResource() {
        _shader->RecreateResources();
    }
}
