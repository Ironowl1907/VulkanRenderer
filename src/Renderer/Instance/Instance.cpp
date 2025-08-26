#include "Instance.h"

#include <iostream>
#include <vulkan/vulkan_raii.hpp>

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
  std::cerr << "VALIDATION LAYER [" << to_string(severity) << "] ["
            << to_string(type) << "]: " << pCallbackData->pMessage << std::endl;
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

// void Instance::TestValidationLayers() {
//   if (!enableValidationLayers) {
//     std::cout << "Validation layers are disabled (Release build)" <<
//     std::endl; return;
//   }
//
//   std::cout << "Testing validation layers..." << std::endl;
//   std::cout << "This should produce a validation error message:" <<
//   std::endl;
//
//   try {
//     // Create an invalid buffer to trigger validation error
//     vk::BufferCreateInfo invalidBufferInfo{};
//     invalidBufferInfo.size =
//         0; // Invalid size - should trigger validation error
//     invalidBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
//     invalidBufferInfo.sharingMode = vk::SharingMode::eExclusive;
//
//     // This should produce a validation layer message about invalid size
//     auto buffer = m_Handler.createBuffer(invalidBufferInfo);
//
//   } catch (const std::exception &e) {
//     std::cout << "Exception during validation test: " << e.what() <<
//     std::endl;
//   }
//
//   std::cout << "If you saw a validation message above, layers are working!"
//             << std::endl;
// }

Instance::Instance() {}

void Instance::Create(const char *appName) {
  std::cout << "Creating Vulkan instance with validation layers: "
            << (enableValidationLayers ? "ENABLED" : "DISABLED") << std::endl;

  vk::ApplicationInfo appInfo{};
  appInfo.pApplicationName = appName;
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = vk::ApiVersion14;

  // Setup validation layers
  std::vector<const char *> requiredLayers;
  if (enableValidationLayers) {
    requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    std::cout << "Validation layers to enable:" << std::endl;
    for (const auto &layer : requiredLayers) {
      std::cout << "  " << layer << std::endl;
    }
  }

  // Check if the required layers are supported by the Vulkan implementation
  auto layerProperties = vk::enumerateInstanceLayerProperties();
  std::cout << "Available instance layers:" << std::endl;
  for (const auto &layer : layerProperties) {
    std::cout << "  " << layer.layerName << " (v" << layer.specVersion << ")"
              << std::endl;
  }

  for (const char *requiredLayer : requiredLayers) {
    bool layerFound = false;
    for (const auto &layerProperty : layerProperties) {
      if (strcmp(layerProperty.layerName, requiredLayer) == 0) {
        layerFound = true;
        break;
      }
    }
    if (!layerFound) {
      throw std::runtime_error(std::string("Required layer not supported: ") +
                               requiredLayer);
    }
  }

  // Get required extensions
  auto extensions = getRequiredExtensions();
  uint32_t extensionCount = static_cast<uint32_t>(extensions.size());

  std::cout << "Required extensions:" << std::endl;
  for (const auto &ext : extensions) {
    std::cout << "  " << ext << std::endl;
  }

  // Check if the required extensions are supported by the Vulkan implementation
  auto extensionProperties = vk::enumerateInstanceExtensionProperties();
  std::cout << "Available instance extensions:" << std::endl;
  for (const auto &ext : extensionProperties) {
    std::cout << "  " << ext.extensionName << " (v" << ext.specVersion << ")"
              << std::endl;
  }

  for (uint32_t i = 0; i < extensionCount; ++i) {
    bool found = false;
    for (const auto &extensionProperty : extensionProperties) {
      if (strcmp(extensionProperty.extensionName, extensions[i]) == 0) {
        found = true;
        break;
      }
    }
    if (!found) {
      throw std::runtime_error(
          std::string("Required extension not supported: ") + extensions[i]);
    }
  }

  // Create instance with debug info for instance creation validation
  vk::InstanceCreateInfo createInfo{};
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
  createInfo.ppEnabledLayerNames = requiredLayers.data();
  createInfo.enabledExtensionCount = extensionCount;
  createInfo.ppEnabledExtensionNames = extensions.data();

  // Setup debug messenger info for instance creation validation
  vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (enableValidationLayers) {
    debugCreateInfo.messageSeverity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    debugCreateInfo.messageType =
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    debugCreateInfo.pfnUserCallback = &debugCallback;
    createInfo.pNext = &debugCreateInfo;
  }

  // Create the instance
  m_Handler = vk::raii::Instance(m_RaiiContext, createInfo);
  std::cout << "Vulkan instance created successfully!" << std::endl;

  // Setup persistent debug messenger
  SetupDebugMessenger();

  if (enableValidationLayers) {
    std::cout << "Debug messenger created successfully!" << std::endl;
    std::cout << "Validation layers are now active and monitoring."
              << std::endl;
  }
}

Instance::~Instance() {}

void Instance::Clear() {}

} // namespace Renderer
