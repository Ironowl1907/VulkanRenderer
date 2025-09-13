#include "Texture.h"
#include "../Buffer/Buffer.h"
#include "../Command/CommandPool.h"
#include "../Device/Device.h"
#include <cstring>
#include <stdexcept>
#include <vulkan/vulkan_enums.hpp>

#include "../Helpers/helpers.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Renderer {

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

  createImage(device, m_width, m_height, m_format, vk::ImageTiling::eOptimal,
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

  createImage(device, width, height, format, vk::ImageTiling::eOptimal, usage,
              vk::MemoryPropertyFlagBits::eDeviceLocal, m_image, m_imageMemory);

  createTexImageView(device, format);
  createSampler(device);

  return true;
}

void Texture::createTexImageView(Device &device, vk::Format format) {
  m_imageView =
      createImageView(device, m_image, format, vk::ImageAspectFlagBits::eColor);
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
