//
// Created by ozzadar on 2022-10-24.
//

#include <youtube_engine/resources/resource_manager.h>

namespace OZZ {
    void ResourceManager::ClearGPUResourcesForReset() {
        for (auto [guid, resource] : _resources) {
            if (auto resPtr = resource.lock()) {
                resPtr->ClearGPUResource();
            }
        }
    }

    void ResourceManager::RecreateGPUResourcesAfterReset() {
        for (auto [guid, resource] : _resources) {
            if (auto resPtr = resource.lock()) {
                resPtr->RecreateGPUResource();
            }
        }
    }
}