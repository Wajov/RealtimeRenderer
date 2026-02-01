#ifndef MODEL_HPP
#define MODEL_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/material.h>

#include "Mesh.hpp"
#include "Image.hpp"

class Model {
public:
    Model(const std::string& path);
    ~Model() = default;
    void Bind();

private:
    void ProcessNode(aiNode* node, const aiScene* scene, const std::string& directory);
    std::shared_ptr<Mesh> ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::string& directory);
    std::shared_ptr<Image> ProcessTexture(aiMaterial* material, aiTextureType type, const std::string& directory);

    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateTextureImage();
    void CreateTextureImageView();
    void CreateTextureSampler();

    std::vector<std::shared_ptr<Mesh>> meshes_;
};

#endif