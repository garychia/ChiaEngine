#ifndef TRANSFORM_GIZMO_HPP
#define TRANSFORM_GIZMO_HPP

#include "Geometry/3D/Point3D.hpp"
#include <cstdint>

// ADR-0001 D6:Transform Gizmo 純數學層。EditorSession 保持 window-agnostic,
// 此層不含任何 GLFW/Window/Renderer 類型。Renderer 層(OpenGL ForwardRenderer)
// 負責根據本類別輸出的線段與顏色繪製 gizmo 視覺。
//
// 責任:
//  1. 綁定目標 Entity(透過 entity index,由 EditorSession::Select 觸發)
//  2. 從視窗射線(ray origin/dir)計算最近軸選取(GetAxisFromRay)
//  3. 從拖曳起點/終點計算變換增量(GetDelta:平移/旋轉/縮放)
//  4. 提供各軸顏色與線段端點,供 Renderer 繪製
//
// 決定論守則:僅使用 Point3D 基本運算(加減乘除、點積/叉積經由成員函數),
// 不使用 -ffast-math / -Ofast / -march=native。
struct TransformGizmo
{
    enum class Axis : uint32_t
    {
        None = 0,
        X = 1,
        Y = 2,
        Z = 3,
        All = 4,
    };

    enum class Mode : uint32_t
    {
        Translate = 0,
        Rotate = 1,
        Scale = 2,
    };

    TransformGizmo();
    ~TransformGizmo();

    // 綁定/解除目標 Entity(entity index = 0 表示無目標)。
    void SetTargetEntity(uint32_t entityIndex);
    uint32_t GetTargetEntity() const { return mTargetEntity; }

    // 設定/取得當前操作模式。
    void SetMode(Mode mode);
    Mode GetMode() const { return mMode; }

    // 取得當前選中的軸(由 GetAxisFromRay 填入)。
    Axis GetSelectedAxis() const { return mSelectedAxis; }

    // 設定 gizmo 在世界空間的位置(通常 = 目標實體的 TransformComponent.position)。
    void SetWorldPosition(const Point3D &pos);
    const Point3D &GetWorldPosition() const { return mWorldPosition; }

    // 軸線長度(世界單位)。預設 1.0。
    void SetAxisLength(float length);
    float GetAxisLength() const { return mAxisLength; }

    // ---- 射線軸選取 ----
    // 從相機射線(rayOrigin + t*rayDir)計算與三個坐標軸線段的最近點,
    // 回傳距離最近的軸。若皆超過選取閾值則回傳 Axis::None。
    // 選取閾值:射線到軸線段的最短距離 <= mPickThreshold(世界單位)。
    Axis GetAxisFromRay(const Point3D &rayOrigin, const Point3D &rayDir);

    // 設定射線選取閾值(世界單位)。預設 0.05f。
    void SetPickThreshold(float threshold);
    float GetPickThreshold() const { return mPickThreshold; }

    // ---- 變換增量計算 ----
    // 根據拖曳起點/終點(世界空間點),計算沿選中軸的變換增量。
    // 回傳值語意依 Mode 而定:
    //   Translate: 增量向量(已投影到選中軸)
    //   Rotate:    增量角度(度,沿選中軸右手定則)
    //   Scale:     增量係數(1.0 = 無變化, >1 放大, <1 縮小)
    // 若無選中軸或無目標,回傳零增量/1.0 係數。
    Point3D GetTranslateDelta(const Point3D &dragStart, const Point3D &dragCurrent) const;
    float GetRotateDelta(const Point3D &dragStart, const Point3D &dragCurrent) const;
    float GetScaleDelta(const Point3D &dragStart, const Point3D &dragCurrent) const;

    // ---- 視覺資訊(供 Renderer 繪製) ----
    // 各軸 RGBA 顏色(X=紅,Y=綠,Z=藍,All=白,None=灰)。
    Point3D GetAxisColor(Axis axis) const;  // 回傳 RGB,alpha 固定 1.0
    // 軸線段端點:從世界位置出發沿軸方向延長 mAxisLength。
    // 回傳 {start, end} 兩點(世界空間)。
    struct AxisLine
    {
        Point3D start;
        Point3D end;
    };
    AxisLine GetAxisLine(Axis axis) const;

  private:
    // 點到線段的最短距離平方(世界單位^2)。
    static float DistSqToSegment(const Point3D &p, const Point3D &a, const Point3D &b);

    // 射線與軸線段的最近點參數 t(沿射線)與軸線段參數 u(0..1)。
    // 回傳射線到線段的最短距離平方。
    static float RayToSegmentDistSq(const Point3D &rayOrigin, const Point3D &rayDir,
                                    const Point3D &segStart, const Point3D &segEnd,
                                    float &outT, float &outU);

    uint32_t mTargetEntity;
    Mode mMode;
    Axis mSelectedAxis;
    Point3D mWorldPosition;
    float mAxisLength;
    float mPickThreshold;
};

#endif // TRANSFORM_GIZMO_HPP
