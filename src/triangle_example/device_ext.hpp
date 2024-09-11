#pragma once
#include <ranges>
#include <vector>
#include <vulkan/vulkan.hpp>

const std::vector<const char*> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

auto support_extensions(const VkPhysicalDevice& device) -> bool {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(
     device, nullptr, &extensionCount, availableExtensions.data());
    return std::ranges::all_of(device_extensions, [&availableExtensions](const auto& ext) {
        return std::ranges::find(availableExtensions, ext, &VkExtensionProperties::extensionName)
               != availableExtensions.end();
    });
}