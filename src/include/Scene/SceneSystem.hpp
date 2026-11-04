#ifndef SCENE_SYSTEM_HPP
#define SCENE_SYSTEM_HPP

#include "Data/DynamicArray.hpp"
#include "Data/Padding.hpp"
#include "Data/String.hpp"
#include "Scene/ParentComponent.hpp"
#include "Scene/TransformComponent.hpp"
#include "Scene/TransformMath.hpp"
#include "Scene/WorldTransformComponent.hpp"
#include "Scene/MaterialComponent.hpp"
#include "Scene/LightComponent.hpp"
#include "System/Module/EngineContext.hpp"
#include "System/Module/IModule.hpp"
#include "System/Operation/Event.hpp"
#include "System/World/Entity.hpp"
#include "System/World/World.hpp"
#include "System/World/WorldHashRecord.hpp" // ReadLine/NextToken/ParseUInt32(嚴格單空格 token)

#include <charconv>
#include <cstdlib>   // std::strtof (from_chars for float 在 libc++/Apple Clang 上未實作)
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <string>
#include <string_view>

namespace scenesystem_detail
{
constexpr size_t NODE_NAME_CAPACITY = 64;

// 節點名稱跟著 entity 存進 World 的元件池,SaveScene/LoadScene 才能原樣帶回。
// 固定長度 char 陣列 → 無 padding、trivially copyable,可安全進 World::Hash()。
struct NodeName
{
    char text[NODE_NAME_CAPACITY] = {};
};
CHIA_PADDING_FREE(NodeName, &NodeName::text);

inline void SetNodeName(NodeName &name, std::string_view text) noexcept
{
    size_t length = text.size();
    if (length >= NODE_NAME_CAPACITY)
        length = NODE_NAME_CAPACITY - 1; // shortcut: 名稱上限 63 byte,超過就截斷
    std::memcpy(name.text, text.data(), length);
    name.text[length] = '\0';
}

inline bool ParseFloat(std::string_view token, float &out) noexcept
{
    if (token.empty())
        return false;
    // std::from_chars for float 在 libc++(Apple Clang) 上未實作(標記 deleted)。
    // 場景檔由我們自己寫入(setprecision(17), C locale 格式),strtof 在預設
    // "C" locale 下是確定性的;endptr 檢查確保整個 token 被消耗(嚴格解析,
    // 不允許尾隨垃圾字元)。
    std::string buf(token);
    char *end = nullptr;
    const float value = std::strtof(buf.c_str(), &end);
    if (end == buf.c_str() || *end != '\0')
        return false;
    out = value;
    return true;
}

// parentIndex 是 uint32,但 -1 代表 root;用 int64 同時容納兩者。
inline bool ParseParent(std::string_view token, int64_t &out) noexcept
{
    if (token.empty())
        return false;
    int64_t value = 0;
    const std::from_chars_result result = std::from_chars(token.data(), token.data() + token.size(), value);
    if (result.ec != std::errc() || result.ptr != token.data() + token.size() || value < -1)
        return false;
    out = value;
    return true;
}

struct SceneNode
{
    uint32_t index = 0;
    int64_t parent = -1;
    std::string name;
    TransformComponent transform;
};

// node <index> <parent> <name> <px> <py> <pz> <rx> <ry> <rz> <sx> <sy> <sz>
// name 可以含空格 → 最後 9 個 token 固定是 TRS,之前整段都算 name。
inline bool ParseNodeLine(std::string_view line, SceneNode &out)
{
    using namespace worldhashrecord_detail;

    size_t pos = 0;
    std::string_view token;
    if (!NextToken(line, pos, token) || token != "node")
        return false;
    if (!NextToken(line, pos, token) || !ParseUInt32(token, out.index))
        return false;
    if (!NextToken(line, pos, token) || !ParseParent(token, out.parent))
        return false;

    const std::string_view rest = line.substr(pos);
    size_t cursor = rest.size();
    for (int i = 0; i < 9; ++i)
    {
        if (cursor == 0)
            return false;
        const size_t space = rest.rfind(' ', cursor - 1);
        if (space == std::string_view::npos)
            return false;
        cursor = space;
    }
    if (cursor == 0)
        return false; // 空 name
    out.name.assign(rest.data(), cursor);

    float values[9];
    size_t valuePos = cursor + 1;
    for (int i = 0; i < 9; ++i)
    {
        if (!NextToken(rest, valuePos, token) || !ParseFloat(token, values[i]))
            return false;
    }
    if (valuePos != rest.size())
        return false;

    out.transform.position = Point3D(values[0], values[1], values[2]);
    out.transform.rotation = Point3D(values[3], values[4], values[5]);
    out.transform.scale = Point3D(values[6], values[7], values[8]);
    return true;
}

// prefab 节点 v2 格式:localIndex + parentLocalIndex + materialId + PBR 欄位 + name + 9 floats
// localIndex 從 0 開始重新編號(Prefab 可實例化到不同場景位置)。
struct PrefabNode
{
    uint32_t localIndex = 0;
    int64_t parentLocalIndex = -1;
    std::string name;
    uint64_t materialId = 0;
    // PBR 欄位(A2.1)
    Point3D albedo = Point3D(1, 1, 1);
    float metallic = 0.0f;
    float roughness = 1.0f;
    uint64_t normalMapId = 0;
    TransformComponent transform;
};

// node v2: <localIndex> <parentLocalIndex> <materialId> <albedo.x> <albedo.y> <albedo.z> <metallic> <roughness> <normalMapId> <name> <9 floats>
// node v1 (向下相容): <localIndex> <parentLocalIndex> <materialId> <name> <9 floats>
// name 可含空格 → 最後 9 個 token 固定是 TRS。
// v2: PBR 欄位在 materialId 之後、name 之前;v1:無 PBR 欄位。
inline bool ParsePrefabNodeLine(std::string_view line, PrefabNode &out)
{
    using namespace worldhashrecord_detail;

    size_t pos = 0;
    std::string_view token;
    if (!NextToken(line, pos, token) || token != "node")
        return false;
    if (!NextToken(line, pos, token) || !ParseUInt32(token, out.localIndex))
        return false;
    if (!NextToken(line, pos, token) || !ParseParent(token, out.parentLocalIndex))
        return false;
    if (!NextToken(line, pos, token))
        return false;
    // materialId:uint64,無符號十進位
    {
        uint64_t value = 0;
        const std::from_chars_result result =
            std::from_chars(token.data(), token.data() + token.size(), value);
        if (result.ec != std::errc() || result.ptr != token.data() + token.size())
            return false;
        out.materialId = value;
    }

    // 判斷 v1/v2:讀取下一個 token。若是數字(浮點數或整數)，視為 v2 PBR 欄位;
    // 若是字母開頭，視為 v1 name。
    const std::string_view rest = line.substr(pos);
    size_t cursor = 0;
    // 跳過前導空格
    while (cursor < rest.size() && rest[cursor] == ' ')
        cursor++;
    bool isV2 = false;
    if (cursor < rest.size())
    {
        char c = rest[cursor];
        isV2 = (c == '-' || c == '+' || (c >= '0' && c <= '9') || c == '.');
    }

    if (isV2)
    {
        // v2:讀取 PBR 欄位 albedo(3 floats), metallic, roughness, normalMapId
        float albedoVals[3];
        for (int i = 0; i < 3; ++i)
        {
            if (!NextToken(rest, cursor, token) || !ParseFloat(token, albedoVals[i]))
                return false;
        }
        out.albedo = Point3D(albedoVals[0], albedoVals[1], albedoVals[2]);

        if (!NextToken(rest, cursor, token) || !ParseFloat(token, out.metallic))
            return false;
        if (!NextToken(rest, cursor, token) || !ParseFloat(token, out.roughness))
            return false;
        if (!NextToken(rest, cursor, token))
            return false;
        {
            uint64_t value = 0;
            const std::from_chars_result result =
                std::from_chars(token.data(), token.data() + token.size(), value);
            if (result.ec != std::errc() || result.ptr != token.data() + token.size())
                return false;
            out.normalMapId = value;
        }

        // 讀取 name:從 cursor 到最後 9 個 float 之前
        size_t nameEnd = rest.size();
        for (int i = 0; i < 9; ++i)
        {
            if (nameEnd == 0)
                return false;
            const size_t space = rest.rfind(' ', nameEnd - 1);
            if (space == std::string_view::npos)
                return false;
            nameEnd = space;
        }
        if (nameEnd == 0)
            return false; // 空 name
        out.name.assign(rest.data() + cursor, nameEnd - cursor);

        // 讀取 9 個 TRS floats
        float values[9];
        size_t valuePos = nameEnd + 1;
        for (int i = 0; i < 9; ++i)
        {
            if (!NextToken(rest, valuePos, token) || !ParseFloat(token, values[i]))
                return false;
        }
        if (valuePos != rest.size())
            return false;

        out.transform.position = Point3D(values[0], values[1], values[2]);
        out.transform.rotation = Point3D(values[3], values[4], values[5]);
        out.transform.scale = Point3D(values[6], values[7], values[8]);
    }
    else
    {
        // v1:向下相容,無 PBR 欄位,name 在 materialId 之後
        const std::string_view restV1 = line.substr(pos);
        size_t nameEnd = restV1.size();
        for (int i = 0; i < 9; ++i)
        {
            if (nameEnd == 0)
                return false;
            const size_t space = restV1.rfind(' ', nameEnd - 1);
            if (space == std::string_view::npos)
                return false;
            nameEnd = space;
        }
        if (nameEnd == 0)
            return false; // 空 name
        out.name.assign(restV1.data(), nameEnd);

        float values[9];
        size_t valuePos = nameEnd + 1;
        for (int i = 0; i < 9; ++i)
        {
            if (!NextToken(restV1, valuePos, token) || !ParseFloat(token, values[i]))
                return false;
        }
        if (valuePos != restV1.size())
            return false;

        out.transform.position = Point3D(values[0], values[1], values[2]);
        out.transform.rotation = Point3D(values[3], values[4], values[5]);
        out.transform.scale = Point3D(values[6], values[7], values[8]);
    }
    return true;
}
} // namespace scenesystem_detail

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
  private:
    // World 是實作細節 — 模組對外只暴露場景圖與節點 transform 的接縫(#81)。
    World world;

    // Entity 激活/停用狀態(按 entity index 索引)。CreateNode 預設 active=true。
    DynamicArray<bool> mActive;

    void EnsureActiveCapacity(uint32_t entityIndex)
    {
        while (mActive.GetNElements() <= entityIndex)
            mActive.Append(false);
    }

  public:
    // ---- Entity 生命週期接縫(不暴露 World facade)----
    bool Alive(Entity entity) const
    {
        return world.Alive(entity);
    }

    // editor 以 entity index(Selection.entityIndex)選取 → 換成完整 handle。
    Entity GetEntityByIndex(uint32_t entityIndex) const
    {
        return world.GetEntityByIndex(entityIndex);
    }

    // ---- Entity 激活/停用接縫(A3.1) ----
    bool IsActive(Entity entity) const
    {
        if (!world.Alive(entity))
            return false;
        uint32_t idx = entity.GetIndex();
        if (idx >= mActive.GetNElements())
            return false;
        return mActive[idx];
    }

    void SetActive(Entity entity, bool active)
    {
        if (!world.Alive(entity))
            return;
        EnsureActiveCapacity(entity.GetIndex());
        mActive[entity.GetIndex()] = active;
    }

    // 取得所有 active 的實體(依 index 升序)。
    DynamicArray<Entity> GetActiveEntities() const
    {
        DynamicArray<Entity> result;
        const uint32_t n = world.GetEntityCount();
        for (uint32_t i = 0; i < n; i++)
        {
            if (i < mActive.GetNElements() && mActive[i])
            {
                result.Append(world.GetEntityByIndex(i));
            }
        }
        return result;
    }

    // ---- 節點 transform typed queries ----
    TransformComponent *GetLocalTransform(Entity entity)
    {
        return world.GetComponent<TransformComponent>(entity);
    }

    WorldTransformComponent *GetWorldTransform(Entity entity)
    {
        return world.GetComponent<WorldTransformComponent>(entity);
    }

    // 建立節點:掛 TransformComponent + ParentComponent(root 的 parent = 空)。
    Entity CreateNode(Entity parent = Entity())
    {
        Entity e = world.CreateEntity();
        world.AddComponent<TransformComponent>(e, TransformComponent());
        ParentComponent p;
        p.parent = parent;
        world.AddComponent<ParentComponent>(e, p);
        scenesystem_detail::NodeName name;
        scenesystem_detail::SetNodeName(name, "Node");
        world.AddComponent<scenesystem_detail::NodeName>(e, name);
        world.AddComponent<MaterialComponent>(e, MaterialComponent());
        EnsureActiveCapacity(e.GetIndex());
        mActive[e.GetIndex()] = true;
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

    // 取得實體的 parent entity(存活檢查;無 parent/無效回 Entity() = null)。
    Entity GetParent(Entity e) const
    {
        if (!world.Alive(e))
            return Entity();
        const ParentComponent *p = world.GetComponent<ParentComponent>(e);
        if (!p)
            return Entity();
        return p->parent;
    }

    // ---- 組件查詢(A3.1) ----
    template <class T> T *GetComponent(Entity entity)
    {
        return world.GetComponent<T>(entity);
    }

    template <class T> const T *GetComponent(Entity entity) const
    {
        return world.GetComponent<T>(entity);
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

    // 讀取節點名稱(index + 存活檢查);無名稱回 "Node"。
    bool GetNodeName(uint32_t entityIndex, String &outName) const
    {
        Entity e = world.GetEntityByIndex(entityIndex);
        if (!world.Alive(e))
            return false;
        const scenesystem_detail::NodeName *pName =
            world.GetComponent<scenesystem_detail::NodeName>(e);
        if (!pName || pName->text[0] == '\0')
        {
            outName = String(u"Node");
            return true;
        }
        outName = String(pName->text);
        return true;
    }

    // 讀取節點材質(index + 存活檢查);未綁定回 materialId = 0。
    bool GetMaterial(uint32_t entityIndex, uint64_t &outMaterialId) const
    {
        Entity e = world.GetEntityByIndex(entityIndex);
        if (!world.Alive(e))
            return false;
        const MaterialComponent *pM = world.GetComponent<MaterialComponent>(e);
        if (!pM)
            return false;
        outMaterialId = pM->materialId;
        return true;
    }

    // 編輯器增量編輯:設定節點材質(materialId = 0 表示解綁)。entity 不存在則 no-op。
    void SetMaterial(uint32_t entityIndex, uint64_t materialId)
    {
        Entity e = world.GetEntityByIndex(entityIndex);
        if (!world.Alive(e))
            return;
        MaterialComponent *pM = world.GetComponent<MaterialComponent>(e);
        if (!pM)
            return;
        pM->materialId = materialId;
    }

    // 讀取節點 PBR 欄位(index + 存活檢查);未綁定回預設值(albedo=1, metallic=0, roughness=1)。
    bool GetPBR(uint32_t entityIndex, Point3D &outAlbedo, float &outMetallic,
                float &outRoughness) const
    {
        Entity e = world.GetEntityByIndex(entityIndex);
        if (!world.Alive(e))
            return false;
        const MaterialComponent *pM = world.GetComponent<MaterialComponent>(e);
        if (!pM)
            return false;
        outAlbedo = pM->albedo;
        outMetallic = pM->metallic;
        outRoughness = pM->roughness;
        return true;
    }

    // 編輯器增量編輯:設定節點 PBR 欄位。entity 不存在則 no-op。
    void SetPBR(uint32_t entityIndex, const Point3D &albedo, float metallic,
                float roughness)
    {
        Entity e = world.GetEntityByIndex(entityIndex);
        if (!world.Alive(e))
            return;
        MaterialComponent *pM = world.GetComponent<MaterialComponent>(e);
        if (!pM)
            return;
        pM->albedo = albedo;
        pM->metallic = metallic;
        pM->roughness = roughness;
    }

    // 讀取節點光源欄位(index + 存活檢查);無光源元件回預設值。
    bool GetLight(uint32_t entityIndex, LightComponent &outLight) const
    {
        Entity e = world.GetEntityByIndex(entityIndex);
        if (!world.Alive(e))
            return false;
        const LightComponent *pL = world.GetComponent<LightComponent>(e);
        if (!pL)
        {
            outLight = LightComponent(); // 預設值
            return true;
        }
        outLight = *pL;
        return true;
    }

    // 編輯器增量編輯:設定節點光源欄位。entity 不存在則 no-op。
    // 若該節點尚無 LightComponent 元件，會自動建立預設元件。
    void SetLight(uint32_t entityIndex, const LightComponent &light)
    {
        Entity e = world.GetEntityByIndex(entityIndex);
        if (!world.Alive(e))
            return;
        LightComponent *pL = world.GetComponent<LightComponent>(e);
        if (!pL)
            pL = world.AddComponent<LightComponent>(e, LightComponent());
        *pL = light;
    }

    // 取得投射陰影的方向光節點(LightComponent.type == Directional && castsShadow == true)。
    DynamicArray<Entity> GetShadowCasters() const
    {
        DynamicArray<Entity> casters;
        ComponentPool<LightComponent> *pPool = world.GetPool<LightComponent>();
        if (!pPool)
            return casters;
        const uint32_t n = pPool->GetNElements();
        for (uint32_t i = 0; i < n; i++)
        {
            const uint32_t owner = pPool->OwnerAt(i);
            const LightComponent *pL = pPool->Get(owner);
            if (!pL || pL->type != LightComponent::Directional || !pL->castsShadow)
                continue;
            casters.Append(world.GetEntityByIndex(owner));
        }
        // index 升序
        for (size_t i = 0; i + 1 < casters.GetNElements(); i++)
        {
            size_t best = i;
            for (size_t j = i + 1; j < casters.GetNElements(); j++)
                if (casters[j].GetIndex() < casters[best].GetIndex())
                    best = j;
            if (best != i)
            {
                Entity tmp = casters[i];
                casters[i] = casters[best];
                casters[best] = tmp;
            }
        }
        return casters;
    }

    // 編輯器增量編輯:設定節點名稱。entity 不存在則 no-op。
    void SetNodeName(uint32_t entityIndex, const String &name)
    {
        Entity e = world.GetEntityByIndex(entityIndex);
        if (!world.Alive(e))
            return;
        scenesystem_detail::NodeName *pName =
            world.GetComponent<scenesystem_detail::NodeName>(e);
        if (!pName)
            return;
        const Str<char> utf8 = name.ToUTF8();
        scenesystem_detail::SetNodeName(*pName, std::string_view(utf8.CStr(), utf8.Length()));
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

    // ---- 場景檔 Save/Load(#164)----
    // 格式:ASCII、一行一節點,見 SceneNode 格式;失敗回 false,不清動現有場景。
    bool SaveScene(const char *path) const
    {
        if (path == nullptr)
            return false;
        std::ofstream out(path, std::ios::out | std::ios::trunc);
        if (!out.is_open())
            return false;

        out << "scene v1\n";
        out << std::dec << std::setprecision(17);

        // setprecision(17) 對 float 足以唯一識別每個位元組合(24-bit 尾數需約 7 位小數，
        // 17 位綽綽有餘);-0.0f 會輸出為 -0 並由 from_chars 解回 -0.0f,因此逐 bit
        // 圓行對所有 float 值都成立(see test AC6)。
        DynamicArray<Entity> nodes;
        DynamicArray<uint32_t> depths;
        GetHierarchy(nodes, depths);
        for (size_t i = 0; i < nodes.GetNElements(); ++i)
        {
            const Entity e = nodes[i];
            const TransformComponent *pTransform = world.GetComponent<TransformComponent>(e);
            if (!pTransform)
                return false;

            const ParentComponent *pParent = world.GetComponent<ParentComponent>(e);
            int64_t parentIndex = -1;
            if (pParent && pParent->parent.GetRaw() != 0 && world.Alive(pParent->parent))
                parentIndex = static_cast<int64_t>(pParent->parent.GetIndex());

            const scenesystem_detail::NodeName *pName = world.GetComponent<scenesystem_detail::NodeName>(e);
            const char *nodeName = (pName && pName->text[0] != '\0') ? pName->text : "Node";

            out << "node " << e.GetIndex() << " " << parentIndex << " " << nodeName << " ";
            out << pTransform->position.x << " " << pTransform->position.y << " " << pTransform->position.z << " ";
            out << pTransform->rotation.x << " " << pTransform->rotation.y << " " << pTransform->rotation.z << " ";
            out << pTransform->scale.x << " " << pTransform->scale.y << " " << pTransform->scale.z << "\n";
        }

        out << "end\n";
        return out.good();
    }

    bool LoadScene(const char *path)
    {
        if (path == nullptr)
            return false;
        std::ifstream in(path, std::ios::in);
        if (!in.is_open())
            return false;

        using namespace worldhashrecord_detail;

        std::string line;
        if (!ReadLine(in, line) || line != "scene v1")
            return false;

        // 先完整解析到區域容器;任何錯誤都在動世界之前返回。
        DynamicArray<scenesystem_detail::SceneNode> parsed;
        DynamicArray<uint32_t> indices;
        bool sawEnd = false;
        while (ReadLine(in, line))
        {
            if (line == "end")
            {
                sawEnd = true;
                break;
            }

            scenesystem_detail::SceneNode node;
            if (!scenesystem_detail::ParseNodeLine(line, node))
                return false;

            // index 不得重複;parent 必須是更早出現的 index 或 -1。
            bool duplicate = false;
            for (size_t i = 0; i < indices.GetNElements(); ++i)
                if (indices[i] == node.index)
                    duplicate = true;
            if (duplicate)
                return false;
            if (node.parent >= 0)
            {
                bool found = false;
                for (size_t i = 0; i < indices.GetNElements(); ++i)
                    if (static_cast<uint32_t>(node.parent) == indices[i])
                        found = true;
                if (!found)
                    return false;
            }

            indices.Append(node.index);
            parsed.Append(node);
        }
        if (!sawEnd)
            return false;
        if (ReadLine(in, line) || !in.eof())
            return false;

        // 解析全成功才清空現有場景,再依檔案順序重建。
        DynamicArray<Entity> roots = GetRoots();
        for (size_t i = 0; i < roots.GetNElements(); ++i)
            DestroyNode(roots[i]);

        DynamicArray<Entity> created;
        for (size_t i = 0; i < parsed.GetNElements(); ++i)
        {
            Entity parent;
            if (parsed[i].parent >= 0)
            {
                for (size_t j = 0; j < indices.GetNElements(); ++j)
                {
                    if (indices[j] == static_cast<uint32_t>(parsed[i].parent))
                    {
                        parent = created[j];
                        break;
                    }
                }
            }

            const Entity e = CreateNode(parent);
            *world.GetComponent<TransformComponent>(e) = parsed[i].transform;
            scenesystem_detail::NodeName *pName = world.GetComponent<scenesystem_detail::NodeName>(e);
            if (pName)
                scenesystem_detail::SetNodeName(*pName, parsed[i].name);
            created.Append(e);
        }
        return true;
    }

    // ---- Prefab 檔 Save/Load(#168 優先級 4)----
    // 格式:ASCII、prefab v1,一行一節點,見 PrefabNode 格式。
    // localIndex 從 0 開始重新編號,Prefab 可實例化到不同場景位置。
    // 失敗回 false,不清動現有場景。
    bool SavePrefab(Entity root, const char *path) const
    {
        if (path == nullptr || !world.Alive(root))
            return false;
        std::ofstream out(path, std::ios::out | std::ios::trunc);
        if (!out.is_open())
            return false;
        out << "prefab v2\n";
        out << std::dec << std::setprecision(17);

        // DFS pre-order 收集子樹所有節點。
        DynamicArray<Entity> nodes;
        CollectSubtree(root, nodes);

        // 建立 entity index -> local index 映射。
        HashTable<uint32_t, uint32_t> indexMap;
        for (size_t i = 0; i < nodes.GetNElements(); i++)
            indexMap.Insert(nodes[i].GetIndex(), static_cast<uint32_t>(i));

        for (size_t i = 0; i < nodes.GetNElements(); i++)
        {
            const Entity e = nodes[i];
            const TransformComponent *pTransform = world.GetComponent<TransformComponent>(e);
            if (!pTransform)
                return false;

            const ParentComponent *pParent = world.GetComponent<ParentComponent>(e);
            int64_t parentLocalIndex = -1;
            if (pParent && pParent->parent.GetRaw() != 0 && world.Alive(pParent->parent))
            {
                typename HashTable<uint32_t, uint32_t>::Iterator itr =
                    indexMap.Find(pParent->parent.GetIndex());
                if (itr != indexMap.Last())
                    parentLocalIndex = static_cast<int64_t>(itr->Value());
            }

            const scenesystem_detail::NodeName *pName =
                world.GetComponent<scenesystem_detail::NodeName>(e);
            const char *nodeName =
                (pName && pName->text[0] != '\0') ? pName->text : "Node";

            uint64_t matId = 0;
            const MaterialComponent *pMat = world.GetComponent<MaterialComponent>(e);
            if (pMat)
                matId = pMat->materialId;

            const Point3D albedo = pMat ? pMat->albedo : Point3D(1, 1, 1);
            const float metallic = pMat ? pMat->metallic : 0.0f;
            const float roughness = pMat ? pMat->roughness : 1.0f;
            const uint64_t normalMapId = pMat ? pMat->normalMapId : 0;

            out << "node " << i << " " << parentLocalIndex << " " << matId << " "
                << albedo.x << " " << albedo.y << " " << albedo.z << " "
                << metallic << " " << roughness << " " << normalMapId << " "
                << nodeName << " ";
            out << pTransform->position.x << " " << pTransform->position.y << " "
                << pTransform->position.z << " ";
            out << pTransform->rotation.x << " " << pTransform->rotation.y << " "
                << pTransform->rotation.z << " ";
            out << pTransform->scale.x << " " << pTransform->scale.y << " "
                << pTransform->scale.z << "\n";
        }
        out << "end\n";
        return out.good();
    }

    // 從 Prefab 檔載入子樹到指定 parent 下。回傳新建立的根節點 Entity。
    Entity LoadPrefab(Entity parent, const char *path)
    {
        Entity nullEntity;
        if (path == nullptr)
            return nullEntity;
        std::ifstream in(path, std::ios::in);
        if (!in.is_open())
            return nullEntity;

        using namespace scenesystem_detail;
        using namespace worldhashrecord_detail;

        std::string line;
        if (!ReadLine(in, line) || (line != "prefab v1" && line != "prefab v2"))
            return nullEntity;

        // 先完整解析到區域容器;任何錯誤都在動世界之前返回。
        DynamicArray<PrefabNode> parsed;
        DynamicArray<uint32_t> indices; // localIndex 順序
        bool sawEnd = false;
        while (ReadLine(in, line))
        {
            if (line == "end")
            {
                sawEnd = true;
                break;
            }
            PrefabNode node;
            if (!ParsePrefabNodeLine(line, node))
                return nullEntity;
            // localIndex 不得重複;parent 必須是更早出現的 localIndex 或 -1。
            bool duplicate = false;
            for (size_t i = 0; i < indices.GetNElements(); ++i)
                if (indices[i] == node.localIndex)
                    duplicate = true;
            if (duplicate)
                return nullEntity;
            if (node.parentLocalIndex >= 0)
            {
                bool found = false;
                for (size_t i = 0; i < indices.GetNElements(); ++i)
                    if (static_cast<uint32_t>(node.parentLocalIndex) == indices[i])
                    {
                        found = true;
                        break;
                    }
                if (!found)
                    return nullEntity;
            }
            indices.Append(node.localIndex);
            parsed.Append(node);
        }
        if (!sawEnd)
            return nullEntity;
        if (ReadLine(in, line) || !in.eof())
            return nullEntity;

        // 依檔案順序建立節點(CreateNode 會自動掛 Transform/Parent/NodeName/Material)。
        DynamicArray<Entity> created;
        for (size_t i = 0; i < parsed.GetNElements(); ++i)
        {
            Entity pEnt;
            if (parsed[i].parentLocalIndex >= 0)
            {
                for (size_t j = 0; j < indices.GetNElements(); ++j)
                {
                    if (indices[j] == static_cast<uint32_t>(parsed[i].parentLocalIndex))
                    {
                        pEnt = created[j];
                        break;
                    }
                }
            }
            const Entity e = CreateNode(pEnt);
            *world.GetComponent<TransformComponent>(e) = parsed[i].transform;
            scenesystem_detail::NodeName *pName =
                world.GetComponent<scenesystem_detail::NodeName>(e);
            if (pName)
                scenesystem_detail::SetNodeName(*pName, parsed[i].name);
            MaterialComponent *pMat = world.GetComponent<MaterialComponent>(e);
            if (pMat)
            {
                pMat->materialId = parsed[i].materialId;
                pMat->albedo = parsed[i].albedo;
                pMat->metallic = parsed[i].metallic;
                pMat->roughness = parsed[i].roughness;
                pMat->normalMapId = parsed[i].normalMapId;
            }
            created.Append(e);
        }
        return created.IsEmpty() ? nullEntity : created[0];
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