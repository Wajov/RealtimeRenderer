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
    // CreateVertexBuffer();
    // CreateIndexBuffer();
    // CreateTextureImage();
    // CreateTextureImageView();
    // CreateTextureSampler();
    // CreateDescriptorSets();
}