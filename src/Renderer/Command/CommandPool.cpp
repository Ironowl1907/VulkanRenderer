#include "CommandPool.h"
#include <cstdint>

namespace Renderer {

// CommandPool constructor
CommandPool::CommandPool(Renderer::Device &device,
                         vk::CommandPoolCreateFlags flags)
    : m_Device(device.GetDevice()),
      m_CommandPool(
          device.GetDevice(),
          vk::CommandPoolCreateInfo{flags, device.GetGraphicsIndex()}),
      m_QueueFamilyIndex(device.GetGraphicsIndex()) {}

// Command buffer allocation
std::unique_ptr<CommandBuffer>
CommandPool::allocatePrimary(uint32_t maxFramesInFlight) {
  vk::CommandBufferAllocateInfo allocInfo{};
  allocInfo.commandPool = m_CommandPool;
  allocInfo.level = vk::CommandBufferLevel::ePrimary;
  allocInfo.commandBufferCount = maxFramesInFlight;

  auto cmdBuffers = vk::raii::CommandBuffers(m_Device, allocInfo);
  return std::unique_ptr<CommandBuffer>(
      new CommandBuffer(std::move(cmdBuffers[0])));
}

// CommandBuffer constructor
CommandBuffer::CommandBuffer(vk::raii::CommandBuffer &&commandBuffer)
    : commandBuffer_(std::move(commandBuffer)) {}

std::vector<std::unique_ptr<CommandBuffer>>
CommandPool::allocatePrimary(uint32_t count, uint32_t maxFramesInFlight) {}

std::unique_ptr<CommandBuffer> CommandPool::allocateSecondary() {}

// Pool management
void reset(vk::CommandPoolResetFlags flags = {}) {}

} // namespace Renderer
