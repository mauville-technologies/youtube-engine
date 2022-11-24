//
// Created by ozzadar on 2022-11-10.
//

#include <youtube_engine/resources/types/mesh.h>
#include <youtube_engine/resources/types/image.h>

#include <youtube_engine/service_locator.h>
#include <youtube_engine/rendering/images.h>

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
        std::unordered_map<Texture::Slot, std::shared_ptr<Image>> textures;

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

        Submesh submesh {std::move(vertices), std::move(indices) };

//        // process material
        if(mesh->mMaterialIndex >= 0) {
            // TODO: Currently we're only pulling the textures out of the materials here, ideally we would also pull the different material settings as well.
            aiMaterial *mat = scene->mMaterials[mesh->mMaterialIndex];

            // Read Diffuse Textures
            for(unsigned int i = 0; i < mat->GetTextureCount(aiTextureType_DIFFUSE); i++) {
                auto textureSlot = static_cast<Texture::Slot>((int)Texture::Slot::DIFFUSE0 + i);

                aiString str;
                mat->GetTexture(aiTextureType_DIFFUSE, i, &str);

                if (auto* texData = scene->GetEmbeddedTexture(str.C_Str())) {
                    if (!texData->mHeight) {
                        // Texture is compressed, treat it as a char array
                        auto* buffer = reinterpret_cast<char*>(texData->pcData);

                        auto imageData = ImageData(buffer, texData->mWidth);
                        submesh.SetTexture(textureSlot , ServiceLocator::GetResourceManager()->Load<Image>(str.C_Str(), imageData));
                    }
                } else {
                    auto imageData = ImageData(str.C_Str());
                    submesh.SetTexture(textureSlot , ServiceLocator::GetResourceManager()->Load<Image>(str.C_Str(), imageData));
                }
            }

            // TODO: Read other texture types

//            vector<Texture> diffuseMaps = loadMaterialTextures(material,
//                                                               aiTextureType_DIFFUSE, "texture_diffuse");
//            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
//            vector<Texture> specularMaps = loadMaterialTextures(material,
//                                                                aiTextureType_SPECULAR, "texture_specular");
//            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        }

        submesh.SetMaterial(ServiceLocator::GetResourceManager()->Load<Material>("materials/default_material.mat"));
        return submesh;
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

    std::weak_ptr<Image> Submesh::SetTexture(Texture::Slot textureSlot, std::shared_ptr<Image> &&image) {
        _textures[textureSlot] = image;
        return _textures[textureSlot];
    }

    std::weak_ptr<Image> Submesh::GetTexture(Texture::Slot textureSlot) {
        return _textures[textureSlot];
    }

    std::weak_ptr<Material> Submesh::SetMaterial(std::shared_ptr<Material> &&material) {
        _material = material;
        return _material;
    }

    std::weak_ptr<Material> Submesh::GetMaterial() {
        return _material;
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