//
// Created by ozzadar on 2022-10-24.
//

#pragma once
#include <youtube_engine/core/entity.h>
#include <vector>
#include <memory>

namespace OZZ {
    class Scene {
    public:
        Scene();
        ~Scene();

        Entity* CreateEntity();
        void RemoveEntity(Entity *entity);

    private:
        entt::registry _registry{};
        std::vector<std::unique_ptr<Entity>> _entities;
    };
}
