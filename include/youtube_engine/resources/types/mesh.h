//
// Created by ozzadar on 2022-10-25.
//

#pragma once
#include <youtube_engine/resources/types/resource.h>
#include <youtube_engine/rendering/types.h>
#include <youtube_engine/rendering/buffer.h>

#include <vector>

struct aiScene;
struct aiNode;
struct aiMesh;

namespace OZZ {
    struct Submesh {
        Submesh(std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices);
        ~Submesh();

        std::shared_ptr<IndexBuffer> _indexBuffer { nullptr };
        std::shared_ptr<VertexBuffer> _vertexBuffer { nullptr };
    private:
        void createResources();
        void freeResources();
    private:
        std::vector<uint32_t> _indices;
        std::vector<Vertex> _vertices;
        std::vector<std::string> _textures;

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

        std::vector<Submesh> _submeshes {};
    private:
        Path _directory {};

        void load(const Path& path);
        void unload();

        void processNode(aiNode* node, const aiScene* scene);
        static Submesh processMesh(aiMesh *mesh, const aiScene* scene);
    };
}