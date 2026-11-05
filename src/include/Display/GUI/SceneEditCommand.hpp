#ifndef SCENE_EDIT_COMMAND_HPP
#define SCENE_EDIT_COMMAND_HPP

#include "Data/String.hpp"
#include "Display/GUI/EditorSession.hpp" // EditorCommand
#include "Scene/SceneSystem.hpp"
#include "Scene/TransformComponent.hpp"

// ADR-0001 D5:場景編輯器的 Undo/Redo 命令。每個命令自包含足夠狀態，
// Apply/Revert 互為逆運算，無需快照整個 World。命令執行後由 UndoStack
// 持有所有權(EditorCommand*)，Clear/dtor 時刪除。全部 headless 可測試。

// ===== CreateNodeCommand =====
// Apply: SceneSystem::CreateNode(parent) → 新節點(掛 Transform/Parent/NodeName/Material)
// Revert: SceneSystem::DestroyNode(entity) → 遞迴刪除該節點及子樹
class CreateNodeCommand : public EditorCommand
{
  public:
    CreateNodeCommand(SceneSystem *pScene, Entity parent = Entity())
        : mpScene(pScene), mParent(parent), mEntityIndex(0), mCreated(false)
    {
    }

    void Apply() override
    {
        if (!mpScene)
            return;
        Entity e = mpScene->CreateNode(mParent);
        mEntityIndex = e.GetIndex();
        mCreated = true;
    }

    void Revert() override
    {
        if (!mpScene || !mCreated)
            return;
        Entity e = mpScene->GetEntityByIndex(mEntityIndex);
        if (mpScene->Alive(e))
            mpScene->DestroyNode(e);
        mCreated = false;
    }

    String Description() const override { return String(u"CreateNode"); }

    uint32_t GetCreatedEntityIndex() const { return mEntityIndex; }

  private:
    SceneSystem *mpScene;
    Entity mParent;
    uint32_t mEntityIndex;
    bool mCreated;
};

// ===== DestroyNodeCommand =====
// Apply: SceneSystem::DestroyNode(entity) → 遞迴刪除節點及子樹
// Revert: SceneSystem::CreateNode(parent) → 在原 parent 下重建節點
//          (新 entity index, 與 undo 後的重新查詢一致;場景 Undo/Redo 本質即如此)
// 需快照: entity index、parent(undo 時知道在哪裡重建)。
class DestroyNodeCommand : public EditorCommand
{
  public:
    DestroyNodeCommand(SceneSystem *pScene, Entity entity)
        : mpScene(pScene), mEntityIndex(entity.GetIndex()),
          mParent(Entity()), mExecuted(false)
    {
    }

    void Apply() override
    {
        if (!mpScene || mExecuted)
            return;
        Entity e = mpScene->GetEntityByIndex(mEntityIndex);
        if (!mpScene->Alive(e))
            return;
        mParent = mpScene->GetParent(e);
        mpScene->DestroyNode(e);
        mExecuted = true;
    }

    void Revert() override
    {
        if (!mpScene || !mExecuted)
            return;
        Entity e = mpScene->CreateNode(mParent);
        mEntityIndex = e.GetIndex(); // 重建後 index 可能不同,UndoStack 視為新節點
        mExecuted = false;
    }

    String Description() const override { return String(u"DestroyNode"); }

    uint32_t GetCreatedEntityIndex() const { return mEntityIndex; }

  private:
    SceneSystem *mpScene;
    uint32_t mEntityIndex;
    Entity mParent;
    bool mExecuted;
};

// ===== SetParentCommand =====
// Apply: SceneSystem::SetParent(entity, newParent)
// Revert: SceneSystem::SetParent(entity, oldParent)
// 首次 Apply 時捕捉當前 parent 作為 oldParent。
class SetParentCommand : public EditorCommand
{
  public:
    SetParentCommand(SceneSystem *pScene, Entity entity, Entity newParent)
        : mpScene(pScene), mEntity(entity), mNewParent(newParent),
          mOldParent(Entity()), mCaptured(false)
    {
    }

    void Apply() override
    {
        if (!mpScene)
            return;
        if (!mCaptured)
        {
            mOldParent = mpScene->GetParent(mEntity);
            mCaptured = true;
        }
        mpScene->SetParent(mEntity, mNewParent);
    }

    void Revert() override
    {
        if (!mpScene)
            return;
        mpScene->SetParent(mEntity, mOldParent);
    }

    String Description() const override { return String(u"SetParent"); }

  private:
    SceneSystem *mpScene;
    Entity mEntity;
    Entity mNewParent;
    Entity mOldParent;
    bool mCaptured;
};

// ===== SetMaterialCommand =====
// Apply: SceneSystem::SetMaterial(entityIndex, newMaterialId)
// Revert: SceneSystem::SetMaterial(entityIndex, oldMaterialId)
// 首次 Apply 時捕捉當前 materialId 作為 oldMaterialId。
class SetMaterialCommand : public EditorCommand
{
  public:
    SetMaterialCommand(SceneSystem *pScene, uint32_t entityIndex, uint64_t newMaterialId)
        : mpScene(pScene), mEntityIndex(entityIndex), mNewMaterialId(newMaterialId),
          mOldMaterialId(0), mCaptured(false)
    {
    }

    void Apply() override
    {
        if (!mpScene)
            return;
        if (!mCaptured)
        {
            mpScene->GetMaterial(mEntityIndex, mOldMaterialId);
            mCaptured = true;
        }
        mpScene->SetMaterial(mEntityIndex, mNewMaterialId);
    }

    void Revert() override
    {
        if (!mpScene)
            return;
        mpScene->SetMaterial(mEntityIndex, mOldMaterialId);
    }

    String Description() const override { return String(u"SetMaterial"); }

  private:
    SceneSystem *mpScene;
    uint32_t mEntityIndex;
    uint64_t mNewMaterialId;
    uint64_t mOldMaterialId;
    bool mCaptured;
};

// ===== SetPBRCommand ====
// Apply: SceneSystem::SetPBR(entityIndex, newAlbedo, newMetallic, newRoughness)
// Revert: SceneSystem::SetPBR(entityIndex, oldAlbedo, oldMetallic, oldRoughness)
// 首次 Apply 時捕捉當前 PBR 欄位作為 old 值。
class SetPBRCommand : public EditorCommand
{
  public:
    SetPBRCommand(SceneSystem *pScene, uint32_t entityIndex, const Point3D &newAlbedo,
                  float newMetallic, float newRoughness)
        : mpScene(pScene), mEntityIndex(entityIndex), mNewAlbedo(newAlbedo),
          mNewMetallic(newMetallic), mNewRoughness(newRoughness),
          mOldAlbedo(Point3D(1, 1, 1)), mOldMetallic(0.0f), mOldRoughness(1.0f),
          mCaptured(false)
    {
    }

    void Apply() override
    {
        if (!mpScene)
            return;
        if (!mCaptured)
        {
            mpScene->GetPBR(mEntityIndex, mOldAlbedo, mOldMetallic, mOldRoughness);
            mCaptured = true;
        }
        mpScene->SetPBR(mEntityIndex, mNewAlbedo, mNewMetallic, mNewRoughness);
    }

    void Revert() override
    {
        if (!mpScene)
            return;
        mpScene->SetPBR(mEntityIndex, mOldAlbedo, mOldMetallic, mOldRoughness);
    }

    String Description() const override { return String(u"SetPBR"); }

  private:
    SceneSystem *mpScene;
    uint32_t mEntityIndex;
    Point3D mNewAlbedo;
    float mNewMetallic;
    float mNewRoughness;
    Point3D mOldAlbedo;
    float mOldMetallic;
    float mOldRoughness;
    bool mCaptured;
};

// ===== SetLightCommand ====
// Apply: SceneSystem::SetLight(entityIndex, newLight)
// Revert: SceneSystem::SetLight(entityIndex, oldLight)
// 首次 Apply 時捕捉當前光源欄位作為 old 值。
class SetLightCommand : public EditorCommand
{
  public:
    SetLightCommand(SceneSystem *pScene, uint32_t entityIndex, const LightComponent &newLight)
        : mpScene(pScene), mEntityIndex(entityIndex), mNewLight(newLight),
          mOldLight(LightComponent(LightComponent::Directional)), mCaptured(false)
    {
    }

    void Apply() override
    {
        if (!mpScene)
            return;
        if (!mCaptured)
        {
            mpScene->GetLight(mEntityIndex, mOldLight);
            mCaptured = true;
        }
        mpScene->SetLight(mEntityIndex, mNewLight);
    }

    void Revert() override
    {
        if (!mpScene)
            return;
        mpScene->SetLight(mEntityIndex, mOldLight);
    }

    String Description() const override { return String(u"SetLight"); }

  private:
    SceneSystem *mpScene;
    uint32_t mEntityIndex;
    LightComponent mNewLight;
    LightComponent mOldLight;
    bool mCaptured;
};

#endif // SCENE_EDIT_COMMAND_HPP
