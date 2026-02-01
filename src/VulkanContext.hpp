#ifndef VULKAN_CONTEXT_HPP
#define VULKAN_CONTEXT_HPP

#include <vector>

#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "QueueFamilyIndices.hpp"
#include "SwapchainSupportDetails.hpp"

#define VULKAN_CHECK(val) VulkanContext::CheckResult((val), #val, __FILE__, __LINE__)

class VulkanContext {
public:
    static VulkanContext& Instance();
    static void CheckResult(VkResult result, const char* func, const char* file, int line);

    void Init(GLFWwindow* window);
    VkInstance GetInstance() const;
    VkSurfaceKHR GetSurface() const;
    SwapchainSupportDetails GetSwapchainSupport() const;
    QueueFamilyIndices GetQueueFamilyIndices() const;
    VkDevice GetDevice() const;
    VkQueue GetGraphicsQueue() const;
    VkQueue GetPresentQueue() const;
    VmaAllocator GetAllocator() const;
    VkCommandPool GetCommandPool() const;

private:
    VulkanContext() = default;
    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext(const VulkanContext&&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&&) = delete;

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
    void CreateSurface(GLFWwindow* window);
    void CreatePhysicalDevice();
    bool IsPhysicalDeviceSuitable(VkPhysicalDevice device);
    bool CheckSwapchainExtensionSupport(VkPhysicalDevice device);
    SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    void CreateDevice();
    void CreateMemoryAllocator();
    void CreateCommandPool();

#ifdef NDEBUG
    const bool ENABLE_VALIDATION_LAYERS = false;
#else
    const bool ENABLE_VALIDATION_LAYERS = true;
#endif
    const std::vector<const char*> VALIDATION_LAYERS = {
        "VK_LAYER_KHRONOS_validation"
    };
    const std::vector<const char*> SWAPCHAIN_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkInstance instance_;
    VkDebugUtilsMessengerEXT messenger_;
    VkSurfaceKHR surface_;
    SwapchainSupportDetails swapchainSupport_;
    QueueFamilyIndices queueFamilyIndices_;
    VkPhysicalDevice physicalDevice_;
    VkDevice device_;
    VkQueue graphicsQueue_, presentQueue_;
    VmaAllocator allocator_;
    VkCommandPool commandPool_;
};

#endif