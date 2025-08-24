#include "Instance.h"

#include <iostream>
#include <vulkan/vulkan_raii.hpp>
// #include <vulkan/vulkan_core.h>

namespace Renderer {

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

std::vector<const char *> getRequiredExtensions() {
  uint32_t glfwExtensionCount = 0;
  auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
  if (enableValidationLayers) {
    extensions.push_back(vk::EXTDebugUtilsExtensionName);
  }

  return extensions;
}

static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {
  std::cerr << "validation layer: type " << to_string(type)
            << " msg: " << pCallbackData->pMessage << std::endl;

  return vk::False;
}

void Instance::SetupDebugMessenger() {
  if (!enableValidationLayers)
    return;

  vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
  vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
      vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
      vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
      vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
  vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{};
  debugUtilsMessengerCreateInfoEXT.messageSeverity = severityFlags;
  debugUtilsMessengerCreateInfoEXT.messageType = messageTypeFlags;
  debugUtilsMessengerCreateInfoEXT.pfnUserCallback = &debugCallback;
  m_DebugMessenger =
      m_Handler.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}

Instance::Instance() {}

void Instance::Create(const char *appName) {
  vk::ApplicationInfo appInfo{};
  appInfo.pApplicationName = appName;
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = vk::ApiVersion14;

  std::vector<const char *> requiredLayers;
  if (enableValidationLayers) {
    requiredLayers.assign(validationLayers.begin(), validationLayers.end());
  }

  // Check if the required layers are supported by the Vulkan implementation
  auto layerProperties = vk::enumerateInstanceLayerProperties();

  for (const char *requiredLayer : requiredLayers) {
    bool layerFound = false;
    for (const auto &layerProperty : layerProperties) {
      if (strcmp(layerProperty.layerName, requiredLayer) == 0) {
        layerFound = true;
        break;
      }
    }
    if (!layerFound) {
      throw std::runtime_error(
          "One or more required layers are not supported!");
    }
  }
  uint32_t glfwExtensionCount = 0;
  auto glfwExtensions = getRequiredExtensions();

  // Check if the required GLFW extensions are supported by the Vulkan
  // implementation.
  auto extensionProperties = vk::enumerateInstanceExtensionProperties();

  for (int i = 0; i < glfwExtensionCount; ++i) {
    bool found = false;
    for (const auto &extensionProperty : extensionProperties) {
      if (strcmp(extensionProperty.extensionName, glfwExtensions[i]) == 0) {
        found = true;
        break;
      }
      if (!found)
        std::runtime_error("Required GLFW Extension not supported! " +
                           std::string(glfwExtensions[i]));
    }
  }
  auto extensions = getRequiredExtensions();
  std::cout << "Using validation Layers: " << '\n';
  for (int i = 0; i < static_cast<uint32_t>(glfwExtensions.size()); i++) {
    std::cout << '\t' << glfwExtensions[i] << '\n';
  }

  std::cout << "End validation layers" << '\n';
  vk::InstanceCreateInfo createInfo{};
  createInfo.pApplicationInfo = &appInfo,
  createInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
  createInfo.ppEnabledLayerNames = requiredLayers.data(),
  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(glfwExtensions.size()),
  createInfo.ppEnabledExtensionNames = glfwExtensions.data(),

  m_Handler = vk::raii::Instance(m_RaiiContext, createInfo);
}
Instance::~Instance() {}

void Instance::Clear() {}

} // namespace Renderer
