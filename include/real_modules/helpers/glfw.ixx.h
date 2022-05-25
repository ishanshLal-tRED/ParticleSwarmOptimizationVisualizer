#include <logging.hxx>

import <GLFW/glfw3.h>;
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>;

export import <span>;

export module Helpers.GLFW;

export namespace Helper{
	inline std::span<const char*> GetGLFWRequiredVkExtensions() {
		uint32_t glfw_extension_count = 0;
		return {glfwGetRequiredInstanceExtensions(&glfw_extension_count), glfw_extension_count};
	}
	inline HWND Win32WindowHandleFromGLFW (GLFWwindow* window) {
		return glfwGetWin32Window(window);
	}
	inline std::pair<uint32_t, uint32_t> GLFWGetFrameBufferSize (GLFWwindow* window) {
		int width, height;
		glfwGetFramebufferSize (window, &width, &height);
		return {uint32_t(width), uint32_t(height)};
	}
};