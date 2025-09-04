#include "Swapchain.h"
#include "../Texture/Texture.h"
#include <limits>
#include <vulkan/vulkan_enums.hpp>

namespace Renderer {

Swapchain::Swapchain(Device &device, Window &window, CommandPool &commandPool)
    : m_commandPool(commandPool) {
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
          window.GetSurface()));
  swapChainCreateInfo.clipped = true;
  swapChainCreateInfo.oldSwapchain = nullptr;

  m_SwapChain = vk::raii::SwapchainKHR(device.GetDevice(), swapChainCreateInfo);
  m_SwapChainImages = m_SwapChain.getImages();

  m_SwapChainImageFormat = swapChainSurfaceFormat.format;
  m_SwapChainExtent = swapChainExtent;

  createDepthResources(device, m_commandPool);
}

void Swapchain::createDepthResources(Device &device, CommandPool &commandPool) {
  m_depthFormat = FindDepthFormat(device);

  Texture::CreateImage(device, m_SwapChainExtent.width,
                       m_SwapChainExtent.height, m_depthFormat,
                       vk::ImageTiling::eOptimal,
                       vk::ImageUsageFlagBits::eDepthStencilAttachment,
                       vk::MemoryPropertyFlagBits::eDeviceLocal, m_depthImage,
                       m_depthImageMemory);

  vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eDepth;
  if (hasStencilComponent(m_depthFormat)) {
    aspect |= vk::ImageAspectFlagBits::eStencil;
  }

  m_depthImageView =
      Texture::CreateImageView(device, m_depthImage, m_depthFormat, aspect);

  Texture::transitionImageLayout(
      device, commandPool, m_depthFormat, vk::ImageLayout::eUndefined,
      vk::ImageLayout::eDepthStencilAttachmentOptimal, m_depthImage);
}

vk::Format Swapchain::FindDepthFormat(Renderer::Device &device) {
  // Try formats in order of preference (with stencil support first)
  return FindSupportedFormat(
      device,
      {vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint,
       vk::Format::eD32Sfloat},
      vk::ImageTiling::eOptimal,
      vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

bool Swapchain::hasStencilComponent(vk::Format format) {
  return format == vk::Format::eD32SfloatS8Uint ||
         format == vk::Format::eD24UnormS8Uint;
}

vk::Format Swapchain::FindSupportedFormat(
    Renderer::Device &device, const std::vector<vk::Format> &candidates,
    vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
  for (vk::Format format : candidates) {
    vk::FormatProperties props =
        device.GetPhysicalDevice().getFormatProperties(format);

    if (tiling == vk::ImageTiling::eLinear &&
        (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == vk::ImageTiling::eOptimal &&
               (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }

  throw std::runtime_error("Failed to find supported depth format!");
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

void Swapchain::RecreateSwapChain(Device &device, Window &window) {
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
