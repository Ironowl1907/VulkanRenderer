#pragma once

#include <cstdint>
#include <vulkan/vulkan_raii.hpp>

#include "../Device/Device.h"

namespace Renderer {

void createImage(Device &device, uint32_t width, uint32_t height,
                 vk::Format format, vk::ImageTiling tiling,
                 vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
                 vk::raii::Image &image, vk::raii::DeviceMemory &imageMemory);

vk::raii::ImageView createImageView(Device &device, vk::raii::Image &image,
                                    vk::Format format,
                                    vk::ImageAspectFlagBits aspect);

} // namespace Renderer
