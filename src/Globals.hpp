#include "Data/String.hpp"
#include "pch.hpp"

#define APPLICATION_NAME "Chia Engine"
#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 800

WindowHandle MainWindow = nullptr;
#if defined(_WIN32)
HINSTANCE AppInstance = nullptr;
#elif (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
GLWFwindow *MainWindow = nullptr;
#else
#error Target operating system is not supported.
#endif

bool FullScreen = false;
String AppName(APPLICATION_NAME);
