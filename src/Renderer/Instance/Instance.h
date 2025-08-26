#pragma once
#include <GLFW/glfw3.h>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace Renderer {
class Instance {
public:
  Instance();
  ~Instance();

  void Create(const char *appName);

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
