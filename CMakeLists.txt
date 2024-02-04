cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
project("Vulkan Turorial")

find_package(glfw3 3.3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(X11 REQUIRED)

set(SRC ${CMAKE_SOURCE_DIR}/src)
file(GLOB_RECURSE SOURCE ${SRC}/*.cpp ${SRC}/**/*.cpp)
file(GLOB_RECURSE HEADERS ${SRC}/*.hpp ${SRC}/**/*.hpp)

add_compile_options(-Wall -Wextra -pedantic -Werror -std=c++20)
add_executable(VulkanTemplate ${SOURCE} ${HEADERS})

target_include_directories(VulkanTemplate PRIVATE ${SRC})
target_link_libraries(VulkanTemplate glfw vulkan dl pthread Xxf86vm Xrandr Xi fmt)

set_target_properties(VulkanTemplate PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/artifacts/bin)