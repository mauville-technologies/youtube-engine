//
// Created by ozzadar on 2022-11-10.
//

#include <youtube_engine/resources/types/mesh.h>
#include <youtube_engine/service_locator.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>

namespace OZZ {

    /*
     *  MESH
     */

    void Mesh::load(const Path& path) {
        auto meshPath = Filesystem::GetAssetPath() / path;

        // Load all submeshes
        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(meshPath.string(), aiProcess_Triangulate | aiProcess_FlipUVs);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
            return;
        }

        _directory = meshPath.parent_path();

        processNode(scene->mRootNode, scene);
    }

    void Mesh::unload() {
        _submeshes.clear();
    }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
    void Mesh::processNode(aiNode *node, const aiScene *scene) {
        // process all the node's meshes (if any)
        for(unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            _submeshes.push_back(processMesh(mesh, scene));
        }

        // then do the same for each of its children
        for(unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }
#pragma clang diagnostic pop

    Submesh Mesh::processMesh(aiMesh *mesh, const aiScene *scene) {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        // process vertices
        for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex {
              .position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z },
              .color = mesh->mColors[0] ? glm::vec4 { mesh->mColors[0][i].r, mesh->mColors[0][i].g, mesh->mColors[0][i].b, mesh->mColors[0][i].a } : glm::vec4 {},
              .uv = mesh->mTextureCoords[0] ? glm::vec2{ mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y } : glm::vec2{},
              .normal { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z},
            };

            vertices.push_back(vertex);
        }

        // process indices
        for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for(unsigned int j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }

//        // process material
//        if(mesh->mMaterialIndex >= 0)
//        {
//            [...]
//        }

        return {std::move(vertices), std::move(indices) };
    }


    /*
     *  SUBMESH
     */

    Submesh::Submesh(std::vector<Vertex> &&vertices, std::vector<uint32_t> &&indices) : _vertices{vertices}, _indices {indices} {
        createResources();
    }

    Submesh::~Submesh() {
        freeResources();
    }

    void Submesh::createResources() {
        _indexBuffer = ServiceLocator::GetRenderer()->CreateIndexBuffer();
        _indexBuffer->UploadData(_indices);

        _vertexBuffer = ServiceLocator::GetRenderer()->CreateVertexBuffer();
        _vertexBuffer->UploadData(_vertices);

        // TODO: LOAD TEXTURES USING RESOURCE MANAGER
    }

    void Submesh::freeResources() {
        _indexBuffer.reset();
        _vertexBuffer.reset();

        _indices.clear();
        _vertices.clear();

        // TODO: FREE TEXTURES USING RESOURCE MANAGER
    }

}