#ifndef GAME_OBJECT_HPP
#define GAME_OBJECT_HPP

#include "Data/DynamicArray.hpp"
#include "Data/String.hpp"
#include "Scene/SceneSystem.hpp"
#include "System/World/Entity.hpp"

// GameObject:Entity 的運行時包裝，提供 activeSelf/activeInHierarchy、
// Instantiate/Destroy/CompareTag/GetComponent 等運行時 API。
// GameObject 不擁有 SceneSystem（由 App/EditorSession 注入），
// 只持有 SceneSystem* 指標與 Entity handle。
class GameObject
{
  public:
    GameObject() : mpSceneSystem(nullptr), mEntity(Entity::FromRaw(0)), mActiveSelf(false), mTag() {}
    GameObject(SceneSystem *pSceneSystem, Entity entity)
        : mpSceneSystem(pSceneSystem), mEntity(entity),
          mActiveSelf(pSceneSystem ? pSceneSystem->IsActive(entity) : false),
          mTag()
    {
    }

    // 設定自身激活狀態(不影響子節點)。activeInHierarchy = activeSelf &&
    // 所有祖先皆 active。
    void SetActive(bool active)
    {
        if (mpSceneSystem)
            mpSceneSystem->SetActive(mEntity, active);
        mActiveSelf = active;
    }

    // 自身激活狀態。
    bool GetActiveSelf() const { return mActiveSelf; }

    // 在場景圖中的實際激活狀態(自身 + 所有祖先皆 active 才為 true)。
    bool GetActiveInHierarchy() const
    {
        if (!mpSceneSystem || !mActiveSelf)
            return false;
        if (!mpSceneSystem->Alive(mEntity))
            return false;

        // 向上走訪 ParentComponent，檢查每個祖先的 active。
        Entity e = mEntity;
        while (mpSceneSystem->Alive(e))
        {
            if (!mpSceneSystem->IsActive(e))
                return false;
            Entity parent = mpSceneSystem->GetParent(e);
            if (!mpSceneSystem->Alive(parent))
                break; // 根節點，停止
            e = parent;
        }
        return true;
    }

    // 建立此 GameObject 的完整副本(含 Transform/Material/Light 等元件)。
    // 回傳新 GameObject，或 nullptr(若原實體已失效)。
    GameObject *Instantiate()
    {
        if (!mpSceneSystem || !mpSceneSystem->Alive(mEntity))
            return nullptr;

        Entity parent = mpSceneSystem->GetParent(mEntity);
        Entity clone = mpSceneSystem->CreateNode(parent);
        CopyComponentsTo(clone);

        GameObject *pClone = new GameObject(mpSceneSystem, clone);
        pClone->mTag = mTag; // 複製標籤
        pClone->mActiveSelf = mActiveSelf; // 複製自身激活狀態
        return pClone;
    }

    // 標記此實體銷毀(先停用，實際由 SceneSystem/World 在幀末移除)。
    void Destroy()
    {
        if (mpSceneSystem)
            mpSceneSystem->SetActive(mEntity, false);
        mActiveSelf = false;
    }

    // 查詢標籤。
    bool CompareTag(const String &tag) const { return mTag == tag; }

    // 設定標籤。
    void SetTag(const String &tag) { mTag = tag; }

    // 取得標籤。
    const String &GetTag() const { return mTag; }

    // 取得元件(模板，委派給 SceneSystem/World)。
    template <class T> T *GetComponent()
    {
        if (!mpSceneSystem)
            return nullptr;
        return mpSceneSystem->GetComponent<T>(mEntity);
    }

    template <class T> const T *GetComponent() const
    {
        if (!mpSceneSystem)
            return nullptr;
        return mpSceneSystem->GetComponent<T>(mEntity);
    }

    // 取得關聯的 SceneSystem。
    SceneSystem *GetSceneSystem() const { return mpSceneSystem; }

    // 取得 Entity handle。
    Entity GetEntity() const { return mEntity; }

    // 是否有效(實體存活)。
    bool IsValid() const { return mpSceneSystem && mpSceneSystem->Alive(mEntity); }

  private:
    // 將已知元件類型複製到目標實體。
    void CopyComponentsTo(Entity dst)
    {
        if (!mpSceneSystem)
            return;

        // MaterialComponent
        if (const MaterialComponent *pM = GetComponent<MaterialComponent>())
            mpSceneSystem->SetMaterial(dst.GetIndex(), pM->materialId);
        // LightComponent
        if (const LightComponent *pL = GetComponent<LightComponent>())
            mpSceneSystem->SetLight(dst.GetIndex(), *pL);
        // TransformComponent:CreateNode 已建立預設，這裡覆寫
        if (const TransformComponent *pT = GetComponent<TransformComponent>())
        {
            if (TransformComponent *pDstT = mpSceneSystem->GetComponent<TransformComponent>(dst))
                *pDstT = *pT;
        }
    }

    SceneSystem *mpSceneSystem;
    Entity mEntity;
    bool mActiveSelf;
    String mTag;
};

#endif // GAME_OBJECT_HPP
