#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
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
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
  }

  void createGraphicsPipeline() {
    vk::raii::ShaderModule shaderModule =
        createShaderModule(readFile("shaders/slang.spv"));

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = shaderModule,
        .pName = "vertMain"};
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = shaderModule,
        .pName = "fragMain"};
    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                        fragShaderStageInfo};

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = vk::PrimitiveTopology::eTriangleList};
    vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1,
                                                      .scissorCount = 1};

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = vk::False,
        .depthBiasSlopeFactor = 1.0f,
        .lineWidth = 1.0f};

    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False};

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = vk::False,
        .colorWriteMask =
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment};

    std::vector dynamicStates = {vk::DynamicState::eViewport,
                                 vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()};

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo;

    m_PipelineLayout = vk::raii::PipelineLayout(m_device, pipelineLayoutInfo);
  }

  void createImageViews() {
    m_SwapChainImageViews.clear();

    vk::ImageViewCreateInfo imageViewCreateInfo{
        .viewType = vk::ImageViewType::e2D,
        .format = m_SwapChainImageFormat,
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
    for (auto image : m_SwapChainImages) {
      imageViewCreateInfo.image = image;
      m_SwapChainImageViews.emplace_back(m_device, imageViewCreateInfo);
    }
  }

  void createSwapChain() {
    auto surfaceCapabilities =
        m_PhysicalDevice.getSurfaceCapabilitiesKHR(m_Surface);

    auto swapChainSurfaceFormat = chooseSwapSurfaceFormat(
        m_PhysicalDevice.getSurfaceFormatsKHR(m_Surface));

    auto swapChainExtent = chooseSwapExtent(surfaceCapabilities);

    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);

    minImageCount = (surfaceCapabilities.maxImageCount > 0 &&
                     minImageCount > surfaceCapabilities.maxImageCount)
                        ? surfaceCapabilities.maxImageCount
                        : minImageCount;

    uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0 &&
        imageCount > surfaceCapabilities.maxImageCount) {
      imageCount = surfaceCapabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .flags = vk::SwapchainCreateFlagsKHR(),
        .surface = m_Surface,
        .minImageCount = minImageCount,
        .imageFormat = swapChainSurfaceFormat.format,
        .imageColorSpace = swapChainSurfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = chooseSwapPresentMode(
            m_PhysicalDevice.getSurfacePresentModesKHR(m_Surface)),
        .clipped = true,
        .oldSwapchain = nullptr,
    };
    m_SwapChain = vk::raii::SwapchainKHR(m_device, swapChainCreateInfo);
    m_SwapChainImages = m_SwapChain.getImages();

    m_SwapChainImageFormat = swapChainSurfaceFormat.format;
    m_SwapChainExtent = swapChainExtent;
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

    auto graphicsIndex = static_cast<uint32_t>(std::distance(
        queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

    // determine a queueFamilyIndex that supports present
    // first check if the graphicsIndex is good enough
    auto presentIndex =
        m_PhysicalDevice.getSurfaceSupportKHR(graphicsIndex, *m_Surface)
            ? graphicsIndex
            : static_cast<uint32_t>(queueFamilyProperties.size());
    if (presentIndex == queueFamilyProperties.size()) {
      // the graphicsIndex doesn't support present -> look for another family
      // index that supports both graphics and present
      for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
        if ((queueFamilyProperties[i].queueFlags &
             vk::QueueFlagBits::eGraphics) &&
            m_PhysicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i),
                                                  *m_Surface)) {
          graphicsIndex = static_cast<uint32_t>(i);
          presentIndex = graphicsIndex;
          break;
        }
      }
      if (presentIndex == queueFamilyProperties.size()) {
        // there's nothing like a single family index that supports both
        // graphics and present -> look for another family index that supports
        // present
        for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
          if (m_PhysicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i),
                                                    *m_Surface)) {
            presentIndex = static_cast<uint32_t>(i);
            break;
          }
        }
      }
    }
    if ((graphicsIndex == queueFamilyProperties.size()) ||
        (presentIndex == queueFamilyProperties.size())) {
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
    features.pNext = &vulkan13Features;
    // create a Device
    float queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = graphicsIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority};

    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = &features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount =
            static_cast<uint32_t>(requiredDeviceExtension.size()),
        .ppEnabledExtensionNames = requiredDeviceExtension.data(),
    };

    deviceCreateInfo.enabledExtensionCount = requiredDeviceExtension.size();
    deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtension.data();

    m_device = vk::raii::Device(m_PhysicalDevice, deviceCreateInfo);
    m_GraphicsQueue = vk::raii::Queue(m_device, graphicsIndex, 0);
    m_PresentQueue = vk::raii::Queue(m_device, presentIndex, 0);
  }

  vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
    for (const auto &availableFormat : availableFormats) {
      if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
          availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
        return availableFormat;
      }
    }

    return availableFormats[0];
  }

  vk::PresentModeKHR chooseSwapPresentMode(
      const std::vector<vk::PresentModeKHR> &availablePresentModes) {
    for (const auto &availablePresentMode : availablePresentModes) {
      if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
        return availablePresentMode;
      }
    }
    return vk::PresentModeKHR::eFifo;
  }

  vk::Extent2D
  chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) {
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(m_Window, &width, &height);

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width,
                             capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height,
                             capabilities.maxImageExtent.height),
    };
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
      break;
    }
  }

  void cleanup() {
    if (m_device != nullptr) {
      m_device.waitIdle();
    }
    m_PipelineLayout.clear();
    m_SwapChainImageViews.clear();
    m_SwapChain.clear();
    m_GraphicsQueue.clear();
    m_PresentQueue.clear();
    m_device.clear();
    m_Surface.clear();
    m_DebugMessenger.clear();
    m_Instance.clear();

    if (m_Window) {
      glfwDestroyWindow(m_Window);
      m_Window = nullptr;
    }
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

  static std::vector<char> readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
      std::cout << "Couldn't open file: " << filename << '\n';
      throw std::runtime_error("error opening file!");
    }

    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    file.close();
    return buffer;
  }

  [[nodiscard]] vk::raii::ShaderModule
  createShaderModule(const std::vector<char> &code) const {
    vk::ShaderModuleCreateInfo createInfo{
        .codeSize = code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t *>(code.data())};
    vk::raii::ShaderModule shaderModule{m_device, createInfo};

    return shaderModule;
  }

private:
  GLFWwindow *m_Window;
  vk::raii::Context m_RaiiContext;
  vk::raii::Instance m_Instance = nullptr;
  vk::raii::DebugUtilsMessengerEXT m_DebugMessenger = nullptr;
  vk::raii::SurfaceKHR m_Surface = nullptr;
  vk::raii::PhysicalDevice m_PhysicalDevice = nullptr;
  vk::raii::Device m_device = nullptr;
  vk::raii::Queue m_GraphicsQueue = nullptr;
  vk::raii::Queue m_PresentQueue = nullptr;

  std::vector<const char *> requiredDeviceExtension = {
      vk::KHRSwapchainExtensionName,
      vk::KHRSpirv14ExtensionName,
      vk::KHRSynchronization2ExtensionName,
      vk::KHRCreateRenderpass2ExtensionName,
      vk::KHRShaderDrawParametersExtensionName,
  };
  vk::Format m_SwapChainImageFormat = vk::Format::eUndefined;
  vk::Extent2D m_SwapChainExtent;
  std::vector<vk::Image> m_SwapChainImages;
  vk::raii::SwapchainKHR m_SwapChain = nullptr;
  std::vector<vk::raii::ImageView> m_SwapChainImageViews;

  vk::raii::PipelineLayout m_PipelineLayout = nullptr;
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
