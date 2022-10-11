#ifndef PCH_HPP
#define PCH_HPP

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define UNICODE

#include <Windows.h>
#include <winbase.h>
#include <windowsx.h>

#elif (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
#else
#error Target operating system is not supported.
#endif

#if defined(DIRECTX_ENABLED)
#include <DirectXMath.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <dxgi1_3.h>
#include <wrl\client.h>

using WindowHandle = HWND;
#elif defined(VULKAN_ENABLED)
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

using WindowHandle = GLFWwindow *;
#else
#include <glad/glad.h>
#include <GLFW/glfw3.h>

using WindowHandle = GLFWwindow *;
#endif

#if defined(_WIN32)
#define STBI_WINDOWS_UTF8
#endif

#endif // PCH_HPP
