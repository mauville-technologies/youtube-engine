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

        enum class ResourceName {
            Unknown,
            ViewOptions,
            Diffuse0,
            Diffuse1,
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
            if (str == "ViewOptions") {
                return ResourceName::ViewOptions;
            }

            if (str == "Diffuse0") {
                return ResourceName::Diffuse0;
            }

            if (str == "Diffuse1") {
                return ResourceName::Diffuse1;
            }

            return ResourceName::Unknown;
        }

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
        std::unordered_map<ShaderResource::ResourceName, ShaderResource> Resources;

        static ShaderData Merge(const ShaderData& first, const ShaderData& second) {
            ShaderData newShaderData {};

            std::unordered_map<ShaderResource::ResourceName, ShaderResource> _mergedResources {};

            // Merge Resources
            for (const auto& [k, v] : first.Resources) {
                _mergedResources[k] = v;
            }

            for (const auto& [k, v] : second.Resources) {
                if (_mergedResources.contains(k)) {
                    std::unordered_map<std::string, ShaderResource::Member> existingMembers {};

                    // Merge resource members
                    for (const auto& member : _mergedResources[k].Members) {
                        existingMembers[member.Name] = member;
                    }

                    for (const auto& member : v.Members) {
                        if (!existingMembers.contains(member.Name)) {
                            existingMembers[member.Name] = member;
                        }
                    }
                } else {
                    _mergedResources[k] = v;
                }
            }

            newShaderData.Resources = _mergedResources;

            return newShaderData;
        }
    };

    class Shader {
    public:
        virtual void Bind() = 0;
        virtual void Load(const std::string&& vertexShader, const std::string&& fragmentShader) = 0;

        virtual ~Shader() = default;

        const ShaderData& GetShaderData() { return _data; }

    protected:
        ShaderData _data;
    };



}
