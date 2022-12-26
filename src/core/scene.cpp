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

        if (width == 0 || height == 0) return;

        std::vector<RenderableObject> ros {};

        auto cameraObjects = _registry.view<TransformComponent, CameraComponent>();

        glm::mat4 viewMatrix;
        glm::mat4 projection;

        bool foundCamera { false };

        for (auto entity : cameraObjects) {
            auto camComponent = cameraObjects.get<CameraComponent>(entity);
            auto transformComponent = cameraObjects.get<TransformComponent>(entity);

            if (camComponent.IsActive()) {
                foundCamera = true;
                viewMatrix = CameraComponent::GetViewMatrix(transformComponent.GetTransform());
                projection = camComponent.GetProjectionMatrix();
                projection[1][1] *= -1;
                break;
            }
        }

        if (!foundCamera) {
            std::cerr << "No Camera in Scene, cannot draw!" << std::endl;
            return;
        }

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
                    .View = viewMatrix,
                    .Projection = projection
            },
        };
        ServiceLocator::GetRenderer()->RenderFrame(sceneParams, ros);
    }
}