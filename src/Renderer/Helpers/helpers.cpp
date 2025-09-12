#include "helpers.h"
#include <vulkan/vulkan_raii.hpp>

namespace Renderer {

void createImage(Device &device, uint32_t width, uint32_t height,
                 vk::Format format, vk::ImageTiling tiling,
                 vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
                 vk::raii::Image &image, vk::raii::DeviceMemory &imageMemory) {

  vk::ImageCreateInfo imageInfo({}, vk::ImageType::e2D, format,
                                {width, height, 1}, 1, 1,
                                vk::SampleCountFlagBits::e1, tiling, usage,
                                vk::SharingMode::eExclusive, 0);

  image = vk::raii::Image(device.GetDevice(), imageInfo);

  vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
  vk::MemoryAllocateInfo allocInfo(
      memRequirements.size,
      device.FindMemoryType(memRequirements.memoryTypeBits, properties));
  imageMemory = vk::raii::DeviceMemory(device.GetDevice(), allocInfo);
  image.bindMemory(*imageMemory, 0);
}

} // namespace Renderer
