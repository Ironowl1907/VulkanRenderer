#pragma once

#include "../Buffer/Buffer.h"
#include <memory>
#include <string>
#include <vulkan/vulkan_raii.hpp>

namespace Renderer {

class Device;
class CommandPool;
class BufferManager;

class Texture {
public:
  Texture() = delete;
  ~Texture() = default;

  Texture(BufferManager &bufferManager);

  bool loadFromFile(Device &device, CommandPool &commandPool,
                    BufferManager &bufferManager, const std::string &filepath);

  bool createFromData(Device &device, CommandPool &commandPool,
                      BufferManager &bufferManager, const unsigned char *data,
                      int width, int height, int channels);

  bool createEmpty(
      Device &device, uint32_t width, uint32_t height,
      vk::Format format = vk::Format::eR8G8B8A8Srgb,
      vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eColorAttachment |
                                  vk::ImageUsageFlagBits::eSampled);

  const vk::raii::Image &getImage() const { return m_image; }
  const vk::raii::ImageView &getImageView() const { return m_imageView; }
  const vk::raii::Sampler &getSampler() const { return m_sampler; }
  uint32_t getWidth() const { return m_width; }
  uint32_t getHeight() const { return m_height; }
  vk::Format getFormat() const { return m_format; }

  // Helpers
  static vk::raii::ImageView createImageView(Device &device, vk::Image &image,
                                             vk::Format format);

  void cleanup();

private:
  void createImage(Device &device, uint32_t width, uint32_t height,
                   vk::Format format, vk::ImageTiling tiling,
                   vk::ImageUsageFlags usage,
                   vk::MemoryPropertyFlags properties);

  void createTexImageView(Device &device, vk::Format format);

  void createSampler(Device &device);

  void transitionImageLayout(Device &device, CommandPool &commandPool,
                             vk::Format format, vk::ImageLayout oldLayout,
                             vk::ImageLayout newLayout);

  void copyBufferToImage(const Device &device, CommandPool &commandPool,
                         const vk::raii::Buffer &buffer, uint32_t width,
                         uint32_t height);

private:
  vk::raii::Image m_image = nullptr;
  vk::raii::DeviceMemory m_imageMemory = nullptr;
  vk::raii::ImageView m_imageView = nullptr;
  vk::raii::Sampler m_sampler = nullptr;

  BufferManager m_bufferManager;

  uint32_t m_width = 0;
  uint32_t m_height = 0;
  vk::Format m_format = vk::Format::eR8G8B8A8Srgb;
};

} // namespace Renderer
