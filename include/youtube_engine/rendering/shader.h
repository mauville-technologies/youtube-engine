//
// Created by ozzadar on 2022-02-01.
//

#pragma once
#include <string>
#include <memory>
#include <unordered_map>

#include <youtube_engine/rendering/buffer.h>
#include <youtube_engine/rendering/texture.h>
#include <youtube_engine/rendering/types.h>

namespace OZZ {
    struct ShaderResource {
        struct Member {
            std::string Name { };
            uint64_t Size { 0 };
            uint64_t Offset { 0 };
            MemberType Type { MemberType::Unknown };
        };

        uint32_t Set { 0 };
        uint32_t Binding { 0 };
        std::string Name {};
        ResourceType Type { ResourceType::Unknown };
        uint64_t Size { 0 };
        std::vector<Member> Members {};
    };

    struct ShaderData {
        std::unordered_map<ResourceName, ShaderResource> Resources;

        static ShaderData Merge(const ShaderData& first, const ShaderData& second) {
            ShaderData newShaderData {};

            std::unordered_map<ResourceName, ShaderResource> _mergedResources {};

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
        friend struct Material;
    public:
        virtual void Bind() = 0;
        virtual void Load(const std::string&& vertexShader, const std::string&& fragmentShader) = 0;

        virtual ~Shader() = default;

        const ShaderData& GetShaderData() { return _data; }

    private:
        virtual void FreeResources() = 0;
        virtual void RecreateResources() = 0;
    protected:
        ShaderData _data;
    };



}
