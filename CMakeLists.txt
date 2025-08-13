cmake_minimum_required(VERSION 3.6)
project(VulkanRenderer)

# Dependencies
include_directories(vendor/stb_image)
include_directories(vendor/tiny_obj_loader)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(Renderer main.cpp)

target_link_libraries(Renderer
    glfw
    vulkan
    dl
    pthread
    X11
    Xxf86vm
    Xrandr
    Xi
)
