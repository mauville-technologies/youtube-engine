//
// Created by ozzadar on 2022-10-25.
//

#include <youtube_engine/core/entity.h>

namespace OZZ {
    Entity::Entity(entt::registry &registry) : _registry(registry) {
        _id = static_cast<uint32_t>(_registry.create());
        std::cout << "Entity with id: " << _id << " created";
    }

    Entity::~Entity() {
        std::cout << "Entity with id: " << _id << " being deleted";
        _registry.destroy((entt::entity) _id);
    }
}