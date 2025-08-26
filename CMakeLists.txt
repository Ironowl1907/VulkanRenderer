cmake_minimum_required(VERSION 3.10)  # Fixed: VERSION 4.0 doesn't exist
project(VulkanRenderer)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Renderer
    main.cpp
)

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

find_package(Threads REQUIRED)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(Renderer PRIVATE -Wall -Wextra)
endif()

