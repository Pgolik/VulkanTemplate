#include "triangle_example/triangle_example.hpp"

#include <fmt/format.h>

int main() {
    try {
        window vk_window;
        vk_window.run();
    } catch(const std::exception& e) {
        fmt::print("Issue: {}", e.what());
        return 1;
    }
    return 0;
}