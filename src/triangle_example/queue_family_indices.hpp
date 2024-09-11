#pragma once

#include "triangle_example/helper.hpp"

#include <optional>
#include <range/v3/view/enumerate.hpp>
#include <ranges>
#include <stdlib.h>
#include <vector>
#include <vulkan/vulkan.hpp>

struct queue_family_indices {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> presentation_family;
    auto is_graphics_supported() const -> bool { return graphics_family.has_value(); }
    auto is_presentation_supported() const -> bool { return presentation_family.has_value(); }
};

auto find_queue_families(VkPhysicalDevice phy_device, VkSurfaceKHR surface)
 -> queue_family_indices {
    auto result = queue_family_indices();
    const auto queue_families = vulcan_function_wrapper<VkQueueFamilyProperties>(
     vkGetPhysicalDeviceQueueFamilyProperties, phy_device);

    for(const auto& [index, value_prop]: ranges::view::enumerate(queue_families)) {
        if(value_prop.queueFlags & VK_QUEUE_GRAPHICS_BIT) { result.graphics_family = index; }

        VkBool32 presentation_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(phy_device, index, surface, &presentation_support);
        if(presentation_support) { result.presentation_family = index; }

        if(result.graphics_family.has_value() && result.presentation_family.has_value()) { break; }
    }
    return result;
}