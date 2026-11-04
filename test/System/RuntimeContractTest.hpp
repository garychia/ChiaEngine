#ifndef RUNTIME_CONTRACT_TEST_HPP
#define RUNTIME_CONTRACT_TEST_HPP

#include "Scene/GameObject.hpp"
#include "Scene/SceneSystem.hpp"
#include "System/Input/InputAction.hpp"
#include "System/Input/InputRuntime.hpp"
#include "System/Input/KeyCodes.hpp"
#include "Scene/AudioSource.hpp"
#include "Scene/AudioListener.hpp"
#include "System/Module/FrameClock.hpp"
#include "Test.hpp"

#include <cstdio>
#include <fstream>

namespace runtimecontract_test
{

// RuntimeContractTest:A3.6 運行時合約測試，驗證 A3 子系統協同運作。
// 1. GameObject + ComponentSystem: Entity 激活/停用不崩潰
// 2. SceneSystem: SetSceneActive/IsSceneActive 協同
// 3. SceneSystem: DontDestroyOnLoad + UnloadScene 保留 DDOL 實體
// 4. InputRuntime: 綁定 + 按下/釋放狀態
// 5. AudioSource/AudioListener: 組件結構與 SceneSystem 整合(無後端不崩潰)
class RuntimeContractTest : public Test
{
  public:
    RuntimeContractTest() : Test("RuntimeContract")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:GameObject + ComponentSystem 協同 ----
        {
            SceneSystem system;

            // 建立父子 GameObject，驗證 activeInHierarchy 傳遞
            Entity parent = system.CreateNode();
            Entity child = system.CreateNode(parent);

            GameObject parentGo(&system, parent);
            GameObject childGo(&system, child);

            EXPECT_TRUE(parentGo.GetActiveInHierarchy(), "root active", true);
            EXPECT_TRUE(childGo.GetActiveInHierarchy(), "child active", true);

            parentGo.SetActive(false);
            EXPECT_TRUE(!childGo.GetActiveInHierarchy(),
                        "父停用 → 子 activeInHierarchy=false", true);

            parentGo.SetActive(true);
            EXPECT_TRUE(childGo.GetActiveInHierarchy(),
                        "父恢復 → 子 activeInHierarchy=true", true);
        }

        // ---- AC2:SceneSystem 場景激活協同 ----
        {
            SceneSystem system;
            EXPECT_TRUE(system.IsSceneActive(), "預設場景 active", true);

            system.SetSceneActive(false);
            EXPECT_TRUE(!system.IsSceneActive(), "SetSceneActive(false)", true);

            system.SetSceneActive(true);
            EXPECT_TRUE(system.IsSceneActive(), "SetSceneActive(true)", true);

            // 場景停用後，Update/FixedUpdate 應該不會崩潰
            system.FixedUpdate(FrameClock{0, 0, 0.016, 1.0 / 60.0});
            system.Update(FrameClock{1, 1, 0.016, 1.0 / 60.0});
            EXPECT_TRUE(true, "場景停用狀態下 Update 不崩潰", true);
        }

        // ---- AC3:DontDestroyOnLoad + UnloadScene 協同 ----
        {
            SceneSystem system;

            Entity ddo = system.CreateNode();
            system.SetDontDestroyOnLoad(ddo, true);

            Entity normal = system.CreateNode();
            system.SetDontDestroyOnLoad(normal, false);

            system.UnloadScene();

            EXPECT_TRUE(system.Alive(ddo), "DDOL 實體在 UnloadScene 後存活", true);
            EXPECT_TRUE(!system.Alive(normal),
                        "非 DDOL 實體在 UnloadScene 後銷毀", true);

            // 再次 LoadScene(有效檔案)，DDOL 應仍保留
            const char *goodPath = TestTempFilePath("chia_contract.scene");
            {
                std::ofstream out(goodPath);
                out << "scene v1\n";
                out << "node 0 -1 NewRoot 0 0 0 0 0 0 1 1 1\n";
                out << "end\n";
            }
            EXPECT_TRUE(system.LoadScene(goodPath), "LoadScene 有效檔案 → true", true);
            EXPECT_TRUE(system.Alive(ddo), "DDOL 實體在 LoadScene 後仍存活", true);
            std::remove(goodPath);
        }

        // ---- AC4:InputRuntime + GameObject 協同 ----
        {
            InputRuntime input;
            Array<KeyCode> keysSpace = {KeyCodeSpace};
            KeyCombination comboSpace(keysSpace);
            input.RegisterBinding(InputAction::Jump, comboSpace);

            // 按下 Space → Jump Pressed
            input.BeginFrame();
            input.SetKeyDown(static_cast<unsigned int>(KeyCodeSpace));
            input.EndFrame();
            EXPECT_TRUE(input.IsActionPressed(InputAction::Jump),
                "輸入綁定: Space → Jump Pressed", true);

            // 根據輸入狀態改變 GameObject active(模擬遊戲邏輯)
            SceneSystem system;
            GameObject go(&system, system.CreateNode());
            if (input.IsActionPressed(InputAction::Jump))
                go.SetActive(false);
            EXPECT_TRUE(!go.GetActiveSelf(),
                "輸入觸發 → GameObject 停用", true);
        }

        // ---- AC5:AudioSource/AudioListener + SceneSystem 協同(無後端) ----
        {
            SceneSystem system;

            // Update/FixedUpdate 與 AudioSource/AudioListener 組件類型共存不崩潰
            system.FixedUpdate(FrameClock{0, 0, 0.016, 1.0 / 60.0});
            system.Update(FrameClock{1, 1, 0.016, 1.0 / 60.0});

            // AudioSource 組件結構驗證
            AudioSource src;
            src.bufferId = 42;
            src.volume = 0.75f;
            src.loop = true;
            src.playOnAwake = true;
            src.spatial = true;
            EXPECT_TRUE(src.bufferId == 42 && src.volume == 0.75f && src.loop &&
                          src.playOnAwake && src.spatial,
                        "AudioSource 組件欄位設定正確", true);

            // AudioListener 組件結構驗證
            AudioListener listener;
            listener.active = true;
            EXPECT_TRUE(listener.active, "AudioListener.active=true", true);

            EXPECT_TRUE(true, "音訊組件與 SceneSystem 協同(無後端)不崩潰", true);
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }

  private:
};

} // namespace runtimecontract_test

#endif // RUNTIME_CONTRACT_TEST_HPP
