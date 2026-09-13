#ifndef SCENE_SYSTEM_HPP
#define SCENE_SYSTEM_HPP

#include "Data/DynamicArray.hpp"
#include "Scene/ParentComponent.hpp"
#include "Scene/TransformComponent.hpp"
#include "Scene/TransformMath.hpp"
#include "Scene/WorldTransformComponent.hpp"
#include "System/Module/EngineContext.hpp"
#include "System/Module/IModule.hpp"
#include "System/Operation/Event.hpp"
#include "System/World/Entity.hpp"
#include "System/World/World.hpp"

// 場景圖模組(Sim 層,確定性,無 GPU,可無頭測試)。
//
// 設計(RFC P7b):
// - 階層只存 parent(D1-A):children / roots 一律由掃描 ParentComponent pool 推導,
//   reparent / destroy 後 child 清單恆正確,不與 World 狀態漂移。
// - world transform 分解 TRS(D2-A):FixedUpdate 依確定性 pre-order 全樹重算,
//   root 的 world = local;child 的 world = Compose(parentWorld, local)。
// - 確定遍歷:roots 依 entity index 升序,children 依 entity index 升序。
// - DestroyNode recursive(連同子孫),實作先 DFS 蒐集再由淺到深摧毀。
//
// 與 Display::Scene 的區別:SceneSystem 是 Sim 側場景(ECS + 階層 + transform);
// Display::Scene 是 View 側 renderable 列表。Sim 不觸碰 View 型別。
class SceneSystem : public IModule
{
  public:
    World world;

    // 建立節點:掛 TransformComponent + ParentComponent(root 的 parent = 空)。
    Entity CreateNode(Entity parent = Entity())
    {
        Entity e = world.CreateEntity();
        world.AddComponent<TransformComponent>(e, TransformComponent());
        ParentComponent p;
        p.parent = parent;
        world.AddComponent<ParentComponent>(e, p);
        return e;
    }

    void SetParent(Entity e, Entity parent)
    {
        if (!world.Alive(e))
            return;
        ParentComponent *p = world.GetComponent<ParentComponent>(e);
        if (p)
            p->parent = parent;
    }

    void ClearParent(Entity e)
    {
        SetParent(e, Entity());
    }

    // ---- 編輯器 seam(#82)----
    // View(InspectorButton/InspectorLayer/SceneWindow)只能經由這些操作存取場景圖,
    // 不直接碰 world field(層規則只擋 Sim→Display,補上 View→Sim 的窄縫)。
    // editor 的 Selection 存 entity index(不用 handle),故 seam 以 index 為參數,
    // 存活/存在性檢查一律在 Sim 內做,View 不需也無法摸元件池。

    // 讀取 local transform(index + 存活檢查);無效回 false。
    // (World::GetComponent 無 const 重載,故本方法非 const — 與 GetChildren 一致。)
    bool GetTransform(uint32_t entityIndex, TransformComponent &out)
    {
        Entity e = world.GetEntityByIndex(entityIndex);
        if (!world.Alive(e))
            return false;
        TransformComponent *pT = world.GetComponent<TransformComponent>(e);
        if (!pT)
            return false;
        out = *pT;
        return true;
    }

    // 編輯器增量編輯:local TRS += delta(axis 對應的 delta 由 View 端組好,
    // Sim 不認識 InspectorAxis)。entity 不存在或無元件則 no-op。
    void EditTransform(uint32_t entityIndex, const TransformComponent &delta)
    {
        Entity e = world.GetEntityByIndex(entityIndex);
        if (!world.Alive(e))
            return;
        TransformComponent *pT = world.GetComponent<TransformComponent>(e);
        if (!pT)
            return;
        pT->position += delta.position;
        pT->rotation += delta.rotation;
        pT->scale += delta.scale;
    }

    // #60 step 1 demo 階層整批建立(#82:SceneWindow 不再自行拼裝 scene 內容)。
    // root → childA + childB;childA → grandchild(4 節點,供 hierarchy 側欄展示)。
    void CreateEditorDemoHierarchy()
    {
        Entity root = CreateNode();
        Entity childA = CreateNode(root);
        CreateNode(root);
        CreateNode(childA);
    }

    // children 依 entity index 升序(掃描 pool,parent == e)。
    DynamicArray<Entity> GetChildren(Entity e) const
    {
        DynamicArray<Entity> children;
        ComponentPool<ParentComponent> *pPool = world.GetPool<ParentComponent>();
        if (!pPool)
            return children;
        const uint32_t n = pPool->GetNElements();
        for (uint32_t i = 0; i < n; i++)
        {
            const uint32_t owner = pPool->OwnerAt(i);
            const ParentComponent *p = pPool->Get(owner);
            if (p && p->parent == e)
                children.Append(world.GetEntityByIndex(owner));
        }
        // selection sort:entity index 升序(確定性 key)
        for (size_t i = 0; i + 1 < children.GetNElements(); i++)
        {
            size_t best = i;
            for (size_t j = i + 1; j < children.GetNElements(); j++)
                if (children[j].GetIndex() < children[best].GetIndex())
                    best = j;
            if (best != i)
            {
                Entity tmp = children[i];
                children[i] = children[best];
                children[best] = tmp;
            }
        }
        return children;
    }

    // 根節點:parent.raw == 0 或 parent 已死(視為 root),依 index 升序。
    DynamicArray<Entity> GetRoots() const
    {
        DynamicArray<Entity> roots;
        ComponentPool<ParentComponent> *pPool = world.GetPool<ParentComponent>();
        if (!pPool)
            return roots;
        const uint32_t n = pPool->GetNElements();
        for (uint32_t i = 0; i < n; i++)
        {
            const uint32_t owner = pPool->OwnerAt(i);
            const ParentComponent *p = pPool->Get(owner);
            if (!p)
                continue;
            const bool isRoot = (p->parent.GetRaw() == 0) || !world.Alive(p->parent);
            if (isRoot)
                roots.Append(world.GetEntityByIndex(owner));
        }
        // index 升序
        for (size_t i = 0; i + 1 < roots.GetNElements(); i++)
        {
            size_t best = i;
            for (size_t j = i + 1; j < roots.GetNElements(); j++)
                if (roots[j].GetIndex() < roots[best].GetIndex())
                    best = j;
            if (best != i)
            {
                Entity tmp = roots[i];
                roots[i] = roots[best];
                roots[best] = tmp;
            }
        }
        return roots;
    }

    // 確定性 pre-order DFS:roots 依 index、children 依 index。
    void Traverse(Callback<void(Entity)> &callback)
    {
        DynamicArray<Entity> roots = GetRoots();
        for (size_t i = 0; i < roots.GetNElements(); i++)
            TraverseSubtree(roots[i], callback);
    }

    // pre-order 扁平化(editor #60 step 1):out[i] = 節點、depths[i] = 深度
    // (root = 0)。順序與 Traverse 相同 — roots 依 index、children 依 index,
    // 確定性,可無頭測試。
    void GetHierarchy(DynamicArray<Entity> &out, DynamicArray<uint32_t> &depths) const
    {
        DynamicArray<Entity> roots = GetRoots();
        for (size_t i = 0; i < roots.GetNElements(); i++)
            GetHierarchySubtree(roots[i], 0, out, depths);
    }

    // recursive destroy:先 DFS 蒐集 descendants,再由淺到深摧毀。
    void DestroyNode(Entity e)
    {
        if (!world.Alive(e))
            return;
        DynamicArray<Entity> toDestroy;
        CollectSubtree(e, toDestroy);
        // 由淺到深:reverse(DFS 結果的最後是子孫)→ 先摧毀最深的
        // DFS 蒐集順序是 pre-order(祖先在前),reverse 後子孫在前 → 安全。
        for (size_t i = toDestroy.GetNElements(); i-- > 0;)
            world.DestroyEntity(toDestroy[i]);
    }

    void OnAttach(EngineContext &context) override
    {
        context.RegisterService<SceneSystem>(this);
    }

    void OnDetach(EngineContext &context) override
    {
        (void)context;
    }

    // 每 tick 全樹重算 world transform(不須 dirty-flag:任何遊戲系統改 local 立即反映)。
    void FixedUpdate(const FrameClock &clock) override
    {
        (void)clock;
        DynamicArray<Entity> roots = GetRoots();
        for (size_t i = 0; i < roots.GetNElements(); i++)
            RecomputeSubtree(roots[i], true, WorldTransformComponent());
    }

    void Update(const FrameClock &clock) override
    {
        (void)clock;
    }

  private:
    void TraverseSubtree(Entity e, Callback<void(Entity)> &callback)
    {
        callback(e);
        DynamicArray<Entity> children = GetChildren(e);
        for (size_t i = 0; i < children.GetNElements(); i++)
            TraverseSubtree(children[i], callback);
    }

    void GetHierarchySubtree(Entity e, uint32_t depth, DynamicArray<Entity> &out,
                             DynamicArray<uint32_t> &depths) const
    {
        out.Append(e);
        depths.Append(depth);
        DynamicArray<Entity> children = GetChildren(e);
        for (size_t i = 0; i < children.GetNElements(); i++)
            GetHierarchySubtree(children[i], depth + 1, out, depths);
    }

    void CollectSubtree(Entity e, DynamicArray<Entity> &out) const
    {
        out.Append(e);
        DynamicArray<Entity> children = GetChildren(e);
        for (size_t i = 0; i < children.GetNElements(); i++)
            CollectSubtree(children[i], out);
    }

    void RecomputeSubtree(Entity e, bool isRoot, const WorldTransformComponent &parentWorld)
    {
        TransformComponent *pLocal = world.GetComponent<TransformComponent>(e);
        if (!pLocal)
            return;
        WorldTransformComponent w;
        if (isRoot)
        {
            w.position = pLocal->position;
            w.rotation = pLocal->rotation;
            w.scale = pLocal->scale;
        }
        else
        {
            w = TransformMath::Compose(parentWorld, *pLocal);
        }
        world.AddComponent<WorldTransformComponent>(e, w); // AddComponent 冪等:已存在則覆寫

        DynamicArray<Entity> children = GetChildren(e);
        for (size_t i = 0; i < children.GetNElements(); i++)
            RecomputeSubtree(children[i], false, w);
    }
};

#endif // SCENE_SYSTEM_HPP