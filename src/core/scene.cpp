//
// Created by ozzadar on 2022-10-24.
//

#include "youtube_engine/core/scene.h"

namespace OZZ {
    Scene::Scene() {

    }

    Scene::~Scene() {
        _entities.clear();
    }

    Entity *Scene::CreateEntity() {
        _entities.push_back(std::make_unique<Entity>(_registry));
        return _entities[_entities.size() - 1].get();
    }

    void Scene::RemoveEntity(Entity *entity) {
        _entities.erase(std::remove_if(_entities.begin(), _entities.end(), [entity](std::unique_ptr<Entity>& tEntity) {
            return entity->GetId() == tEntity->GetId();
        }), _entities.end());
    }
}