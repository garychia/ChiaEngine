#include "Globals.hpp"

const unsigned long WINDOW_WIDTH = 1000;
const unsigned long WINDOW_HEIGHT = 800;

WindowHandle MainWindow = nullptr;

#if defined(_WIN32)
HINSTANCE AppInstance = nullptr;
#elif (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
GLWFwindow *MainWindow = nullptr;
#else
#error Target operating system is not supported.
#endif

bool FullScreen = false;
const String AppName("Chia Engine");
