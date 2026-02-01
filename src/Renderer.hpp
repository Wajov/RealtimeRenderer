#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>

#include "Mesh.hpp"
#include "QueueFamilyIndices.hpp"
#include "SwapchainSupportDetails.hpp"
#include "Vertex.hpp"
#include "VulkanContext.hpp"

class Renderer {
public:
    Renderer(uint32_t width, uint32_t height);
    ~Renderer();
    void Run();

private:
    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    uint32_t width_, height_;
    GLFWwindow* window_;
    VkInstance instance_;
    VkSurfaceKHR surface_;
    VkDevice device_;
    VkQueue graphicsQueue_, presentQueue_;
    VmaAllocator allocator_;
    VkCommandPool commandPool_;
    VkSwapchainKHR swapchain_;
    VkFormat swapchainImageFormat_;
    VkExtent2D swapchainImageExtent_;
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;
    VkImage swapchainDepthImage_;
    VmaAllocation swapchainDepthAllocation_;
    VkImageView swapchainDepthImageView_;
    VkRenderPass renderPass_;
    VkShaderModule vertShaderModule_, fragShaderModule_;
    VkDescriptorSetLayout descriptorSetLayout_;
    VkPipelineLayout pipelineLayout_;
    VkPipeline graphicsPipeline_;
    std::vector<VkFramebuffer> swapchainFramebuffers_;
    std::vector<VkCommandBuffer> commandBuffers_;
    std::vector<VkBuffer> uniformBuffers_;
    std::vector<VmaAllocation> uniformAllocations_;
    VkDescriptorPool descriptorPool_;
    std::vector<VkDescriptorSet> descriptorSets_;
    std::vector<VkSemaphore> imageAvailableSemaphores_, renderFinishedSemaphores_;
    std::vector<VkFence> inFlightFences_;
    uint32_t currentFrame_ = 0;
    bool framebufferResized_ = false;
    std::shared_ptr<Mesh> mesh_;

    void InitWindow();
    static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
    void InitVulkan();
    void CreateSwapchain();
    VkExtent2D ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    VkSurfaceFormatKHR ChooseSwapchainFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR ChooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
    void CreateSwapchainImageViews();
    void CreateSwapchainDepthResources();
    void CreateRenderPass();
    void CreateDescriptorSetLayout();
    void CreateGraphicsPipeline();
    static std::vector<char> ReadFile(const std::string& fileName);
    VkShaderModule CreateShaderModule(const std::vector<char>& code);
    void CreateSwapchainFramebuffers();
    void CreateCommandBuffers();
    void CreateUniformBuffers();
    void CreateDescriptorPool();
    void CreateDescriptorSets();
    void CreateSyncObjects();
    void InitScene();
    void MainLoop();
    void DrawFrame();
    void RecreateSwapchain();
    void CleanupSwapchain();
    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void UpdateUniformBuffer(uint32_t currentImage);
    void Cleanup();
};

#endif