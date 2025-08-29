#pragma once

#include "../Command/CommandBuffer.h"
#include "../Device/Device.h"
#include <vulkan/vulkan_raii.hpp>

namespace Renderer {

class CommandPool {
public:
  CommandPool(Renderer::Device &device, vk::CommandPoolCreateFlags flags = {});

  CommandPool(const CommandPool &) = delete;
  CommandPool &operator=(const CommandPool &) = delete;
  CommandPool(CommandPool &&) = default;

  std::unique_ptr<CommandBuffer> allocatePrimary();
  std::vector<std::unique_ptr<CommandBuffer>>
  allocatePrimary(uint32_t maxFramesInFlight);
  std::unique_ptr<CommandBuffer> allocateSecondary();

  // Pool management
  void reset(vk::CommandPoolResetFlags flags = {});

  const vk::raii::CommandPool &Get() const { return m_CommandPool; }

private:
  const vk::raii::Device &m_Device;
  vk::raii::CommandPool m_CommandPool;
  uint32_t m_QueueFamilyIndex;
};
} // namespace Renderer
