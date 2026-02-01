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
    Mesh(const std::string& meshPath, const std::string& texturePath);
    ~Mesh();

    void Bind();
    VkDescriptorImageInfo GetTextureInfo() const;
    void Render(VkCommandBuffer commandBuffer) const;

private:
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateTextureImage();
    void CreateTextureImageView();
    void CreateTextureSampler();

    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    std::shared_ptr<Image> texture_;

    VkBuffer vertexBuffer_, indexBuffer_;
    VmaAllocation vertexAllocation_, indexAllocation_, textureAllocation_;
    VkImage textureImage_;
    VkImageView textureImageView_;
    VkSampler textureSampler_;
};

#endif