#define VMA_IMPLEMENTATION
#include "VulkanContext.hpp"

#include <iostream>
#include <unordered_set>

#include <vulkan/vk_enum_string_helper.h>

VulkanContext& VulkanContext::Instance()
{
    static VulkanContext instance;
    return instance;
}

void VulkanContext::CheckResult(VkResult result, const char* func, const char* file, int line)
{
    if (result != VK_SUCCESS) {
        std::cerr << file << "(" << line << + "): " << func << " " << string_VkResult(result) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void VulkanContext::Init(GLFWwindow* window)
{
    CreateInstance();
    CreateValidationLayers();
    CreateSurface(window);
    CreatePhysicalDevice();
    CreateDevice();
    CreateMemoryAllocator();
    CreateCommandPool();
}

VkInstance VulkanContext::GetInstance() const
{
    return instance_;
}

VkSurfaceKHR VulkanContext::GetSurface() const
{
    return surface_;
}

SwapchainSupportDetails VulkanContext::GetSwapchainSupport() const
{
    return swapchainSupport_;
}

QueueFamilyIndices VulkanContext::GetQueueFamilyIndices() const
{
    return queueFamilyIndices_;
}

VkDevice VulkanContext::GetDevice() const
{
    return device_;
}

VkQueue VulkanContext::GetGraphicsQueue() const
{
    return graphicsQueue_;
}

VkQueue VulkanContext::GetPresentQueue() const
{
    return presentQueue_;
}

VmaAllocator VulkanContext::GetAllocator() const
{
    return allocator_;
}

VkCommandPool VulkanContext::GetCommandPool() const
{
    return commandPool_;
}

void VulkanContext::CreateAndCopyImage(uint32_t width, uint32_t height, uint32_t channels, unsigned char* pixels,
    VkImageUsageFlagBits usage, VkImage& image, VmaAllocation& allocation)
{
    VkDeviceSize imageSize = width * height * 4;
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        stagingBuffer, stagingAllocation);

    void* data;
    vmaMapMemory(allocator_, stagingAllocation, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vmaUnmapMemory(allocator_, stagingAllocation);

    CreateImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | usage, {}, image, allocation);
    TransitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CopyBufferToImage(stagingBuffer, image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    TransitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vmaDestroyBuffer(allocator_, stagingBuffer, stagingAllocation);
}

void VulkanContext::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlagBits allocationFlags,
    VkBuffer& buffer, VmaAllocation& allocation)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocationInfo{};
    allocationInfo.flags = allocationFlags;
    allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
    VULKAN_CHECK(vmaCreateBuffer(allocator_, &bufferInfo, &allocationInfo, &buffer, &allocation, nullptr));
}

void VulkanContext::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
    VkImageUsageFlags usage, VmaAllocationCreateFlagBits allocationFlags, VkImage& image, VmaAllocation& allocation)
{
    VkImageCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.extent.width = static_cast<uint32_t>(width);
    createInfo.extent.height = static_cast<uint32_t>(height);
    createInfo.extent.depth = 1;
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.format = format;
    createInfo.tiling = tiling;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.usage = usage;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocationInfo{};
    allocationInfo.flags = allocationFlags;
    allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
    VULKAN_CHECK(vmaCreateImage(allocator_, &createInfo, &allocationInfo, &image, &allocation, nullptr));
}

VkImageView VulkanContext::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectMask)
{
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.subresourceRange.aspectMask = aspectMask;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    VULKAN_CHECK(vkCreateImageView(device_, &createInfo, nullptr, &imageView));

    return imageView;
}

void VulkanContext::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    auto commandBuffer = BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        std::cerr << "Unsupported layout transition" << std::endl;
        exit(EXIT_FAILURE);
    }
    vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    EndSingleTimeCommands(commandBuffer);
}

void VulkanContext::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    auto commandBuffer = BeginSingleTimeCommands();

    VkBufferCopy copy{};
    copy.srcOffset = 0;
    copy.dstOffset = 0;
    copy.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copy);

    EndSingleTimeCommands(commandBuffer);
}

void VulkanContext::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    auto commandBuffer = BeginSingleTimeCommands();

    VkBufferImageCopy copy{};
    copy.bufferOffset = 0;
    copy.bufferRowLength = 0;
    copy.bufferImageHeight = 0;
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.mipLevel = 0;
    copy.imageSubresource.baseArrayLayer = 0;
    copy.imageSubresource.layerCount = 1;
    copy.imageOffset = {0, 0, 0};
    copy.imageExtent = {width, height, 1};
    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

    EndSingleTimeCommands(commandBuffer);
}

VkCommandBuffer VulkanContext::BeginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandPool = commandPool_;
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    VULKAN_CHECK(vkAllocateCommandBuffers(device_, &allocateInfo, &commandBuffer));

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VULKAN_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    return commandBuffer;
}

void VulkanContext::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    VULKAN_CHECK(vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE));

    vkQueueWaitIdle(graphicsQueue_);
    vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);
}

VulkanContext::~VulkanContext()
{
    vkDestroyCommandPool(device_, commandPool_, nullptr);

    vmaDestroyAllocator(allocator_);

    vkDestroyDevice(device_, nullptr);

    if (ENABLE_VALIDATION_LAYERS) {
        DestroyDebugUtilsMessengerEXT(instance_, messenger_, nullptr);
    }

    vkDestroySurfaceKHR(instance_, surface_, nullptr);

    vkDestroyInstance(instance_, nullptr);
}

void VulkanContext::CreateInstance()
{
    if (ENABLE_VALIDATION_LAYERS && !CheckValidationLayerSupport()) {
        std::cerr << "Check validation layer support failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    auto extensions = GetRequiredExtensions();

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    if (ENABLE_VALIDATION_LAYERS) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    VULKAN_CHECK(vkCreateInstance(&createInfo, nullptr, &instance_));
}

bool VulkanContext::CheckValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layerProperties(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());

    std::unordered_set<std::string> layers;
    std::transform(layerProperties.begin(), layerProperties.end(), std::inserter(layers, layers.begin()),
        [](const VkLayerProperties& properties) {
            return properties.layerName ? properties.layerName : "";
        }
    );

    for (const auto& validationLayer : VALIDATION_LAYERS) {
        if (layers.find(validationLayer) == layers.end()) {
            return false;
        }
    }
    return true;
}

std::vector<const char*> VulkanContext::GetRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (ENABLE_VALIDATION_LAYERS) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void VulkanContext::CreateValidationLayers()
{
    if (!ENABLE_VALIDATION_LAYERS) {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
    VULKAN_CHECK(CreateDebugUtilsMessengerEXT(instance_, &createInfo, nullptr, &messenger_));
}

VkResult VulkanContext::CreateDebugUtilsMessengerEXT(VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void VulkanContext::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger,
    const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, messenger, pAllocator);
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    std::cerr << "Validation Layers: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

void VulkanContext::CreateSurface(GLFWwindow* window)
{
    VULKAN_CHECK(glfwCreateWindowSurface(instance_, window, nullptr, &surface_));
}

void VulkanContext::CreatePhysicalDevice()
{
    physicalDevice_ = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    if (deviceCount == 0) {
        std::cerr << "No physical device found" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());
    for (const auto& device : devices) {
        if (IsPhysicalDeviceSuitable(device)) {
            physicalDevice_ = device;
            break;
        }
    }

    if (physicalDevice_ == VK_NULL_HANDLE) {
        std::cerr << "Suitable physical device not found" << std::endl;
        exit(EXIT_FAILURE);
    }
}

bool VulkanContext::IsPhysicalDeviceSuitable(VkPhysicalDevice device)
{
    if (!CheckSwapchainExtensionSupport(device)) {
        return false;
    }

    swapchainSupport_ = QuerySwapchainSupport(device);
    if (swapchainSupport_.formats.empty() || swapchainSupport_.presentModes.empty()) {
        return false;
    }

    queueFamilyIndices_ = FindQueueFamilies(device);
    return queueFamilyIndices_.IsComplete();
}

bool VulkanContext::CheckSwapchainExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensionProperties(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensionProperties.data());

    std::unordered_set<std::string> extensions;
    std::transform(extensionProperties.begin(), extensionProperties.end(),
        std::inserter(extensions, extensions.begin()),
        [](const VkExtensionProperties& properties) {
            return properties.extensionName ? properties.extensionName : "";
        }
    );

    for (const auto& swapchainExtension : SWAPCHAIN_EXTENSIONS) {
        if (extensions.find(swapchainExtension) == extensions.end()) {
            return false;
        }
    }
    return true;
}

SwapchainSupportDetails VulkanContext::QuerySwapchainSupport(VkPhysicalDevice device)
{
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);
    if (formatCount > 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);
    if (presentModeCount > 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, details.presentModes.data());
    }

    return details;
}

QueueFamilyIndices VulkanContext::FindQueueFamilies(VkPhysicalDevice device)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int32_t i = 0;
    QueueFamilyIndices indices;
    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueCount > 0 && (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            indices.graphicsFamilyIndex = i;
        }

        VkBool32 supportPresent = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &supportPresent);
        if (queueFamilies[i].queueCount > 0 && supportPresent) {
            indices.presentFamilyIndex = i;
        }

        if (indices.IsComplete()) {
            break;
        }
    }

    return indices;
}

void VulkanContext::CreateDevice()
{
    float queuePriority = 1.0f;

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::unordered_set<int32_t> uniqueQueueFamilies = {queueFamilyIndices_.graphicsFamilyIndex,
        queueFamilyIndices_.presentFamilyIndex};
    for (auto queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(SWAPCHAIN_EXTENSIONS.size());
    createInfo.ppEnabledExtensionNames = SWAPCHAIN_EXTENSIONS.data();
    VULKAN_CHECK(vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_));

    vkGetDeviceQueue(device_, queueFamilyIndices_.graphicsFamilyIndex, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, queueFamilyIndices_.presentFamilyIndex, 0, &presentQueue_);
}

void VulkanContext::CreateMemoryAllocator()
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0;
    allocatorInfo.physicalDevice = physicalDevice_;
    allocatorInfo.device = device_;
    allocatorInfo.instance = instance_;
    VULKAN_CHECK(vmaCreateAllocator(&allocatorInfo, &allocator_));
}

void VulkanContext::CreateCommandPool()
{
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = queueFamilyIndices_.graphicsFamilyIndex;
    VULKAN_CHECK(vkCreateCommandPool(device_, &createInfo, nullptr, &commandPool_));
}