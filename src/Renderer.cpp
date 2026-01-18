#include "Renderer.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#include "UniformBufferObject.hpp"

Renderer::Renderer(uint32_t width, uint32_t height) :
    width_(width),
    height_(height),
    window_(nullptr) {}

Renderer::~Renderer() = default;

void Renderer::Run()
{
    InitWindow();
    InitVulkan();
    MainLoop();
    Cleanup();
}

void Renderer::InitWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window_ = glfwCreateWindow(width_, height_, "LearnVulkan", nullptr, nullptr);

    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, FramebufferResizeCallback);
}

void Renderer::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    renderer->framebufferResized_ = true;
}

void Renderer::InitVulkan()
{
    CreateInstance();
    CreateValidationLayers();
    CreateSurface();
    CreatePhysicalDevice();
    CreateDevice();
    CreateMemoryAllocator();
    CreateSwapchain();
    CreateSwapchainImageViews();
    CreateSwapchainDepthResources();
    CreateRenderPass();
    CreateDescriptorSetLayout();
    CreateGraphicsPipeline();
    CreateSwapchainFramebuffers();
    CreateCommandPool();
    CreateCommandBuffers();
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateUniformBuffers();
    CreateTextureImage();
    CreateTextureImageView();
    CreateTextureSampler();
    CreateDescriptorPool();
    CreateDescriptorSets();
    CreateSyncObjects();
}

void Renderer::CreateInstance()
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

bool Renderer::CheckValidationLayerSupport()
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

std::vector<const char*> Renderer::GetRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (ENABLE_VALIDATION_LAYERS) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void Renderer::CreateValidationLayers()
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

VkResult Renderer::CreateDebugUtilsMessengerEXT(VkInstance instance,
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

void Renderer::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger,
    const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, messenger, pAllocator);
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    std::cerr << "Validation Layers: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

void Renderer::CreateSurface()
{
    VULKAN_CHECK(glfwCreateWindowSurface(instance_, window_, nullptr, &surface_));
}

void Renderer::CreatePhysicalDevice()
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

bool Renderer::IsPhysicalDeviceSuitable(VkPhysicalDevice device)
{
    if (!CheckSwapchainExtensionSupport(device)) {
        return false;
    }

    auto swapchainSupport = QuerySwapchainSupport(device);
    if (swapchainSupport.formats.empty() || swapchainSupport.presentModes.empty()) {
        return false;
    }

    auto indices = FindQueueFamilies(device);
    return indices.IsComplete();
}

bool Renderer::CheckSwapchainExtensionSupport(VkPhysicalDevice device)
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

SwapchainSupportDetails Renderer::QuerySwapchainSupport(VkPhysicalDevice device)
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

QueueFamilyIndices Renderer::FindQueueFamilies(VkPhysicalDevice device)
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

void Renderer::CreateDevice()
{
    auto indices = FindQueueFamilies(physicalDevice_);
    float queuePriority = 1.0f;

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::unordered_set<int32_t> uniqueQueueFamilies = {indices.graphicsFamilyIndex, indices.presentFamilyIndex};
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

    vkGetDeviceQueue(device_, indices.graphicsFamilyIndex, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, indices.presentFamilyIndex, 0, &presentQueue_);
}

void Renderer::CreateMemoryAllocator()
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0; // 或你使用的版本
    allocatorInfo.physicalDevice = physicalDevice_;
    allocatorInfo.device = device_;
    allocatorInfo.instance = instance_;
    VULKAN_CHECK(vmaCreateAllocator(&allocatorInfo, &allocator_));
}

void Renderer::CreateSwapchain()
{
    SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(physicalDevice_);

    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0) {
        imageCount = std::min(imageCount, swapchainSupport.capabilities.maxImageCount);
    }

    auto format = ChooseSwapchainFormat(swapchainSupport.formats);
    swapchainImageFormat_ = format.format;
    auto presentMode = ChooseSwapchainPresentMode(swapchainSupport.presentModes);
    swapchainImageExtent_ = ChooseSwapchainExtent(swapchainSupport.capabilities);

    QueueFamilyIndices indices = FindQueueFamilies(physicalDevice_);
    std::vector<uint32_t> queueFamilyIndices = {
        static_cast<uint32_t>(indices.graphicsFamilyIndex),
        static_cast<uint32_t>(indices.presentFamilyIndex)
    };

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface_;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = format.format;
    createInfo.imageColorSpace = format.colorSpace;
    createInfo.imageExtent = swapchainImageExtent_;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (indices.graphicsFamilyIndex != indices.presentFamilyIndex) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    VULKAN_CHECK(vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_));

    uint32_t swapchainImageCount;
    vkGetSwapchainImagesKHR(device_, swapchain_, &swapchainImageCount, nullptr);
    swapchainImages_.resize(swapchainImageCount);
    vkGetSwapchainImagesKHR(device_, swapchain_, &swapchainImageCount, swapchainImages_.data());
}

VkExtent2D Renderer::ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int32_t width, height;
        glfwGetFramebufferSize(window_, &width, &height);
        width_ = width;
        height_ = height;
        return {std::clamp(width_, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp(height_, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
    }
}

VkSurfaceFormatKHR Renderer::ChooseSwapchainFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
        return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }
    return formats[0];
}

VkPresentModeKHR Renderer::ChooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& presentModes)
{
    static const std::unordered_map<VkPresentModeKHR, uint32_t> PRESENT_MODE_PRIORITY = {
        {VK_PRESENT_MODE_MAILBOX_KHR, 0},
        {VK_PRESENT_MODE_IMMEDIATE_KHR, 1},
        {VK_PRESENT_MODE_FIFO_KHR, 2}
    };

    auto mode = VK_PRESENT_MODE_FIFO_KHR;
    auto priority = 2;
    for (const auto& presentMode : presentModes) {
        auto iter = PRESENT_MODE_PRIORITY.find(presentMode);
        if (iter != PRESENT_MODE_PRIORITY.end() && iter->second < priority) {
            mode = presentMode;
            priority = iter->second;
        }
    }
    return mode;
}

void Renderer::CreateSwapchainImageViews()
{
    swapchainImageViews_.resize(swapchainImages_.size());
    for (uint32_t i = 0; i < swapchainImages_.size(); i++) {
        swapchainImageViews_[i] = CreateImageView(swapchainImages_[i], swapchainImageFormat_,
            VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

VkImageView Renderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectMask)
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

void Renderer::CreateSwapchainDepthResources()
{
    CreateImage(swapchainImageExtent_.width, swapchainImageExtent_.height, VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, {}, swapchainDepthImage_,
        swapchainDepthAllocation_);
    swapchainDepthImageView_ = CreateImageView(swapchainDepthImage_, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);
    TransitionImageLayout(swapchainDepthImage_, VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
}

void Renderer::CreateRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat_;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency subpassDependency{};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &colorAttachment;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &subpassDependency;
    VULKAN_CHECK(vkCreateRenderPass(device_, &createInfo, nullptr, &renderPass_));
}

void Renderer::CreateDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> bindings = {uboLayoutBinding, samplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    createInfo.pBindings = bindings.data();

    VULKAN_CHECK(vkCreateDescriptorSetLayout(device_, &createInfo, nullptr, &descriptorSetLayout_));
}

void Renderer::CreateGraphicsPipeline()
{
    auto vertShaderCode = ReadFile("shader/shader.vert.spv");
    auto fragShaderCode = ReadFile("shader/shader.frag.spv");
    vertShaderModule_ = CreateShaderModule(vertShaderCode);
    fragShaderModule_ = CreateShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule_;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule_;
    fragShaderStageInfo.pName = "main";

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};
    auto vertexBindingDescription = Vertex::GetBindingDescription();
    auto vertexAttributeDescriptions = Vertex::GetAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputStateInfo{};
    vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateInfo.vertexBindingDescriptionCount = 1;
    vertexInputStateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    vertexInputStateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size());
    vertexInputStateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo{};
    inputAssemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchainImageExtent_.width);
    viewport.height = static_cast<float>(swapchainImageExtent_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainImageExtent_;

    VkPipelineViewportStateCreateInfo viewportStateInfo{};
    viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateInfo.viewportCount = 1;
    viewportStateInfo.pViewports = &viewport;
    viewportStateInfo.scissorCount = 1;
    viewportStateInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizerStateInfo{};
    rasterizerStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerStateInfo.depthClampEnable = VK_FALSE;
    rasterizerStateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerStateInfo.lineWidth = 1.0f;
    rasterizerStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizerStateInfo.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisamplingStateInfo{};
    multisamplingStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingStateInfo.sampleShadingEnable = VK_FALSE;
    multisamplingStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo{};
    colorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateInfo.attachmentCount = 1;
    colorBlendStateInfo.pAttachments = &colorBlendAttachment;

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateInfo.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout_;
    VULKAN_CHECK(vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_));

    VkGraphicsPipelineCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    createInfo.pStages = shaderStages.data();
    createInfo.pVertexInputState = &vertexInputStateInfo;
    createInfo.pInputAssemblyState = &inputAssemblyStateInfo;
    createInfo.pViewportState = &viewportStateInfo;
    createInfo.pRasterizationState = &rasterizerStateInfo;
    createInfo.pMultisampleState = &multisamplingStateInfo;
    createInfo.pColorBlendState = &colorBlendStateInfo;
    createInfo.pDynamicState = &dynamicStateInfo;
    createInfo.layout = pipelineLayout_;
    createInfo.renderPass = renderPass_;
    createInfo.subpass = 0;
    VULKAN_CHECK(vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &createInfo, nullptr, &graphicsPipeline_));

    vkDestroyShaderModule(device_, fragShaderModule_, nullptr);
    vkDestroyShaderModule(device_, vertShaderModule_, nullptr);
}

std::vector<char> Renderer::ReadFile(const std::string& path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path << std::endl;
        exit(EXIT_FAILURE);
    }

    auto size = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), size);
    file.close();

    return buffer;
}

VkShaderModule Renderer::CreateShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VULKAN_CHECK(vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule));
    return shaderModule;
}

void Renderer::CreateSwapchainFramebuffers()
{
    swapchainFramebuffers_.resize(swapchainImageViews_.size());
    for (uint32_t i = 0; i < swapchainImageViews_.size(); i++) {
        std::vector<VkImageView> attachments = {swapchainImageViews_[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass_;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapchainImageExtent_.width;
        framebufferInfo.height = swapchainImageExtent_.height;
        framebufferInfo.layers = 1;
        VULKAN_CHECK(vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &swapchainFramebuffers_[i]));
    }
}

void Renderer::CreateCommandPool()
{
    auto queueFamilyIndices = FindQueueFamilies(physicalDevice_);

    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamilyIndex;
    VULKAN_CHECK(vkCreateCommandPool(device_, &createInfo, nullptr, &commandPool_));
}

void Renderer::CreateCommandBuffers()
{
    commandBuffers_.resize(swapchainFramebuffers_.size());

    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = commandPool_;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());
    VULKAN_CHECK(vkAllocateCommandBuffers(device_, &allocateInfo, commandBuffers_.data()));
}

void Renderer::CreateVertexBuffer()
{
    CreateAndCopyBuffer(VERTICES, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffer_, vertexAllocation_);
}

void Renderer::CreateIndexBuffer()
{
    CreateAndCopyBuffer(INDICES, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBuffer_, indexAllocation_);
}

template<typename T>
void Renderer::CreateAndCopyBuffer(std::vector<T> data, VkBufferUsageFlags usage, VkBuffer& buffer,
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

void Renderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlagBits allocationFlags,
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

void Renderer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    auto commandBuffer = BeginSingleTimeCommands();

    VkBufferCopy copy{};
    copy.srcOffset = 0;
    copy.dstOffset = 0;
    copy.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copy);

    EndSingleTimeCommands(commandBuffer);
}

VkCommandBuffer Renderer::BeginSingleTimeCommands()
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

void Renderer::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
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

void Renderer::CreateUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    uniformBuffers_.resize(swapchainImages_.size());
    uniformAllocations_.resize(swapchainImages_.size());

    for (uint32_t i = 0; i < swapchainImages_.size(); i++) {
        CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, uniformBuffers_[i], uniformAllocations_[i]);
    }
}

void Renderer::CreateTextureImage()
{
    int32_t width, height, channels;
    auto pixels = stbi_load("texture/texture.jpg", &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        std::cerr << "Failed to load texture image" << std::endl;
        exit(EXIT_FAILURE);
    }

    CreateAndCopyImage(width, height, channels, pixels, VK_IMAGE_USAGE_SAMPLED_BIT, textureImage_, textureAllocation_);

    stbi_image_free(pixels);
}

void Renderer::CreateAndCopyImage(uint32_t width, uint32_t height, uint32_t channels, unsigned char* pixels,
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
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | usage, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        textureImage_, textureAllocation_);
    TransitionImageLayout(textureImage_, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CopyBufferToImage(stagingBuffer, textureImage_, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    TransitionImageLayout(textureImage_, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vmaDestroyBuffer(allocator_, stagingBuffer, stagingAllocation);
}

void Renderer::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
        VmaAllocationCreateFlagBits allocationFlags, VkImage& image, VmaAllocation& allocation)
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

void Renderer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
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

void Renderer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
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

void Renderer::CreateTextureImageView()
{
    textureImageView_ = CreateImageView(textureImage_, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Renderer::CreateTextureSampler()
{
    VkSamplerCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.anisotropyEnable = VK_TRUE;
    createInfo.maxAnisotropy = 16;
    createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    createInfo.unnormalizedCoordinates = VK_FALSE;
    createInfo.compareEnable = VK_FALSE;
    createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.mipLodBias = 0.0f;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = 0.0f;
    VULKAN_CHECK(vkCreateSampler(device_, &createInfo, nullptr, &textureSampler_));
}

void Renderer::CreateDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> poolSizes(2);
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(swapchainImages_.size());
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(swapchainImages_.size());

    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    createInfo.pPoolSizes = poolSizes.data();
    createInfo.maxSets = static_cast<uint32_t>(swapchainImages_.size());
    VULKAN_CHECK(vkCreateDescriptorPool(device_, &createInfo, nullptr, &descriptorPool_));
}

void Renderer::CreateDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(swapchainImages_.size(), descriptorSetLayout_);

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = descriptorPool_;
    allocateInfo.descriptorSetCount = static_cast<uint32_t>(swapchainImages_.size());
    allocateInfo.pSetLayouts = layouts.data();
    descriptorSets_.resize(swapchainImages_.size());
    VULKAN_CHECK(vkAllocateDescriptorSets(device_, &allocateInfo, descriptorSets_.data()));

    for (uint32_t i = 0; i < swapchainImages_.size(); i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers_[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView_;
        imageInfo.sampler = textureSampler_;

        std::vector<VkWriteDescriptorSet> writeDescriptorSets(2);

        writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[0].dstSet = descriptorSets_[i];
        writeDescriptorSets[0].dstBinding = 0;
        writeDescriptorSets[0].dstArrayElement = 0;
        writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[0].descriptorCount = 1;
        writeDescriptorSets[0].pBufferInfo = &bufferInfo;

        writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[1].dstSet = descriptorSets_[i];
        writeDescriptorSets[1].dstBinding = 1;
        writeDescriptorSets[1].dstArrayElement = 0;
        writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSets[1].descriptorCount = 1;
        writeDescriptorSets[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(),
            0, nullptr);
    }
}

void Renderer::CreateSyncObjects()
{
    imageAvailableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VULKAN_CHECK(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]));
        VULKAN_CHECK(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]));
        VULKAN_CHECK(vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[i]));
    }
}

void Renderer::MainLoop()
{
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        DrawFrame();
    }

    vkDeviceWaitIdle(device_);
}

void Renderer::DrawFrame()
{
    vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, std::numeric_limits<uint64_t>::max());
    vkResetFences(device_, 1, &inFlightFences_[currentFrame_]);

    uint32_t imageIndex;
    auto acquireResult = vkAcquireNextImageKHR(device_, swapchain_, std::numeric_limits<uint64_t>::max(),
        imageAvailableSemaphores_[currentFrame_], VK_NULL_HANDLE, &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();
        return;
    } else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        std::cerr << "Failed to acquire swapchain image" << std::endl;
        exit(EXIT_FAILURE);
    }

    UpdateUniformBuffer(imageIndex);
    vkResetCommandBuffer(commandBuffers_[imageIndex], 0);
    RecordCommandBuffer(commandBuffers_[imageIndex], imageIndex);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphores_[currentFrame_];
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers_[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphores_[currentFrame_];
    VULKAN_CHECK(vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphores_[currentFrame_];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain_;
    presentInfo.pImageIndices = &imageIndex;

    auto presentResult = vkQueuePresentKHR(presentQueue_, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || framebufferResized_) {
        framebufferResized_ = false;
        RecreateSwapchain();
    } else if (presentResult != VK_SUCCESS) {
        std::cerr << "Failed to present swapchain image" << std::endl;
        exit(EXIT_FAILURE);
    }

    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::RecreateSwapchain()
{
    int32_t width = 0, height = 0;
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window_, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device_);

    CleanupSwapchain();

    CreateSwapchain();
    CreateSwapchainImageViews();
    CreateSwapchainFramebuffers();
}

void Renderer::CleanupSwapchain()
{
    for (auto framebuffer : swapchainFramebuffers_) {
        vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }

    for (auto imageView : swapchainImageViews_) {
        vkDestroyImageView(device_, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device_, swapchain_, nullptr);
}

void Renderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VULKAN_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    VkClearValue clearColor{0.0f, 0.0f, 0.0f, 1.0f};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass_;
    renderPassInfo.framebuffer = swapchainFramebuffers_[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchainImageExtent_;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchainImageExtent_.width);
    viewport.height = static_cast<float>(swapchainImageExtent_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainImageExtent_;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    std::vector<VkBuffer> vertexBuffers = {vertexBuffer_};
    std::vector<VkDeviceSize> offsets = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer_, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_, 0, 1,
        &descriptorSets_[imageIndex], 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(INDICES.size()), 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    VULKAN_CHECK(vkEndCommandBuffer(commandBuffer));
}

void Renderer::UpdateUniformBuffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f),
        static_cast<float>(swapchainImageExtent_.width) / static_cast<float>(swapchainImageExtent_.height),
        0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    void* data;
    vmaMapMemory(allocator_, uniformAllocations_[currentImage], &data);
    memcpy(data, &ubo, sizeof(ubo));
    vmaUnmapMemory(allocator_, uniformAllocations_[currentImage]);
}

void Renderer::Cleanup()
{
    CleanupSwapchain();

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyFence(device_, inFlightFences_[i], nullptr);
        vkDestroySemaphore(device_, renderFinishedSemaphores_[i], nullptr);
        vkDestroySemaphore(device_, imageAvailableSemaphores_[i], nullptr);
    }

    vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);

    vkDestroySampler(device_, textureSampler_, nullptr);
    vkDestroyImageView(device_, textureImageView_, nullptr);
    vmaDestroyImage(allocator_, textureImage_, textureAllocation_);

    for (uint32_t i = 0; i < swapchainImages_.size(); i++) {
        vmaDestroyBuffer(allocator_, uniformBuffers_[i], uniformAllocations_[i]);
    }

    vmaDestroyBuffer(allocator_, indexBuffer_, indexAllocation_);
    vmaDestroyBuffer(allocator_, vertexBuffer_, vertexAllocation_);

    vkDestroyCommandPool(device_, commandPool_, nullptr);

    vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
    vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);

    vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);

    vkDestroyRenderPass(device_, renderPass_, nullptr);

    vmaDestroyAllocator(allocator_);

    vkDestroyDevice(device_, nullptr);

    vkDestroySurfaceKHR(instance_, surface_, nullptr);

    if (ENABLE_VALIDATION_LAYERS) {
        DestroyDebugUtilsMessengerEXT(instance_, messenger_, nullptr);
    }

    vkDestroyInstance(instance_, nullptr);

    glfwDestroyWindow(window_);
    glfwTerminate();
}

template void Renderer::CreateAndCopyBuffer<Vertex>(std::vector<Vertex> data, VkBufferUsageFlags usage,
    VkBuffer& buffer, VmaAllocation& allocation);
template void Renderer::CreateAndCopyBuffer<uint32_t>(std::vector<uint32_t> data, VkBufferUsageFlags usage,
    VkBuffer& buffer, VmaAllocation& allocation);