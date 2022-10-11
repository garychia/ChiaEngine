#ifndef WINDOW_INFO_HPP
#define WINDOW_INFO_HPP

#include "Camera.hpp"
#include "Data/String.hpp"
#include "Scene.hpp"
#include "Types/Types.hpp"

struct WindowInfo
{
    String title;

    unsigned int width;

    unsigned int height;

    unsigned int positionFromTop;

    unsigned int positionFromLeft;

    bool fullScreen;

    WindowInfo(const String &title = String(""), bool fullScreen = false, unsigned int width = 800,
               unsigned int height = 400, unsigned int positionFromTop = 10, unsigned int positionFromLeft = 10);
};

#endif
