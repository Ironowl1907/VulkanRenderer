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

// Single command buffer allocation
std::unique_ptr<CommandBuffer> CommandPool::allocatePrimary() {
  vk::CommandBufferAllocateInfo allocInfo{};
  allocInfo.commandPool = m_CommandPool;
  allocInfo.level = vk::CommandBufferLevel::ePrimary;
  allocInfo.commandBufferCount = 1;

  auto cmdBuffers = vk::raii::CommandBuffers(m_Device, allocInfo);
  return std::unique_ptr<CommandBuffer>(
      new CommandBuffer(std::move(cmdBuffers[0])));
}

std::vector<std::unique_ptr<CommandBuffer>>
CommandPool::allocatePrimary(uint32_t maxFramesInFlight) {
  vk::CommandBufferAllocateInfo allocInfo{};
  allocInfo.commandPool = m_CommandPool;
  allocInfo.level = vk::CommandBufferLevel::ePrimary;
  allocInfo.commandBufferCount = maxFramesInFlight;

  auto cmdBuffers = vk::raii::CommandBuffers(m_Device, allocInfo);

  std::vector<std::unique_ptr<CommandBuffer>> result;
  result.reserve(maxFramesInFlight);

  for (uint32_t i = 0; i < maxFramesInFlight; ++i) {
    result.push_back(std::unique_ptr<CommandBuffer>(
        new CommandBuffer(std::move(cmdBuffers[i]))));
  }

  return result;
}

std::unique_ptr<CommandBuffer> CommandPool::allocateSecondary() {
  vk::CommandBufferAllocateInfo allocInfo{};
  allocInfo.commandPool = m_CommandPool;
  allocInfo.level = vk::CommandBufferLevel::eSecondary;
  allocInfo.commandBufferCount = 1;

  auto cmdBuffers = vk::raii::CommandBuffers(m_Device, allocInfo);
  return std::unique_ptr<CommandBuffer>(
      new CommandBuffer(std::move(cmdBuffers[0])));
}

void CommandPool::reset(vk::CommandPoolResetFlags flags) {
  m_CommandPool.reset(flags);
}

CommandBuffer::CommandBuffer(vk::raii::CommandBuffer &&commandBuffer)
    : commandBuffer_(std::move(commandBuffer)) {}

} // namespace Renderer
