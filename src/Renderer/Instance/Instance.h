#pragma once
#include <GLFW/glfw3.h>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace Renderer {
class Instance {
public:
  Instance(const char *appName);
  ~Instance();

  vk::Instance Get() { return *m_Handler; }
  vk::raii::Instance &GetRaii() { return m_Handler; }

  void Clear();

  // void TestValidationLayers();

private:
  void SetupDebugMessenger();

private:
  vk::raii::Instance m_Handler = nullptr;
  vk::raii::DebugUtilsMessengerEXT m_DebugMessenger = nullptr; // Fixed type
  const std::vector<const char *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};
  vk::raii::Context m_RaiiContext;
};
} // namespace Renderer
