#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>

#include "Model.hpp"
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
// #ifdef NDEBUG
//     const bool ENABLE_VALIDATION_LAYERS = false;
// #else
//     const bool ENABLE_VALIDATION_LAYERS = true;
// #endif
    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    // const std::vector<const char*> VALIDATION_LAYERS = {
    //     "VK_LAYER_KHRONOS_validation"
    // };
    // const std::vector<const char*> SWAPCHAIN_EXTENSIONS = {
    //     VK_KHR_SWAPCHAIN_EXTENSION_NAME
    // };
    const std::vector<Vertex> VERTICES = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}}
    };
    const std::vector<uint32_t> INDICES = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };

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
    VkImage swapchainDepthImage_, textureImage_;
    VmaAllocation swapchainDepthAllocation_, textureAllocation_;
    VkImageView swapchainDepthImageView_, textureImageView_;
    VkRenderPass renderPass_;
    VkShaderModule vertShaderModule_, fragShaderModule_;
    VkDescriptorSetLayout descriptorSetLayout_;
    VkPipelineLayout pipelineLayout_;
    VkPipeline graphicsPipeline_;
    std::vector<VkFramebuffer> swapchainFramebuffers_;
    std::vector<VkCommandBuffer> commandBuffers_;
    VkBuffer vertexBuffer_, indexBuffer_;
    VmaAllocation vertexAllocation_, indexAllocation_;
    std::vector<VkBuffer> uniformBuffers_;
    std::vector<VmaAllocation> uniformAllocations_;
    VkSampler textureSampler_;
    VkDescriptorPool descriptorPool_;
    std::vector<VkDescriptorSet> descriptorSets_;
    std::vector<VkSemaphore> imageAvailableSemaphores_, renderFinishedSemaphores_;
    std::vector<VkFence> inFlightFences_;
    uint32_t currentFrame_ = 0;
    bool framebufferResized_ = false;
    std::shared_ptr<Model> model_;

    void InitWindow();
    static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
    void InitVulkan();
    void CreateSwapchain();
    VkExtent2D ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    VkSurfaceFormatKHR ChooseSwapchainFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR ChooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
    void CreateSwapchainImageViews();
    VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectMask);
    void CreateSwapchainDepthResources();
    void CreateRenderPass();
    void CreateDescriptorSetLayout();
    void CreateGraphicsPipeline();
    static std::vector<char> ReadFile(const std::string& fileName);
    VkShaderModule CreateShaderModule(const std::vector<char>& code);
    void CreateSwapchainFramebuffers();
    void CreateCommandBuffers();
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    template<typename T>
    void CreateAndCopyBuffer(std::vector<T> data, VkBufferUsageFlags usage, VkBuffer& buffer,
        VmaAllocation& allocation);
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlagBits allocationFlags,
        VkBuffer& buffer, VmaAllocation& allocation);
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
    void CreateUniformBuffers();
    void CreateTextureImage();
    void CreateAndCopyImage(uint32_t width, uint32_t height, uint32_t channels, unsigned char* pixels,
        VkImageUsageFlagBits usage, VkImage& image, VmaAllocation& allocation);
    void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
        VmaAllocationCreateFlagBits allocationFlags, VkImage& image, VmaAllocation& allocation);
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void CreateTextureImageView();
    void CreateTextureSampler();
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