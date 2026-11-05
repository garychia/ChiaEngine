#ifndef AUDIO_LISTENER_HPP
#define AUDIO_LISTENER_HPP

#include "Geometry/3D/Point3D.hpp"

// AudioListener:3D 音訊監聽者元件，通常附著於攝影機 Entity。
// 一個場景中應只有一個作用中的 listener。position/forward/up 定義
// 監聽者在世界空間的姿態，供 AudioRuntime 在 Update 中更新 miniaudio
// 的 listener 狀態(3D 音訊位置運算依據)。
// trivially copyable、無 padding → 可安全進 ComponentPool::Hash。
struct AudioListener
{
    Point3D position = Point3D(0, 0, 0);   // 監聽者世界位置
    Point3D forward = Point3D(0, 0, -1);   // 監聽者朝向
    Point3D up = Point3D(0, 1, 0);         // 監聽者上方向
    bool active = true;                    // 是否為作用中的 listener

    AudioListener() = default;
    AudioListener(const Point3D &pos, const Point3D &fwd, const Point3D &up_)
        : position(pos), forward(fwd), up(up_)
    {
    }
};

#endif // AUDIO_LISTENER_HPP
