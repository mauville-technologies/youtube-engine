//
// Created by ozzadar on 2022-10-24.
//

#pragma once
#include <entt/entt.hpp>
#include <iostream>

// include all component headers for ease of use
#include <youtube_engine/core/components/transform_component.h>
#include <youtube_engine/core/components/mesh_component.h>
#include <youtube_engine/core/components/camera_component.h>

namespace OZZ {
    class Entity {
    public:
        explicit Entity(entt::registry& registry);
        ~Entity();

        template<typename T, typename... Args>
        T& AddComponent(Args &&... args) {
            if (_registry.all_of<T>((entt::entity) _id)) {
                std::cout << "Entity " << _id << " already has component." << std::endl;
                assert(false && "Entity already has component.");
            }

            return _registry.emplace<T>((entt::entity) _id, std::forward<Args>(args)...);
        }

        template <typename T>
        void RemoveComponent() {
            _registry.remove<T>((entt::entity) _id);
        }

        template <typename T>
        T& GetComponent() {
            return _registry.get<T>((entt::entity) _id);
        }

        template <typename T, typename... Args>
        T& UpdateComponent(Args&& ... args) {
            return _registry.replace<T>((entt::entity) _id, std::forward<Args>(args)...);
        }

        [[nodiscard]] inline uint32_t GetId() const { return _id; }

    private:
        uint32_t _id;
        entt::registry& _registry;
    };
}
