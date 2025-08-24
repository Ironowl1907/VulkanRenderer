#pragma once

#include "../Instance/Instance.h"
#include <cstdint>
#include <vulkan/vulkan_raii.hpp>

namespace Renderer {

class Device {
public:
  Device();
  ~Device();

  void Create(Renderer::Instance &instance, const vk::SurfaceKHR &surface);

  vk::raii::PhysicalDevice &GetPhysicalDevice() { return m_PhysicalDevice; }
  vk::raii::Device &GetDevice() { return m_Device; }

  void PickPhysicalDevice(Renderer::Instance &instance);
  void CreateLogicalDevice(const vk::SurfaceKHR &surface);

  // TODO: HACK implementation, remove for a queue handler
  vk::raii::Queue GetGraphicsQueue() { return m_GraphicsQueue; }
  vk::raii::Queue GetPresentQueue() { return m_PresentQueue; }

  uint32_t GetGraphicsIndex() { return m_GraphicsIndex; }
  uint32_t GetPresentIndex() { return m_PresentIndex; }

  void clean();

private:
  vk::raii::PhysicalDevice m_PhysicalDevice = nullptr;
  vk::raii::Device m_Device = nullptr;

  // TODO : Remove
  vk::raii::Queue m_GraphicsQueue = nullptr;
  vk::raii::Queue m_PresentQueue = nullptr;

  std::vector<const char *> m_RequiredDeviceExtensions = {
      vk::KHRSwapchainExtensionName,
      vk::KHRSpirv14ExtensionName,
      vk::KHRSynchronization2ExtensionName,
      vk::KHRCreateRenderpass2ExtensionName,
      vk::KHRShaderDrawParametersExtensionName,
  };

  uint32_t m_GraphicsIndex;
  uint32_t m_PresentIndex;
};
} // namespace Renderer
