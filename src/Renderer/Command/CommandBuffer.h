#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace Renderer {

class CommandBuffer {
public:
  ~CommandBuffer() = default;

  CommandBuffer(const CommandBuffer &) = delete;
  CommandBuffer &operator=(const CommandBuffer &) = delete;
  CommandBuffer(CommandBuffer &&) = default;
  CommandBuffer &operator=(CommandBuffer &&) = default;

  // Recording
  void begin(vk::CommandBufferUsageFlags flags = {});
  void end();
  void reset(vk::CommandBufferResetFlags flags = {});

  void bindVertexBuffer(uint32_t binding, const vk::Buffer &buffer,
                        vk::DeviceSize offset = 0);
  void bindVertexBuffers(uint32_t firstBinding,
                         const std::vector<vk::Buffer> &buffers,
                         const std::vector<vk::DeviceSize> &offsets = {});
  void bindIndexBuffer(const vk::Buffer &buffer, vk::IndexType indexType,
                       vk::DeviceSize offset = 0);
  void bindDescriptorSets(vk::PipelineBindPoint bindPoint,
                          const vk::PipelineLayout &layout, uint32_t firstSet,
                          const std::vector<vk::DescriptorSet> &sets,
                          const std::vector<uint32_t> &dynamicOffsets = {});
  void bindPipeline(vk::PipelineBindPoint bindPoint,
                    const vk::Pipeline &pipeline);

  void draw(uint32_t vertexCount, uint32_t instanceCount = 1,
            uint32_t firstVertex = 0, uint32_t firstInstance = 0);
  void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1,
                   uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                   uint32_t firstInstance = 0);

  // Access to underlying RAII object
  const vk::raii::CommandBuffer &get() const { return commandBuffer_; }
  operator const vk::raii::CommandBuffer &() const { return commandBuffer_; }

  // For queue submission
  vk::CommandBuffer getHandle() const { return *commandBuffer_; }

private:
  friend class CommandPool; // For constuction
  CommandBuffer(vk::raii::CommandBuffer &&commandBuffer);

  vk::raii::CommandBuffer commandBuffer_;
};
} // namespace Renderer
