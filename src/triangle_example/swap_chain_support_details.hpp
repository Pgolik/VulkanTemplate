#pragma once
#include "triangle_example/helper.hpp"
#include "triangle_example/queue_family_indices.hpp"

#include <algorithm> // Necessary for std::clamp
#include <fmt/format.h>
#include <limits> // Necessary for std::numeric_limits
#include <optional>
#include <ranges>
#include <vector>
#include <vulkan/vulkan.hpp>

struct swap_chain_support_details {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

VkExtent2D chooseSwapExtent(GLFWwindow* const window,
                            const VkSurfaceCapabilitiesKHR& capabilities) {
    if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        VkExtent2D actualExtent = {};

        actualExtent.width = std::clamp(static_cast<uint32_t>(width),
                                        capabilities.minImageExtent.width,
                                        capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(static_cast<uint32_t>(height),
                                         capabilities.minImageExtent.height,
                                         capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

auto chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    auto found = std::ranges::find_if(availableFormats, [](const auto& f) {
        return f.format == VK_FORMAT_B8G8R8A8_SRGB
               && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    });
    return found != availableFormats.end() ? *found : *availableFormats.begin();
}

auto chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    auto found = std::ranges::find_if(
     availablePresentModes, [](const auto& m) { return m == VK_PRESENT_MODE_MAILBOX_KHR; });
    return found != availablePresentModes.end() ? *found : VK_PRESENT_MODE_FIFO_KHR;
}

auto querySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
 -> swap_chain_support_details {
    swap_chain_support_details details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    details.formats = vulcan_function_wrapper<VkSurfaceFormatKHR>(
     vkGetPhysicalDeviceSurfaceFormatsKHR, device, surface);
    details.presentModes = vulcan_function_wrapper<VkPresentModeKHR>(
     vkGetPhysicalDeviceSurfacePresentModesKHR, device, surface);
    return details;
}

auto createSwapChainInfo(GLFWwindow* const window,
                         const VkPhysicalDevice& physicalDevice,
                         const VkSurfaceKHR& surface,
                         const VkSwapchainKHR& old_swap_chain) {
    swap_chain_support_details swapChainSupport = querySwapChainSupport(physicalDevice, surface);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(window, swapChainSupport.capabilities);
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if(swapChainSupport.capabilities.maxImageCount > 0
       && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.oldSwapchain = old_swap_chain;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    auto queue_indices = find_queue_families(physicalDevice, surface);
    uint32_t queueFamilyIndices[] = {queue_indices.graphics_family.value(),
                                     queue_indices.presentation_family.value()};
    if(queue_indices.graphics_family != queue_indices.presentation_family) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;     // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    return createInfo;
}