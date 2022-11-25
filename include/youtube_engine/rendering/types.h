//
// Created by ozzadar on 2022-02-02.
//

#pragma once

#include <glm/glm.hpp>

namespace OZZ {
    struct Vertex {
        glm::vec3 position;
        glm::vec4 color {1.f, 1.f, 1.f, 1.f};
        glm::vec2 uv;
        glm::vec3 normal {0.f, 0.f, 0.f };
    };

    enum class ColorType {
        FLOAT,
        UNSIGNED_CHAR3,
        UNSIGNED_CHAR4,
    };

    enum class ResourceName {
        Unknown,
        CameraData,
        ModelData,
        Diffuse0,
        Diffuse1,
        EndTextures
    };

    enum class ResourceType {
        Unknown,
        PushConstant,
        Uniform,
        Sampler
    };

    enum class MemberType {
        Unknown,
        Void,
        Boolean,
        SByte,
        UByte,
        Short,
        UShort,
        Int,
        UInt,
        Int64,
        UInt64,
        AtomicCounter,
        Half,
        Float,
        Double,
        Struct,
        Image,
        SampledImage,
        Sampler,
        AccelerationStructure,
        RayQuery,
        // These types are derived types.
        Vec2,
        Vec3,
        Vec4,
        Mat2,
        Mat3,
        Mat4
    };

    static ResourceName ResourceNameFromString(const std::string& str) {
        if (str == "CameraData") {
            return ResourceName::CameraData;
        }

        if (str == "ModelData") {
            return ResourceName::ModelData;
        }

        if (str == "Diffuse0") {
            return ResourceName::Diffuse0;
        }

        if (str == "Diffuse1") {
            return ResourceName::Diffuse1;
        }

        return ResourceName::Unknown;
    }
}