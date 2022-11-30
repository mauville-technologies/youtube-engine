//
// Created by ozzadar on 2022-10-25.
//

#pragma once
#include <youtube_engine/resources/types/resource.h>
#include <youtube_engine/resources/types/image.h>
#include <youtube_engine/resources/types/material.h>

#include <youtube_engine/rendering/types.h>
#include <youtube_engine/rendering/texture.h>
#include <youtube_engine/rendering/buffer.h>

#include <vector>
#include <unordered_map>
#include <string>

struct aiScene;
struct aiNode;
struct aiMesh;

namespace OZZ {
    struct Submesh {
        Submesh(std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices);
        ~Submesh();

        std::weak_ptr<Image> SetTexture(ResourceName textureSlot, std::shared_ptr<Image>&& image);
        std::weak_ptr<Image> GetTexture(ResourceName textureSlot);

        std::weak_ptr<Material> SetMaterial(std::shared_ptr<Material>&& material);
        [[nodiscard]] std::weak_ptr<Material> GetMaterial() const;

        std::shared_ptr<IndexBuffer> _indexBuffer { nullptr };
        std::shared_ptr<VertexBuffer> _vertexBuffer { nullptr };
    private:
        void createResources();
        void freeResources();
    private:
        std::vector<uint32_t> _indices;
        std::vector<Vertex> _vertices;
        std::unordered_map<ResourceName, std::shared_ptr<Image>> _textures;
        std::shared_ptr<Material> _material { nullptr };
    };

    struct Mesh : public Resource {
    public:
        explicit Mesh(const Path& path) : Resource(path, Resource::Type::MESH) {
            load(path);
        }

        ~Mesh() override {
            // Unload
            unload();
        }

        std::vector<Submesh>& GetSubmeshes() { return _submeshes; }

    private:
        std::vector<Submesh> _submeshes {};
        Path _directory {};


    private:
        void load(const Path& path);
        void unload();

        void processNode(aiNode* node, const aiScene* scene);
        Submesh processMesh(aiMesh *mesh, const aiScene* scene);
    };
}