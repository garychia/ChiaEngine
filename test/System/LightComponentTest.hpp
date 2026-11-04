#ifndef LIGHT_COMPONENT_TEST_HPP
#define LIGHT_COMPONENT_TEST_HPP

#include "Data/String.hpp"
#include "Display/GUI/EditorSession.hpp"
#include "Scene/LightComponent.hpp"
#include "Scene/SceneSystem.hpp"
#include "Test.hpp"

#include <cstdio>
#include <fstream>

namespace lightcomponent_test
{

class LightComponentTest : public Test
{
  public:
    LightComponentTest() : Test("LightComponent")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE(GetName());

        // ---- AC1:LightComponent 預設值 ----
        {
            LightComponent light;
            EXPECT_TRUE(light.type == LightComponent::Directional,
                        "預設 type=Directional", true);
            EXPECT_TRUE(light.color.x == 1.f && light.color.y == 1.f && light.color.z == 1.f,
                        "預設 color=(1,1,1)", true);
            EXPECT_TRUE(light.intensity == 1.f, "預設 intensity=1", true);
            EXPECT_TRUE(light.radius == 10.f, "預設 radius=10", true);
        }

        // ---- AC2:Point Light 預設值 ----
        {
            LightComponent light(LightComponent::Point);
            EXPECT_TRUE(light.type == LightComponent::Point, "Point light type", true);
            EXPECT_TRUE(light.color.x == 1.f && light.color.y == 1.f && light.color.z == 1.f,
                        "Point light default color", true);
            EXPECT_TRUE(light.intensity == 1.f, "Point light default intensity", true);
            EXPECT_TRUE(light.radius == 10.f, "Point light default radius", true);
        }

        // ---- AC3:SceneSystem::SetLight/GetLight 往返 ----
        {
            SceneSystem system;
            Entity root = system.CreateNode();

            LightComponent light;
            light.type = LightComponent::Point;
            light.color = Point3D(1, 0, 0);
            light.intensity = 2.5f;
            light.radius = 50.0f;
            system.SetLight(root.GetIndex(), light);

            LightComponent outLight;
            EXPECT_TRUE(system.GetLight(root.GetIndex(), outLight),
                        "GetLight 回傳 true", true);
            EXPECT_TRUE(outLight.type == LightComponent::Point,
                        "type 往返正確", true);
            EXPECT_TRUE(outLight.color.x == 1.f && outLight.color.y == 0.f &&
                          outLight.color.z == 0.f,
                        "color 往返正確", true);
            EXPECT_TRUE(outLight.intensity == 2.5f, "intensity 往返正確", true);
            EXPECT_TRUE(outLight.radius == 50.0f, "radius 往返正確", true);
        }

        // ---- AC4:EditorSession::SetSelectedEntityLight 透過 UndoStack ----
        {
            SceneSystem system;
            EditorSession session(&system, nullptr, nullptr);

            Entity root = system.CreateNode();
            session.Select(root.GetIndex());

            LightComponent newLight;
            newLight.type = LightComponent::Directional;
            newLight.color = Point3D(0, 1, 0);
            newLight.intensity = 3.0f;
            newLight.radius = 0.0f;
            session.SetSelectedEntityLight(newLight);

            // UndoStack 應有一個 SetLightCommand
            EXPECT_TRUE(session.GetUndoStack().GetUndoCount() == 1,
                        "UndoStack 有 1 個 SetLightCommand", true);

            // undo 應該回復到預設值
            LightComponent outLight;
            session.GetUndoStack().Undo();
            EXPECT_TRUE(system.GetLight(root.GetIndex(), outLight),
                        "Undo 後 GetLight 回傳 true", true);
            EXPECT_TRUE(outLight.type == LightComponent::Directional,
                        "Undo 後 type=Directional", true);
            EXPECT_TRUE(outLight.color.x == 1.f && outLight.color.y == 1.f &&
                          outLight.color.z == 1.f,
                        "Undo 後 color=(1,1,1)", true);
            EXPECT_TRUE(outLight.intensity == 1.f, "Undo 後 intensity=1", true);
            EXPECT_TRUE(outLight.radius == 10.f, "Undo 後 radius=10", true);

            // redo 應該回到設定值
            session.GetUndoStack().Redo();
            EXPECT_TRUE(system.GetLight(root.GetIndex(), outLight),
                        "Redo 後 GetLight 回傳 true", true);
            EXPECT_TRUE(outLight.type == LightComponent::Directional,
                        "Redo 後 type=Directional", true);
            EXPECT_TRUE(outLight.color.x == 0.f && outLight.color.y == 1.f &&
                          outLight.color.z == 0.f,
                        "Redo 後 color=(0,1,0)", true);
            EXPECT_TRUE(outLight.intensity == 3.0f, "Redo 後 intensity=3", true);
            EXPECT_TRUE(outLight.radius == 0.0f, "Redo 後 radius=0", true);
        }

        SUCCESS_MESSAGE(GetName());
        return true;
    }

  private:
};

} // namespace lightcomponent_test

#endif // LIGHT_COMPONENT_TEST_HPP
