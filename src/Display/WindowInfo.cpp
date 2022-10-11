#include "WindowInfo.hpp"

WindowInfo::WindowInfo(const String &title, bool fullScreen, unsigned int width, unsigned int height,
                       unsigned int positionFromTop, unsigned int positionFromLeft)
    : title(title), fullScreen(fullScreen), width(width), height(height), positionFromTop(positionFromTop),
      positionFromLeft(positionFromLeft)
{
}
