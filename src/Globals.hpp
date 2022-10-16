#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include "Data/String.hpp"
#include "pch.hpp"

extern const unsigned long WINDOW_WIDTH;
extern const unsigned long WINDOW_HEIGHT;

extern WindowHandle MainWindow;
#if defined(_WIN32)
extern HINSTANCE AppInstance;
#elif (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
extern GLWFwindow *MainWindow;
#else
#error Target operating system is not supported.
#endif

extern bool FullScreen;
extern const String AppName;
#endif // GLOBALS_HPP
