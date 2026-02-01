#include <algorithm>
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "Model.hpp"

Model::Model(const std::string& path)
{
    Assimp::Importer importer;
    const auto* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs |
        aiProcess_GenSmoothNormals);
    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
        std::cerr << "Failed to load model: " << std::endl << importer.GetErrorString() << std::endl;
        return;
    }

    std::string directory = path.substr(0, path.find_last_of('/'));
    ProcessNode(scene->mRootNode, scene, directory);
}

void Model::Bind()
{
    // CreateVertexBuffer();
    // CreateIndexBuffer();
    // CreateTextureImage();
    // CreateTextureImageView();
    // CreateTextureSampler();
    // CreateDescriptorSets();
}

void Model::ProcessNode(aiNode* node, const aiScene* scene, const std::string& directory)
{
    for (uint32_t i = 0; i < node->mNumMeshes; i++) {
        meshes_.push_back(ProcessMesh(scene->mMeshes[node->mMeshes[i]], scene, directory));
    }
    for (uint32_t i = 0; i < node->mNumChildren; i++) {
        ProcessNode(node->mChildren[i], scene, directory);
    }
}

std::shared_ptr<Mesh> Model::ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::string& directory)
{
    std::vector<Vertex> vertices;
    for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        if (mesh->mTextureCoords[0] != nullptr) {
            vertex.uv = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }

        vertices.push_back(vertex);
    }

    std::vector<uint32_t> indices;
    for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
        for (uint32_t j = 0; j < mesh->mFaces[i].mNumIndices; j++) {
            indices.push_back(mesh->mFaces[i].mIndices[j]);
        }
    }

    auto* material = scene->mMaterials[mesh->mMaterialIndex];

    auto texture = ProcessTexture(material, aiTextureType_DIFFUSE, directory);

    return std::make_shared<Mesh>(vertices, indices, texture);
}

std::shared_ptr<Image> Model::ProcessTexture(aiMaterial* material, aiTextureType type, const std::string& directory)
{
    if (material->GetTextureCount(type) > 0) {
        aiString nameTemp;
        material->GetTexture(type, 0, &nameTemp);
        std::string filename = nameTemp.C_Str();
        return std::make_shared<Image>(directory + "/" + filename);
    } else {
        return nullptr;
    }
}