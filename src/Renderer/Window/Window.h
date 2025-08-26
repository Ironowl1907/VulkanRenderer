#pragma once

#include "../Instance/Instance.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Renderer {

class Window {
public:
  Window();
  ~Window();

  void Create();
  void Clean();

  void CreateSurface(Renderer::Instance &instance);

  void InitWindow(uint32_t width, uint32_t height,
                  void (*resizeCallback)(GLFWwindow *window, int width,
                                         int height));

  void Cleanup();

private:
  GLFWwindow *m_Window;
  vk::raii::SurfaceKHR m_Surface;
};

} // namespace Renderer
