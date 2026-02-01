#ifndef MESH_HPP
#define MESH_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Image.hpp"
#include "Vertex.hpp"

class Mesh {
public:
    Mesh(const std::string& path);
    ~Mesh() = default;

    const std::vector<Vertex>& GetVertices() const;
    const std::vector<uint32_t>& GetIndices() const;
    void Bind();

private:
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
};

#endif