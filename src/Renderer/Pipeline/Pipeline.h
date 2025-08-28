#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "../Device/Device.h"
#include "../Swapchain/Swapchain.h"

namespace Renderer {

class Pipeline {
public:
  Pipeline(Renderer::Device &device, Renderer::Swapchain &swapchain);
  ~Pipeline();

  vk::raii::Pipeline &Get() { return m_GraphicsPipeline; }

private:
  static std::vector<char> readFile(const std::string &filename);

  [[nodiscard]] vk::raii::ShaderModule
  createShaderModule(const std::vector<char> &code, Renderer::Device &device);

private:
  vk::raii::PipelineLayout m_PipelineLayout = nullptr;
  vk::raii::Pipeline m_GraphicsPipeline = nullptr;
};

} // namespace Renderer
