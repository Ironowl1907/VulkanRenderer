#include "Swapchain.h"
#include "../Texture/Texture.h"
#include <iostream>
#include <limits>

namespace Renderer {

Swapchain::Swapchain(Renderer::Device &device, Renderer::Window &window) {
  Create(device, window);
}

Swapchain::~Swapchain() {}

void Swapchain::Create(Renderer::Device &device, Renderer::Window &window) {

  auto surfaceCapabilities =
      device.GetPhysicalDevice().getSurfaceCapabilitiesKHR(window.GetSurface());

  auto swapChainSurfaceFormat = ChooseSwapSurfaceFormat(
      device.GetPhysicalDevice().getSurfaceFormatsKHR(window.GetSurface()));

  auto swapChainExtent = ChooseSwapExtent(window, surfaceCapabilities);

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

  vk::SwapchainCreateInfoKHR swapChainCreateInfo{};
  swapChainCreateInfo.flags = vk::SwapchainCreateFlagsKHR();
  swapChainCreateInfo.surface = window.GetSurface();
  swapChainCreateInfo.minImageCount = minImageCount;
  swapChainCreateInfo.imageFormat = swapChainSurfaceFormat.format;
  swapChainCreateInfo.imageColorSpace = swapChainSurfaceFormat.colorSpace;
  swapChainCreateInfo.imageExtent = swapChainExtent;
  swapChainCreateInfo.imageArrayLayers = 1;
  swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
  swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
  swapChainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
  swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  swapChainCreateInfo.presentMode = ChooseSwapPresentMode(
      device.GetPhysicalDevice().getSurfacePresentModesKHR(
          window.GetSurface())),
  swapChainCreateInfo.clipped = true;
  swapChainCreateInfo.oldSwapchain = nullptr;
  m_SwapChain = vk::raii::SwapchainKHR(device.GetDevice(), swapChainCreateInfo);
  m_SwapChainImages = m_SwapChain.getImages();

  m_SwapChainImageFormat = swapChainSurfaceFormat.format;
  m_SwapChainExtent = swapChainExtent;
}

vk::Extent2D
Swapchain::ChooseSwapExtent(Renderer::Window &window,
                            const vk::SurfaceCapabilitiesKHR &capabilities) {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  }
  int width, height;
  glfwGetFramebufferSize(window.GetWindow(), &width, &height);

  return {
      std::clamp<uint32_t>(width, capabilities.minImageExtent.width,
                           capabilities.maxImageExtent.width),
      std::clamp<uint32_t>(height, capabilities.minImageExtent.height,
                           capabilities.maxImageExtent.height),
  };
}

vk::SurfaceFormatKHR Swapchain::ChooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
        availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

vk::PresentModeKHR Swapchain::ChooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR> &availablePresentModes) {
  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
      return availablePresentMode;
    }
  }
  return vk::PresentModeKHR::eFifo;
}

void Swapchain::CreateImageViews(Renderer::Device &device) {
  m_SwapChainImageViews.clear();

  vk::ImageViewCreateInfo imageViewCreateInfo{};
  imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
  imageViewCreateInfo.format = m_SwapChainImageFormat;
  imageViewCreateInfo.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1,
                                          0, 1};
  for (auto image : m_SwapChainImages) {
    imageViewCreateInfo.image = image;
    m_SwapChainImageViews.emplace_back(device.GetDevice(), imageViewCreateInfo);
  }
}

void Swapchain::CleanupSwapChain() {
  m_SwapChainImageViews.clear();
  m_SwapChain.clear();

  m_SwapChain = nullptr;
}

void Swapchain::RecreateSwapChain(Renderer::Device &device,
                                  Renderer::Window &window) {
  int width = 0, height = 0;
  glfwGetFramebufferSize(window.GetWindow(), &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window.GetWindow(), &width, &height);
    glfwWaitEvents();
  }

  device.GetDevice().waitIdle();

  CleanupSwapChain();
  Create(device, window);
  CreateImageViews(device);
}

} // namespace Renderer
