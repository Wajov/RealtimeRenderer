#include "Renderer.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include <glm/gtc/matrix_transform.hpp>

#include "Image.hpp"
#include "UniformBufferObject.hpp"

Renderer::Renderer(uint32_t width, uint32_t height) :
    width_(width),
    height_(height),
    window_(nullptr) {}

Renderer::~Renderer() = default;

void Renderer::Run()
{
    InitScene();
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
    auto& context = VulkanContext::Instance();
    context.Init(window_);
    instance_ = context.GetInstance();
    surface_ = context.GetSurface();
    device_ = context.GetDevice();
    graphicsQueue_ = context.GetGraphicsQueue();
    presentQueue_ = context.GetPresentQueue();
    allocator_ = context.GetAllocator();
    commandPool_ = context.GetCommandPool();

    CreateSwapchain();
    CreateSwapchainImageViews();
    CreateSwapchainDepthResources();
    CreateRenderPass();
    CreateDescriptorSetLayout();
    CreateGraphicsPipeline();
    CreateSwapchainFramebuffers();
    CreateCommandBuffers();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateSyncObjects();

    mesh_->Bind();

    CreateDescriptorSets();
}

void Renderer::CreateSwapchain()
{
    auto swapchainSupport = VulkanContext::Instance().GetSwapchainSupport();

    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0) {
        imageCount = std::min(imageCount, swapchainSupport.capabilities.maxImageCount);
    }

    auto format = ChooseSwapchainFormat(swapchainSupport.formats);
    swapchainImageFormat_ = format.format;
    auto presentMode = ChooseSwapchainPresentMode(swapchainSupport.presentModes);
    swapchainImageExtent_ = ChooseSwapchainExtent(swapchainSupport.capabilities);

    auto indices = VulkanContext::Instance().GetQueueFamilyIndices();
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
        swapchainImageViews_[i] = VulkanContext::Instance().CreateImageView(swapchainImages_[i], swapchainImageFormat_,
            VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void Renderer::CreateSwapchainDepthResources()
{
    VulkanContext::Instance().CreateImage(swapchainImageExtent_.width, swapchainImageExtent_.height,
        VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, {},
        swapchainDepthImage_, swapchainDepthAllocation_);
    swapchainDepthImageView_ = VulkanContext::Instance().CreateImageView(swapchainDepthImage_, VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_ASPECT_DEPTH_BIT);
    VulkanContext::Instance().TransitionImageLayout(swapchainDepthImage_, VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
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

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency subpassDependency{};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::vector<VkAttachmentDescription> attachments = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments = attachments.data();
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

    VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo{};
    depthStencilStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateInfo.depthTestEnable = VK_TRUE;
    depthStencilStateInfo.depthWriteEnable = VK_TRUE;
    depthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilStateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateInfo.stencilTestEnable = VK_FALSE;

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
    createInfo.pDepthStencilState = &depthStencilStateInfo;
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
        std::vector<VkImageView> attachments = {swapchainImageViews_[i], swapchainDepthImageView_};

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

void Renderer::CreateUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    uniformBuffers_.resize(swapchainImages_.size());
    uniformAllocations_.resize(swapchainImages_.size());

    for (uint32_t i = 0; i < swapchainImages_.size(); i++) {
        VulkanContext::Instance().CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, uniformBuffers_[i], uniformAllocations_[i]);
    }
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
        imageInfo.imageView = mesh_->textureImageView_;
        imageInfo.sampler = mesh_->textureSampler_;

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

void Renderer::InitScene()
{
    mesh_ = std::make_shared<Mesh>("model/marry/Marry.obj");
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
    CreateSwapchainDepthResources();
    CreateSwapchainFramebuffers();
}

void Renderer::CleanupSwapchain()
{
    for (auto framebuffer : swapchainFramebuffers_) {
        vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }

    vkDestroyImageView(device_, swapchainDepthImageView_, nullptr);
    vmaDestroyImage(allocator_, swapchainDepthImage_, swapchainDepthAllocation_);

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

    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass_;
    renderPassInfo.framebuffer = swapchainFramebuffers_[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchainImageExtent_;
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

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

    std::vector<VkBuffer> vertexBuffers = {mesh_->vertexBuffer_};
    std::vector<VkDeviceSize> offsets = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
    vkCmdBindIndexBuffer(commandBuffer, mesh_->indexBuffer_, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_, 0, 1,
        &descriptorSets_[imageIndex], 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh_->GetIndices().size()), 1, 0, 0, 0);

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

    for (uint32_t i = 0; i < swapchainImages_.size(); i++) {
        vmaDestroyBuffer(allocator_, uniformBuffers_[i], uniformAllocations_[i]);
    }

    vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
    vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);

    vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);

    vkDestroyRenderPass(device_, renderPass_, nullptr);

    glfwDestroyWindow(window_);
    glfwTerminate();
}