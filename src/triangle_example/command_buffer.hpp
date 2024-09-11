#pragma once
#include "triangle_example/queue_family_indices.hpp"

#include <vulkan/vulkan.hpp>

auto create_command_buffer_pool(const VkDevice& device,
                                const queue_family_indices& queue_family_indices,
                                const VkStructureType& type,
                                const VkCommandPoolCreateFlags& flag) {
    VkCommandPool command_pool {};
    VkCommandPoolCreateInfo poolInfo {};
    poolInfo.sType = type;
    poolInfo.flags = flag;
    poolInfo.queueFamilyIndex = queue_family_indices.graphics_family.value();

    if(vkCreateCommandPool(device, &poolInfo, nullptr, &command_pool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
    return command_pool;
}

auto create_command_buffer(const VkDevice& device,
                           const VkCommandPool& command_pool,
                           const VkCommandBufferLevel& level,
                           uint32_t max_buf_count) -> std::vector<VkCommandBuffer> {
    std::vector<VkCommandBuffer> command_buffers(max_buf_count);
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = command_pool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = max_buf_count;

    if(vkAllocateCommandBuffers(device, &allocInfo, command_buffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
    return command_buffers;
}