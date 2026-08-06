#ifndef FRAME_HPP
#define FRAME_HPP

#include "Camera.hpp"
#include "Data/DynamicArray.hpp"
#include "Data/Pointers.hpp"
#include "Data/String.hpp"
#include "Display/Color.hpp"
#include "Display/GUI/GUILayout.hpp"
#include "Display/IRenderable.hpp"
#include "Geometry/3D/Point3D.hpp"
#include "Types/Types.hpp"

#include <cstdint>

// 渲染命令流:Sim/View 產出,IFrameExecutor 執行。
// Frame 是「模擬」與「呈現」之間唯一的貨幣 — 兩層不互相呼叫,只交換 Frame。
//
// 典型錄製順序:
//   frame.BeginFrame();
//   frame.SetCamera(scene.GetCamera());
//   for (auto &renderable : scene) frame.DrawRenderable(*renderable);
//   frame.EndFrame();
//   executor.Execute(frame);
class Frame
{
  public:
    enum class Command
    {
        // ── 保留(相容,邏輯不變)──────────────────────────────
        BeginFrame,     // 幀起點:清畫面、acquire swapchain image
        SetCamera,      // 設定本幀相機
        DrawRenderable, // 畫一個 IRenderable
        DrawGUILayout,  // 畫 GUI 佈局
        EndFrame,       // 幀終點:提交 + 呈現
        // ── 新增(值語意,可序列化)────────────────────────────
        PushTransform,  // 推入一組絕對 world TRS(executor 合成 Mat4)
        PopTransform,   // 彈出
        BindMaterial,   // 綁定目前材質(content-hash id)
        DrawMesh,       // 用 transform stack top 畫一個 mesh(id)
        SetViewport,    // 設視口(x, y, width, height)
        DrawText        // 用 transform stack top 畫文字(fontId, text, size, color)
    };

    // PushTransform 的 TRS 酬載,與 TransformComponent / WorldTransformComponent 同構。
    struct TransformPayload
    {
        Point3D position;
        Point3D rotation; // degree Euler,同 BuildWorldMatrix 慣例
        Point3D scale;
        TransformPayload(const Point3D &pos = Point3D(), const Point3D &rot = Point3D(),
                         const Point3D &s = Point3D(1, 1, 1))
            : position(pos), rotation(rot), scale(s)
        {
        }
    };

    struct ViewportPayload
    {
        float x;
        float y;
        float width;
        float height;
        ViewportPayload(float x = 0, float y = 0, float width = 0, float height = 0)
            : x(x), y(y), width(width), height(height)
        {
        }
    };

    // 命令酬載:未使用欄位為空(nullptr / 0 / 空 String)。
    // 注意:含 String 酬載 → 不再 trivially copyable(讓步換所有權與可序列化)。
    struct CommandData
    {
        Command command;
        // ── 保留:legacy 指標(未使用欄位為 nullptr)──
        Camera *pCamera;
        const IRenderable *pRenderable;
        GUILayout *pLayout;
        // ── 新增:值酬載 ──
        TransformPayload transform;
        uint64_t meshId;     // mesh 的 content-hash id
        uint64_t materialId; // material 的 content-hash id
        ViewportPayload viewport;
        uint64_t fontId;
        String text; // Frame 擁有所有權 → 序列化/round-trip 不需外部指標
        float textSize;
        Color textColor;
    };

  private:
    DynamicArray<CommandData> commands;

  public:
    Frame() : commands()
    {
    }

    void BeginFrame()
    {
        Append(Command::BeginFrame);
    }

    void SetCamera(WeakPtr<Camera> pCamera)
    {
        Append(Command::SetCamera).pCamera = pCamera.operator->(); // 無效相機 → nullptr(安全)
    }

    void DrawRenderable(const IRenderable &renderable)
    {
        Append(Command::DrawRenderable).pRenderable = &renderable;
    }

    void DrawGUILayout(GUILayout &layout)
    {
        Append(Command::DrawGUILayout).pLayout = &layout;
    }

    void EndFrame()
    {
        Append(Command::EndFrame);
    }

    // ── 新增(值語意)─────────────────────────────────────────

    void PushTransform(const TransformPayload &trs)
    {
        Append(Command::PushTransform).transform = trs;
    }

    void PushTransform(const Point3D &position, const Point3D &rotation, const Point3D &scale)
    {
        Append(Command::PushTransform).transform = TransformPayload(position, rotation, scale);
    }

    void PopTransform()
    {
        Append(Command::PopTransform);
    }

    void BindMaterial(uint64_t materialId)
    {
        Append(Command::BindMaterial).materialId = materialId;
    }

    void DrawMesh(uint64_t meshId)
    {
        Append(Command::DrawMesh).meshId = meshId;
    }

    void SetViewport(float x, float y, float width, float height)
    {
        Append(Command::SetViewport).viewport = ViewportPayload(x, y, width, height);
    }

    void DrawText(uint64_t fontId, const String &text, float size, const Color &color)
    {
        CommandData &cmd = Append(Command::DrawText);
        cmd.fontId = fontId;
        cmd.text = text;
        cmd.textSize = size;
        cmd.textColor = color;
    }

    void Clear()
    {
        commands.RemoveAll();
    }

    size_t GetNumCommands() const
    {
        return commands.GetNElements();
    }

    const CommandData &GetCommand(size_t index) const
    {
        return commands[index];
    }

    // ── 序列化 / 反序列化(驗收 1、2)────────────────────────
    // 依 append 順序輸出命令流字串;相同場景 → 位元組相同(確定性)。
    // 值命令直接印 payload;legacy 指標命令印其語意內容而非指標。
    String Serialize() const
    {
        String out;
        const size_t count = commands.GetNElements();
        for (size_t i = 0; i < count; i++)
        {
            const CommandData &cmd = commands[i];
            String line;
            switch (cmd.command)
            {
                case Command::BeginFrame:
                    line = String(u"BeginFrame");
                    break;
                case Command::EndFrame:
                    line = String(u"EndFrame");
                    break;
                case Command::PopTransform:
                    line = String(u"PopTransform");
                    break;
                case Command::PushTransform:
                {
                    line = String(u"PushTransform");
                    AppendFloat(line, cmd.transform.position.x);
                    AppendFloat(line, cmd.transform.position.y);
                    AppendFloat(line, cmd.transform.position.z);
                    AppendFloat(line, cmd.transform.rotation.x);
                    AppendFloat(line, cmd.transform.rotation.y);
                    AppendFloat(line, cmd.transform.rotation.z);
                    AppendFloat(line, cmd.transform.scale.x);
                    AppendFloat(line, cmd.transform.scale.y);
                    AppendFloat(line, cmd.transform.scale.z);
                    break;
                }
                case Command::BindMaterial:
                    line = String(u"BindMaterial");
                    AppendInt(line, cmd.materialId);
                    break;
                case Command::DrawMesh:
                    line = String(u"DrawMesh");
                    AppendInt(line, cmd.meshId);
                    break;
                case Command::SetViewport:
                {
                    line = String(u"SetViewport");
                    AppendFloat(line, cmd.viewport.x);
                    AppendFloat(line, cmd.viewport.y);
                    AppendFloat(line, cmd.viewport.width);
                    AppendFloat(line, cmd.viewport.height);
                    break;
                }
                case Command::DrawText:
                {
                    // 文字放最後(可能含空格),size + RGBA 在前面為數字 token。
                    line = String(u"DrawText");
                    AppendInt(line, cmd.fontId);
                    AppendFloat(line, cmd.textSize);
                    AppendFloat(line, cmd.textColor.R);
                    AppendFloat(line, cmd.textColor.G);
                    AppendFloat(line, cmd.textColor.B);
                    AppendFloat(line, cmd.textColor.A);
                    if (cmd.text.Length() > 0)
                        line = line + String(u" ") + cmd.text;
                    break;
                }
                case Command::SetCamera:
                {
                    line = String(u"SetCamera");
                    if (cmd.pCamera)
                    {
                        const Point3D pos = cmd.pCamera->GetPosition();
                        const Point3D rot = cmd.pCamera->GetRotation();
                        AppendFloat(line, pos.x);
                        AppendFloat(line, pos.y);
                        AppendFloat(line, pos.z);
                        AppendFloat(line, rot.x);
                        AppendFloat(line, rot.y);
                        AppendFloat(line, rot.z);
                        AppendFloat(line, cmd.pCamera->GetAngleOfView());
                        AppendFloat(line, cmd.pCamera->GetDistanceToNearPlane());
                        AppendFloat(line, cmd.pCamera->GetDistanceToFarPlane());
                    }
                    else
                    {
                        for (int k = 0; k < 9; k++) // 無效相機 → 全零(確定)
                            AppendFloat(line, 0.0f);
                    }
                    break;
                }
                case Command::DrawRenderable:
                {
                    line = String(u"DrawRenderable");
                    if (cmd.pRenderable)
                    {
                        AppendInt(line, static_cast<uint64_t>(cmd.pRenderable->GetIdentifier()));
                        const Point3D pos = cmd.pRenderable->GetPosition();
                        const Point3D rot = cmd.pRenderable->GetRotation();
                        const Point3D scale = cmd.pRenderable->GetScale();
                        AppendFloat(line, pos.x);
                        AppendFloat(line, pos.y);
                        AppendFloat(line, pos.z);
                        AppendFloat(line, rot.x);
                        AppendFloat(line, rot.y);
                        AppendFloat(line, rot.z);
                        AppendFloat(line, scale.x);
                        AppendFloat(line, scale.y);
                        AppendFloat(line, scale.z);
                    }
                    else
                    {
                        AppendInt(line, 0);
                        for (int k = 0; k < 9; k++)
                            AppendFloat(line, 0.0f);
                    }
                    break;
                }
                case Command::DrawGUILayout:
                {
                    line = String(u"DrawGUILayout");
                    if (cmd.pLayout)
                    {
                        const DynamicArray<SharedPtr<GUILayer>> &layers = cmd.pLayout->GetLayers();
                        for (size_t li = 0; li < layers.GetNElements(); li++)
                        {
                            AppendInt(line, static_cast<uint64_t>(layers[li]->GetIdentifier()));
                            const DynamicArray<SharedPtr<IGUI>> &components = layers[li]->GetComponents();
                            for (size_t ci = 0; ci < components.GetNElements(); ci++)
                                AppendInt(line, static_cast<uint64_t>(components[ci]->GetIdentifier()));
                        }
                    }
                    break;
                }
            }
            AppendSeparatedLine(out, line);
        }
        return out;
    }

    // 重建值命令流的 Frame(測試用)。legacy 指標命令(SetCamera/DrawRenderable/
    // DrawGUILayout)無法無頭重建 → 只記錄命令、依 null 指標重建;
    // round-trip 驗收只針對新命令。
    static Frame Deserialize(const String &serialized)
    {
        Frame frame;
        const char16_t *p = serialized.CStr();
        const size_t total = serialized.Length();
        size_t start = 0;
        while (start <= total)
        {
            size_t end = start;
            while (end < total && p[end] != (char16_t)'\n')
                end++;
            if (end > start)
                ParseLine(frame, p, start, end);
            if (end >= total)
                break;
            start = end + 1;
        }
        return frame;
    }

  private:
    // ── 序列化 helper ────────────────────────────────────────────
    static String FixedFloat(float value)
    {
        // 固定 6 位小數、略去 locale 影響(Str::FromFloat 純整數運算) → 確定字形。
        return Str<char16_t>::FromFloat(value, 6);
    }

    static String NumberStr(uint64_t value)
    {
        return Str<char16_t>::FromInt(value);
    }

    static void AppendInt(String &out, uint64_t value)
    {
        if (out.Length() > 0)
            out = out + String(u" ");
        out = out + NumberStr(value);
    }

    static void AppendFloat(String &out, float value)
    {
        if (out.Length() > 0)
            out = out + String(u" ");
        out = out + FixedFloat(value);
    }

    static void AppendSeparatedLine(String &out, const String &line)
    {
        if (out.Length() > 0)
            out = out + String(u"\n");
        out = out + line;
    }

    CommandData &Append(Command command)
    {
        CommandData data;
        data.command = command;
        data.pCamera = nullptr;
        data.pRenderable = nullptr;
        data.pLayout = nullptr;
        data.meshId = 0;
        data.materialId = 0;
        data.fontId = 0;
        data.textSize = 0;
        data.textColor = Color();
        commands.Append(Types::Move(data));
        return commands.GetLast();
    }

    // ── 反序列化 helper ────────────────────────────────────────
    static float ParseFloat(const char16_t *p, size_t len)
    {
        if (p == nullptr || len == 0)
            return 0.0f; // 空 / 非法 token → 0
        size_t i = 0;
        bool negative = false;
        if (p[i] == (char16_t)'-' || p[i] == (char16_t)'+')
        {
            negative = p[i] == (char16_t)'-';
            i++;
        }
        double value = 0.0;
        bool anyDigit = false;
        for (; i < len && p[i] >= (char16_t)'0' && p[i] <= (char16_t)'9'; i++)
        {
            value = value * 10.0 + static_cast<double>(p[i] - '0');
            anyDigit = true;
        }
        if (i < len && p[i] == (char16_t)'.')
        {
            i++;
            double fraction = 1.0;
            for (; i < len && p[i] >= (char16_t)'0' && p[i] <= (char16_t)'9'; i++)
            {
                fraction *= 0.1;
                value += static_cast<double>(p[i] - '0') * fraction;
                anyDigit = true;
            }
        }
        if (!anyDigit)
            return 0.0f;
        return negative ? static_cast<float>(-value) : static_cast<float>(value);
    }

    static uint64_t ParseUInt64(const char16_t *p, size_t len)
    {
        if (p == nullptr || len == 0)
            return 0;
        uint64_t value = 0;
        for (size_t i = 0; i < len && p[i] >= (char16_t)'0' && p[i] <= (char16_t)'9'; i++)
        {
            const uint64_t digit = static_cast<uint64_t>(p[i] - '0');
            value = value * 10 + digit;
        }
        return value;
    }

    struct Token
    {
        size_t start;
        size_t len;
    };

    static void TokenizeLine(const char16_t *p, size_t start, size_t end, DynamicArray<Token> &tokens)
    {
        size_t i = start;
        while (i < end)
        {
            while (i < end && (p[i] == (char16_t)' ' || p[i] == (char16_t)'\t'))
                i++;
            if (i >= end)
                break;
            const size_t t0 = i;
            while (i < end && p[i] != (char16_t)' ' && p[i] != (char16_t)'\t')
                i++;
            Token t;
            t.start = t0;
            t.len = i - t0;
            tokens.Append(t);
        }
    }

    static void ParseLine(Frame &frame, const char16_t *p, size_t start, size_t end)
    {
        DynamicArray<Token> tokens;
        TokenizeLine(p, start, end, tokens);
        const size_t n = tokens.GetNElements();
        if (n == 0)
            return;
        const Token &nameTok = tokens[0];
        String name(&p[nameTok.start], nameTok.len);
        auto floatTok = [&](size_t idx) -> float
        {
            return idx < n ? ParseFloat(p + tokens[idx].start, tokens[idx].len) : 0.0f;
        };
        auto uintTok = [&](size_t idx) -> uint64_t
        {
            return idx < n ? ParseUInt64(p + tokens[idx].start, tokens[idx].len) : 0ull;
        };

        if (name == String(u"BeginFrame")) { frame.BeginFrame(); return; }
        if (name == String(u"EndFrame")) { frame.EndFrame(); return; }
        if (name == String(u"PopTransform")) { frame.PopTransform(); return; }
        if (name == String(u"PushTransform"))
        {
            CommandData &cmd = frame.Append(Command::PushTransform);
            cmd.transform.position.x = floatTok(1);
            cmd.transform.position.y = floatTok(2);
            cmd.transform.position.z = floatTok(3);
            cmd.transform.rotation.x = floatTok(4);
            cmd.transform.rotation.y = floatTok(5);
            cmd.transform.rotation.z = floatTok(6);
            cmd.transform.scale.x = floatTok(7);
            cmd.transform.scale.y = floatTok(8);
            cmd.transform.scale.z = floatTok(9);
            return;
        }
        if (name == String(u"BindMaterial")) { frame.BindMaterial(uintTok(1)); return; }
        if (name == String(u"DrawMesh")) { frame.DrawMesh(uintTok(1)); return; }
        if (name == String(u"SetViewport"))
        {
            frame.SetViewport(floatTok(1), floatTok(2), floatTok(3), floatTok(4));
            return;
        }
        if (name == String(u"DrawText"))
        {
            // tokens: 1=fontId, 2=size, 3..6=r,g,b,a, 7..=text(單空格接合)
            const float r = floatTok(3), g = floatTok(4), b = floatTok(5), a = floatTok(6);
            String text;
            for (size_t k = 7; k < n; k++)
            {
                if (k > 7)
                    text = text + String(u" ");
                text = text + String(&p[tokens[k].start], tokens[k].len);
            }
            frame.DrawText(uintTok(1), text, floatTok(2), Color(r, g, b, a));
            return;
        }
        // legacy 指標命令:無法無頭重建,僅記錄命令 + null 指標
        if (name == String(u"SetCamera")) { frame.Append(Command::SetCamera); return; }
        if (name == String(u"DrawRenderable")) { frame.Append(Command::DrawRenderable); return; }
        if (name == String(u"DrawGUILayout")) { frame.Append(Command::DrawGUILayout); return; }
    }
};

#endif // FRAME_HPP