#pragma once

#include <filesystem>
#include <fstream>
#include <ranges>
#include <vector>
#include <vulkan/vulkan.hpp>

auto load_file(const std::filesystem::path& p) {
    std::ifstream file(p, std::ios::binary);
    if(!file.is_open()) { throw std::runtime_error("failed to open file!"); }
    file.unsetf(std::ios::skipws);
    std::istream_iterator<char> start(file), end;
    return std::vector<char> {start, end};
}

auto gat_shader_info(const VkShaderModule& shader, VkShaderStageFlagBits stage) {
    VkPipelineShaderStageCreateInfo fragShaderStageInfo {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = stage;
    fragShaderStageInfo.module = shader;
    fragShaderStageInfo.pName = "main";
    return fragShaderStageInfo;
}

auto get_vert_shader_stage_inf(const VkShaderModule& vert_module) {
    return gat_shader_info(vert_module, VK_SHADER_STAGE_VERTEX_BIT);
}

auto get_frag_shader_stage_inf(const VkShaderModule& frag_module) {
    return gat_shader_info(frag_module, VK_SHADER_STAGE_FRAGMENT_BIT);
}

auto get_shader_module(const auto& device, const std::filesystem::path& p) {
    auto code = load_file(p);
    VkShaderModuleCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule {};
    if(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    return shaderModule;
}