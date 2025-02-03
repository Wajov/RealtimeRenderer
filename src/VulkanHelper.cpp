#include "VulkanHelper.hpp"

#include <iostream>

#include <vulkan/vk_enum_string_helper.h>

VulkanHelper::VulkanHelper() = default;

VulkanHelper::~VulkanHelper() = default;

void VulkanHelper::VulkanCheck(VkResult result, const char* func, const char* file, int line)
{
    if (result != VK_SUCCESS) {
        std::cerr << file << "(" << line << + "): " << func << " " << string_VkResult(result) << std::endl;
        exit(EXIT_FAILURE);
    }
}