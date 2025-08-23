#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <vector>

namespace Renderer {

class Instance {
public:
  Instance();
  ~Instance();

  void Create(const char *appName);
  vk::Instance &Get() { return m_Handler; }

  void Clear();

private:
  void SetupDebugMessenger();

private:
  vk::Instance m_Handler = nullptr;
  vk::DebugUtilsMessengerEXT m_DebugMessenger = nullptr;
  const std::vector<const char *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};
};
} // namespace Renderer
