#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();
    void Run();

private:
    int width_, height_;
    GLFWwindow* window_;
    VkInstance instance_;

    void InitWindow();
    void CreateInstance();
    void InitVulkan();
    void MainLoop();
    void Cleanup();
};

#endif