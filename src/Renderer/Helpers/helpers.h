#pragma once

#include <cstdint>
#include <vulkan/vulkan_raii.hpp>

#include "../Command/CommandPool.h"
#include "../Device/Device.h"

namespace Renderer {

void createImage(Device &device, uint32_t width, uint32_t height,
                 vk::Format format, vk::ImageTiling tiling,
                 vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
                 vk::raii::Image &image, vk::raii::DeviceMemory &imageMemory);

vk::raii::ImageView createImageView(Device &device, vk::raii::Image &image,
                                    vk::Format format,
                                    vk::ImageAspectFlagBits aspect);

void transitionImageLayout(Device &device, CommandPool &commandPool,
                           vk::Format format, vk::ImageLayout oldLayout,
                           vk::ImageLayout newLayout, vk::raii::Image &image);

bool hasStencilComponent(vk::Format format);

} // namespace Renderer
