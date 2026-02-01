#ifndef VULKAN_CONTEXT_HPP
#define VULKAN_CONTEXT_HPP

#include <vector>

#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#define VULKAN_CHECK(val) VulkanContext::CheckResult((val), #val, __FILE__, __LINE__)

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    int32_t graphicsFamilyIndex = -1;
    int32_t presentFamilyIndex = -1;

    bool IsComplete() const
    {
        return graphicsFamilyIndex >= 0 && presentFamilyIndex >= 0;
    }
};

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

    template<typename T>
    void CreateAndCopyBuffer(const std::vector<T>& data, VkBufferUsageFlags usage, VkBuffer& buffer,
        VmaAllocation& allocation)
    {
        VkDeviceSize bufferSize = sizeof(T) * data.size();

        VkBuffer stagingBuffer;
        VmaAllocation stagingAllocation;
        CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            stagingBuffer, stagingAllocation);

        void* stagingData;
        vmaMapMemory(allocator_, stagingAllocation, &stagingData);
        memcpy(stagingData, data.data(), static_cast<size_t>(bufferSize));
        vmaUnmapMemory(allocator_, stagingAllocation);

        CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, {}, buffer, allocation);

        CopyBuffer(stagingBuffer, buffer, bufferSize);

        vmaDestroyBuffer(allocator_, stagingBuffer, stagingAllocation);
    }

    void CreateAndCopyImage(uint32_t width, uint32_t height, uint32_t channels, unsigned char* pixels,
        VkImageUsageFlagBits usage, VkImage& image, VmaAllocation& allocation);
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlagBits allocationFlags,
        VkBuffer& buffer, VmaAllocation& allocation);
    void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
        VmaAllocationCreateFlagBits allocationFlags, VkImage& image, VmaAllocation& allocation);
    VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectMask);
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

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