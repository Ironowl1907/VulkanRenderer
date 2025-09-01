#include "Texture.h"
#include "../Buffer/Buffer.h"
#include "../Command/CommandPool.h"
#include "../Device/Device.h"
#include <cstring>
#include <stdexcept>

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
    return false;
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
              vk::MemoryPropertyFlagBits::eDeviceLocal);

  transitionImageLayout(device, commandPool, m_format,
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eTransferDstOptimal);

  copyBufferToImage(device, commandPool, stagingBuffer, m_width, m_height);

  transitionImageLayout(device, commandPool, m_format,
                        vk::ImageLayout::eTransferDstOptimal,
                        vk::ImageLayout::eShaderReadOnlyOptimal);

  createImageView(device, m_format);
  createSampler(device);

  return true;
}

bool Texture::createEmpty(Device &device, uint32_t width, uint32_t height,
                          vk::Format format, vk::ImageUsageFlags usage) {
  m_width = width;
  m_height = height;
  m_format = format;

  createImage(device, width, height, format, vk::ImageTiling::eOptimal, usage,
              vk::MemoryPropertyFlagBits::eDeviceLocal);

  createImageView(device, format);
  createSampler(device);

  return true;
}

void Texture::createImage(Device &device, uint32_t width, uint32_t height,
                          vk::Format format, vk::ImageTiling tiling,
                          vk::ImageUsageFlags usage,
                          vk::MemoryPropertyFlags properties) {

  vk::ImageCreateInfo imageInfo({}, vk::ImageType::e2D, format,
                                {width, height, 1}, 1, 1,
                                vk::SampleCountFlagBits::e1, tiling, usage,
                                vk::SharingMode::eExclusive, 0);

  m_image = vk::raii::Image(device.GetDevice(), imageInfo);

  vk::MemoryRequirements memRequirements = m_image.getMemoryRequirements();
  vk::MemoryAllocateInfo allocInfo(
      memRequirements.size,
      m_bufferManager.FindMemoryType(device, memRequirements.memoryTypeBits,
                                     properties));
  m_imageMemory = vk::raii::DeviceMemory(device.GetDevice(), allocInfo);
  m_image.bindMemory(*m_imageMemory, 0);
}

void Texture::createImageView(const Device &device, vk::Format format) {
} // namespace Renderer

void Texture::createSampler(Device &device) {}

void Texture::transitionImageLayout(const Device &device,
                                    CommandPool &commandPool, vk::Format format,
                                    vk::ImageLayout oldLayout,
                                    vk::ImageLayout newLayout) {}

void Texture::copyBufferToImage(const Device &device, CommandPool &commandPool,
                                const vk::raii::Buffer &buffer, uint32_t width,
                                uint32_t height) {}

void Texture::cleanup() {
  m_sampler.clear();
  m_imageView.clear();
  m_imageMemory.clear();
  m_image.clear();
}

} // namespace Renderer
