# ChiaEngine 下一阶段规划 — A1 多场景加载/卸载 + 场景 v2 格式

## 目标
支持场景栈（Push/Pop），场景卸载时安全销毁实体；场景文件 v2 格式支持 prefab_ref 跨场景引用，为 A2 渲染/A3 运行时/A4 示例游戏奠定基础。

## 约束
- C++17 stdlib only；无 `-ffast-math` / `-Ofast` / `-march=native`
- `EditorSession` 保持 window-agnostic（无 GLFW/Window 类型）
- 每个变更：Issue → feature 分支 → commit `#N` → PR → CI 全绿 → 关闭 Issue
- 本地：`cmake --build build --parallel --target TestMain` exit 0；`xvfb-run -a ./build/bin/TestMain` 344+ passed / 0 failed

## 任务拆分

| 任务 | 文件 | 内容 |
|------|------|------|
| A1.1 | `src/include/Scene/SceneSystem.hpp`<br>`src/source/Scene/SceneSystem.cpp` | 新增 `SceneStack`：`PushScene(Entity root)` / `PopScene()` / `GetActiveScene()`；`PopScene` 销毁该场景下所有实体及组件 |
| A1.2 | `src/include/Scene/SceneSystem.hpp`<br>`src/source/Scene/SceneSystem.cpp` | 扩展 `SaveScene`/`LoadScene` 为 v2 格式：文件头 `scene v2`，增加 `scene <name>` 块，支持 `prefab_ref <path> <localIndex>` |
| A1.3 | `src/include/Scene/PrefabNode.hpp`<br>`src/source/Scene/PrefabNode.cpp`（或现有） | `PrefabNode` 增加 `prefabRef` 字段（可选）；`ParsePrefabNodeLine` 支持解析 `prefab_ref` |
| A1.4 | `test/System/SceneMultiTest.hpp`<br>`test/System/SystemModule.cpp` | 新增测试：加载两个场景 → 卸载第一个 → 验证第二个场景实体仍存活；加载带 `prefab_ref` 的场景 v2 文件 |
| A1.5 | `src/include/Display/GUI/EditorSession.hpp`<br>`src/source/Display/GUI/EditorSession.cpp` | 新增 `LoadSceneFile(path)` / `UnloadCurrentScene()` API，供 editor 菜单调用 |

## 验收标准
1. 构建通过，`TestMain` 0 failed
2. 加载场景 A 和 B，卸载 A，B 的实体与 transform 完整，无内存泄漏（ASan/UBSan 无报错）
3. 场景 v2 文件可被 `LoadScene` 解析并实例化 prefab 引用

## Git 工作流
1. `gh issue create` — 标题：`[A1] 多场景加载/卸载 + 场景 v2 格式`
2. `git checkout -b feat/editor-scenes-multi`
3. 开发，每个 commit 关联 `#N`
4. `gh pr create --base main --head feat/editor-scenes-multi`
5. CI 全绿后 merge，`gh issue close N --comment "Resolved by PR #M"`

## 后续里程碑（预告）
- A2：基础渲染管线（方向光/点光照、PBR 材质预览、阴影贴图）
- A3：运行时系统（GameRuntime 主循环、InputMapper、运行时 prefab 实例化）
- A4：示例游戏（samples/first_game，WASD 移动方块）
