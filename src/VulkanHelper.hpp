#ifndef VULKAN_HELPER_HPP
#define VULKAN_HELPER_HPP

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#define VULKAN_CHECK(val) VulkanHelper::VulkanCheck((val), #val, __FILE__, __LINE__)

class VulkanHelper {
public:
    VulkanHelper();
    ~VulkanHelper();
    static void VulkanCheck(VkResult result, const char* func, const char* file, int line);
};

#endif