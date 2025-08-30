#include "Pipeline.h"
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>

namespace Renderer {

struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;

  static vk::VertexInputBindingDescription getBindingDescription() {
    return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
  }

  static std::array<vk::VertexInputAttributeDescription, 2>
  getAttributeDescriptions() {
    return {{vk::VertexInputAttributeDescription{
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
             }}};
  }
};

Pipeline::Pipeline(Renderer::Device &device, Renderer::Swapchain &swapchain,
                   uint32_t maxFramesInFlight,
                   std::vector<vk::raii::Buffer> &uniformBuffers,
                   size_t uniformBufferObjectSize) {
  vk::raii::ShaderModule shaderModule =
      createShaderModule(readFile("shaders/slang.spv"), device);

  CreateDescriptorPool(maxFramesInFlight);
  CreateDescriptorSets(device, maxFramesInFlight, uniformBuffers,
                       uniformBufferObjectSize);

  vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
  vertShaderStageInfo.module = shaderModule;
  vertShaderStageInfo.pName = "vertMain";

  vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
  fragShaderStageInfo.module = shaderModule;
  fragShaderStageInfo.pName = "fragMain";

  vk::PipelineShaderStageCreateInfo shaderStages[] = {
      vertShaderStageInfo,
      fragShaderStageInfo,
  };

  vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = 2;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

  vk::PipelineViewportStateCreateInfo viewportState{};
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  vk::PipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.depthClampEnable = vk::False;
  rasterizer.rasterizerDiscardEnable = vk::False;
  rasterizer.polygonMode = vk::PolygonMode::eFill;
  rasterizer.cullMode = vk::CullModeFlagBits::eBack;
  rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
  rasterizer.depthBiasEnable = vk::False;
  rasterizer.depthBiasSlopeFactor = 1.0f;
  rasterizer.lineWidth = 1.0f;

  vk::PipelineMultisampleStateCreateInfo multisampling{};
  multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
  multisampling.sampleShadingEnable = vk::False;

  vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.blendEnable = vk::False;
  colorBlendAttachment.colorWriteMask =
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

  vk::PipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.logicOpEnable = vk::False;
  colorBlending.logicOp = vk::LogicOp::eCopy;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport,
                                                 vk::DynamicState::eScissor};

  vk::PipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  CreateDescriptorSetLayout(device);

  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &*m_DescriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 0;

  m_PipelineLayout =
      vk::raii::PipelineLayout(device.GetDevice(), pipelineLayoutInfo);

  vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
  pipelineRenderingCreateInfo.colorAttachmentCount = 1;
  pipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchain.GetFormat();

  vk::GraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.pNext = &pipelineRenderingCreateInfo;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = m_PipelineLayout;
  pipelineInfo.renderPass = nullptr;

  m_GraphicsPipeline =
      vk::raii::Pipeline(device.GetDevice(), nullptr, pipelineInfo);
}
Pipeline::~Pipeline() {}

std::vector<char> Pipeline::readFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  std::cout << "Opening file: " << filename << '\n'
            << '\t' << "Current Directory: " << std::filesystem::current_path()
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

[[nodiscard]] vk::raii::ShaderModule
Pipeline::createShaderModule(const std::vector<char> &code,
                             Renderer::Device &device) {
  vk::ShaderModuleCreateInfo createInfo{};
  createInfo.codeSize = code.size() * sizeof(char);
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
  vk::raii::ShaderModule shaderModule{device.GetDevice(), createInfo};

  return shaderModule;
}

void Pipeline::CreateDescriptorSetLayout(Renderer::Device &device) {
  vk::DescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

  vk::DescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.flags = {};
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &uboLayoutBinding;
  m_DescriptorSetLayout =
      vk::raii::DescriptorSetLayout(device.GetDevice(), layoutInfo, nullptr);
}

void Pipeline::CreateDescriptorPool(uint32_t maxFramesInFlight) {
  vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer,
                                  maxFramesInFlight);

  vk::DescriptorPoolCreateInfo poolInfo{};
  poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
  poolInfo.maxSets = maxFramesInFlight;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
}

void Pipeline::CreateDescriptorSets(
    Renderer::Device &device, uint32_t maxFramesInFlight,
    std::vector<vk::raii::Buffer> &uniformBuffers,
    size_t uniformBufferObjectSize) {
  std::vector<vk::DescriptorSetLayout> layouts(maxFramesInFlight,
                                               *m_DescriptorSetLayout);
  vk::DescriptorSetAllocateInfo allocInfo{};
  allocInfo.descriptorPool = m_DescriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
  allocInfo.pSetLayouts = layouts.data();

  m_DescriptorSets.clear();
  m_DescriptorSets = device.GetDevice().allocateDescriptorSets(allocInfo);

  for (size_t i = 0; i < maxFramesInFlight; i++) {
    vk::DescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = uniformBufferObjectSize;

    vk::WriteDescriptorSet descriptorWrite{};
    descriptorWrite.dstSet = m_DescriptorSets[i];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    descriptorWrite.pBufferInfo = &bufferInfo;

    device.GetDevice().updateDescriptorSets(descriptorWrite, {});
  }
}

} // namespace Renderer
