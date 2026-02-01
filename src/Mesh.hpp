#ifndef MESH_HPP
#define MESH_HPP

#include <memory>
#include <vector>

#include "Vertex.hpp"
#include "Image.hpp"

class Mesh {
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
        const std::shared_ptr<Image>& texture);
    ~Mesh() = default;

private:
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    std::shared_ptr<Image> texture_;
};

#endif