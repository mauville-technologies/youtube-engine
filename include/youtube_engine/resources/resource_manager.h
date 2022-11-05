//
// Created by ozzadar on 2022-10-24.
//

#pragma once
#include <unordered_map>
#include <memory>
#include <string>


#include <youtube_engine/platform/filesystem.h>
#include <youtube_engine/resources/types/resource.h>
#include <youtube_engine/resources/types/material.h>

namespace OZZ {

    class ResourceManager {
    public:
        template <typename ResourceType>
        std::shared_ptr<ResourceType> Load(const Path& path) {
            static_assert(std::is_base_of<Resource, ResourceType>::value, "ResourceType must inherit from Resource");

            auto res = _resources[path.string()].lock();
            if(!res) {
                // assuming constructor loads resource
                _resources[path.string()] = res = std::make_shared<ResourceType>(path);
            }

            auto return_value = std::dynamic_pointer_cast<ResourceType>(res);
            if(!return_value) {
                throw std::runtime_error(std::string("Resource '") + path.string() + "' is already loaded as another type");
            }
            return return_value;
        }

    private:
        std::unordered_map<Resource::GUID, std::weak_ptr<Resource>> _resources;
    };
}
