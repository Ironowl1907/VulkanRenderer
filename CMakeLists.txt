cmake_minimum_required(VERSION 4.0)
project(VulkanRenderer)

# Dependencies
# include_directories(vendor/stb_image)
# include_directories(vendor/tiny_obj_loader)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(Renderer main.cpp ./src/Renderer/Instance/Instance.cpp)

add_custom_target(run
  COMMAND ./build/Renderer
  DEPENDS Renderer
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

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
