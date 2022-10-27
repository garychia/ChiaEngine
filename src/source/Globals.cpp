#include "Globals.hpp"

const unsigned long DEFAULT_WINDOW_WIDTH = 1000;
const unsigned long DEFAULT_WINDOW_HEIGHT = 800;

WindowHandle MainWindow = nullptr;

#if defined(DIRECTX_ENABLED)
HINSTANCE AppInstance;
#elif defined(VULKAN_ENABLED)
VkInstance VulkanInstance;
#endif

bool FullScreen = false;
const String AppName("Chia Engine");

const Version AppVersion{1, 0, 0};
const Version EngineVersion{1, 0, 0};
