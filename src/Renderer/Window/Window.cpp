#define GLFW_INCLUDE_VULKAN
#include "Window.h" // Include your header after GLFW/Vulkan

namespace Renderer {

void Window::CreateSurface(Renderer::Instance &instance) {
  VkSurfaceKHR _surface;
  if (glfwCreateWindowSurface(instance.Get(), m_Window, nullptr, &_surface) !=
      0) {
    throw std::runtime_error("failed to create window surface!");
  }
  m_Surface = vk::raii::SurfaceKHR(instance.GetRaii(), _surface);
}

void Window::InitWindow(uint32_t width, uint32_t height,
                        void (*resizeCallback)(GLFWwindow *window, int width,
                                               int height)) {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  m_Window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
  glfwSetWindowUserPointer(m_Window, this);
  glfwSetFramebufferSizeCallback(m_Window, resizeCallback);
}

void Window::Cleanup() {
  glfwDestroyWindow(m_Window);
  glfwTerminate();
}
} // namespace Renderer
