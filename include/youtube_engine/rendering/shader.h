//
// Created by ozzadar on 2022-02-01.
//

#pragma once
#include <string>
#include <memory>
#include <unordered_map>
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
            std::string Name { };
            uint64_t Size { 0 };
            uint64_t Offset { 0 };
            MemberType Type { MemberType::Unknown };
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

        static ShaderData Merge(const ShaderData& first, const ShaderData& second) {
            ShaderData newShaderData {};

            std::unordered_map<std::string, ShaderResource> _mergedResources {};

            // Merge Resources
            for (const auto& resource : first.Resources) {
                _mergedResources[resource.Name] = resource;
            }

            for (const auto& resource : second.Resources) {
                if (_mergedResources.contains(resource.Name)) {
                    std::unordered_map<std::string, ShaderResource::Member> existingMembers {};

                    // Merge resource members
                    for (const auto& member : _mergedResources[resource.Name].Members) {
                        existingMembers[member.Name] = member;
                    }

                    for (const auto& member : resource.Members) {
                        if (!existingMembers.contains(member.Name)) {
                            existingMembers[member.Name] = member;
                        }
                    }
                } else {
                    _mergedResources[resource.Name] = resource;
                }
            }

            for (const auto& [key, resource] : _mergedResources) {
                newShaderData.Resources.push_back(resource);
            }

            return newShaderData;
        }
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

    protected:
        ShaderData _data;
    };



}
