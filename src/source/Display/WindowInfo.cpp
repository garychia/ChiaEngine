#include "Display/WindowInfo.hpp"

#include "App/App.hpp"

WindowInfo::WindowInfo(const AppInfo *pAppInfo, const String &title, bool fullScreen, unsigned int width,
                       unsigned int height, unsigned int positionFromTop, unsigned int positionFromLeft)
    : pAppInfo(pAppInfo), title(title), fullScreen(fullScreen), border(positionFromLeft, positionFromTop, width, height)
{
}

WindowInfo::WindowInfo(const AppInfo *pAppInfo, const String &title, bool fullScreen, const Border &border)
    : pAppInfo(pAppInfo), title(title), fullScreen(fullScreen), border(border)
{
}

Point2D WindowInfo::GetTopLeftPosition(float parentWidth, float parentHeight) const
{
    float xPosition = border.xPos;
    float yPosition = border.yPos;
    if (border.xPos.IsRelative())
        xPosition *= parentWidth;
    if (border.yPos.IsRelative())
        yPosition *= parentHeight;
    return {xPosition, yPosition};
}

float WindowInfo::GetWidth(float parentWidth) const
{
    if (border.width.IsRelative())
        return border.width * parentWidth;
    return border.width;
}

float WindowInfo::GetHeight(float parentHeight) const
{
    if (border.height.IsRelative())
        return border.height * parentHeight;
    return border.height;
}
