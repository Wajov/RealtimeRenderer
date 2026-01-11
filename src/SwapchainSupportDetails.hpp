#ifndef SWAPCHAIN_SUPPORT_DETAILS_HPP
#define SWAPCHAIN_SUPPORT_DETAILS_HPP

#include <vector>

#include <vulkan/vulkan.h>

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

#endif