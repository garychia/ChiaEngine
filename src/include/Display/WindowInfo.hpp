#ifndef WINDOW_INFO_HPP
#define WINDOW_INFO_HPP

#include "Data/String.hpp"
#include "Display/GUI/Border.hpp"
#include "Geometry/2D/Point2D.hpp"
#include "Types/Types.hpp"

struct AppInfo;

struct WindowInfo
{
    const AppInfo *pAppInfo;

    String title;

    Border border;

    bool fullScreen;

    WindowInfo(const AppInfo *pAppInfo, const String &title = String(""), bool fullScreen = false, unsigned int width = 800,
               unsigned int height = 400, unsigned int positionFromTop = 10, unsigned int positionFromLeft = 10);

    WindowInfo(const AppInfo *pAppInfo, const String &title = String(""), bool fullScreen = false,
               const Border &border = Border(10, 10, 800, 400));

    Point2D GetTopLeftPosition(float parentWidth = 0.f, float parentHeight = 0.f) const;

    float GetWidth(float parentWidth = 0.f) const;

    float GetHeight(float parentHeight = 0.f) const;
};

#endif
