#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <glm/glm.hpp>

#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

#include "src/Renderer/Buffer/Buffer.h"
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

struct UniformBufferObject {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
};

class HelloTriangleApplication {
public:
  void run() {
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

    m_CommandPool = std::make_unique<Renderer::CommandPool>(
        *m_DeviceHand,
        vk::CommandPoolCreateFlags::BitsType::eResetCommandBuffer);

    m_BufferManager = std::make_unique<Renderer::BufferManager>();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();

    m_GraphicsPipeline = std::make_unique<Renderer::Pipeline>(
        *m_DeviceHand, *m_SwapChain, MAX_FRAMES_IN_FLIGHT, m_UniformBuffers,
        sizeof(UniformBufferObject));

    m_CommandBuffers = m_CommandPool->allocatePrimary(MAX_FRAMES_IN_FLIGHT);
    createSyncObjects();
  }

  void createIndexBuffer() {
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    vk::raii::Buffer stagingBuffer({});
    vk::raii::DeviceMemory stagingBufferMemory({});
    m_BufferManager->CreateBuffer(*m_DeviceHand, bufferSize,
                                  vk::BufferUsageFlagBits::eTransferSrc,
                                  vk::MemoryPropertyFlagBits::eHostVisible |
                                      vk::MemoryPropertyFlagBits::eHostCoherent,
                                  stagingBuffer, stagingBufferMemory);

    void *data = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(data, indices.data(), (size_t)bufferSize);
    stagingBufferMemory.unmapMemory();

    m_BufferManager->CreateBuffer(*m_DeviceHand, bufferSize,
                                  vk::BufferUsageFlagBits::eTransferDst |
                                      vk::BufferUsageFlagBits::eIndexBuffer,
                                  vk::MemoryPropertyFlagBits::eDeviceLocal,
                                  m_IndexBuffer, m_IndexBufferMemory);

    m_BufferManager->CopyBuffer(*m_DeviceHand, *m_CommandPool, stagingBuffer,
                                m_IndexBuffer, bufferSize);
  }

  void createUniformBuffers() {
    m_UniformBuffers.clear();
    m_UniformBuffersMapped.clear();
    m_UniformBuffersMemory.clear();

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
      vk::raii::Buffer buffer({});
      vk::raii::DeviceMemory bufferMem({});
      m_BufferManager->CreateBuffer(
          *m_DeviceHand, bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
          vk::MemoryPropertyFlagBits::eHostVisible |
              vk::MemoryPropertyFlagBits::eHostCoherent,
          buffer, bufferMem);
      m_UniformBuffers.emplace_back(std::move(buffer));
      m_UniformBuffersMemory.emplace_back(std::move(bufferMem));
      m_UniformBuffersMapped.emplace_back(
          m_UniformBuffersMemory[i].mapMemory(0, bufferSize));
    }
  }

  void createVertexBuffer() {
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    vk::raii::Buffer stagingBuffer({});
    vk::raii::DeviceMemory stagingBufferMemory({});
    m_BufferManager->CreateBuffer(*m_DeviceHand, bufferSize,
                                  vk::BufferUsageFlagBits::eTransferSrc,
                                  vk::MemoryPropertyFlagBits::eHostVisible |
                                      vk::MemoryPropertyFlagBits::eHostCoherent,
                                  stagingBuffer, stagingBufferMemory);

    void *data = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    stagingBufferMemory.unmapMemory();

    m_BufferManager->CreateBuffer(*m_DeviceHand, bufferSize,
                                  vk::BufferUsageFlagBits::eTransferDst |
                                      vk::BufferUsageFlagBits::eVertexBuffer,
                                  vk::MemoryPropertyFlagBits::eDeviceLocal,
                                  m_VertexBuffer, m_VertexBufferMemory);

    m_BufferManager->CopyBuffer(*m_DeviceHand, *m_CommandPool, stagingBuffer,
                                m_VertexBuffer, bufferSize);
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

    updateUniformBuffer(m_CurrentFrame);

    m_DeviceHand->GetDevice().resetFences(*m_InFlightFences[m_CurrentFrame]);

    m_CommandBuffers[m_CurrentFrame]->reset();
    recordCommandBuffer(imageIndex);

    vk::PipelineStageFlags waitDestinationStageMask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput);

    vk::SubmitInfo submitInfo{};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &*m_ImageAvailableSemaphores[m_CurrentFrame];
    submitInfo.pWaitDstStageMask = &waitDestinationStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &*(m_CommandBuffers[m_CurrentFrame]->get());
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &*m_RenderFinishedSemaphores[imageIndex];

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

  void updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     currentTime - startTime)
                     .count();

    UniformBufferObject ubo{};
    ubo.model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
                       glm::vec3(0.0f, 0.0f, 1.0f));

    ubo.view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                      glm::vec3(0.0f, 0.0f, 1.0f));

    ubo.proj = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(m_SwapChain->GetExtend2D().width) /
            static_cast<float>(m_SwapChain->GetExtend2D().height),
        0.1f, 10.0f);

    // Flip the Y coordinate
    ubo.proj[1][1] *= -1;

    // Now we wrtite the new uniform buffer in the used one
    memcpy(m_UniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
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

    m_CommandBuffers[m_CurrentFrame]->get().bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, m_GraphicsPipeline->GetLayout(), 0,
        *m_GraphicsPipeline->GetDescriptorSets()[m_CurrentFrame], nullptr);

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

private:
  std::unique_ptr<Renderer::Instance> m_Instance;
  std::unique_ptr<Renderer::Window> m_Window;
  std::unique_ptr<Renderer::Device> m_DeviceHand;
  std::unique_ptr<Renderer::Swapchain> m_SwapChain;
  std::unique_ptr<Renderer::Pipeline> m_GraphicsPipeline;
  std::unique_ptr<Renderer::BufferManager> m_BufferManager;

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

  std::vector<vk::raii::Buffer> m_UniformBuffers;
  std::vector<vk::raii::DeviceMemory> m_UniformBuffersMemory;
  std::vector<void *> m_UniformBuffersMapped;
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
