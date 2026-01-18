#ifndef UNIFORM_BUFFER_OBJECT_HPP
#define UNIFORM_BUFFER_OBJECT_HPP

#include <glm/glm.hpp>

struct UniformBufferObject {
    glm::mat4 model, view, proj;
};

#endif