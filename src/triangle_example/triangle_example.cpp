#include "triangle_example/triangle_example.hpp"

#include "consts.hpp"
#include "triangle_example/color_blending_config.hpp"
#include "triangle_example/command_buffer.hpp"
#include "triangle_example/descriptor_layout.hpp"
#include "triangle_example/device_ext.hpp"
#include "triangle_example/dynamic_state.hpp"
#include "triangle_example/helper.hpp"
#include "triangle_example/image_view_details.hpp"
#include "triangle_example/multisampling.hpp"
#include "triangle_example/queue_family_indices.hpp"
#include "triangle_example/rasterizer.hpp"
#include "triangle_example/read_shaders.hpp"
#include "triangle_example/swap_chain_support_details.hpp"
#include "triangle_example/vertex_input.hpp"

#include <fmt/printf.h>
#include <range/v3/to_container.hpp>
#include <range/v3/view/for_each.hpp>
#include <ranges>
#include <span>
#include <stdexcept>

namespace rv = ranges::view;

auto window::frame_buffer_change_callback(GLFWwindow* win, int width, int height) -> void {
    std::ignore = width;
    std::ignore = height;
    static_cast<window*>(glfwGetWindowUserPointer(win))->framebuffer_resized = true;
};

auto create_debug_utils_messenger_EXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* create_info,
                                      const VkAllocationCallbacks* allocator,
                                      VkDebugUtilsMessengerEXT* debug_messenger) -> VkResult {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
     instance, "vkCreateDebugUtilsMessengerEXT");
    if(func != nullptr) {
        return func(instance, create_info, allocator, debug_messenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroy_debug_utils_messenger_EXT(VkInstance instance,
                                       VkDebugUtilsMessengerEXT debugMessenger,
                                       const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
     instance, "vkDestroyDebugUtilsMessengerEXT");
    if(func != nullptr) { func(instance, debugMessenger, pAllocator); }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
               VkDebugUtilsMessageTypeFlagsEXT messageType,
               const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
               void* pUserData) {
    std::ignore = messageType;
    std::ignore = pUserData;
    if(messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) return VK_FALSE;
    fmt::print("validation layer: {}\n", pCallbackData->pMessage);
    return VK_FALSE;
}

auto contains(std::ranges::range auto in, std::ranges::range auto is) -> bool {
    return !std::ranges::all_of(is, [&in](const auto& element) {
        if(std::ranges::find_if(
            in, [element](const auto& el) { return strncmp(el, element, 256) == 0; })
           == in.end()) {
            fmt::print("Not in extensions {}\n, {}", element, fmt::join(in, ", "));
            return false;
        }
        fmt::print("Enable extension {}\n", element);
        return true;
    });
}

auto get_debug_utils_messenger_info() {
    return VkDebugUtilsMessengerCreateInfoEXT {
     .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
     .pNext = nullptr,
     .flags = {},
     .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
     .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                    | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                    | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
     .pfnUserCallback = debug_callback,
     .pUserData = nullptr};
}

void window::set_debug_callback_message() {
    auto create_info = get_debug_utils_messenger_info();
    if(create_debug_utils_messenger_EXT(vulkan_instance, &create_info, nullptr, &debug_messenger)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

auto window::check_validation_layer_support() -> bool {
    auto available_layers =
     vulcan_function_wrapper<VkLayerProperties>(vkEnumerateInstanceLayerProperties);
    return contains(available_layers | std::views::transform(&VkLayerProperties::layerName),
                    supported_layers);
}

window::window(): physical_device(VK_NULL_HANDLE) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    vulkan_window =
     glfwCreateWindow(consts::HEIGHT, consts::WIDTH, "Vulkan window", nullptr, nullptr);
    glfwSetWindowUserPointer(vulkan_window, this);
    glfwSetFramebufferSizeCallback(vulkan_window, frame_buffer_change_callback);
    init_vulkan();
}

auto window::create_view_port() -> VkViewport {
    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swap_chain_extent.width;
    viewport.height = (float)swap_chain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
}

auto window::create_scissor() -> VkRect2D {
    VkRect2D scissor {};
    scissor.offset = {0, 0};
    scissor.extent = swap_chain_extent;
    return scissor;
}

auto window::get_view_point_and_scissor() -> VkPipelineViewportStateCreateInfo {
    VkPipelineViewportStateCreateInfo viewportState {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    return viewportState;
}

void window::create_pipeline_layout() {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
    if(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipeline_layout)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

auto window::clean_up_swap_chain() -> void {
    for(auto framebuffer: swap_chain_framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    for(auto i_v: swap_chain_image_views) { vkDestroyImageView(device, i_v, nullptr); }
    vkDestroySwapchainKHR(device, swap_chain, nullptr);
}

auto window::recreate_swap_chain() -> void {
    int width = 0, height = 0;
    glfwGetFramebufferSize(vulkan_window, &width, &height);
    while(width == 0 || height == 0) {
        glfwGetFramebufferSize(vulkan_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    auto old_swap_chain = swap_chain;
    create_swap_chain_device(old_swap_chain);
    vkDestroySwapchainKHR(device, old_swap_chain, nullptr);

    for(auto i_v: swap_chain_image_views) { vkDestroyImageView(device, i_v, nullptr); }
    create_image_view();

    for(auto framebuffer: swap_chain_framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    create_frame_buffers();
}

void window::create_frame_buffers() {
    swap_chain_framebuffers.resize(swap_chain_image_views.size());
    for(auto [index, elem]: rv::enumerate(swap_chain_image_views)) {
        VkImageView attachments[] = {elem};
        VkFramebufferCreateInfo framebufferInfo {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = render_pass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swap_chain_extent.width;
        framebufferInfo.height = swap_chain_extent.height;
        framebufferInfo.layers = 1;
        if(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swap_chain_framebuffers[index])
           != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

auto window::create_index_buffer() -> void {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    const auto staging = create_buffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    const auto staging_mem =
     allocate_buffer(device,
                     physical_device,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     staging);

    void* data = nullptr;
    vkMapMemory(device, staging_mem, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), bufferSize);
    vkUnmapMemory(device, staging_mem);
    index_buffer = create_buffer(
     device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    index_buffer_memory =
     allocate_buffer(device, physical_device, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer);
    copy_buffer(device, staging, index_buffer, bufferSize, command_pool, graphics_queue);
    vkDestroyBuffer(device, staging, nullptr);
    vkFreeMemory(device, staging_mem, nullptr);
}

auto window::create_uniform_buffer() -> void {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniform_buffers.resize(consts::MAX_FRAMES_IN_FLIGHT);
    uniform_buffers_memory.resize(consts::MAX_FRAMES_IN_FLIGHT);
    uniform_buffers_mapped.resize(consts::MAX_FRAMES_IN_FLIGHT);

    for(size_t i = 0; i < consts::MAX_FRAMES_IN_FLIGHT; i++) {
        uniform_buffers[i] = create_buffer(device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        uniform_buffers_memory[i] =
         allocate_buffer(device,
                         physical_device,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         uniform_buffers[i]);
        vkMapMemory(
         device, uniform_buffers_memory[i], 0, bufferSize, 0, &uniform_buffers_mapped[i]);
    }
}

auto window::create_vertex_buffer() -> void {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    const auto staging = create_buffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    const auto staging_mem =
     allocate_buffer(device,
                     physical_device,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     staging);

    void* data = nullptr;
    vkMapMemory(device, staging_mem, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), bufferSize);
    vkUnmapMemory(device, staging_mem);
    vertex_buffer = create_buffer(
     device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vertex_buffer_memory =
     allocate_buffer(device, physical_device, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer);
    copy_buffer(device, staging, vertex_buffer, bufferSize, command_pool, graphics_queue);
    vkDestroyBuffer(device, staging, nullptr);
    vkFreeMemory(device, staging_mem, nullptr);
}

void window::create_graphics_pipeline() {
    auto vertShaderModule = get_shader_module(device, SHADERS_DIR "/shader.vert.spv");
    auto fragShaderModule = get_shader_module(device, SHADERS_DIR "/shader.frag.spv");

    auto vert_stage_info = get_vert_shader_stage_inf(vertShaderModule);
    auto frag_stage_info = get_frag_shader_stage_inf(fragShaderModule);

    VkPipelineShaderStageCreateInfo shaderStages[] = {vert_stage_info, frag_stage_info};
    VkGraphicsPipelineCreateInfo pipelineInfo {};

    const auto bindingDescription = Vertex::getBindingDescription();
    const auto attributeDescriptions = Vertex::getAttributeDescriptions();
    const auto vert_info = get_vertex_pipeline_info(bindingDescription, attributeDescriptions);
    const auto ass_info = get_input_assembly_pipeline_info();
    const auto view_point_info = get_view_point_and_scissor();
    const auto rastr_info = get_rasterizer_info();
    const auto multisam_info = get_multisampling_info();
    const auto blending = get_attachment_color_blending();
    const auto color_blend = get_global_color_blending_info(blending);
    const auto dynamic_info = get_dynamics_state();

    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vert_info;
    pipelineInfo.pInputAssemblyState = &ass_info;
    pipelineInfo.pViewportState = &view_point_info;
    pipelineInfo.pRasterizationState = &rastr_info;
    pipelineInfo.pMultisampleState = &multisam_info;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &color_blend;
    pipelineInfo.pDynamicState = &dynamic_info;
    pipelineInfo.layout = pipeline_layout;
    pipelineInfo.renderPass = render_pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1;              // Optional

    if(vkCreateGraphicsPipelines(
        device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphics_pipeline)
       != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

auto window::create_surface() -> void {
    if(glfwCreateWindowSurface(vulkan_instance, vulkan_window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

auto window::supported_extensions() -> std::vector<const char*> {
    uint32_t glfw_extensions_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);
    auto glfw_extensions_iterable = std::span(glfw_extensions, glfw_extensions_count);
    auto extended_glfw_extensions =
     std::vector(glfw_extensions_iterable.begin(), glfw_extensions_iterable.end());
    if(consts::enable_debug) {
        extended_glfw_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extended_glfw_extensions;
}

auto is_device_suitable(const VkPhysicalDevice& phy_device, const VkSurfaceKHR& surface)
 -> std::size_t {
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(phy_device, &device_properties);
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(phy_device, &device_features);

    const auto queue_family = find_queue_families(phy_device, surface);
    auto score = device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 1000 : 0;
    score += queue_family.is_graphics_supported() ? 100 : 0;
    score += queue_family.is_presentation_supported() ? 100 : 0;
    const auto device_ext_support = support_extensions(phy_device);
    score += device_ext_support ? 100 : 0;
    if(device_ext_support) {
        const auto swap_chain = querySwapChainSupport(phy_device, surface);
        score += !swap_chain.formats.empty() ? 200 : 0;
        score += !swap_chain.presentModes.empty() ? 200 : 0;
    }
    score += device_features.geometryShader ? 1 : 0;
    return score;
}

void window::pick_physical_device() {
    auto devices =
     vulcan_function_wrapper<VkPhysicalDevice>(vkEnumeratePhysicalDevices, vulkan_instance);
    if(devices.size() == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }
    auto found_physical_device =
     devices | std::views::filter([](const auto& val) { return val != VK_NULL_HANDLE; });
    if(found_physical_device.empty()) { throw std::runtime_error("Failed to find a suitable GPU"); }
    auto accepted_physical_device =
     std::ranges::max_element(found_physical_device, [this](const auto& lhs, const auto& rhs) {
         return is_device_suitable(lhs, surface) < is_device_suitable(rhs, surface);
     });
    if(accepted_physical_device != found_physical_device.end()
       && is_device_suitable(*accepted_physical_device, surface) != 0) {
        physical_device = *accepted_physical_device;
        return;
    }
    throw std::runtime_error("Failed to find GPUs with required features enabled!");
}

void window::init_vulkan() {
    create_instance();
    fmt::print("create_instance Finish \n");
    set_debug_callback_message();
    fmt::print("set_debug_callback_message Finish \n");
    create_surface();
    fmt::print("create_surface Finish \n");
    pick_physical_device();
    fmt::print("pick_physical_device Finish \n");
    create_logic_device();
    fmt::print("create_logic_device Finish \n");
    create_swap_chain_device();
    fmt::print("create_swap_chain_device Finish \n");
    create_image_view();
    fmt::print("create_image_view Finish \n");
    create_render_pass();
    fmt::print("create_render_pass Finish \n");
    // descriptor_set_layout = create_descriptor_set_layout(device);
    create_pipeline_layout();
    fmt::print("create_pipeline_layout Finish \n");
    create_graphics_pipeline();
    fmt::print("create_graphics_pipeline Finish \n");
    create_frame_buffers();
    fmt::print("create_frame_buffers Finish \n");
    // description_pool = create_descriptor_pool(device);
    // descriptor_sets = create_descriptor_sets(device, description_pool, descriptor_set_layout);
    // sprawdzone
    const auto queue_fam_ind = find_queue_families(physical_device, surface);
    command_pool = create_command_buffer_pool(device,
                                              queue_fam_ind,
                                              VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                              VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    command_buffers = create_command_buffer(
     device, command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, consts::MAX_FRAMES_IN_FLIGHT);
    create_vertex_buffer();
    create_index_buffer();
    // create_uniform_buffer();
    // configure_descriptors(device, uniform_buffers, descriptor_sets);
    create_sync_objects();
};

// sprawdzone
auto window::start_render_pass(const VkCommandBuffer& commandBuffer, uint32_t imageIndex) -> void {
    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                  // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional
    if(vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = render_pass;
    renderPassInfo.framebuffer = swap_chain_framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swap_chain_extent;
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
    const auto viewport = create_view_port();
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    const auto scissor = create_scissor();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    VkBuffer vertexBuffers[] = {vertex_buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, index_buffer, 0, VK_INDEX_TYPE_UINT16);
    // vkCmdBindDescriptorSets(commandBuffer,
    //                         VK_PIPELINE_BIND_POINT_GRAPHICS,
    //                         pipeline_layout,
    //                         0,
    //                         1,
    //                         &descriptor_sets[current_frame],
    //                         0,
    //                         nullptr);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
    if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

// sprawdzone
auto window::create_sync_objects() -> void {
    image_available_semaphores.resize(consts::MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores.resize(consts::MAX_FRAMES_IN_FLIGHT);
    in_flight_fences.resize(consts::MAX_FRAMES_IN_FLIGHT);
    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for(const auto index: std::views::iota(0, consts::MAX_FRAMES_IN_FLIGHT)) {
        if(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &image_available_semaphores[index])
            != VK_SUCCESS
           || vkCreateSemaphore(device, &semaphoreInfo, nullptr, &render_finished_semaphores[index])
               != VK_SUCCESS
           || vkCreateFence(device, &fenceInfo, nullptr, &in_flight_fences[index]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphores!");
        }
    }
}

auto window::draw_frame() -> void {
    vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);
    uint32_t imageIndex {};
    auto acquire_result = vkAcquireNextImageKHR(device,
                                                swap_chain,
                                                UINT64_MAX,
                                                image_available_semaphores[current_frame],
                                                VK_NULL_HANDLE,
                                                &imageIndex);
    if(acquire_result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swap_chain();
        return;
    } else if(acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }
    vkResetCommandBuffer(command_buffers[current_frame], 0);
    start_render_pass(command_buffers[current_frame], imageIndex);
    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {image_available_semaphores[current_frame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &command_buffers[current_frame];
    VkSemaphore signalSemaphores[] = {render_finished_semaphores[current_frame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    // update_uniform_data(current_frame);
    vkResetFences(device, 1, &in_flight_fences[current_frame]);
    if(vkQueueSubmit(graphics_queue, 1, &submitInfo, in_flight_fences[current_frame])
       != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }
    VkPresentInfoKHR presentInfo {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {swap_chain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional
    auto present_result = vkQueuePresentKHR(presentation_queue, &presentInfo);
    if(present_result == VK_ERROR_OUT_OF_DATE_KHR || framebuffer_resized
       || present_result == VK_SUBOPTIMAL_KHR) {
        framebuffer_resized = false;
        recreate_swap_chain();
    } else if(present_result != VK_SUCCESS) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }
    current_frame = (current_frame + 1) % consts::MAX_FRAMES_IN_FLIGHT;
}

auto window::create_image_view() -> void {
    swap_chain_image_views.resize(images.size());
    for(const auto& [index, image]: images | rv::enumerate) {
        auto view_info = create_info_view_image(image, swap_chain_image_format);
        if(vkCreateImageView(device, &view_info, nullptr, &swap_chain_image_views[index])
           != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

auto window::update_uniform_data(uint32_t currentImage) -> void {
    // static auto startTime = std::chrono::high_resolution_clock::now();
    // auto currentTime = std::chrono::high_resolution_clock::now();
    // float time =
    //  std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    UniformBufferObject ubo {};
    // ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.model = glm::mat4(1.0f);
    ubo.view = glm::lookAt(
     glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    // ubo.view = glm::mat4(1.0f);
    ubo.proj = glm::perspective(
     glm::radians(45.0f), swap_chain_extent.width / (float)swap_chain_extent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;
    memcpy(uniform_buffers_mapped[currentImage], &ubo, sizeof(ubo));
}

auto window::create_swap_chain_device(const VkSwapchainKHR& old_swap_chain) -> void {
    auto swap_chain_info =
     createSwapChainInfo(vulkan_window, physical_device, surface, old_swap_chain);
    if(vkCreateSwapchainKHR(device, &swap_chain_info, nullptr, &swap_chain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }
    swap_chain_image_format = swap_chain_info.imageFormat;
    swap_chain_extent = swap_chain_info.imageExtent;
    images = vulcan_function_wrapper<VkImage>(vkGetSwapchainImagesKHR, device, swap_chain);
}

auto window::create_logic_device() -> void {
    const auto queue = find_queue_families(physical_device, surface);
    const auto priority = 1.0f;
    std::array queue_families {queue.graphics_family.value(), queue.presentation_family.value()};
    std::vector<VkDeviceQueueCreateInfo> queue_create_info =
     queue_families | std::views::transform([&priority](const auto& family) {
         return VkDeviceQueueCreateInfo {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                         .pNext = nullptr,
                                         .flags = VkDeviceQueueCreateFlags {},
                                         .queueFamilyIndex = family,
                                         .queueCount = 1,
                                         .pQueuePriorities = &priority};
     })
     | ranges::to_vector;
    VkPhysicalDeviceFeatures device_features {};
    VkDeviceCreateInfo create_info {
     .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
     .pNext = nullptr,
     .flags = VkDeviceCreateFlags {},
     .queueCreateInfoCount = 1,
     .pQueueCreateInfos = queue_create_info.data(),
     .enabledLayerCount = 0,
     .ppEnabledLayerNames = nullptr,
     .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
     .ppEnabledExtensionNames = device_extensions.data(),
     .pEnabledFeatures = &device_features,
    };
    if(vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
    vkGetDeviceQueue(device, queue.graphics_family.value(), 0, &graphics_queue);
    vkGetDeviceQueue(device, queue.presentation_family.value(), 0, &presentation_queue);
}

void window::create_instance() {
    VkApplicationInfo app_info {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                .pNext = nullptr,
                                .pApplicationName = "Hello Triangle",
                                .applicationVersion = VK_MAKE_API_VERSION(0, 0, 0, 1),
                                .pEngineName = "No engine",
                                .engineVersion = VK_MAKE_API_VERSION(0, 0, 0, 1),
                                .apiVersion = VK_API_VERSION_1_0};

    auto extensions_list = supported_extensions();
    auto debug_utils_create_info = get_debug_utils_messenger_info();
    VkInstanceCreateInfo create_info {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                      .pNext = &debug_utils_create_info,
                                      .flags = {},
                                      .pApplicationInfo = &app_info,
                                      .enabledLayerCount = supported_layers.size(),
                                      .ppEnabledLayerNames = supported_layers.data(),
                                      .enabledExtensionCount =
                                       static_cast<uint32_t>(extensions_list.size()),
                                      .ppEnabledExtensionNames = extensions_list.data()};

    if(check_validation_layer_support()) { throw std::runtime_error("Layers not supported!"); }

    if(vkCreateInstance(&create_info, nullptr, &vulkan_instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

void window::create_render_pass() {
    VkAttachmentDescription colorAttachment {};
    colorAttachment.format = swap_chain_image_format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDependency dependency {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDescription subpass {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    if(vkCreateRenderPass(device, &renderPassInfo, nullptr, &render_pass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void window::main_loop() {
    while(!glfwWindowShouldClose(vulkan_window)) {
        glfwPollEvents();
        draw_frame();
    }
    vkDeviceWaitIdle(device);
}
void window::cleanup() {
    destroy_debug_utils_messenger_EXT(vulkan_instance, debug_messenger, nullptr);
    vkDestroyCommandPool(device, command_pool, nullptr);
    for(const auto index: std::views::iota(0, consts::MAX_FRAMES_IN_FLIGHT)) {
        vkDestroySemaphore(device, image_available_semaphores[index], nullptr);
        vkDestroySemaphore(device, render_finished_semaphores[index], nullptr);
        vkDestroyFence(device, in_flight_fences[index], nullptr);
    }
    clean_up_swap_chain();
    // for(size_t i = 0; i < consts::MAX_FRAMES_IN_FLIGHT; i++) {
    //     vkDestroyBuffer(device, uniform_buffers[i], nullptr);
    //     vkFreeMemory(device, uniform_buffers_memory[i], nullptr);
    // }
    vkDestroyBuffer(device, vertex_buffer, nullptr);
    vkFreeMemory(device, vertex_buffer_memory, nullptr);
    vkDestroyBuffer(device, index_buffer, nullptr);
    vkFreeMemory(device, index_buffer_memory, nullptr);
    // vkDestroyDescriptorPool(device, description_pool, nullptr);
    vkDestroyPipeline(device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    // vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
    vkDestroyRenderPass(device, render_pass, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(vulkan_instance, surface, nullptr);
    vkDestroyInstance(vulkan_instance, nullptr);
    glfwDestroyWindow(vulkan_window);
    glfwTerminate();
}
void window::run() {
    main_loop();
    cleanup();
}