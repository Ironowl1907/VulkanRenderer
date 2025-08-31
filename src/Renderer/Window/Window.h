#pragma once

#include "../Instance/Instance.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Renderer {

class Window {
public:
  Window(uint32_t width, uint32_t height,
         void (*resizeCallback)(GLFWwindow *window, int width, int height),
         void *application);
  ~Window();

  void Clean();

  void CreateSurface(Renderer::Instance &instance);

  GLFWwindow *GetWindow() { return m_Window; }
  vk::raii::SurfaceKHR &GetSurface() { return m_Surface; }

private:
  GLFWwindow *m_Window;
  vk::raii::SurfaceKHR m_Surface = nullptr;
};

} // namespace Renderer
