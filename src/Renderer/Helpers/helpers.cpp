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

vk::raii::ImageView createImageView(Device &device, vk::raii::Image &image,
                                    vk::Format format,
                                    vk::ImageAspectFlagBits aspect) {

  vk::ImageViewCreateInfo viewInfo({}, image, vk::ImageViewType::e2D, format,
                                   {}, {aspect, 0, 1, 0, 1});
  return vk::raii::ImageView(device.GetDevice(), viewInfo);
}

void transitionImageLayout(Device &device, CommandPool &commandPool,
                           vk::Format format, vk::ImageLayout oldLayout,
                           vk::ImageLayout newLayout, vk::raii::Image &image) {
  auto commandBuffer = commandPool.beginSingleTimeCommands(device);

  vk::ImageMemoryBarrier barrier{};
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = *image; // deref raii::Image

  // Pick aspect mask based on format
  if (format == vk::Format::eD16Unorm ||
      format == vk::Format::eX8D24UnormPack32 ||
      format == vk::Format::eD32Sfloat) {
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
  } else if (format == vk::Format::eS8Uint) {
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eStencil;
  } else if (format == vk::Format::eD16UnormS8Uint ||
             format == vk::Format::eD24UnormS8Uint ||
             format == vk::Format::eD32SfloatS8Uint) {
    barrier.subresourceRange.aspectMask =
        vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
  } else {
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  }

  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  vk::PipelineStageFlags sourceStage;
  vk::PipelineStageFlags destinationStage;

  if (oldLayout == vk::ImageLayout::eUndefined &&
      newLayout == vk::ImageLayout::eTransferDstOptimal) {
    // Undefined → TransferDst (for uploading to color images)
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

    sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
    destinationStage = vk::PipelineStageFlagBits::eTransfer;
  } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
             newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
    // TransferDst → ShaderRead (sampling a texture)
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    sourceStage = vk::PipelineStageFlagBits::eTransfer;
    destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
  } else if (oldLayout == vk::ImageLayout::eUndefined &&
             newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
    // Undefined → DepthStencilAttachment (for depth images)
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead |
                            vk::AccessFlagBits::eDepthStencilAttachmentWrite;

    sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
    destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
  } else {
    throw std::invalid_argument("Unsupported layout transition!");
  }

  commandBuffer.pipelineBarrier(sourceStage, destinationStage, {},
                                nullptr, // memory barriers
                                nullptr, // buffer barriers
                                barrier  // single image barrier
  );

  commandPool.endSingleTimeCommands(device, commandBuffer);
}

bool hasStencilComponent(vk::Format format) {
  return format == vk::Format::eD32SfloatS8Uint ||
         format == vk::Format::eD24UnormS8Uint;
}

} // namespace Renderer
