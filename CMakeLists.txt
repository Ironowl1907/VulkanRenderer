cmake_minimum_required(VERSION 3.10)  # Fixed: VERSION 4.0 doesn't exist
project(VulkanRenderer)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Renderer
    main.cpp
    ./src/Renderer/Instance/Instance.cpp
    ./src/Renderer/Device/Device.cpp
    ./src/Renderer/Window/Window.cpp
)

add_custom_target(run
    COMMAND ./build/Renderer
    DEPENDS Renderer
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

set(VULKAN_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/1.4.321.1/x86_64")
set(Vulkan_INCLUDE_DIR "${VULKAN_SDK_PATH}/include")
set(Vulkan_LIBRARY "${VULKAN_SDK_PATH}/lib/libvulkan.so")

if(NOT EXISTS "${VULKAN_SDK_PATH}")
  message(FATAL_ERROR "Vulkan SDK not found at: ${VULKAN_SDK_PATH}")
endif()

if(NOT EXISTS "${Vulkan_INCLUDE_DIR}")
  message(FATAL_ERROR "Vulkan headers not found at: ${Vulkan_INCLUDE_DIR}")
endif()

if(NOT EXISTS "${Vulkan_LIBRARY}")
  message(FATAL_ERROR "Vulkan library not found at: ${Vulkan_LIBRARY}")
endif()

find_package(glfw3 REQUIRED)
find_package(X11 REQUIRED)

target_include_directories(Renderer PRIVATE
    ${Vulkan_INCLUDE_DIR}
    ./src
)

target_link_libraries(Renderer PRIVATE
    ${Vulkan_LIBRARY}
    glfw
    ${CMAKE_DL_LIBS}
    Threads::Threads
    ${X11_LIBRARIES}
    ${X11_Xxf86vm_LIB}
    ${X11_Xrandr_LIB}
    ${X11_Xi_LIB}
)

find_package(Threads REQUIRED)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(Renderer PRIVATE -Wall -Wextra)
endif()

message(STATUS "Using Vulkan SDK at: ${VULKAN_SDK_PATH}")
message(STATUS "Vulkan include dir: ${Vulkan_INCLUDE_DIR}")
message(STATUS "Vulkan library: ${Vulkan_LIBRARY}")
