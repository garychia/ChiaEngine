#ifndef PHYSICS_OVERLAP_TEST_HPP
#define PHYSICS_OVERLAP_TEST_HPP

#include "Test.hpp"
#include "Physics/AABBCollider.hpp"
#include "Physics/SphereCollider.hpp"
#include "Physics/Overlap.hpp"

// 純函式 overlap 預測式測試(無引擎、無 World)。
// 驗證 AC1:AABB-AABB / sphere-sphere / AABB-sphere 三種配對的
// 分離 / 相切 / 重疊三種情境(相切含邊界,計入 overlap)。
namespace overlap_test
{

class PhysicsOverlapTest : public Test
{
  public:
    PhysicsOverlapTest() : Test("PhysicsOverlap")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("AABB-AABB separating");
        {
            // X 軸分離:dx = 4 > hx1+hx2 = 3
            EXPECT_TRUE(!Overlaps(AABBCollider{Point3D(0, 0, 0), Point3D(1, 1, 1)},
                                  AABBCollider{Point3D(4, 0, 0), Point3D(2, 1, 1)}),
                        "X 方向分離 → 不重疊.", true);
            // Y 軸分離
            EXPECT_TRUE(!Overlaps(AABBCollider{Point3D(0, 0, 0), Point3D(1, 1, 1)},
                                  AABBCollider{Point3D(0, 5, 0), Point3D(1, 0.5f, 1)}),
                        "Y 方向分離 → 不重疊.", true);
            // Z 軸分離
            EXPECT_TRUE(!Overlaps(AABBCollider{Point3D(0, 0, 0), Point3D(1, 1, 1)},
                                  AABBCollider{Point3D(0, 0, -8), Point3D(1, 1, 1)}),
                        "Z 方向分離 → 不重疊.", true);
        }

        TEST_MESSAGE("AABB-AABB touching(邊界包含)");
        {
            // 邊對邊相切:dx = 3 == hx1+hx2 → 算 overlap
            EXPECT_TRUE(Overlaps(AABBCollider{Point3D(0, 0, 0), Point3D(1, 1, 1)},
                                 AABBCollider{Point3D(3, 0, 0), Point3D(2, 1, 1)}),
                        "X 相切(含邊界) → 重疊.", true);
            // 面對面相切
            EXPECT_TRUE(Overlaps(AABBCollider{Point3D(0, 0, 0), Point3D(1, 1, 1)},
                                 AABBCollider{Point3D(0, 2, 0), Point3D(1, 1, 1)}),
                        "Y 相切(含邊界) → 重疊.", true);
        }

        TEST_MESSAGE("AABB-AABB overlapping");
        {
            // 中心重疊(inside)
            EXPECT_TRUE(Overlaps(AABBCollider{Point3D(0, 0, 0), Point3D(1, 1, 1)},
                                 AABBCollider{Point3D(0, 0, 0), Point3D(2, 2, 2)}),
                        "包含 → 重疊.", true);
            // 部分重疊
            EXPECT_TRUE(Overlaps(AABBCollider{Point3D(0, 0, 0), Point3D(1, 1, 1)},
                                 AABBCollider{Point3D(1.5f, 0, 0), Point3D(1, 1, 1)}),
                        "部分重疊 → 重疊.", true);
        }

        TEST_MESSAGE("Sphere-sphere separating");
        {
            // 距離 6 > r1+r2 = 5 → 分離
            EXPECT_TRUE(!Overlaps(SphereCollider{Point3D(0, 0, 0), 2.0f},
                                  SphereCollider{Point3D(6, 0, 0), 3.0f}),
                        "分離 → 不重疊.", true);
        }

        TEST_MESSAGE("Sphere-sphere touching(邊界包含)");
        {
            // 距離 5 == r1+r2 → 相切,算 overlap
            EXPECT_TRUE(Overlaps(SphereCollider{Point3D(0, 0, 0), 2.0f},
                                 SphereCollider{Point3D(5, 0, 0), 3.0f}),
                        "相切(含邊界) → 重疊.", true);
        }

        TEST_MESSAGE("Sphere-sphere overlapping");
        {
            // 距離 3 < r1+r2 = 5
            EXPECT_TRUE(Overlaps(SphereCollider{Point3D(0, 0, 0), 2.0f},
                                 SphereCollider{Point3D(3, 0, 0), 3.0f}),
                        "重疊 → 重疊.", true);
            // 同心(包含)
            EXPECT_TRUE(Overlaps(SphereCollider{Point3D(0, 0, 0), 1.0f},
                                 SphereCollider{Point3D(0, 0, 0), 5.0f}),
                        "同心包含 → 重疊.", true);
        }

        TEST_MESSAGE("AABB-sphere separating");
        {
            // 球心 (5,0,0),在盒邊外 3 單位,r=2 → 距離 3 > 2
            EXPECT_TRUE(!Overlaps(AABBCollider{Point3D(0, 0, 0), Point3D(1, 1, 1)},
                                  SphereCollider{Point3D(5, 0, 0), 2.0f}),
                        "球在盒邊外 → 不重疊.", true);
        }

        TEST_MESSAGE("AABB-sphere touching(邊界包含)");
        {
            // 球表面剛好切在盒面:球心 (3,0,0),盒 half=1,dx=2==r → 相切
            EXPECT_TRUE(Overlaps(AABBCollider{Point3D(0, 0, 0), Point3D(1, 1, 1)},
                                 SphereCollider{Point3D(3, 0, 0), 2.0f}),
                        "相切(含邊界) → 重疊.", true);
        }

        TEST_MESSAGE("AABB-sphere overlapping");
        {
            // 一般:球心 (2,0.5,0) 距盒面 1, r=1.5 → 重疊
            EXPECT_TRUE(Overlaps(AABBCollider{Point3D(0, 0, 0), Point3D(1, 1, 1)},
                                 SphereCollider{Point3D(2, 0.5f, 0), 1.5f}),
                        "部分重疊 → 重疊.", true);
            // 球心在盒內 → 包含
            EXPECT_TRUE(Overlaps(AABBCollider{Point3D(0, 0, 0), Point3D(1, 1, 1)},
                                 SphereCollider{Point3D(0, 0, 0), 3.0f}),
                        "球心在盒內 → 重疊.", true);

            // 對稱性:參數反過來(AABB-sphere ≅ sphere-AABB)
            EXPECT_TRUE(Overlaps(SphereCollider{Point3D(2, 0.5f, 0), 1.5f},
                                 AABBCollider{Point3D(0, 0, 0), Point3D(1, 1, 1)}),
                        "sphere-AABB 對稱 → 重疊.", true);
        }

        SUCCESS_MESSAGE("PhysicsOverlap");
        return true;
    }
};

} // namespace overlap_test

#endif // PHYSICS_OVERLAP_TEST_HPP
