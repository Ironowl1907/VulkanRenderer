#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "../Command/CommandPool.h"
#include "../Device/Device.h"

namespace Renderer {

class BufferManager {
public:
  BufferManager() {}
  ~BufferManager() {}

  void CreateBuffer(Renderer::Device &device, vk::DeviceSize size,
                    vk::BufferUsageFlags usage,
                    vk::MemoryPropertyFlags properties,
                    vk::raii::Buffer &buffer,
                    vk::raii::DeviceMemory &bufferMemory);

  void CopyBuffer(Renderer::Device &device, Renderer::CommandPool &commandPool,
                  vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer,
                  vk::DeviceSize size);

  uint32_t FindMemoryType(Renderer::Device &device, uint32_t typeFilter,
                          vk::MemoryPropertyFlags properties);

private:
private:
};

} // namespace Renderer
