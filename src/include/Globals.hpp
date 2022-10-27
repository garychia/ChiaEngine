#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include "Data/String.hpp"
#include "Version.hpp"
#include "pch.hpp"

extern const unsigned long DEFAULT_WINDOW_WIDTH;
extern const unsigned long DEFAULT_WINDOW_HEIGHT;

extern WindowHandle MainWindow;
#if defined(DIRECTX_ENABLED)
extern HINSTANCE AppInstance;
#elif defined(VULKAN_ENABLED)
extern VkInstance VulkanInstance;
#endif

extern bool FullScreen;

extern const Version AppVersion; 
extern const Version EngineVersion;
#endif // GLOBALS_HPP
