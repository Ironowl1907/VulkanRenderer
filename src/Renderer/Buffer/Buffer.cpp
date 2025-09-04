#include "Buffer.h"

namespace Renderer {

void BufferManager::CreateBuffer(Renderer::Device &device, vk::DeviceSize size,
                                 vk::BufferUsageFlags usage,
                                 vk::MemoryPropertyFlags properties,
                                 vk::raii::Buffer &buffer,
                                 vk::raii::DeviceMemory &bufferMemory) {
  vk::BufferCreateInfo bufferInfo{};
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = vk::SharingMode::eExclusive;

  buffer = vk::raii::Buffer(device.GetDevice(), bufferInfo);

  vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

  vk::MemoryAllocateInfo allocInfo{};
  allocInfo.allocationSize = memRequirements.size,
  allocInfo.memoryTypeIndex =
      device.FindMemoryType(memRequirements.memoryTypeBits, properties),

  bufferMemory = vk::raii::DeviceMemory(device.GetDevice(), allocInfo);

  buffer.bindMemory(*bufferMemory, 0);
}

void BufferManager::CopyBuffer(Renderer::Device &device,
                               Renderer::CommandPool &commandPool,
                               vk::raii::Buffer &srcBuffer,
                               vk::raii::Buffer &dstBuffer,
                               vk::DeviceSize size) {

  vk::CommandBufferAllocateInfo allocInfo{};
  allocInfo.commandPool = *commandPool.Get();
  allocInfo.level = vk::CommandBufferLevel::ePrimary;
  allocInfo.commandBufferCount = 1;

  vk::raii::CommandBuffer commandCopyBuffer =
      std::move(device.GetDevice().allocateCommandBuffers(allocInfo).front());

  // commandCopyBuffer.begin(vk::CommandBufferBeginInfo{
  //     .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  vk::CommandBufferBeginInfo beginInfo{};
  beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

  commandCopyBuffer.begin(beginInfo);

  commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer,
                               vk::BufferCopy{0, 0, size});

  commandCopyBuffer.end();

  vk::SubmitInfo subInfo{};
  subInfo.commandBufferCount = 1;
  subInfo.pCommandBuffers = &*commandCopyBuffer;
  device.GetGraphicsQueue().submit(subInfo, nullptr);

  device.GetGraphicsQueue().waitIdle();
}
} // namespace Renderer
