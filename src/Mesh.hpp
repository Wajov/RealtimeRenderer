#ifndef MESH_HPP
#define MESH_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Image.hpp"
#include "Vertex.hpp"
#include "VulkanContext.hpp"

class Mesh {
public:
    Mesh(const std::string& path);
    ~Mesh();

    const std::vector<Vertex>& GetVertices() const;
    const std::vector<uint32_t>& GetIndices() const;
    void Bind();

private:
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateTextureImage();
    void CreateTextureImageView();
    void CreateTextureSampler();

    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;

public:
    VkBuffer vertexBuffer_, indexBuffer_;
    VmaAllocation vertexAllocation_, indexAllocation_, textureAllocation_;
    VkImage textureImage_;
    VkImageView textureImageView_;
    VkSampler textureSampler_;
};

#endif