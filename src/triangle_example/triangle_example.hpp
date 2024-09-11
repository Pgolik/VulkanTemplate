#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <chrono>
#include <fmt/printf.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <stdexcept>
#include <vulkan/vulkan.h>

constexpr std::array<const char*, 1> supported_layers = {{"VK_LAYER_KHRONOS_validation"}};

struct window {
    window();
    void run();

private:
    static void frame_buffer_change_callback(GLFWwindow* win, int width, int height);
    void init_vulkan();
    void create_surface();
    void set_debug_callback_message();
    void pick_physical_device();
    void main_loop();
    void cleanup();
    void create_instance();
    void create_logic_device();
    void create_swap_chain_device(const VkSwapchainKHR& old_swap_chain = VK_NULL_HANDLE);
    void create_image_view();
    std::vector<const char*> supported_extensions();
    bool check_validation_layer_support();
    void create_pipeline_layout();
    void create_graphics_pipeline();
    void create_render_pass();
    auto create_view_port() -> VkViewport;
    auto create_scissor() -> VkRect2D;
    auto get_view_point_and_scissor() -> VkPipelineViewportStateCreateInfo;
    void create_frame_buffers();
    void start_render_pass(const VkCommandBuffer& commandBuffer, uint32_t imageIndex);
    void draw_frame();
    void create_sync_objects();
    void recreate_swap_chain();
    void clean_up_swap_chain();
    void create_vertex_buffer();
    void create_index_buffer();
    void create_uniform_buffer();
    void update_uniform_data(uint32_t currentImage);

    GLFWwindow* vulkan_window;
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memory;
    std::vector<VkBuffer> uniform_buffers;
    std::vector<VkDeviceMemory> uniform_buffers_memory;
    std::vector<void*> uniform_buffers_mapped;
    VkDescriptorPool description_pool;
    VkSwapchainKHR swap_chain;
    VkInstance vulkan_instance;
    std::vector<VkImageView> swap_chain_image_views;
    VkSurfaceKHR surface;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue presentation_queue;
    VkQueue graphics_queue;
    std::vector<VkImage> images;
    VkFormat swap_chain_image_format;
    VkExtent2D swap_chain_extent;
    std::vector<VkDescriptorSet> descriptor_sets;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkRenderPass render_pass;
    VkPipeline graphics_pipeline;
    std::vector<VkFramebuffer> swap_chain_framebuffers;
    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers;
    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;
    uint32_t current_frame = 0;
    bool framebuffer_resized = false;
};