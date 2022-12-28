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
#include <youtube_engine/resources/types/mesh.h>

namespace OZZ {

    class ResourceManager {
        friend class VulkanRenderer;
    public:
        template <typename ResourceType, typename ...Args>
        std::shared_ptr<ResourceType> Load(const Path& path, Args&& ...args) {
            static_assert(std::is_base_of<Resource, ResourceType>::value, "ResourceType must inherit from Resource");

            auto res = _resources[path.string()].lock();
            if(!res) {
                // assuming constructor loads resource
                _resources[path.string()] = res = std::make_shared<ResourceType>(path, std::forward<Args>(args)...);
            }

            auto return_value = std::dynamic_pointer_cast<ResourceType>(res);
            if(!return_value) {
                throw std::runtime_error(std::string("Resource '") + path.string() + "' is already loaded as another type");
            }
            return return_value;
        }

    private:

        void ClearGPUResourcesForReset();
        void RecreateGPUResourcesAfterReset();
        std::unordered_map<Resource::GUID, std::weak_ptr<Resource>> _resources;
    };
}
