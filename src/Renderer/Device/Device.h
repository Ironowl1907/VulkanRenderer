#pragma once

#include "../Instance/Instance.h"
#include <cstdint>
#include <vulkan/vulkan.hpp>

namespace Renderer {

class Device {
public:
  Device();
  ~Device();

  void Create();

  vk::PhysicalDevice &GetPhysicalDeivice() { return m_PhysicalDevice; }
  vk::Device &GetDevice() { return m_Device; }

  void PickPhysicalDevice(Renderer::Instance &instance);
  void CreateLogicalDevice(vk::SurfaceKHR &surface);

  // TODO: HACK implementation, remove for a queue handler
  vk::Queue GetGraphicsQueue() { return m_GraphicsQueue; }
  vk::Queue GetPresentQueue() { return m_PresentQueue; }

  void clean();

private:
  vk::PhysicalDevice m_PhysicalDevice;
  vk::Device m_Device;

  std::vector<const char *> m_RequiredDeviceExtensions = {
      vk::KHRSwapchainExtensionName,
      vk::KHRSpirv14ExtensionName,
      vk::KHRSynchronization2ExtensionName,
      vk::KHRCreateRenderpass2ExtensionName,
      vk::KHRShaderDrawParametersExtensionName,
  };

  uint32_t m_GraphicsIndex;
  uint32_t m_PresentIndex;

  // TODO : Remove
  vk::Queue m_GraphicsQueue = nullptr;
  vk::Queue m_PresentQueue = nullptr;
};
} // namespace Renderer
