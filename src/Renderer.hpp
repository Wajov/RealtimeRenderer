#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <cstdint>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <QueueFamilyIndices.hpp>

class Renderer {
public:
    Renderer(uint32_t width, uint32_t height);
    ~Renderer();
    void Run();

private:
    const std::vector<const char*> VALIDATION_LAYERS = {
        "VK_LAYER_KHRONOS_validation"
    };
    const std::vector<const char*> SWAPCHAIN_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
#ifdef NDEBUG
    const bool ENABLE_VALIDATION_LAYERS = false;
#else
    const bool ENABLE_VALIDATION_LAYERS = true;
#endif

    uint32_t width_, height_;
    GLFWwindow* window_;
    VkInstance instance_;
    VkDebugUtilsMessengerEXT messenger_;
    VkSurfaceKHR surface_;
    VkPhysicalDevice physicalDevice_;
    VkDevice device_;
    VkQueue graphicsQueue_, presentQueue_;

    void InitWindow();
    void InitVulkan();
    void CreateInstance();
    bool CheckValidationLayerSupport();
    std::vector<const char*> GetRequiredExtensions();
    void CreateValidationLayers();
    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pMessenger);
    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger,
        const VkAllocationCallbacks* pAllocator);
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
    void CreateSurface();
    void CreatePhysicalDevice();
    bool IsPhysicalDeviceSuitable(VkPhysicalDevice device);
    bool CheckSwapchainExtensionSupport(VkPhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    void CreateDevice();
    void MainLoop();
    void Cleanup();
};

#endif