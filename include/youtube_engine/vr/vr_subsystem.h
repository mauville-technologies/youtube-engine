//
// Created by ozzadar on 2023-01-01.
//

#pragma once
#include <string>
#include <youtube_engine/rendering/renderer.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <tuple>

namespace OZZ {
    enum class VRBackend {
        OpenXR
    };

    struct VRSettings {
        std::string ApplicationName;
        RendererAPI Renderer;
    };

    struct FieldOfView {
        float AngleDown;
        float AngleLeft;
        float AngleRight;
        float AngleUp;
    };

    struct EyePoseInfo {
        FieldOfView FOV;
        glm::quat Orientation;
        glm::vec3 Position;
    };

    class VirtualRealitySubsystem {
    public:
        virtual void Init(VRSettings) = 0;
        virtual void Reset() = 0;
        virtual void Shutdown() = 0;
        virtual bool Update() = 0;

        virtual VRBackend GetBackendType() = 0;
        virtual std::tuple<int, int> GetSwapchainImageRectDimensions() = 0;

        virtual bool IsInitialized() = 0;
        virtual ~VirtualRealitySubsystem() = default;
    };
}
