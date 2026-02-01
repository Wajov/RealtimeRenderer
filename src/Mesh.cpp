#include "Mesh.hpp"

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
    const std::shared_ptr<Image>& texture) :
    vertices_(vertices),
    indices_(indices),
    texture_(texture) {}