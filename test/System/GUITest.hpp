#ifndef SYSTEM_GUI_TEST_HPP
#define SYSTEM_GUI_TEST_HPP

#include "Test.hpp"
#include "Data/Pointers.hpp"
#include "Display/GUI/Button.hpp"
#include "Display/GUI/GUILayout.hpp"
#include "Display/GUI/GUILayer.hpp"
#include "Geometry/2D/Point2D.hpp"
#include "Types/Types.hpp"

// GUITest(issue #64 — GUI hit-test 輸入派發):
// GUILayout::DispatchMouseDown/Up/Move 把滑鼠事件路由到 IInteractable。
// 在此之前 Button 的 clickEvent/pressEvent/releaseEvent/hoverEvent 從未觸發
// (Window 只路由子視窗、Panel 直接 return false),這是 #60 editor 的前置。
//
// 覆蓋:
//   1. 按鈕內 down+up → clickEvent 觸發(且 press/release 各一次)
//   2. 按鈕外 down/up → 不命中、不消費、無事件
//   3. down 內 + up 外 → 不是點擊,release 也不觸發(Button::OnMouseUp 自帶 guard)
//   4. down 外 + up 內 → 不是點擊(沒有 pressed 目標,up 不消費)
//   5. 重疊元件 → 後加入者(最上層)優先,底層不受影響
//   6. hover:游標移入觸發、移出不觸發
//
// 座標語境:GLFW cursor pos = 視窗 px(top-left 原點,y 向下),與
// IGUI::GetTopLeftPosition/WithIn 相同空間,直接比對。
// 全檔包在 namespace guitest 內:所有 test header 編譯進同一個 TU,
// 裸 using namespace 仍會造成全域 lookup 碰撞(SystemModule.cpp 的 pitfall)。
namespace guitest
{

class GUITest : public Test
{
  private:
    GUILayout layout;

    SharedPtr<Button> pButtonA; // rect (100,50)-(300,90)
    SharedPtr<Button> pButtonB; // rect (200,60)-(280,80),與 A 部分重疊

    int clickCountA = 0;
    int pressCountA = 0;
    int releaseCountA = 0;
    int hoverCountA = 0;
    int clickCountB = 0;

  public:
    GUITest()
        : Test("GUITest"), layout(), pButtonA(), pButtonB()
    {
        Point2D windowSize(800.f, 600.f);
        auto pLayer = SharedPtr<GUILayer>::Construct(windowSize, Border(0.f, 0.f, 800.f, 600.f));
        pButtonA = pLayer->AddComponent<Button>(windowSize, Border(100.f, 50.f, 200.f, 40.f));
        pButtonB = pLayer->AddComponent<Button>(windowSize, Border(200.f, 60.f, 80.f, 20.f));
        pButtonA->clickEvent.Subscribe(this, &GUITest::OnClickA);
        pButtonA->pressEvent.Subscribe(this, &GUITest::OnPressA);
        pButtonA->releaseEvent.Subscribe(this, &GUITest::OnReleaseA);
        pButtonA->hoverEvent.Subscribe(this, &GUITest::OnHoverA);
        pButtonB->clickEvent.Subscribe(this, &GUITest::OnClickB);
        layout.AddLayer(pLayer);
    }

    void OnClickA()
    {
        clickCountA++;
    }

    void OnPressA()
    {
        pressCountA++;
    }

    void OnReleaseA()
    {
        releaseCountA++;
    }

    void OnHoverA()
    {
        hoverCountA++;
    }

    void OnClickB()
    {
        clickCountB++;
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // 1. 按鈕內 down+up → 完整點擊
        EXPECT_TRUE(layout.DispatchMouseDown(Point2D(150.f, 70.f)), "down inside A should hit", true);
        EXPECT_TRUE(layout.DispatchMouseUp(Point2D(150.f, 70.f)), "up inside A should complete click", true);
        EXPECT_TRUE(clickCountA == 1, "click inside should fire once", true);
        EXPECT_TRUE(pressCountA == 1, "press should fire on down", true);
        EXPECT_TRUE(releaseCountA == 1, "release should fire on up", true);

        // 2. 按鈕外 → 不命中、不消費
        EXPECT_TRUE(!layout.DispatchMouseDown(Point2D(10.f, 10.f)), "down outside should miss", true);
        EXPECT_TRUE(!layout.DispatchMouseUp(Point2D(10.f, 10.f)), "up outside with no pressed target should miss", true);
        EXPECT_TRUE(clickCountA == 1, "outside click should not fire", true);

        // 3. down 在內 + up 在外 → 不是點擊(up 消費:結束這次 GUI 按下)
        EXPECT_TRUE(layout.DispatchMouseDown(Point2D(150.f, 70.f)), "down inside (release-outside case)", true);
        EXPECT_TRUE(layout.DispatchMouseUp(Point2D(400.f, 200.f)), "up outside still consumes the GUI press", true);
        EXPECT_TRUE(clickCountA == 1, "down inside + up outside is not a click", true);
        EXPECT_TRUE(releaseCountA == 1, "release outside should not fire (Button guard)", true);

        // 4. down 在外 + up 在內 → 不是點擊(無 pressed 目標,up 不消費)
        EXPECT_TRUE(!layout.DispatchMouseDown(Point2D(400.f, 200.f)), "down outside clears pressed target", true);
        EXPECT_TRUE(!layout.DispatchMouseUp(Point2D(150.f, 70.f)), "up inside without pressed target should miss", true);
        EXPECT_TRUE(clickCountA == 1, "outside-down + inside-up is not a click", true);

        // 5. 重疊:B 後加入(z 較高)覆蓋 A 的 (200,60)-(280,80) → 點 B 不點 A
        EXPECT_TRUE(layout.DispatchMouseDown(Point2D(240.f, 70.f)), "down on overlap should hit topmost B", true);
        EXPECT_TRUE(layout.DispatchMouseUp(Point2D(240.f, 70.f)), "up on overlap should complete click", true);
        EXPECT_TRUE(clickCountB == 1, "overlap click should fire on topmost B", true);
        EXPECT_TRUE(clickCountA == 1, "overlap click should NOT fire on A", true);

        // 6. hover:移入 A 的專屬區域(在 B 外)觸發,移出不再觸發
        EXPECT_TRUE(layout.DispatchMouseMove(Point2D(110.f, 55.f)), "move inside A-only region should hover", true);
        EXPECT_TRUE(hoverCountA == 1, "hover inside should fire once", true);
        EXPECT_TRUE(!layout.DispatchMouseMove(Point2D(10.f, 10.f)), "move outside should not hover", true);
        EXPECT_TRUE(hoverCountA == 1, "hover count unchanged outside", true);

        SUCCESS_MESSAGE(GetName());
        return true;
    }
};

} // namespace guitest

#endif // SYSTEM_GUI_TEST_HPP
