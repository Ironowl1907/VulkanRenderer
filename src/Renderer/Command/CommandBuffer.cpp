#include "CommandBuffer.h"

namespace Renderer {

void CommandBuffer::begin(vk::CommandBufferUsageFlags flags) {}
void CommandBuffer::end() {}
void CommandBuffer::reset(vk::CommandBufferResetFlags flags) {
  commandBuffer_.reset(flags);
}

void CommandBuffer::bindVertexBuffer(uint32_t binding, const vk::Buffer &buffer,
                                     vk::DeviceSize offset) {}
void CommandBuffer::bindVertexBuffers(
    uint32_t firstBinding, const std::vector<vk::Buffer> &buffers,
    const std::vector<vk::DeviceSize> &offsets) {}

void CommandBuffer::bindIndexBuffer(const vk::Buffer &buffer,
                                    vk::IndexType indexType,
                                    vk::DeviceSize offset) {}
void CommandBuffer::bindDescriptorSets(
    vk::PipelineBindPoint bindPoint, const vk::PipelineLayout &layout,
    uint32_t firstSet, const std::vector<vk::DescriptorSet> &sets,
    const std::vector<uint32_t> &dynamicOffsets) {}

void CommandBuffer::bindPipeline(vk::PipelineBindPoint bindPoint,
                                 const vk::Pipeline &pipeline) {}

void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount,
                         uint32_t firstVertex, uint32_t firstInstance) {}
void CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount,
                                uint32_t firstIndex, int32_t vertexOffset,
                                uint32_t firstInstance) {}

CommandBuffer::CommandBuffer(vk::raii::CommandBuffer &&commandBuffer)
    : commandBuffer_(std::move(commandBuffer)) {}

} // namespace Renderer
