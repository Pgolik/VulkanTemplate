#pragma once
#include "triangle_example/command_buffer.hpp"

#include <array>
#include <fmt/printf.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }
    static auto getAttributeDescriptions() {
        return std::array {VkVertexInputAttributeDescription {.location = 0,
                                                              .binding = 0,
                                                              .format = VK_FORMAT_R32G32_SFLOAT,
                                                              .offset = offsetof(Vertex, pos)},
                           VkVertexInputAttributeDescription {.location = 1,
                                                              .binding = 0,
                                                              .format = VK_FORMAT_R32G32B32_SFLOAT,
                                                              .offset = offsetof(Vertex, color)}};
    }
};
uint32_t find_memory_type(uint32_t typeFilter,
                          const VkMemoryPropertyFlags& properties,
                          const VkPhysicalDevice& phy_device) {
    VkPhysicalDeviceMemoryProperties memProperties {};
    vkGetPhysicalDeviceMemoryProperties(phy_device, &memProperties);
    auto indexes =
     std::views::iota(static_cast<uint32_t>(0), memProperties.memoryTypeCount)
     | std::views::filter([&typeFilter](const auto& index) { return typeFilter & (1 << index); })
     | std::views::filter([&memProperties, properties](const auto& index) {
           return (memProperties.memoryTypes[index].propertyFlags & properties) == properties;
       });
    if(indexes.empty()) { throw std::runtime_error("failed to find suitable memory type!"); }
    return *indexes.begin();
}

const std::vector<Vertex> vertices = {{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                      {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
                                      {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
                                      {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};
const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};

auto get_vertex_pipeline_info(const auto& binding_dec, const auto& attribute_dec) {
    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &binding_dec; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_dec.size());
    vertexInputInfo.pVertexAttributeDescriptions = attribute_dec.data(); // Optional
    return vertexInputInfo;
}

auto get_input_assembly_pipeline_info() {
    VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    return inputAssembly;
}

VkDeviceMemory allocate_buffer(const VkDevice& device,
                               const VkPhysicalDevice& phy_dev,
                               const VkMemoryPropertyFlags& properties,
                               const VkBuffer& buffer) {
    VkDeviceMemory bufferMemory {};
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
     find_memory_type(memRequirements.memoryTypeBits, properties, phy_dev);

    if(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
    return bufferMemory;
}

VkBuffer
create_buffer(const VkDevice& device, const VkDeviceSize& size, const VkBufferUsageFlags& usage) {
    VkBufferCreateInfo bufferInfo {};
    VkBuffer buffer {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    return buffer;
}

void copy_buffer(const VkDevice& device,
                 VkBuffer srcBuffer,
                 VkBuffer dstBuffer,
                 const VkDeviceSize& size,
                 const VkCommandPool& command_pool,
                 const VkQueue& queue) {
    auto command_buff =
     create_command_buffer(device, command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1)[0];
    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buff, &beginInfo);
    VkBufferCopy copyRegion {};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(command_buff, srcBuffer, dstBuffer, 1, &copyRegion);
    vkEndCommandBuffer(command_buff);
    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &command_buff;
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(device, command_pool, 1, &command_buff);
}