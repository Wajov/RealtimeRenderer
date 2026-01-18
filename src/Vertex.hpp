#ifndef VERTEX_HPP
#define VERTEX_HPP

#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct Vertex {
    glm::vec3 position, color;
    glm::vec2 uv;

    static VkVertexInputBindingDescription GetBindingDescription();
    static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
};

#endif