//
// Created by ozzadar on 2022-02-01.
//

#pragma once
#include <string>
#include <memory>
#include <youtube_engine/rendering/buffer.h>
#include <youtube_engine/rendering/texture.h>

namespace OZZ {

    struct ShaderResource {
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

        struct Member {
            std::string Name;
            uint64_t Size;
            uint64_t Offset;
            MemberType Type;
        };

        uint32_t Set { 0 };
        uint32_t Binding { 0 };
        std::string Name {};
        ShaderResource::ResourceType Type { ResourceType::Unknown };
        uint64_t Size { 0 };
        std::vector<Member> Members {};
    };

    struct ShaderData {
        std::vector<ShaderResource> Resources {};
    };

    class Shader {
    public:
        virtual void Bind() = 0;
        virtual void Load(const std::string&& vertexShader, const std::string&& fragmentShader) = 0;
        virtual void AddUniformBuffer(std::shared_ptr<UniformBuffer> buffer) = 0;
        virtual void AddTexture(std::shared_ptr<Texture> texture) = 0;

        virtual ~Shader() = default;
        //TODO: Figure out uniforms

        const ShaderData& GetShaderData() { return _data; }

    private:
        ShaderData _data;
    };



}
