#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

const std::vector validationLayers = {"VK_LAYER_KHRONOS_validation"};

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

class HelloTriangleApplication {
public:
  void run() {
    std::cout << "Using debug validation layers: "
              << ((enableValidationLayers) ? "YES" : "NO") << '\n';
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_Window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
  }
  void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
  }
  void createSurface() {
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(*m_Instance, m_Window, nullptr, &_surface) !=
        0) {
      throw std::runtime_error("failed to create window surface!");
    }
    m_Surface = vk::raii::SurfaceKHR(m_Instance, _surface);
  }

  void createLogicalDevice() {
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        m_PhysicalDevice.getQueueFamilyProperties();

    auto graphicsQueueFamilyProperty = queueFamilyProperties.end();
    for (auto it = queueFamilyProperties.begin();
         it != queueFamilyProperties.end(); ++it) {
      if (it->queueFlags & vk::QueueFlagBits::eGraphics) {

        // Check if this graphics queue family also supports presentation
        auto index = static_cast<uint32_t>(
            std::distance(queueFamilyProperties.begin(), it));

        if (m_PhysicalDevice.getSurfaceSupportKHR(index, m_Surface)) {
          graphicsQueueFamilyProperty = it;
          break;
        }
        // Keep first graphics queue as fallback
        if (graphicsQueueFamilyProperty == queueFamilyProperties.end()) {
          graphicsQueueFamilyProperty = it;
        }
      }
    }
    if (graphicsQueueFamilyProperty == queueFamilyProperties.end()) {
      throw std::runtime_error("No graphics queue family found!");
    }
    auto graphicsIndex = static_cast<uint32_t>(std::distance(
        queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

    // Verify the selected queue supports presentation
    if (!m_PhysicalDevice.getSurfaceSupportKHR(graphicsIndex, m_Surface)) {
      throw std::runtime_error(
          "Graphics queue family does not support presentation!");
    }

    // Create a chain of feature structures
    vk::StructureChain<vk::PhysicalDeviceFeatures2,
                       vk::PhysicalDeviceVulkan13Features,
                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
        featureChain = {
            {},
            {.dynamicRendering = true},
            {.extendedDynamicState = true},
        };

    // Ensure swapchain extension is included
    std::vector<const char *> deviceExtensions = requiredDeviceExtension;
    bool hasSwapchainExt = false;
    for (const auto &ext : deviceExtensions) {
      if (strcmp(ext, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
        hasSwapchainExt = true;
        break;
      }
    }
    if (!hasSwapchainExt) {
      deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    float queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = graphicsIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };
    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
    };
    m_device = vk::raii::Device(m_PhysicalDevice, deviceCreateInfo);
    m_GraphicsQueue = vk::raii::Queue(m_device, graphicsIndex, 0);
  }

  void pickPhysicalDevice() {
    auto devices = m_Instance.enumeratePhysicalDevices();

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

      for (const auto &requiredExtension : requiredDeviceExtension) {
        bool extensionFound = false;
        for (const auto &availableExtension : availableDeviceExtensions) {
          if (strcmp(availableExtension.extensionName, requiredExtension) ==
              0) {
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

      bool supportsRequiredFeatures =
          features.template get<vk::PhysicalDeviceVulkan13Features>()
              .dynamicRendering &&
          features
              .template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>()
              .extendedDynamicState;

      if (supportsVulkan1_3 && supportsGraphics &&
          supportsAllRequiredExtensions && supportsRequiredFeatures) {
        m_PhysicalDevice = device;
        return;
      }
    }
    throw std::runtime_error("failed to find a suitable GPU!");
  }

  void setupDebugMessenger() {
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
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
        .messageSeverity = severityFlags,
        .messageType = messageTypeFlags,
        .pfnUserCallback = &debugCallback,
    };
    m_DebugMessenger = m_Instance.createDebugUtilsMessengerEXT(
        debugUtilsMessengerCreateInfoEXT);
  }

  void createInstance() {
    constexpr vk::ApplicationInfo appInfo{
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion14,
    };

    std::vector<const char *> requiredLayers;
    if (enableValidationLayers) {
      requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }

    // Check if the required layers are supported by the Vulkan implementation
    auto layerProperties = m_RaiiContext.enumerateInstanceLayerProperties();
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
    auto extensionProperties =
        m_RaiiContext.enumerateInstanceExtensionProperties();

    for (int i = 0; i < glfwExtensionCount; ++i) {
      bool found = false;
      for (const auto &extensionProperty : extensionProperties) {
        if (strcmp(extensionProperty.extensionName, glfwExtensions[i])) {
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
    vk::InstanceCreateInfo createInfo{
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(glfwExtensions.size()),
        .ppEnabledExtensionNames = glfwExtensions.data(),
    };

    m_Instance = vk::raii::Instance(m_RaiiContext, createInfo);
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(m_Window)) {
      glfwPollEvents();
    }
  }

  void cleanup() {
    glfwDestroyWindow(m_Window);
    glfwTerminate();
  }

  static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
      vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
      vk::DebugUtilsMessageTypeFlagsEXT type,
      const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {
    std::cerr << "validation layer: type " << to_string(type)
              << " msg: " << pCallbackData->pMessage << std::endl;

    return vk::False;
  }

  uint32_t findQueueFamilies(VkPhysicalDevice device) {
    // find the index of the first queue family that supports graphics
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        m_PhysicalDevice.getQueueFamilyProperties();

    // get the first index into queueFamilyProperties which supports graphics
    auto graphicsQueueFamilyProperty =
        std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
                     [](vk::QueueFamilyProperties const &qfp) {
                       return qfp.queueFlags & vk::QueueFlagBits::eGraphics;
                     });

    return static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(),
                                               graphicsQueueFamilyProperty));
  }

private:
  GLFWwindow *m_Window;

  vk::raii::Context m_RaiiContext;
  vk::raii::Instance m_Instance = nullptr;

  vk::raii::DebugUtilsMessengerEXT m_DebugMessenger = nullptr;
  vk::raii::SurfaceKHR m_Surface = nullptr;

  vk::raii::PhysicalDevice m_PhysicalDevice = nullptr;

  std::vector<const char *> requiredDeviceExtension = {
      vk::KHRSwapchainExtensionName,
      vk::KHRSpirv14ExtensionName,
      vk::KHRSynchronization2ExtensionName,
      vk::KHRCreateRenderpass2ExtensionName,
  };

  vk::raii::Device m_device = nullptr;
  vk::raii::Queue m_GraphicsQueue = nullptr;
};

int main() {
  HelloTriangleApplication app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
