#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <glm/glm.hpp>

#include "../Device/Device.h"
#include "../Swapchain/Swapchain.h"
#include "../Texture/Texture.h"

namespace Renderer {

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;
  glm::vec2 texCoord;

  static vk::VertexInputBindingDescription getBindingDescription() {
    return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
  }

  static std::array<vk::VertexInputAttributeDescription, 3>
  getAttributeDescriptions() {
    return {{
        vk::VertexInputAttributeDescription{
            0,                         // location
            0,                         // binding
            vk::Format::eR32G32Sfloat, // format
            offsetof(Vertex, pos)      // offset
        },
        vk::VertexInputAttributeDescription{
            1,                            // location
            0,                            // binding
            vk::Format::eR32G32B32Sfloat, // format
            offsetof(Vertex, color)       // offset
        },
        vk::VertexInputAttributeDescription{
            2,                         // location
            0,                         // binding
            vk::Format::eR32G32Sfloat, // format
            offsetof(Vertex, texCoord) // offset
        },
    }};
  }
};

class Pipeline {
public:
  Pipeline(Renderer::Device &device, Renderer::Swapchain &swapchain,
           uint32_t maxFramesInFlight,
           std::vector<vk::raii::Buffer> &uniformBuffers,
           size_t uniformBufferObjectSize, Texture &texture);
  ~Pipeline();

  vk::raii::Pipeline &Get() { return m_GraphicsPipeline; }
  vk::raii::PipelineLayout &GetLayout() { return m_PipelineLayout; }
  std::vector<vk::raii::DescriptorSet> &GetDescriptorSets() {
    return m_DescriptorSets;
  }

private:
  static std::vector<char> readFile(const std::string &filename);

  [[nodiscard]] vk::raii::ShaderModule
  createShaderModule(const std::vector<char> &code, Renderer::Device &device);

  void CreateDescriptorSetLayout(Renderer::Device &device);
  void CreateDescriptorPool(Renderer::Device &device,
                            uint32_t maxFramesInFlight);
  void CreateDescriptorSets(Renderer::Device &device,
                            uint32_t maxFramesInFlight,
                            std::vector<vk::raii::Buffer> &uniformBuffers,
                            size_t uniformBufferObjectSize, Texture &texture);

private:
  vk::raii::PipelineLayout m_PipelineLayout = nullptr;
  vk::raii::Pipeline m_GraphicsPipeline = nullptr;

  vk::raii::DescriptorSetLayout m_DescriptorSetLayout = nullptr;
  vk::raii::DescriptorPool m_DescriptorPool = nullptr;
  std::vector<vk::raii::DescriptorSet> m_DescriptorSets;
};

} // namespace Renderer
