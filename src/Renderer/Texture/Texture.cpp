#include "Texture.h"
#include "../Buffer/Buffer.h"
#include "../Command/CommandPool.h"
#include "../Device/Device.h"
#include <cstring>
#include <iostream>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Renderer {

vk::raii::ImageView Texture::createImageView(Device &device,
                                             vk::raii::Image &image,
                                             vk::Format format) {
  vk::ImageViewCreateInfo viewInfo(
      {}, image, vk::ImageViewType::e2D, format, {},
      {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
  return vk::raii::ImageView(device.GetDevice(), viewInfo);
}

Texture::Texture(BufferManager &bufferManager)
    : m_bufferManager(bufferManager) {}

bool Texture::loadFromFile(Device &device, CommandPool &commandPool,
                           BufferManager &bufferManager,
                           const std::string &filepath) {
  int texWidth, texHeight, texChannels;
  stbi_uc *pixels = stbi_load(filepath.c_str(), &texWidth, &texHeight,
                              &texChannels, STBI_rgb_alpha);

  if (!pixels) {
    throw std::runtime_error("Failed to load texture image: " + filepath);
  }

  // Force 4 channels (RGBA)
  texChannels = 4;
  bool result = createFromData(device, commandPool, bufferManager, pixels,
                               texWidth, texHeight, texChannels);
  stbi_image_free(pixels);
  return result;
}

bool Texture::createFromData(Device &device, CommandPool &commandPool,
                             BufferManager &bufferManager,
                             const unsigned char *data, int width, int height,
                             int channels) {
  if (!data)
    return false;

  m_width = static_cast<uint32_t>(width);
  m_height = static_cast<uint32_t>(height);
  m_format = vk::Format::eR8G8B8A8Srgb;

  vk::DeviceSize imageSize = width * height * 4; // Always RGBA

  // Create staging buffer
  vk::raii::Buffer stagingBuffer = nullptr;
  vk::raii::DeviceMemory stagingBufferMemory = nullptr;

  bufferManager.CreateBuffer(device, imageSize,
                             vk::BufferUsageFlagBits::eTransferSrc,
                             vk::MemoryPropertyFlagBits::eHostVisible |
                                 vk::MemoryPropertyFlagBits::eHostCoherent,
                             stagingBuffer, stagingBufferMemory);

  void *mappedData = stagingBufferMemory.mapMemory(0, imageSize);
  memcpy(mappedData, data, static_cast<size_t>(imageSize));
  stagingBufferMemory.unmapMemory();

  CreateImage(device, m_width, m_height, m_format, vk::ImageTiling::eOptimal,
              vk::ImageUsageFlagBits::eTransferDst |
                  vk::ImageUsageFlagBits::eSampled,
              vk::MemoryPropertyFlagBits::eDeviceLocal, m_image, m_imageMemory);

  transitionImageLayout(device, commandPool, m_format,
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eTransferDstOptimal, m_image);

  copyBufferToImage(device, commandPool, stagingBuffer, m_width, m_height);

  transitionImageLayout(device, commandPool, m_format,
                        vk::ImageLayout::eTransferDstOptimal,
                        vk::ImageLayout::eShaderReadOnlyOptimal, m_image);

  createTexImageView(device, m_format);
  createSampler(device);

  return true;
}

bool Texture::createEmpty(Device &device, uint32_t width, uint32_t height,
                          vk::Format format, vk::ImageUsageFlags usage) {
  m_width = width;
  m_height = height;
  m_format = format;

  CreateImage(device, width, height, format, vk::ImageTiling::eOptimal, usage,
              vk::MemoryPropertyFlagBits::eDeviceLocal, m_image, m_imageMemory);

  createTexImageView(device, format);
  createSampler(device);

  return true;
}

void Texture::CreateImage(Renderer::Device &device, uint32_t width,
                          uint32_t height, vk::Format format,
                          vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                          vk::MemoryPropertyFlags properties,
                          vk::raii::Image &image,
                          vk::raii::DeviceMemory &imageMemory) {
  vk::ImageCreateInfo imageInfo{};
  imageInfo.imageType = vk::ImageType::e2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = vk::ImageLayout::eUndefined;
  imageInfo.usage = usage;
  imageInfo.samples = vk::SampleCountFlagBits::e1;
  imageInfo.sharingMode = vk::SharingMode::eExclusive;

  image = vk::raii::Image(device.GetDevice(), imageInfo);

  vk::MemoryRequirements memRequirements = image.getMemoryRequirements();

  vk::MemoryAllocateInfo allocInfo{};
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      device.FindMemoryType(memRequirements.memoryTypeBits, properties);

  imageMemory = vk::raii::DeviceMemory(device.GetDevice(), allocInfo);
  image.bindMemory(*imageMemory, 0);
}

vk::raii::ImageView Texture::CreateImageView(Renderer::Device &device,
                                             vk::Image image, vk::Format format,
                                             vk::ImageAspectFlags aspectFlags) {
  vk::ImageViewCreateInfo viewInfo{};
  viewInfo.image = image;
  viewInfo.viewType = vk::ImageViewType::e2D;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  return vk::raii::ImageView(device.GetDevice(), viewInfo);
}

void Texture::createTexImageView(Device &device, vk::Format format) {
  m_imageView = createImageView(device, m_image, format);
}

void Texture::createSampler(Device &device) {

  vk::PhysicalDeviceProperties properties =
      device.GetPhysicalDevice().getProperties();
  vk::SamplerCreateInfo samplerInfo(
      {}, vk::Filter::eLinear, vk::Filter::eLinear,
      vk::SamplerMipmapMode::eLinear, vk::SamplerAddressMode::eRepeat,
      vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, 0, 1,
      properties.limits.maxSamplerAnisotropy, vk::False,
      vk::CompareOp::eAlways);
  samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
  samplerInfo.unnormalizedCoordinates = vk::False;
  samplerInfo.compareEnable = vk::False;
  samplerInfo.compareOp = vk::CompareOp::eAlways;

  samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 1.0f;

  m_sampler = vk::raii::Sampler(device.GetDevice(), samplerInfo);
}

void Texture::transitionImageLayout(Device &device, CommandPool &commandPool,
                                    vk::Format format,
                                    vk::ImageLayout oldLayout,
                                    vk::ImageLayout newLayout,
                                    vk::raii::Image &image) {
  auto commandBuffer = commandPool.beginSingleTimeCommands(device);

  // Pick correct aspect mask
  vk::ImageAspectFlags aspectMask;
  if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
    if (format == vk::Format::eD32SfloatS8Uint ||
        format == vk::Format::eD24UnormS8Uint) {
      aspectMask =
          vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    } else {
      aspectMask = vk::ImageAspectFlagBits::eDepth;
    }
  } else {
    aspectMask = vk::ImageAspectFlagBits::eColor;
  }

  vk::ImageMemoryBarrier barrier(
      {}, {}, oldLayout, newLayout, VK_QUEUE_FAMILY_IGNORED,
      VK_QUEUE_FAMILY_IGNORED, image,
      vk::ImageSubresourceRange(aspectMask, 0, 1, 0, 1));

  vk::PipelineStageFlags sourceStage;
  vk::PipelineStageFlags destinationStage;

  if (oldLayout == vk::ImageLayout::eUndefined &&
      newLayout == vk::ImageLayout::eTransferDstOptimal) {
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

    sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
    destinationStage = vk::PipelineStageFlagBits::eTransfer;

  } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
             newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    sourceStage = vk::PipelineStageFlagBits::eTransfer;
    destinationStage = vk::PipelineStageFlagBits::eFragmentShader;

  } else if (oldLayout == vk::ImageLayout::eUndefined &&
             newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead |
                            vk::AccessFlagBits::eDepthStencilAttachmentWrite;

    sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
    destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;

  } else {
    throw std::invalid_argument("Unsupported layout transition!");
  }

  commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr,
                                barrier);

  commandPool.endSingleTimeCommands(device, commandBuffer);
}

void Texture::copyBufferToImage(Device &device, CommandPool &commandPool,
                                const vk::raii::Buffer &buffer, uint32_t width,
                                uint32_t height) {
  auto commandBuffer = commandPool.beginSingleTimeCommands(device);

  vk::BufferImageCopy region;
  region.bufferOffset = 0;
  region.bufferRowLength = 0;   // Tightly packed
  region.bufferImageHeight = 0; // Tightly packed

  region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = vk::Offset3D{0, 0, 0};
  region.imageExtent = vk::Extent3D{width, height, 1};

  commandBuffer.copyBufferToImage(*buffer, *m_image,
                                  vk::ImageLayout::eTransferDstOptimal, region);

  commandPool.endSingleTimeCommands(device, commandBuffer);
}

void Texture::cleanup() {
  m_sampler.clear();
  m_imageView.clear();
  m_imageMemory.clear();
  m_image.clear();
}

} // namespace Renderer
