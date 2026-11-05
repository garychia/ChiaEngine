#include "Display/GUI/TransformGizmo.hpp"
#include "Math/Math.hpp"

#include <cmath>
#include <cstdio>

TransformGizmo::TransformGizmo()
    : mTargetEntity(0),
      mMode(Mode::Translate),
      mSelectedAxis(Axis::None),
      mWorldPosition(0.f, 0.f, 0.f),
      mAxisLength(1.0f),
      mPickThreshold(0.05f)
{
}

TransformGizmo::~TransformGizmo()
{
}

void TransformGizmo::SetTargetEntity(uint32_t entityIndex)
{
    mTargetEntity = entityIndex;
    if (entityIndex == 0)
    {
        mSelectedAxis = Axis::None;
    }
}

void TransformGizmo::SetMode(Mode mode)
{
    mMode = mode;
    // 模式切換時重置軸選取,避免殘留高亮
    mSelectedAxis = Axis::None;
}

void TransformGizmo::SetWorldPosition(const Point3D &pos)
{
    mWorldPosition = pos;
}

void TransformGizmo::SetAxisLength(float length)
{
    if (length > 0.f)
        mAxisLength = length;
}

TransformGizmo::Axis TransformGizmo::GetAxisFromRay(const Point3D &rayOrigin, const Point3D &rayDir)
{
    if (mTargetEntity == 0)
        return TransformGizmo::Axis::None;

    const float threshSq = mPickThreshold * mPickThreshold;

    // 三個坐標軸的端點(從世界位置出發)
    const Point3D axisDirs[3] = {
        Point3D(1.f, 0.f, 0.f),   // X
        Point3D(0.f, 1.f, 0.f),   // Y
        Point3D(0.f, 0.f, 1.f),   // Z
    };
    const Axis axes[3] = {TransformGizmo::Axis::X, TransformGizmo::Axis::Y, TransformGizmo::Axis::Z};

    Axis bestAxis = Axis::None;
    float bestDistSq = threshSq;  // 只接受 < 閾值的命中

    for (int i = 0; i < 3; i++)
    {
        const Point3D segStart = mWorldPosition;
        const Point3D segEnd = mWorldPosition + axisDirs[i] * mAxisLength;

        float t = 0.f, u = 0.f;
        const float distSq = RayToSegmentDistSq(rayOrigin, rayDir, segStart, segEnd, t, u);
        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            bestAxis = axes[i];
        }
    }

    mSelectedAxis = bestAxis;
    return bestAxis;
}

void TransformGizmo::SetPickThreshold(float threshold)
{
    if (threshold >= 0.f)
        mPickThreshold = threshold;
}

Point3D TransformGizmo::GetTranslateDelta(const Point3D &dragStart,
                                          const Point3D &dragCurrent) const
{
    if (mTargetEntity == 0 || mSelectedAxis == Axis::None)
        return Point3D(0.f, 0.f, 0.f);

    // 拖曳向量(世界空間)
    const Point3D dragVec = dragCurrent - dragStart;

    // 選中軸方向
    Point3D axisDir(0.f, 0.f, 0.f);
    switch (mSelectedAxis)
    {
    case Axis::X:
        axisDir = Point3D(1.f, 0.f, 0.f);
        break;
    case Axis::Y:
        axisDir = Point3D(0.f, 1.f, 0.f);
        break;
    case Axis::Z:
        axisDir = Point3D(0.f, 0.f, 1.f);
        break;
    default:
        return Point3D(0.f, 0.f, 0.f);
    }

    // 將拖曳向量投影到軸方向: dot(dragVec, axisDir) * axisDir
    const float t = dragVec.x * axisDir.x + dragVec.y * axisDir.y + dragVec.z * axisDir.z;
    return axisDir * t;
}

float TransformGizmo::GetRotateDelta(const Point3D &dragStart,
                                     const Point3D &dragCurrent) const
{
    if (mTargetEntity == 0 || mSelectedAxis == Axis::None)
        return 0.f;

    // 選中軸方向
    Point3D axisDir(0.f, 0.f, 0.f);
    switch (mSelectedAxis)
    {
    case Axis::X:
        axisDir = Point3D(1.f, 0.f, 0.f);
        break;
    case Axis::Y:
        axisDir = Point3D(0.f, 1.f, 0.f);
        break;
    case Axis::Z:
        axisDir = Point3D(0.f, 0.f, 1.f);
        break;
    default:
        return 0.f;
    }

    // 將拖曳起點/終點投影到垂直於軸的平面,計算圓周角度
    const Point3D toStart = dragStart - mWorldPosition;
    const Point3D toCurrent = dragCurrent - mWorldPosition;

    // 移除軸向分量
    const float tStart = toStart.x * axisDir.x + toStart.y * axisDir.y + toStart.z * axisDir.z;
    const float tCurrent = toCurrent.x * axisDir.x + toCurrent.y * axisDir.y + toCurrent.z * axisDir.z;
    const Point3D projStart = toStart - axisDir * tStart;
    const Point3D projCurrent = toCurrent - axisDir * tCurrent;

    const float projStartLenSq = projStart.x * projStart.x + projStart.y * projStart.y + projStart.z * projStart.z;
    const float projCurrentLenSq = projCurrent.x * projCurrent.x + projCurrent.y * projCurrent.y + projCurrent.z * projCurrent.z;
    if (projStartLenSq <= 0.f || projCurrentLenSq <= 0.f)
        return 0.f;

    // 兩向量夾角(cos)
    const float dot = projStart.x * projCurrent.x + projStart.y * projCurrent.y + projStart.z * projCurrent.z;
    const float denom = Math::Power(projStartLenSq * projCurrentLenSq, 0.5f);
    if (denom <= 0.f)
        return 0.f;

    float cosAngle = dot / denom;
    if (cosAngle > 1.f)
        cosAngle = 1.f;
    else if (cosAngle < -1.f)
        cosAngle = -1.f;

    // 夾角(度),方向由叉積與軸向的點積決定
    const float angleRad = std::acos(cosAngle);
    const Point3D cross = projStart.Cross(projCurrent);
    const float sign = (cross.x * axisDir.x + cross.y * axisDir.y + cross.z * axisDir.z) >= 0.f ? 1.f : -1.f;

    // 弧度轉度數: 180/pi
    constexpr float kRadToDeg = 57.295779513f;
    return angleRad * kRadToDeg * sign;
}

float TransformGizmo::GetScaleDelta(const Point3D &dragStart,
                                    const Point3D &dragCurrent) const
{
    if (mTargetEntity == 0 || mSelectedAxis == Axis::None)
        return 1.f;

    // 選中軸方向
    Point3D axisDir(0.f, 0.f, 0.f);
    switch (mSelectedAxis)
    {
    case Axis::X:
        axisDir = Point3D(1.f, 0.f, 0.f);
        break;
    case Axis::Y:
        axisDir = Point3D(0.f, 1.f, 0.f);
        break;
    case Axis::Z:
        axisDir = Point3D(0.f, 0.f, 1.f);
        break;
    default:
        return 1.f;
    }

    // 拖曳向量投影到軸方向,正值=放大,負值=縮小
    const Point3D dragVec = dragCurrent - dragStart;
    const float t = dragVec.x * axisDir.x + dragVec.y * axisDir.y + dragVec.z * axisDir.z;

    // 係數:正向拖曳放大(1 + t*axisLength),負向拖曳縮小(1/(1+|t|*axisLength)),始終>0
    if (t >= 0.f)
        return 1.f + t * mAxisLength;
    return 1.f / (1.f + (-t) * mAxisLength);
}

Point3D TransformGizmo::GetAxisColor(Axis axis) const
{
    switch (axis)
    {
    case Axis::X:
        return Point3D(1.f, 0.f, 0.f);   // 紅
    case Axis::Y:
        return Point3D(0.f, 1.f, 0.f);   // 綠
    case Axis::Z:
        return Point3D(0.f, 0.f, 1.f);   // 藍
    case Axis::All:
        return Point3D(1.f, 1.f, 1.f);   // 白
    case Axis::None:
    default:
        return Point3D(0.5f, 0.5f, 0.5f);// 灰
    }
}

TransformGizmo::AxisLine TransformGizmo::GetAxisLine(Axis axis) const
{
    AxisLine line;
    line.start = mWorldPosition;

    Point3D dir(0.f, 0.f, 0.f);
    switch (axis)
    {
    case Axis::X:
        dir = Point3D(1.f, 0.f, 0.f);
        break;
    case Axis::Y:
        dir = Point3D(0.f, 1.f, 0.f);
        break;
    case Axis::Z:
        dir = Point3D(0.f, 0.f, 1.f);
        break;
    default:
        line.end = mWorldPosition;
        return line;
    }

    line.end = mWorldPosition + dir * mAxisLength;
    return line;
}

float TransformGizmo::DistSqToSegment(const Point3D &p, const Point3D &a, const Point3D &b)
{
    const Point3D ab = b - a;
    const Point3D ap = p - a;
    const float abLenSq = ab.x * ab.x + ab.y * ab.y + ab.z * ab.z;
    if (abLenSq <= 0.f)
    {
        // 退化線段,回傳到端點的距離
        const Point3D d = p - a;
        return d.x * d.x + d.y * d.y + d.z * d.z;
    }

    // 投影參數 t = dot(ap, ab) / |ab|^2,夾到 [0,1]
    const float t = (ap.x * ab.x + ap.y * ab.y + ap.z * ab.z) / abLenSq;
    const float tc = (t < 0.f) ? 0.f : (t > 1.f ? 1.f : t);

    const Point3D closest = a + ab * tc;
    const Point3D d = p - closest;
    return d.x * d.x + d.y * d.y + d.z * d.z;
}

float TransformGizmo::RayToSegmentDistSq(const Point3D &rayOrigin, const Point3D &rayDir,
                                         const Point3D &segStart, const Point3D &segEnd,
                                         float &outT, float &outU)
{
    // 解射線與線段的最近點問題。
    // 射線: P(t) = rayOrigin + t*rayDir, t >= 0
    // 線段: Q(u) = segStart + u*(segEnd-segStart), u in [0,1]
    // 最小化 |P(t) - Q(u)|^2

    const Point3D d = rayDir;                      // 射線方向
    const Point3D e = segEnd - segStart;           // 線段方向
    const Point3D r = rayOrigin - segStart;        // 射線原點到線段起點

    const float a = d.x * d.x + d.y * d.y + d.z * d.z;      // |d|^2
    const float e_ = e.x * e.x + e.y * e.y + e.z * e.z;     // |e|^2
    const float f = d.x * e.x + d.y * e.y + d.z * e.z;      // d·e

    // 如果射線方向與線段方向幾乎平行,退化為點到線段距離
    const float denom = a * e_ - f * f;
    if (denom <= 1e-6f)
    {
        outT = 0.f;
        // 點 rayOrigin 到線段的最近點
        const Point3D ap = rayOrigin - segStart;
        float u = (ap.x * e.x + ap.y * e.y + ap.z * e.z) / e_;
        if (u < 0.f)
            u = 0.f;
        else if (u > 1.f)
            u = 1.f;
        outU = u;
        return DistSqToSegment(rayOrigin, segStart, segEnd);
    }

    // 解線性系統求 t, u
    const float c = d.x * r.x + d.y * r.y + d.z * r.z;  // d·r
    const float g = e.x * r.x + e.y * r.y + e.z * r.z;  // e·r

    outT = (f * g - e_ * c) / denom;
    outU = (a * g - f * c) / denom;

    // 夾 t 到 >= 0(射線), u 到 [0,1](線段)
    if (outT < 0.f)
        outT = 0.f;
    if (outU < 0.f)
        outU = 0.f;
    else if (outU > 1.f)
        outU = 1.f;

    // 重新計算最近點距離
    const Point3D pClosest = rayOrigin + d * outT;
    const Point3D qClosest = segStart + e * outU;
    const Point3D diff = pClosest - qClosest;
    return diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
}
