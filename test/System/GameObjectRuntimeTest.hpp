#ifndef GAME_OBJECT_RUNTIME_TEST_HPP
#define GAME_OBJECT_RUNTIME_TEST_HPP

#include "Scene/GameObject.hpp"
#include "Scene/LightComponent.hpp"
#include "Scene/MaterialComponent.hpp"
#include "Scene/SceneSystem.hpp"
#include "Test.hpp"

#include <cstdio>

namespace gameobjectruntime_test
{

// GameObjectRuntimeTest:A3.1 GameObject 執行時契約。
// 1. SetActive/GetActive 往返
// 2. activeInHierarchy 隨父節點變更(父停用 → 子也視為停用)
// 3. Instantiate 建立完整副本(含 Transform/Material/Light 元件)
// 4. Destroy 標記實體為停用
// 5. CompareTag 查詢正確
class GameObjectRuntimeTest : public Test
{
  public:
    GameObjectRuntimeTest() : Test("GameObjectRuntime")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:SetActive/GetActive 往返 ----
        {
            SceneSystem system;
            GameObject go(&system, system.CreateNode());

            EXPECT_TRUE(go.GetActiveSelf(), "預設 activeSelf=true", true);
            EXPECT_TRUE(go.GetActiveInHierarchy(), "預設 activeInHierarchy=true", true);

            go.SetActive(false);
            EXPECT_TRUE(!go.GetActiveSelf(), "SetActive(false) → activeSelf=false", true);
            EXPECT_TRUE(!go.GetActiveInHierarchy(),
                        "SetActive(false) → activeInHierarchy=false", true);

            go.SetActive(true);
            EXPECT_TRUE(go.GetActiveSelf(), "SetActive(true) → activeSelf=true", true);
            EXPECT_TRUE(go.GetActiveInHierarchy(),
                        "SetActive(true) → activeInHierarchy=true", true);
        }

        // ---- AC2:activeInHierarchy 隨父節點變更 ----
        {
            SceneSystem system;

            Entity parent = system.CreateNode();
            Entity child = system.CreateNode(parent);

            GameObject parentGo(&system, parent);
            GameObject childGo(&system, child);

            EXPECT_TRUE(childGo.GetActiveInHierarchy(),
                        "父子皆 active → child activeInHierarchy=true", true);

            parentGo.SetActive(false);
            EXPECT_TRUE(childGo.GetActiveSelf(),
                        "父停用後 child activeSelf 不變(仍 true)", true);
            EXPECT_TRUE(!childGo.GetActiveInHierarchy(),
                        "父停用後 child activeInHierarchy=false", true);

            parentGo.SetActive(true);
            EXPECT_TRUE(childGo.GetActiveInHierarchy(),
                        "父恢復後 child activeInHierarchy=true", true);
        }

        // ---- AC3:Instantiate 建立完整副本 ----
        {
            SceneSystem system;
            GameObject go(&system, system.CreateNode());

            // 設定 Material + Light
            system.SetMaterial(go.GetEntity().GetIndex(), 42);
            LightComponent light;
            light.type = LightComponent::Point;
            light.color = Point3D(1, 0, 0);
            system.SetLight(go.GetEntity().GetIndex(), light);
            go.SetTag("Player");

            GameObject *clone = go.Instantiate();
            EXPECT_TRUE(clone != nullptr, "Instantiate 回傳非 null", true);
            if (clone)
            {
                EXPECT_TRUE(clone->IsValid(), "複製體有效", true);
                EXPECT_TRUE(clone->GetEntity() != go.GetEntity(),
                            "複製體 Entity 不同於原件", true);

                // Material 複製
                uint64_t matId = 0;
                system.GetMaterial(clone->GetEntity().GetIndex(), matId);
                EXPECT_TRUE(matId == 42, "Instantiate 複製 MaterialComponent", true);

                // Light 複製
                LightComponent clonedLight;
                system.GetLight(clone->GetEntity().GetIndex(), clonedLight);
                EXPECT_TRUE(clonedLight.type == LightComponent::Point,
                            "Instantiate 複製 LightComponent.type", true);
                EXPECT_TRUE(clonedLight.color.x == 1.f && clonedLight.color.y == 0.f &&
                              clonedLight.color.z == 0.f,
                            "Instantiate 複製 LightComponent.color", true);

                // Tag 複製(由 GameObject::mTag 複製)
                EXPECT_TRUE(clone->CompareTag("Player"), "Instantiate 複製 Tag", true);

                // 父層級保持(原件無父 → 複製體也無父)
                EXPECT_TRUE(system.GetParent(clone->GetEntity()).GetRaw() == 0,
                            "Instantiate 保持無父層級", true);

                delete clone;
            }
        }

        // ---- AC4:Destroy 標記實體為停用 ----
        {
            SceneSystem system;
            GameObject go(&system, system.CreateNode());

            EXPECT_TRUE(go.GetActiveSelf(), "Destroy 前 activeSelf=true", true);

            go.Destroy();
            EXPECT_TRUE(!go.GetActiveSelf(), "Destroy 後 activeSelf=false", true);
            EXPECT_TRUE(!system.IsActive(go.GetEntity()),
                        "SceneSystem.IsActive 也為 false", true);
        }

        // ---- AC5:CompareTag 查詢正確 ----
        {
            SceneSystem system;
            GameObject go(&system, system.CreateNode());

            go.SetTag("Enemy");
            EXPECT_TRUE(go.CompareTag("Enemy"), "CompareTag(Enemy) = true", true);
            EXPECT_TRUE(!go.CompareTag("Player"), "CompareTag(Player) = false", true);
            EXPECT_TRUE(!go.CompareTag(""), "CompareTag(空) = false", true);
        }

        // ---- AC6:GetComponent<T>() 模板查詢 ----
        {
            SceneSystem system;
            GameObject go(&system, system.CreateNode());

            system.SetMaterial(go.GetEntity().GetIndex(), 7);
            uint64_t *pMat = nullptr;
            (void)pMat;
            // 透過 GetComponent<MaterialComponent>() 取得並檢查 materialId
            const MaterialComponent *pM = go.GetComponent<MaterialComponent>();
            EXPECT_TRUE(pM != nullptr, "GetComponent<MaterialComponent>() 非 null", true);
            if (pM)
                EXPECT_TRUE(pM->materialId == 7, "MaterialComponent.materialId=7", true);
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }

  private:
};

} // namespace gameobjectruntime_test

#endif // GAME_OBJECT_RUNTIME_TEST_HPP
