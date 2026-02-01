#include <algorithm>
#include <iostream>

#include <tiny_obj_loader.h>

#include "Mesh.hpp"

Mesh::Mesh(const std::string& path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, error;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &error, path.c_str())) {
        std::cerr << "Load model failed: " << error << std::endl;
        exit(EXIT_FAILURE);
    }

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex;
            vertex.position = glm::vec3(attrib.vertices[3 * index.vertex_index],
                attrib.vertices[3 * index.vertex_index + 1], attrib.vertices[3 * index.vertex_index + 2]);
            vertex.uv = glm::vec2(attrib.texcoords[2 * index.texcoord_index],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]);
            vertices_.push_back(vertex);
            indices_.push_back(indices_.size());
        }
    }
}

Mesh::~Mesh()
{
    const auto& context = VulkanContext::Instance();
    auto device = context.GetDevice();
    auto allocator = context.GetAllocator();

    vkDestroySampler(device, textureSampler_, nullptr);
    vkDestroyImageView(device, textureImageView_, nullptr);
    vmaDestroyImage(allocator, textureImage_, textureAllocation_);

    vmaDestroyBuffer(allocator, indexBuffer_, indexAllocation_);
    vmaDestroyBuffer(allocator, vertexBuffer_, vertexAllocation_);
}

const std::vector<Vertex>& Mesh::GetVertices() const
{
    return vertices_;
}

const std::vector<uint32_t>& Mesh::GetIndices() const
{
    return indices_;
}

void Mesh::Bind()
{
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateTextureImage();
    CreateTextureImageView();
    CreateTextureSampler();
}

void Mesh::CreateVertexBuffer()
{
    VulkanContext::Instance().CreateAndCopyBuffer(vertices_, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffer_,
        vertexAllocation_);
}

void Mesh::CreateIndexBuffer()
{
    VulkanContext::Instance().CreateAndCopyBuffer(indices_, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBuffer_,
        indexAllocation_);
}

void Mesh::CreateTextureImage()
{
    auto textureImage = std::make_shared<Image>("model/marry/MC003_Kozakura_Mari.png");
    if (!textureImage->IsValid()) {
        std::cerr << "Failed to load texture image" << std::endl;
        exit(EXIT_FAILURE);
    }

    VulkanContext::Instance().CreateAndCopyImage(textureImage->GetWidth(), textureImage->GetHeight(),
        textureImage->GetChannels(), textureImage->GetPixels(), VK_IMAGE_USAGE_SAMPLED_BIT, textureImage_,
        textureAllocation_);
}

void Mesh::CreateTextureImageView()
{
    textureImageView_ = VulkanContext::Instance().CreateImageView(textureImage_, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT);
}

void Mesh::CreateTextureSampler()
{
    VkSamplerCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.anisotropyEnable = VK_TRUE;
    createInfo.maxAnisotropy = 16;
    createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    createInfo.unnormalizedCoordinates = VK_FALSE;
    createInfo.compareEnable = VK_FALSE;
    createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.mipLodBias = 0.0f;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = 0.0f;
    VULKAN_CHECK(vkCreateSampler(VulkanContext::Instance().GetDevice(), &createInfo, nullptr, &textureSampler_));
}