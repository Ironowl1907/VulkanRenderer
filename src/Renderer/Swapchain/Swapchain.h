#pragma once

#include "../Device/Device.h"
#include "../Window/Window.h"

#include <vulkan/vulkan_raii.hpp>

namespace Renderer {

class Swapchain {
public:
  Swapchain(Renderer::Device &device, Renderer::Window &window);
  ~Swapchain();

  void Create(Renderer::Device &device, Renderer::Window &window);
  vk::raii::SwapchainKHR &Get() { return m_SwapChain; }

  vk::Format &GetFormat() { return m_SwapChainImageFormat; }
  vk::Extent2D &GetExtend2D() { return m_SwapChainExtent; }
  std::vector<vk::Image> GetImages() { return m_SwapChainImages; }
  std::vector<vk::raii::ImageView> &GetImageViews() {
    return m_SwapChainImageViews;
  }

  void CreateImageViews(Renderer::Device &device);

  void RecreateSwapChain(Renderer::Device &deivce, Renderer::Window &window);

private:
  vk::Extent2D ChooseSwapExtent(Renderer::Window &window,
                                const vk::SurfaceCapabilitiesKHR &capabilities);

  vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(
      const std::vector<vk::SurfaceFormatKHR> &availableFormats);

  vk::PresentModeKHR ChooseSwapPresentMode(
      const std::vector<vk::PresentModeKHR> &availablePresentModes);

  void CleanupSwapChain();

private:
  vk::Format m_SwapChainImageFormat = vk::Format::eUndefined;
  vk::Extent2D m_SwapChainExtent;
  std::vector<vk::Image> m_SwapChainImages;
  vk::raii::SwapchainKHR m_SwapChain = nullptr;
  std::vector<vk::raii::ImageView> m_SwapChainImageViews;
};
} // namespace Renderer
