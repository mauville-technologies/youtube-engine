//
// Created by ozzadar on 2022-11-01.
//

#include <youtube_engine/resources/types/material.h>
#include <youtube_engine/service_locator.h>

namespace OZZ {
    Material::Material(const Path &path) : Resource(path, Resource::Type::SHADER) {
        load(path);
    }

    Material::~Material() {
        unload();
    }

    void Material::load(const Path &path) {
        assert(Filesystem::DoesFileExist(path) && (std::string("Cannot load shader at path: ") + path.string()).c_str());

        auto renderer = ServiceLocator::GetRenderer();
        _shader = renderer->CreateShader();
        std::string vertexPath {};
        std::string fragmentPath {};

        _shader->Load(std::move(vertexPath), std::move(fragmentPath));
    }

    void Material::unload() {

    }

}
