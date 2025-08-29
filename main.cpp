#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>

#include <glm/glm.hpp>

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "src/Renderer/Command/CommandBuffer.h"
#include "src/Renderer/Command/CommandPool.h"
#include "src/Renderer/Device/Device.h"
#include "src/Renderer/Instance/Instance.h"
#include "src/Renderer/Pipeline/Pipeline.h"
#include "src/Renderer/Swapchain/Swapchain.h"
#include "src/Renderer/Window/Window.h"

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

    m_Window = std::make_unique<Renderer::Window>(WIDTH, HEIGHT,
                                                  framebufferResizeCallback);
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  void initVulkan() {
    m_Instance = std::make_unique<Renderer::Instance>("Vulkan App");
    m_Window->CreateSurface(*m_Instance);
    m_DeviceHand = std::make_unique<Renderer::Device>(*m_Instance,
                                                      *m_Window->GetSurface());

    m_SwapChain =
        std::make_unique<Renderer::Swapchain>(*m_DeviceHand, *m_Window);
    m_SwapChain->CreateImageViews(*m_DeviceHand);

    m_GraphicsPipeline =
        std::make_unique<Renderer::Pipeline>(*m_DeviceHand, *m_SwapChain);

    m_CommandPool = std::make_unique<Renderer::CommandPool>(
        *m_DeviceHand,
        vk::CommandPoolCreateFlags::BitsType::eResetCommandBuffer);

    createVertexBuffer();
    createIndexBuffer();
    m_CommandBuffers = m_CommandPool->allocatePrimary(MAX_FRAMES_IN_FLIGHT);
    createSyncObjects();

    // This should trigger a validation error
    VkBuffer invalidBuffer = VK_NULL_HANDLE;
    vkDestroyBuffer(*m_DeviceHand->GetDevice(), invalidBuffer,
                    nullptr); // Should produce validation error
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
    vk::raii::Buffer stagingBuffer(m_DeviceHand->GetDevice(), stagingInfo);
    vk::MemoryRequirements memRequirementsStaging =
        stagingBuffer.getMemoryRequirements();
    vk::MemoryAllocateInfo memoryAllocateInfoStaging{
        .allocationSize = memRequirementsStaging.size,
        .memoryTypeIndex =
            findMemoryType(memRequirementsStaging.memoryTypeBits,
                           vk::MemoryPropertyFlagBits::eHostVisible |
                               vk::MemoryPropertyFlagBits::eHostCoherent),
    };
    vk::raii::DeviceMemory stagingBufferMemory(m_DeviceHand->GetDevice(),
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

    m_VertexBuffer = vk::raii::Buffer(m_DeviceHand->GetDevice(), bufferInfo);

    vk::MemoryRequirements memRequirements =
        m_VertexBuffer.getMemoryRequirements();
    vk::MemoryAllocateInfo memoryAllocateInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex =
            findMemoryType(memRequirements.memoryTypeBits,
                           vk::MemoryPropertyFlagBits::eDeviceLocal)};
    m_VertexBufferMemory =
        vk::raii::DeviceMemory(m_DeviceHand->GetDevice(), memoryAllocateInfo);

    m_VertexBuffer.bindMemory(*m_VertexBufferMemory, 0);

    copyBuffer(stagingBuffer, m_VertexBuffer, stagingInfo.size);
  }

  void copyBuffer(vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer,
                  vk::DeviceSize size) {

    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = *m_CommandPool->Get(),
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    };

    vk::raii::CommandBuffer commandCopyBuffer = std::move(
        m_DeviceHand->GetDevice().allocateCommandBuffers(allocInfo).front());

    commandCopyBuffer.begin(vk::CommandBufferBeginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer,
                                 vk::BufferCopy{0, 0, size});

    commandCopyBuffer.end();

    m_DeviceHand->GetGraphicsQueue().submit(
        vk::SubmitInfo{.commandBufferCount = 1,
                       .pCommandBuffers = &*commandCopyBuffer},
        nullptr);
    m_DeviceHand->GetGraphicsQueue().waitIdle();
  }

  void createSyncObjects() {
    m_ImageAvailableSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
    m_RenderFinishedSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
    m_InFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < m_SwapChain->GetImages().size(); i++) {
      m_ImageAvailableSemaphores.emplace_back(
          m_DeviceHand->GetDevice().createSemaphore({}));
      m_RenderFinishedSemaphores.emplace_back(
          m_DeviceHand->GetDevice().createSemaphore({}));

      vk::FenceCreateInfo fenceInfo{};
      fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
      m_InFlightFences.emplace_back(
          m_DeviceHand->GetDevice().createFence(fenceInfo));
    }
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(m_Window->GetWindow())) {
      glfwPollEvents();
      drawFrame();
    }
    m_DeviceHand->GetDevice().waitIdle();
  }

  void drawFrame() {
    while (vk::Result::eTimeout ==
           m_DeviceHand->GetDevice().waitForFences(
               *m_InFlightFences[m_CurrentFrame], vk::True, UINT64_MAX))
      ;

    auto [result, imageIndex] = m_SwapChain->Get().acquireNextImage(
        UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR ||
        result == vk::Result::eSuboptimalKHR) {
      m_FramebufferResized = false;
      m_SwapChain->RecreateSwapChain(*m_DeviceHand, *m_Window);
      return;
    }

    if (result != vk::Result::eSuccess &&
        result != vk::Result::eSuboptimalKHR) {
      throw std::runtime_error("failed to acquire swap chain image!");
    }

    m_DeviceHand->GetDevice().resetFences(*m_InFlightFences[m_CurrentFrame]);

    m_CommandBuffers[m_CurrentFrame]->reset();
    recordCommandBuffer(imageIndex);

    vk::PipelineStageFlags waitDestinationStageMask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput);

    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores =
            &*m_ImageAvailableSemaphores[m_CurrentFrame], // Wait for acquire
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*(m_CommandBuffers[m_CurrentFrame]->get()),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores =
            &*m_RenderFinishedSemaphores[imageIndex], // Signal based on IMAGE
                                                      // INDEX
    };

    m_DeviceHand->GetGraphicsQueue().submit(submitInfo,
                                            *m_InFlightFences[m_CurrentFrame]);

    // Present the rendered image
    const vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores =
            &*m_RenderFinishedSemaphores[imageIndex], // Wait for render of
                                                      // THIS image
        .swapchainCount = 1,
        .pSwapchains = &*m_SwapChain->Get(),
        .pImageIndices = &imageIndex,
    };

    result = m_DeviceHand->GetPresentQueue().presentKHR(presentInfoKHR);

    switch (result) {
    case vk::Result::eSuccess:
      break;

    case vk::Result::eErrorOutOfDateKHR:
    case vk::Result::eSuboptimalKHR:
      m_SwapChain->RecreateSwapChain(*m_DeviceHand, *m_Window);
      break;
    default:
      break;
    }

    if (m_FramebufferResized) {
      m_FramebufferResized = false;
      m_SwapChain->RecreateSwapChain(*m_DeviceHand, *m_Window);
    }

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  void cleanup() {

    m_DeviceHand->GetDevice().waitIdle();

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

    m_SwapChain->RecreateSwapChain(*m_DeviceHand, *m_Window);
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
    m_CommandBuffers[m_CurrentFrame]->get().begin({});
    transition_image_layout(
        imageIndex, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {}, // srcAccessMask (no need to wait for previous operations)
        vk::AccessFlagBits2::eColorAttachmentWrite,        // dstAccessMask
        vk::PipelineStageFlagBits2::eTopOfPipe,            // srcStage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput // dstStage
    );

    vk::ClearValue clearColor =
        vk::ClearValue{{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}}};

    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView = m_SwapChain->GetImageViews()[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor};

    vk::RenderingInfo renderingInfo = {
        .renderArea = {.offset = {0, 0}, .extent = m_SwapChain->GetExtend2D()},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentInfo};

    // Start Rendering
    m_CommandBuffers[m_CurrentFrame]->get().beginRendering(renderingInfo);

    vk::Viewport viewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(m_SwapChain->GetExtend2D().width),
        .height = static_cast<float>(m_SwapChain->GetExtend2D().height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vk::Rect2D scissor{
        .offset = {0, 0},
        .extent = m_SwapChain->GetExtend2D(),
    };

    m_CommandBuffers[m_CurrentFrame]->get().setViewport(0, viewport);
    m_CommandBuffers[m_CurrentFrame]->get().setScissor(0, scissor);

    m_CommandBuffers[m_CurrentFrame]->get().bindPipeline(
        vk::PipelineBindPoint::eGraphics, *m_GraphicsPipeline->Get());

    m_CommandBuffers[m_CurrentFrame]->get().bindVertexBuffers(
        0, *m_VertexBuffer, {0});
    m_CommandBuffers[m_CurrentFrame]->get().bindIndexBuffer(
        m_IndexBuffer, 0, vk::IndexType::eUint16);

    m_CommandBuffers[m_CurrentFrame]->get().drawIndexed(indices.size(), 1, 0, 0,
                                                        0);

    m_CommandBuffers[m_CurrentFrame]->get().endRendering();

    transition_image_layout(
        imageIndex, vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,         // srcAccessMask
        {},                                                 // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe           // dstStage
    );

    m_CommandBuffers[m_CurrentFrame]->get().end();
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
        .image = m_SwapChain->GetImages()[imageIndex],
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
    m_CommandBuffers[m_CurrentFrame]->get().pipelineBarrier2(dependencyInfo);
  }

  uint32_t findQueueFamilies(VkPhysicalDevice device) {
    // find the index of the first queue family that supports graphics
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        m_DeviceHand->GetPhysicalDevice().getQueueFamilyProperties();

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

  static void framebufferResizeCallback(GLFWwindow *window, int width,
                                        int height) {
    auto app = reinterpret_cast<HelloTriangleApplication *>(
        glfwGetWindowUserPointer(window));
    app->m_FramebufferResized = true;
  }

  uint32_t findMemoryType(uint32_t typeFilter,
                          vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties =
        m_DeviceHand->GetPhysicalDevice().getMemoryProperties();

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
    buffer = vk::raii::Buffer(m_DeviceHand->GetDevice(), bufferInfo);

    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex =
            findMemoryType(memRequirements.memoryTypeBits, properties),
    };

    bufferMemory = vk::raii::DeviceMemory(m_DeviceHand->GetDevice(), allocInfo);

    buffer.bindMemory(*bufferMemory, 0);
  }

private:
  std::unique_ptr<Renderer::Instance> m_Instance;
  std::unique_ptr<Renderer::Window> m_Window;
  std::unique_ptr<Renderer::Device> m_DeviceHand;
  std::unique_ptr<Renderer::Swapchain> m_SwapChain;
  std::unique_ptr<Renderer::Pipeline> m_GraphicsPipeline;

  std::unique_ptr<Renderer::CommandPool> m_CommandPool;
  std::vector<std::unique_ptr<Renderer::CommandBuffer>> m_CommandBuffers;

  // Syncronization primitives
  std::vector<vk::raii::Semaphore> m_ImageAvailableSemaphores;
  std::vector<vk::raii::Semaphore> m_RenderFinishedSemaphores;
  std::vector<vk::raii::Fence> m_InFlightFences;
  uint32_t m_CurrentFrame = 0;

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
