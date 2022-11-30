//
// Created by ozzadar on 2022-10-24.
//

#include <youtube_engine/core/scene.h>
#include <youtube_engine/service_locator.h>
#include <youtube_engine/rendering/renderables.h>

#include <glm/glm.hpp>
namespace OZZ {
    Scene::Scene() {

    }

    Scene::~Scene() {
        std::cout << "Clearing scene" << std::endl;
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

    void Scene::Draw() {
        auto [width, height] = ServiceLocator::GetWindow()->GetWindowExtents();

        std::vector<RenderableObject> ros {};
        // Calculate the camera
        auto view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        auto projection = glm::perspective(glm::radians(70.f), width / (float) height, 0.1f, 999.0f);
        projection[1][1] *= -1;

        // Loop through all renderable objects
        auto renderableObjects = _registry.view<TransformComponent, MeshComponent>();

        for (auto entity : renderableObjects) {
            RenderableObject ro {
                .Mesh = renderableObjects.get<MeshComponent>(entity).GetMesh(),
                .ModelBuffer = renderableObjects.get<MeshComponent>(entity).GetModelBuffer(),
                .Transform = renderableObjects.get<TransformComponent>(entity).GetTransform()
            };

            ros.push_back(ro);
        }

        SceneParams sceneParams {
            .Camera = {
                    .View = view,
                    .Projection = projection
            },
        };
        ServiceLocator::GetRenderer()->RenderFrame(sceneParams, ros);
    }
}