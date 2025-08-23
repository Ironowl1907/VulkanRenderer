#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

#include <glm/glm.hpp>

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "src/Renderer/Instance/Instance.h"

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector validationLayers = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;

  static vk::VertexInputBindingDescription getBindingDescription() {
    return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
  }

  static std::array<vk::VertexInputAttributeDescription, 2>
  getAttributeDescriptions() {
    return {
        vk::VertexInputAttributeDescription{
            .location = 0,
            .binding = 0,
            .format = vk::Format::eR32G32Sfloat,
            .offset = offsetof(Vertex, pos),
        },
        vk::VertexInputAttributeDescription{
            .location = 1,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(Vertex, color),
        },
    };
  }
};

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
};
const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
};

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
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_Window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, framebufferResizeCallback);
  }
  void initVulkan() {
    m_Instance.Create("Vulkan App");
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
    createCommandPool();
    createVertexBuffer();
    createIndexBuffer();
    createCommandBuffers();
    createSyncObjects();
  }

  void createIndexBuffer() {
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    vk::raii::Buffer stagingBuffer({});
    vk::raii::DeviceMemory stagingBufferMemory({});
    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible |
                     vk::MemoryPropertyFlagBits::eHostCoherent,
                 stagingBuffer, stagingBufferMemory);

    void *data = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(data, indices.data(), (size_t)bufferSize);
    stagingBufferMemory.unmapMemory();

    createBuffer(bufferSize,
                 vk::BufferUsageFlagBits::eTransferDst |
                     vk::BufferUsageFlagBits::eIndexBuffer,
                 vk::MemoryPropertyFlagBits::eDeviceLocal, m_IndexBuffer,
                 m_IndexBufferMemory);

    copyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);
  }

  void createVertexBuffer() {
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    vk::BufferCreateInfo stagingInfo{
        .size = bufferSize,
        .usage = vk::BufferUsageFlagBits::eTransferSrc,
        .sharingMode = vk::SharingMode::eExclusive,
    };
    vk::raii::Buffer stagingBuffer(m_Device, stagingInfo);
    vk::MemoryRequirements memRequirementsStaging =
        stagingBuffer.getMemoryRequirements();
    vk::MemoryAllocateInfo memoryAllocateInfoStaging{
        .allocationSize = memRequirementsStaging.size,
        .memoryTypeIndex =
            findMemoryType(memRequirementsStaging.memoryTypeBits,
                           vk::MemoryPropertyFlagBits::eHostVisible |
                               vk::MemoryPropertyFlagBits::eHostCoherent),
    };
    vk::raii::DeviceMemory stagingBufferMemory(m_Device,
                                               memoryAllocateInfoStaging);

    stagingBuffer.bindMemory(stagingBufferMemory, 0);
    void *dataStaging = stagingBufferMemory.mapMemory(0, stagingInfo.size);
    memcpy(dataStaging, vertices.data(), stagingInfo.size);
    stagingBufferMemory.unmapMemory();

    vk::BufferCreateInfo bufferInfo{
        .size = bufferSize,
        .usage = vk::BufferUsageFlagBits::eVertexBuffer |
                 vk::BufferUsageFlagBits::eTransferDst,
        .sharingMode = vk::SharingMode::eExclusive,
    };

    m_VertexBuffer = vk::raii::Buffer(m_Device, bufferInfo);

    vk::MemoryRequirements memRequirements =
        m_VertexBuffer.getMemoryRequirements();
    vk::MemoryAllocateInfo memoryAllocateInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex =
            findMemoryType(memRequirements.memoryTypeBits,
                           vk::MemoryPropertyFlagBits::eDeviceLocal)};
    m_VertexBufferMemory = vk::raii::DeviceMemory(m_Device, memoryAllocateInfo);

    m_VertexBuffer.bindMemory(*m_VertexBufferMemory, 0);

    copyBuffer(stagingBuffer, m_VertexBuffer, stagingInfo.size);
  }

  void copyBuffer(vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer,
                  vk::DeviceSize size) {

    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = m_CommandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    };

    vk::raii::CommandBuffer commandCopyBuffer =
        std::move(m_Device.allocateCommandBuffers(allocInfo).front());

    commandCopyBuffer.begin(vk::CommandBufferBeginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer,
                                 vk::BufferCopy{0, 0, size});

    commandCopyBuffer.end();

    m_GraphicsQueue.submit(
        vk::SubmitInfo{.commandBufferCount = 1,
                       .pCommandBuffers = &*commandCopyBuffer},
        nullptr);
    m_GraphicsQueue.waitIdle();
  }

  void createSyncObjects() {
    m_ImageAvailableSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
    m_RenderFinishedSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
    m_InFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < m_SwapChainImages.size(); i++) {
      m_ImageAvailableSemaphores.emplace_back(m_Device.createSemaphore({}));
      m_RenderFinishedSemaphores.emplace_back(m_Device.createSemaphore({}));

      vk::FenceCreateInfo fenceInfo{};
      fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
      m_InFlightFences.emplace_back(m_Device.createFence(fenceInfo));
    }
  }

  void createCommandBuffers() {
    m_CommandBuffers.clear();
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = m_CommandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
    };
    m_CommandBuffers = vk::raii::CommandBuffers(m_Device, allocInfo);
  }

  void createCommandPool() {
    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = m_GraphicsIndex,
    };

    m_CommandPool = vk::raii::CommandPool(m_Device, poolInfo);
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
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;

    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

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

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = 0, .pushConstantRangeCount = 0};

    m_PipelineLayout = vk::raii::PipelineLayout(m_Device, pipelineLayoutInfo);

    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &m_SwapChainImageFormat};
    vk::GraphicsPipelineCreateInfo pipelineInfo{
        .pNext = &pipelineRenderingCreateInfo,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = m_PipelineLayout,
        .renderPass = nullptr};

    m_GraphicsPipeline = vk::raii::Pipeline(m_Device, nullptr, pipelineInfo);
  }

  void createImageViews() {
    m_SwapChainImageViews.clear();

    vk::ImageViewCreateInfo imageViewCreateInfo{
        .viewType = vk::ImageViewType::e2D,
        .format = m_SwapChainImageFormat,
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
    for (auto image : m_SwapChainImages) {
      imageViewCreateInfo.image = image;
      m_SwapChainImageViews.emplace_back(m_Device, imageViewCreateInfo);
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
    m_SwapChain = vk::raii::SwapchainKHR(m_Device, swapChainCreateInfo);
    m_SwapChainImages = m_SwapChain.getImages();

    m_SwapChainImageFormat = swapChainSurfaceFormat.format;
    m_SwapChainExtent = swapChainExtent;
  }

  void createSurface() {
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(m_Instance.Get(), m_Window, nullptr,
                                &_surface) != 0) {
      throw std::runtime_error("failed to create window surface!");
    }
    m_Surface = vk::raii::SurfaceKHR(m_Instance.GetRaii(), _surface);
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

    m_GraphicsIndex = static_cast<uint32_t>(std::distance(
        queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

    // determine a queueFamilyIndex that supports present
    // first check if the m_GraphicIndex is good enough
    m_PresentIndex =
        m_PhysicalDevice.getSurfaceSupportKHR(m_GraphicsIndex, *m_Surface)
            ? m_GraphicsIndex
            : static_cast<uint32_t>(queueFamilyProperties.size());
    if (m_PresentIndex == queueFamilyProperties.size()) {
      // the m_GraphicIndex doesn't support present -> look for another family
      // index that supports both graphics and present
      for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
        if ((queueFamilyProperties[i].queueFlags &
             vk::QueueFlagBits::eGraphics) &&
            m_PhysicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i),
                                                  *m_Surface)) {
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
                                                    *m_Surface)) {
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
    features.pNext = &vulkan13Features;
    // create a Device
    float queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = m_GraphicsIndex,
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

    m_Device = vk::raii::Device(m_PhysicalDevice, deviceCreateInfo);
    m_GraphicsQueue = vk::raii::Queue(m_Device, m_GraphicsIndex, 0);
    m_PresentQueue = vk::raii::Queue(m_Device, m_PresentIndex, 0);
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
    auto devices = m_Instance.GetRaii().enumeratePhysicalDevices();
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

      auto vulkan13Features =
          features.template get<vk::PhysicalDeviceVulkan13Features>();
      bool supportsRequiredFeatures =
          vulkan13Features.dynamicRendering &&
          vulkan13Features.synchronization2 &&
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

  void mainLoop() {
    while (!glfwWindowShouldClose(m_Window)) {
      glfwPollEvents();
      drawFrame();
    }
    m_Device.waitIdle();
  }

  void drawFrame() {
    while (vk::Result::eTimeout ==
           m_Device.waitForFences(*m_InFlightFences[m_CurrentFrame], vk::True,
                                  UINT64_MAX))
      ;

    auto [result, imageIndex] = m_SwapChain.acquireNextImage(
        UINT64_MAX,
        m_ImageAvailableSemaphores[m_CurrentFrame], // Use frame-based
                                                    // semaphore for acquire
        nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR ||
        result == vk::Result::eSuboptimalKHR) {
      m_FramebufferResized = false;
      recreateSwapChain();
      return;
    }

    if (result != vk::Result::eSuccess &&
        result != vk::Result::eSuboptimalKHR) {
      throw std::runtime_error("failed to acquire swap chain image!");
    }

    m_Device.resetFences(*m_InFlightFences[m_CurrentFrame]);

    m_CommandBuffers[m_CurrentFrame].reset();
    recordCommandBuffer(imageIndex);

    vk::PipelineStageFlags waitDestinationStageMask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput);

    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores =
            &*m_ImageAvailableSemaphores[m_CurrentFrame], // Wait for acquire
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*m_CommandBuffers[m_CurrentFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores =
            &*m_RenderFinishedSemaphores[imageIndex], // Signal based on IMAGE
                                                      // INDEX
    };

    m_GraphicsQueue.submit(submitInfo, *m_InFlightFences[m_CurrentFrame]);

    // Present the rendered image
    const vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores =
            &*m_RenderFinishedSemaphores[imageIndex], // Wait for render of
                                                      // THIS image
        .swapchainCount = 1,
        .pSwapchains = &*m_SwapChain,
        .pImageIndices = &imageIndex,
    };

    result = m_PresentQueue.presentKHR(presentInfoKHR);

    switch (result) {
    case vk::Result::eSuccess:
      break;

    case vk::Result::eErrorOutOfDateKHR:
    case vk::Result::eSuboptimalKHR:
      recreateSwapChain();
      break;
    default:
      break;
    }

    if (m_FramebufferResized) {
      m_FramebufferResized = false;
      recreateSwapChain();
    }

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  void cleanup() {
    if (m_Device != nullptr) {
      m_Device.waitIdle();
    }

    m_IndexBufferMemory.clear();
    m_IndexBuffer.clear();

    m_VertexBufferMemory.clear();
    m_VertexBuffer.clear();
    for (auto &a : m_ImageAvailableSemaphores)
      a.clear();
    for (auto &a : m_RenderFinishedSemaphores)
      a.clear();
    for (auto &a : m_InFlightFences)
      a.clear();
    for (auto &a : m_CommandBuffers)
      a.clear();

    m_CommandPool.clear();
    m_GraphicsPipeline.clear();
    m_PipelineLayout.clear();
    cleanupSwapChain();
    m_GraphicsQueue.clear();
    m_PresentQueue.clear();
    m_Device.clear();
    m_Surface.clear();

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

  void recordCommandBuffer(uint32_t imageIndex) {
    m_CommandBuffers[m_CurrentFrame].begin({});
    transition_image_layout(
        imageIndex, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {}, // srcAccessMask (no need to wait for previous operations)
        vk::AccessFlagBits2::eColorAttachmentWrite,        // dstAccessMask
        vk::PipelineStageFlagBits2::eTopOfPipe,            // srcStage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput // dstStage
    );

    vk::ClearValue clearColor =
        vk::ClearValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};

    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView = m_SwapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor};

    vk::RenderingInfo renderingInfo = {
        .renderArea = {.offset = {0, 0}, .extent = m_SwapChainExtent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentInfo};

    // Start Rendering
    m_CommandBuffers[m_CurrentFrame].beginRendering(renderingInfo);

    vk::Viewport viewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(m_SwapChainExtent.width),
        .height = static_cast<float>(m_SwapChainExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vk::Rect2D scissor{
        .offset = {0, 0},
        .extent = m_SwapChainExtent,
    };

    m_CommandBuffers[m_CurrentFrame].setViewport(0, viewport);
    m_CommandBuffers[m_CurrentFrame].setScissor(0, scissor);

    m_CommandBuffers[m_CurrentFrame].bindPipeline(
        vk::PipelineBindPoint::eGraphics, *m_GraphicsPipeline);

    m_CommandBuffers[m_CurrentFrame].bindVertexBuffers(0, *m_VertexBuffer, {0});
    m_CommandBuffers[m_CurrentFrame].bindIndexBuffer(m_IndexBuffer, 0,
                                                     vk::IndexType::eUint16);

    m_CommandBuffers[m_CurrentFrame].drawIndexed(indices.size(), 1, 0, 0, 0);

    m_CommandBuffers[m_CurrentFrame].endRendering();

    transition_image_layout(
        imageIndex, vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,         // srcAccessMask
        {},                                                 // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe           // dstStage
    );

    m_CommandBuffers[m_CurrentFrame].end();
  }

  void transition_image_layout(uint32_t imageIndex, vk::ImageLayout oldLayout,
                               vk::ImageLayout newLayout,
                               vk::AccessFlags2 srcAccessMask,
                               vk::AccessFlags2 dstAccessMask,
                               vk::PipelineStageFlags2 srcStageMask,
                               vk::PipelineStageFlags2 dstStageMask) {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = m_SwapChainImages[imageIndex],
        .subresourceRange =
            {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };
    vk::DependencyInfo dependencyInfo = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    };
    m_CommandBuffers[m_CurrentFrame].pipelineBarrier2(dependencyInfo);
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
    std::cout << "Opening file: " << filename << '\n'
              << '\t'
              << "Current Directory: " << std::filesystem::current_path()
              << '\n';

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

  void cleanupSwapChain() {
    m_SwapChainImageViews.clear();
    m_SwapChain.clear();

    m_SwapChain = nullptr;
  }

  void recreateSwapChain() {
    // std::cout << "Recreating the swapchain" << std::endl;
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_Window, &width, &height);
    while (width == 0 || height == 0) {
      glfwGetFramebufferSize(m_Window, &width, &height);
      glfwWaitEvents();
    }

    m_Device.waitIdle();

    cleanupSwapChain();
    createSwapChain();
    createImageViews();
  }

  [[nodiscard]] vk::raii::ShaderModule
  createShaderModule(const std::vector<char> &code) const {
    vk::ShaderModuleCreateInfo createInfo{
        .codeSize = code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t *>(code.data())};
    vk::raii::ShaderModule shaderModule{m_Device, createInfo};

    return shaderModule;
  }

  static void framebufferResizeCallback(GLFWwindow *window, int width,
                                        int height) {
    auto app = reinterpret_cast<HelloTriangleApplication *>(
        glfwGetWindowUserPointer(window));
    app->m_FramebufferResized = true;
  }

  uint32_t findMemoryType(uint32_t typeFilter,
                          vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties =
        m_PhysicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((typeFilter & (1 << i)) &&
          (memProperties.memoryTypes[i].propertyFlags & properties) ==
              properties) {
        return i;
      }
    }

    throw std::runtime_error("failed to find suitable memory type!");
  }

  void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                    vk::MemoryPropertyFlags properties,
                    vk::raii::Buffer &buffer,
                    vk::raii::DeviceMemory &bufferMemory) {
    vk::BufferCreateInfo bufferInfo{
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
    };
    buffer = vk::raii::Buffer(m_Device, bufferInfo);

    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex =
            findMemoryType(memRequirements.memoryTypeBits, properties),
    };

    bufferMemory = vk::raii::DeviceMemory(m_Device, allocInfo);

    buffer.bindMemory(*bufferMemory, 0);
  }

private:
  GLFWwindow *m_Window;
  Renderer::Instance m_Instance;
  vk::raii::SurfaceKHR m_Surface = nullptr;
  vk::raii::PhysicalDevice m_PhysicalDevice = nullptr;
  vk::raii::Device m_Device = nullptr;
  vk::raii::Queue m_GraphicsQueue = nullptr;
  vk::raii::Queue m_PresentQueue = nullptr;
  uint32_t m_GraphicsIndex = 0;
  uint32_t m_PresentIndex = 0;

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

  vk::raii::Pipeline m_GraphicsPipeline = nullptr;

  vk::raii::CommandPool m_CommandPool = nullptr;
  std::vector<vk::raii::CommandBuffer> m_CommandBuffers;

  // Syncronization primitives
  std::vector<vk::raii::Semaphore> m_ImageAvailableSemaphores;
  std::vector<vk::raii::Semaphore> m_RenderFinishedSemaphores;
  std::vector<vk::raii::Fence> m_InFlightFences;
  uint32_t m_CurrentFrame = 0;
  uint32_t m_SemaphoreIndex = 0;

  bool m_FramebufferResized = false;

  vk::raii::Buffer m_VertexBuffer = nullptr;
  vk::raii::DeviceMemory m_VertexBufferMemory = nullptr;

  vk::raii::Buffer m_IndexBuffer = nullptr;
  vk::raii::DeviceMemory m_IndexBufferMemory = nullptr;
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
