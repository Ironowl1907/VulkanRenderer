#include "Device.h"
#include <vulkan/vulkan_structs.hpp>

namespace Renderer {

Device::Device(Renderer::Instance &instance, const vk::SurfaceKHR &surface) {
  PickPhysicalDevice(instance);
  CreateLogicalDevice(surface);
}

Device::~Device() {}

void Device::PickPhysicalDevice(Renderer::Instance &instance) {
  auto devices = instance.GetRaii().enumeratePhysicalDevices();
  for (const auto &device : devices) {
    // Check if the device supports the Vulkan 1.3 API version
    bool supportsVulkan1_3 =
        device.getProperties().apiVersion >= VK_API_VERSION_1_3;
    // Check if any of the queue families support graphics operations
    auto queueFamilies = device.getQueueFamilyProperties();
    bool supportsGraphics = false;
    for (const auto &qfp : queueFamilies) {
      if (qfp.queueFlags & vk::QueueFlagBits::eGraphics) {
        supportsGraphics = true;
        break;
      }
    }
    // Check if all required device extensions are available
    auto availableDeviceExtensions =
        device.enumerateDeviceExtensionProperties();
    bool supportsAllRequiredExtensions = true;
    for (const auto &requiredExtension : m_RequiredDeviceExtensions) {
      bool extensionFound = false;
      for (const auto &availableExtension : availableDeviceExtensions) {
        if (strcmp(availableExtension.extensionName, requiredExtension) == 0) {
          extensionFound = true;
          break;
        }
      }
      if (!extensionFound) {
        supportsAllRequiredExtensions = false;
        break;
      }
    }
    auto features = device.template getFeatures2<
        vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

    auto vulkan13Features =
        features.template get<vk::PhysicalDeviceVulkan13Features>();
    auto basicFeatures = features.template get<vk::PhysicalDeviceFeatures2>();

    bool supportsRequiredFeatures =
        vulkan13Features.dynamicRendering &&
        vulkan13Features.synchronization2 &&
        features
            .template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>()
            .extendedDynamicState &&
        basicFeatures.features
            .samplerAnisotropy; // Check for anisotropy support

    if (supportsVulkan1_3 && supportsGraphics &&
        supportsAllRequiredExtensions && supportsRequiredFeatures) {
      m_PhysicalDevice = vk::raii::PhysicalDevice(instance.GetRaii(), *device);
      return;
    }
  }
  throw std::runtime_error("failed to find a suitable GPU!");
}

void Device::CreateLogicalDevice(const vk::SurfaceKHR &surface) {
  if (m_PhysicalDevice == nullptr) {
    throw std::runtime_error("Physical device not initialized!");
  }

  // find the index of the first queue family that supports graphics
  std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
      m_PhysicalDevice.getQueueFamilyProperties();

  auto graphicsQueueFamilyProperty = queueFamilyProperties.end();
  for (auto qfp = queueFamilyProperties.begin();
       qfp < queueFamilyProperties.end(); qfp++) {
    if (qfp->queueFlags & vk::QueueFlagBits::eGraphics) {
      {
        graphicsQueueFamilyProperty = qfp;
      }
    }
  }
  if (graphicsQueueFamilyProperty == queueFamilyProperties.end()) {
    std::runtime_error("no available queue with graphics support");
  }

  m_GraphicsIndex = static_cast<uint32_t>(std::distance(
      queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

  // determine a queueFamilyIndex that supports present
  // first check if the m_GraphicIndex is good enough
  m_PresentIndex =
      m_PhysicalDevice.getSurfaceSupportKHR(m_GraphicsIndex, surface)
          ? m_GraphicsIndex
          : static_cast<uint32_t>(queueFamilyProperties.size());
  if (m_PresentIndex == queueFamilyProperties.size()) {
    // the m_GraphicIndex doesn't support present -> look for another family
    // index that supports both graphics and present
    for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
      if ((queueFamilyProperties[i].queueFlags &
           vk::QueueFlagBits::eGraphics) &&
          m_PhysicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i),
                                                surface)) {
        m_GraphicsIndex = static_cast<uint32_t>(i);
        m_PresentIndex = m_GraphicsIndex;
        break;
      }
    }
    if (m_PresentIndex == queueFamilyProperties.size()) {
      // there's nothing like a single family index that supports both
      // graphics and present -> look for another family index that supports
      // present
      for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
        if (m_PhysicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i),
                                                  surface)) {
          m_PresentIndex = static_cast<uint32_t>(i);
          break;
        }
      }
    }
  }
  if ((m_GraphicsIndex == queueFamilyProperties.size()) ||
      (m_PresentIndex == queueFamilyProperties.size())) {
    throw std::runtime_error(
        "Could not find a queue for graphics or present -> terminating");
  }

  // query for Vulkan 1.3 features
  auto features = m_PhysicalDevice.getFeatures2();
  vk::PhysicalDeviceVulkan13Features vulkan13Features;
  vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
      extendedDynamicStateFeatures;
  vulkan13Features.dynamicRendering = vk::True;
  extendedDynamicStateFeatures.extendedDynamicState = vk::True;
  vulkan13Features.pNext = &extendedDynamicStateFeatures;
  vulkan13Features.synchronization2 = vk::True;
  features.features.samplerAnisotropy = vk::True;
  features.pNext = &vulkan13Features;
  // create a Device
  float queuePriority = 0.0f;
  vk::DeviceQueueCreateInfo deviceQueueCreateInfo{};
  deviceQueueCreateInfo.queueFamilyIndex = m_GraphicsIndex;
  deviceQueueCreateInfo.queueCount = 1;
  deviceQueueCreateInfo.pQueuePriorities = &queuePriority;

  vk::DeviceCreateInfo deviceCreateInfo{};
  deviceCreateInfo.pNext = &features;
  deviceCreateInfo.queueCreateInfoCount = 1;
  deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
  deviceCreateInfo.enabledExtensionCount =
      static_cast<uint32_t>(m_RequiredDeviceExtensions.size());
  deviceCreateInfo.ppEnabledExtensionNames = m_RequiredDeviceExtensions.data();

  deviceCreateInfo.ppEnabledExtensionNames = m_RequiredDeviceExtensions.data();

  // m_Device = vk::Device(m_PhysicalDevice, deviceCreateInfo);
  m_Device = m_PhysicalDevice.createDevice(deviceCreateInfo);
  m_GraphicsQueue = m_Device.getQueue(m_GraphicsIndex, 0);
  m_PresentQueue = m_Device.getQueue(m_PresentIndex, 0);
}

void Device::clean() {}
} // namespace Renderer
