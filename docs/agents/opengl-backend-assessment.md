# OpenGL Backend Assessment (issue #54)

## Why

Issue #54 (`OpenGL backend: GUI path unimplemented`, severity 🟠 design) reports two
TODOs in `src/source/Display/OpenGLRenderer.cpp` — `:212 TODO: GUI support` and
`:309 TODO: GUI rendering` — and asks whether to **implement GUI for OpenGL** or
**deprecate the OpenGL backend**. This document is a read-only investigation of the
worktree at commit `e968787`; no source files were modified and nothing was committed.
It verifies each of the preliminary findings below, goes beyond them (construction
sites, exact compile blockers, full implementation scope, deprecation plan), and ends
with a recommendation and effort estimates.

**Headline result:** every preliminary finding was **confirmed** (with refinements),
and the investigation found two *additional* blockers the issue does not mention —
the `Renderer` wrapper itself has no OpenGL branch (so the OpenGL build cannot even
compile today, even with GLAD vendored), and the shared GLFW window layer hardcodes
`GLFW_CLIENT_API = GLFW_NO_API`, which makes GL context creation fail at runtime.
**Recommendation: deprecate** (issue option 2). The minimal legacy fix would be dead
code; the Frame-port fix is a multi-week project that re-implements an entire second
executor for a backend nobody has touched since its initial commit.

---

## Verified findings (with evidence)

### F1 — `OpenGLRenderer` is never constructed; `Renderer` has no OpenGL branch ✅ confirmed

`src/include/Display/Renderer.hpp` selects the specialized backend exclusively:

```
Renderer.hpp:8   #ifdef DIRECTX_ENABLED
Renderer.hpp:9   #include "Display/DirectX/DirectXRenderer.hpp"
Renderer.hpp:10  #elif VULKAN_ENABLED
Renderer.hpp:11  #include "Display/Vulkan/VulkanRenderer.hpp"
Renderer.hpp:12  #endif
Renderer.hpp:19  #ifdef DIRECTX_ENABLED
Renderer.hpp:20      DirectXRenderer specializedRenderer;
Renderer.hpp:21  #elif VULKAN_ENABLED
Renderer.hpp:22      VulkanRenderer specializedRenderer;
Renderer.hpp:23  #endif
```

There is **no `#elif OPENGL_ENABLED` branch** — under `OPENGL_ENABLED` neither
`DirectXRenderer` nor `VulkanRenderer` is declared, so `specializedRenderer` does not
exist and `src/source/Display/Renderer.cpp` (which calls `specializedRenderer.*` in
every method, e.g. `Renderer.cpp:80 return specializedRenderer.Execute(frame);`)
**fails to compile**. This is a *compile-time* dead path, not merely a runtime one.
`OpenGLRenderer` is included by nothing except its own translation unit.

### F2 — GLAD is not vendored; the OpenGL CMake branch only warns ✅ confirmed (refined)

`src/CMakeLists.txt` backend source selection:

```
src/CMakeLists.txt:60   if (DIRECTX_ENABLED)          → Windows/Window.cpp + DirectX files
src/CMakeLists.txt:70   elseif(VULKAN_ENABLED)        → GLFW/Window.cpp + Vulkan files
src/CMakeLists.txt:79   else()                        → OpenGL fallback:
src/CMakeLists.txt:81       if(EXISTS ${CMAKE_SOURCE_DIR}/src/include/3rdparty/glad/glad.c)
src/CMakeLists.txt:82           append glad.c to sources
src/CMakeLists.txt:84       else()
src/CMakeLists.txt:84           message(WARNING "glad.c not found — generate it from
                                https://glad.dav1d.de/ (OpenGL 3.3 Core, Generate a loader) …")
src/CMakeLists.txt:86-93      append OpenGLHelper + OpenGLRenderer + GLFW/Window sources
```

`src/include/3rdparty/` contains **only `stb_image.h`** — no `glad/` directory, no
`glad.c`, no `glad.h` anywhere in the worktree. The issue's option 1 ("GLAD loader
already present") is **refuted**: the loader is *not* present; it must be generated
from glad.dav1d.de and placed in `src/include/3rdparty/glad/` (documented in
`README.md:32-42`). The include path is wired: root `CMakeLists.txt:58` appends
`${CMAKE_SOURCE_DIR}/src/include/3rdparty` so `#include <glad/glad.h>`
(`OpenGLRenderer.hpp:15`, `OpenGLHelper.hpp:10`) would resolve once the directory exists.

**Refinement (compile blockers beyond GLAD):** even with GLAD vendored, the OpenGL
build cannot compile because of F1. Full list in section B.

### F3 — `Window::Render()` is Frame-driven; `OpenGLRenderer` has no `Execute` ✅ confirmed

`src/source/Display/GLFW/Window.cpp:197-221` (used by **both** the Vulkan and OpenGL
branches — `src/CMakeLists.txt:72-73` and `91-92`):

```
Window.cpp:204   Frame frame;
Window.cpp:205   frame.BeginFrame();
Window.cpp:208   frame.SetCamera(pScene->GetCamera());
Window.cpp:210-211  for (...) frame.DrawRenderable(*renderables[i]);
Window.cpp:215   frame.DrawGUILayout(*pGUILayout);
Window.cpp:217   GUIFrameProjector::ProjectLabels(*pGUILayout, GlyphAtlas::GetDefault(), … , frame);  // DrawText
Window.cpp:219   frame.EndFrame();
Window.cpp:220   renderer.Execute(frame);
```

`OpenGLRenderer` implements only the legacy `IRenderer` interface
(`OpenGLRenderer.hpp:22 class OpenGLRenderer : public IRenderer`) — it has **no
`IFrameExecutor::Execute(const Frame&)`** and does not inherit `IFrameExecutor`.
Only `VulkanRenderer` implements `IFrameExecutor`
(`VulkanRenderer.hpp:17 class VulkanRenderer : public IRenderer, public IFrameExecutor`).
So even if constructed, `OpenGLRenderer` cannot be driven by `Window::Render`.

### F4 — The two TODOs ✅ confirmed (exact lines)

```
OpenGLRenderer.cpp:209  bool OpenGLRenderer::LoadGUILayout(GUILayout &layout)
OpenGLRenderer.cpp:212      // TODO: GUI support        ← issue :212
OpenGLRenderer.cpp:306  void OpenGLRenderer::Render(GUILayout &layout)
OpenGLRenderer.cpp:309      // TODO: GUI rendering      ← issue :309
```

Both are silent no-ops (`(void)layout; return true;` / empty body).

### F5 — The renderer lives in `Window`; nothing references `OpenGLRenderer` ✅ confirmed

- `src/include/Display/Window.hpp:44` — `Renderer renderer;` member; initialized at
  `GLFW/Window.cpp:117` (`renderer()`), driven at `Window.cpp:178`
  (`renderer.Initialize(this)`), `:194` (`renderer.Update()`), `:220`
  (`renderer.Execute(frame)`), `:273` (`ApplyCamera`), `:280` (`OnWindowResized`).
- `src/source/App/App.cpp:51-57` only forwards to
  `WindowManager::GetSingleton().UpdateWindows()/RenderWindows()` — no renderer access.
- Repo-wide grep for `OpenGLRenderer`: only its own `.cpp`/`.hpp`,
  `Shader.hpp:30` (`friend class OpenGLRenderer;`, guarded by `#ifdef OPENGL_ENABLED`),
  and `src/CMakeLists.txt:89-90`. **No construction site anywhere.**

---

## A. Construction sites / reachability (beyond the preliminary list)

- **No construction site:** grep found no `new OpenGLRenderer`, no member of type
  `OpenGLRenderer`, no factory. The only "reaches" are the `Shader` friend declaration
  (`Shader.hpp:30`) and the CMake source list.
- **`OpenGLHelper` is used only by `OpenGLRenderer`** (9 call sites, all inside
  `OpenGLRenderer.cpp`). Nothing else links it. Vulkan *copied* the matrix conventions
  instead (`VulkanRenderer.cpp:47` comment: "矩陣慣例與 OpenGLHelper 一致(該檔只在
  OPENGL build 編譯,故在此複製)"); `Scene/TransformMath.hpp:10` and
  `TransformComponent.hpp:9` reference the convention in comments only — no code
  dependency.
- **Legacy `IRenderer` callers:** the only remaining `renderer.Render(Scene&)` /
  `renderer.Render(GUILayout&)` callers are the **DirectX-only** window path
  `src/source/Display/Windows/Window.cpp:106,108` (compiled only under
  `DIRECTX_ENABLED`, `src/CMakeLists.txt:67-68`). The GLFW path (`Panel::Render` →
  `Window::Render`) is fully Frame-driven; `Panel.cpp:25-29` explicitly notes the
  legacy `LoadGUILayout`/`Render(layout)` route was abandoned because it was a no-op
  under Vulkan.
- **Bonus finding — DirectX build is also Frame-broken (static analysis):**
  `Renderer::Execute` (`Renderer.cpp:78-80`) calls `specializedRenderer.Execute(frame)`
  unconditionally, but `DirectXRenderer` implements only `IRenderer` (no `Execute` in
  `DirectXRenderer.hpp`). Since `Renderer.cpp` is in the unconditional source list
  (`src/CMakeLists.txt:27`), a `DIRECTX_ENABLED` build would fail to compile
  `Renderer.cpp` today. This is derived by reading the code, not verified on a Windows
  build — but it shows the `Renderer` wrapper is Vulkan-centric and both non-Vulkan
  backends are out of sync with the Frame architecture.

## B. Compile feasibility — exactly what is missing today

To compile the OpenGL backend *and link it into the app*, in dependency order:

1. **GLAD loader** — generate `glad.h` + `glad.c` from
   https://glad.dav1d.de/ (OpenGL 3.3 Core, "Generate a loader") and place in
   `src/include/3rdparty/glad/` (`README.md:32-42`; CMake warning at
   `src/CMakeLists.txt:84`). Without it: `#include <glad/glad.h>`
   (`OpenGLRenderer.hpp:15`) fails. **This is the only blocker the issue/README names.**
2. **`Renderer.hpp` OpenGL branch** — add `#elif OPENGL_ENABLED` to both the include
   block (`Renderer.hpp:8-12`) and the member block (`Renderer.hpp:19-23`) so
   `specializedRenderer` exists. Without it `Renderer.cpp` cannot compile (F1). *Not
   mentioned in the issue.*
3. **Runtime (not compile) blockers that survive both fixes:**
   - `GLFW/WindowManager.cpp:14-15` runs `glfwInit(); glfwWindowHint(GLFW_CLIENT_API,
     GLFW_NO_API);` **unconditionally** in the singleton constructor. The singleton is
     constructed at `ChiaApp.cpp:19` (`WindowManager::GetSingleton().ConstructWindow…`)
     *before* any window exists, so every later `glfwCreateWindow` gets a
     context-less window. `Window::Show` (`GLFW/Window.cpp:160`) sets **no** client-API
     or context-version hints (GLFW hints persist until changed).
   - `OpenGLRenderer::Initialize` (`OpenGLRenderer.cpp:86-99`) ignores its `pWindow`
     parameter and creates a **second, private GLFW window** via
     `OpenGLHelper::CreateWindow` (`OpenGLHelper.cpp:23-44`: `glfwCreateWindow` →
     `glfwMakeContextCurrent` → `gladLoadGLLoader`). With the `GLFW_NO_API` hint active,
     context creation fails, `gladLoadGLLoader` fails, the window is destroyed and
     `Initialize` returns false → `Window::Show` fails (`GLFW/Window.cpp:178-179`) →
     `ChiaApp::Execute` exits with failure (`ChiaApp.cpp:24-25`).
   - Contrast with Vulkan: `VulkanRenderer::CreateSurface` uses **the Window's**
     handle (`VulkanRenderer.cpp:243 glfwCreateWindowSurface(…, pWindow->GetHandle(), …)`).
4. **Already available (no work needed):** `glm 0.9.9.8` and `glfw 3.3.8` via
   `FetchContent` (root `CMakeLists.txt:36-47`, only when `NOT DIRECTX_ENABLED`);
   `find_package(OpenGL REQUIRED)` + `OpenGL::GL` link (`CMakeLists.txt:56`,
   `src/CMakeLists.txt:164-168`); `stb_image.h` is vendored
   (`src/include/3rdparty/stb_image.h`); `OpenGLHelper.cpp` is fully wrapped in
   `#ifdef OPENGL_ENABLED` (`OpenGLHelper.cpp:8,166`) and `OpenGLRenderer` is only
   compiled in the OpenGL branch, so no cross-contamination of the Vulkan build.

**Bottom line:** three missing pieces (GLAD, `Renderer.hpp` branch, GLFW
de-Vulkanization), of which the issue names only one.

## C. Full-implementation scope — what "make OpenGL render GUI end-to-end" costs

### (a) MINIMAL legacy path — implement `LoadGUILayout` + `Render(GUILayout&)` (~0.5–1 day)

Reuse what already exists in `OpenGLRenderer.cpp`:
`LoadGUILayout` iterates `layout.GetLayers()` → `layer.GetComponents()` and calls
`LoadRenderable(*component, Scene::SceneType::GUI)` for each; `Render(GUILayout&)`
calls `RenderRenderable(*component)` for each. Layers and components are
`IRenderable`s (`GUILayer : public IGUI : public Rectangle : public IRenderable` —
`GUILayer.hpp:14`, `IGUI.hpp:18`, `Primitives.hpp:30`), so this is the same trick
`VulkanRenderer::Execute`'s `DrawGUILayout` case uses (`VulkanRenderer.cpp:1102-1121`).
The default shader already carries a `gui` vertex attribute and texture-modulate
support (`OpenGLRenderer.cpp:11-69`), and `CreateInputBuffer` sets `gui = 1` for
`SceneType::GUI` (`OpenGLRenderer.cpp:503`).

**But who calls it?** On the GLFW path, nobody. `Window::Render` records a `Frame`
and calls `renderer.Execute(frame)` (`GLFW/Window.cpp:220`); `Panel::Render` is just
`Window::Render` (`Panel.cpp:33-37`). To make the legacy path reachable you would have
to **revert `GLFW/Window.cpp` to the legacy render loop** (like the DirectX-only
`Windows/Window.cpp:106-108`), throwing away the Frame pipeline (camera command,
`DrawText` labels via `GUIFrameProjector`, transform stack, deterministic
serialization — `Frame.hpp:16-29`). That is a regression of the P2/P6/P7 architecture
(`docs/ARCHITECTURE.md:16-17, 164-213`), not a fix. Verdict: **dead-code-producing
work; not recommended.** (This is the minimal fix for the `:212`/`:309` TODOs only in
the literal sense.)

### (b) FRAME path — port `VulkanRenderer::Execute(Frame)` semantics (~2–4 weeks)

OpenGL already has ~60% of the primitives (`LoadRenderable`/`RenderRenderable`/UBO
matrices/embedded shader). Port inventory, diffed against `VulkanRenderer.cpp`:

| Frame command | Vulkan reference | OpenGL work | size |
|---|---|---|---|
| `BeginFrame` | `VulkanRenderer.cpp:1187-1228` (acquire, render pass, clear `(0.02,0.04,0.08,1)`) | `glClearColor` + `glClear` — note `OpenGLRenderer::Render(Scene&)` today does **no clear at all** (`OpenGLRenderer.cpp:286-303`) | ~10 LOC |
| `SetCamera` | `VulkanRenderer.cpp:1092-1094` | store `pActiveCamera`; refresh `viewMatrix`/`projMatrix` (helpers exist, `OpenGLHelper.cpp:139-153`) | ~10 LOC |
| `DrawRenderable` | `VulkanRenderer.cpp:1095-1101` | `LoadRenderable` + `RenderRenderable` (exist) | ~5 LOC |
| `DrawGUILayout` | `VulkanRenderer.cpp:1102-1121` | layers+components as renderables (exist; same trick as (a)) | ~20 LOC |
| `EndFrame` | `VulkanRenderer.cpp:1230-1276` (submit + present) | `glfwSwapBuffers(pWindow)` + `glfwPollEvents` | ~5 LOC |
| `PushTransform`/`PopTransform` | `VulkanRenderer.cpp:1127-1141` (glm mat4 stack, depth 64) | identical logic + `DynamicArray<glm::mat4>` | ~25 LOC |
| `BindMaterial` | `VulkanRenderer.cpp:1142-1146` (records id; pipeline TODO) | same stub | ~5 LOC |
| `DrawMesh` | `RegisterMeshGeometry`+`RecordMeshDrawCommands` (`VulkanRenderer.cpp:2035-2125`, ~90 LOC, content-hash `meshCache`) | new mesh cache keyed by `meshId` → VAO/VBO/EBO + draw with stack-top world | ~90 LOC |
| `SetViewport` | `VulkanRenderer.cpp:1153-1172` | `glViewport` + `glScissor` | ~10 LOC |
| `DrawText` | `CreateFontAtlasTexture`+`RecordTextDrawCommands` (`VulkanRenderer.cpp:2144-2283`, ~140 LOC: GlyphAtlas→texture, `TextLayout` quads→vertex buffer, textured quads, world matrix) | `GlyphAtlas`/`TextLayout` are engine-side (shared); GL: atlas→`glTexImage2D`, quads→VBO, reuse default shader's `uTexture`+`uUseTexture`+`cmode&1` modulate (matches Vulkan's comment `VulkanRenderer.cpp:2262`) | ~150 LOC |
| Non-GUI gaps | — | (1) conditional `GLFW_CLIENT_API` hint or `glfwDefaultWindowHints()` (`WindowManager.cpp:15`); (2) `Window::Show` must set 3.3-core hints before `glfwCreateWindow` (`GLFW/Window.cpp:160`; `OpenGLHelper::InitGLFW` already sets them at `OpenGLHelper.cpp:15-17` but only applies to the renderer's own window); (3) render into `pWindow->GetHandle()` instead of a private second window (`OpenGLRenderer.cpp:91-94`); (4) `Renderer.hpp` `OPENGL_ENABLED` branch + `OpenGLRenderer : public IRenderer, public IFrameExecutor` + `Execute`; (5) resize → `glViewport` already handled (`OpenGLRenderer.cpp:252-259`) | ~40 LOC |

Rough total: **~400–500 LOC of new/ported code** in `OpenGLRenderer.*` (+ a small
`WindowManager.cpp`/`Window.cpp` edit), then **desktop GL testing** (the repo's GPU
verification is Vulkan-specific: `scripts/vlm_verify.py`, `docs/agents/visual-verification.md`)
and the transform-stack/mesh/text contract tests are executor-agnostic but currently
mock/Vulkan-only (`test/System/RendererContractTest.hpp:65-76`). Realistic effort:
**2–4 weeks** for one engineer familiar with the Vulkan executor, including
validation. This also permanently doubles every future Frame-command change
(`PushTransform`→Vulkan+GL).

## D. Deprecation scope — concrete change plan

1. **CMake** (root `CMakeLists.txt:26-30`): change the `OPENGL_ENABLED` option
   description to `"Use OpenGL as the rendering engine. (DEPRECATED)"` and emit a
   `message(WARNING …)` when it is enabled (same pattern as the GLAD warning,
   `src/CMakeLists.txt:84`). Optionally add a status line naming Vulkan as the
   supported backend.
2. **README.md**: fix the backend table/note — `README.md:9-15` lists OpenGL ✅ on
   all platforms and line 15 says *"Currently only the DirectX backend has a working
   OpenGL implementation; Vulkan and OpenGL backends are stubs"*, which is stale
   (Vulkan is the working reference backend today; DirectX is the legacy one). Mark
   the OpenGL rows deprecated; retitle the `### OpenGL Backend Setup` section
   (`README.md:32-42`) "OpenGL Backend (deprecated)"; update the build-options table
   (`README.md:57-59`).
3. **Source comments**: add a `// DEPRECATED — …` header block to
   `OpenGLRenderer.hpp`, `OpenGLRenderer.cpp`, `OpenGLHelper.hpp`, `OpenGLHelper.cpp`
   stating: not wired into the Frame/IFrameExecutor architecture, not compiled by
   default, kept only as the non-Vulkan fallback.
4. **Keep or delete the files?** **Keep** for now. The `else()` branch
   (`src/CMakeLists.txt:79-94`) is the only consumer of `OpenGLHelper`/`OpenGLRenderer`,
   and deleting them requires deleting the `else()` branch, which in turn makes
   Vulkan mandatory (`find_package(Vulkan REQUIRED)`, `CMakeLists.txt:53`) and breaks
   machines without a Vulkan SDK. Keep the four files as deprecated fallback; a later
   "Vulkan-only" migration issue can delete the branch + files together.
5. **Confirm nothing else depends on the OpenGL fallback:** the Vulkan branch lists
   its own copies of `GLFW/Window.cpp` + `GLFW/WindowManager.cpp`
   (`src/CMakeLists.txt:72-73`), so removing the `else()` branch does **not** break
   the Vulkan build. `OpenGLHelper` references elsewhere are comments only
   (`VulkanRenderer.cpp:47`, `TransformMath.hpp:10`, `TransformComponent.hpp:9`).
   The `#ifdef OPENGL_ENABLED` guard in `Shader.hpp:29-31` is inert when the option is
   off and can be removed alongside.
6. **Follow-up (separate issue):** the DirectX build is also Frame-broken
   (`Renderer.cpp:80` vs `DirectXRenderer` without `Execute`) — decide DirectX's fate
   explicitly rather than letting it rot next to OpenGL.

## E. Recommendation

**Deprecate the OpenGL backend (issue option 2).**

One-line rationale: the v2 architecture is Frame + `IFrameExecutor`
(`docs/ARCHITECTURE.md:16-17, 186-213`), Vulkan is the only executor, the GLFW window
layer is Vulkan-tuned (`GLFW_NO_API` hardcoded), GLAD is not vendored, the backend has
not been touched since its initial commit (`git log`: 1 commit for
`OpenGLRenderer.cpp`/`OpenGLHelper.cpp` vs 12 for `VulkanRenderer.cpp`), and the
minimal legacy fix would be unreachable dead code while the Frame port is a multi-week
second-executor project with no stated demand.

Supporting facts (see sections above): no construction site exists (F1, §A); the
OpenGL build cannot compile today (F1+F2, §B); it would fail at runtime even after
compiling (§B.3); `Window::Render` is Frame-driven and cannot drive it (F3); the GUI
is already rendered end-to-end on Vulkan through the exact renderable-reuse trick the
OpenGL port would need (F3/F4, §C); README already admits staleness (`README.md:15`);
the issue itself leans deprecation "if Vulkan is strategic", and Vulkan demonstrably
is (it is the reference executor, has text rendering, swapchain resize handling, and
the only automated renderer-contract surface).

### Effort estimates

| Option | Effort | Notes |
|---|---|---|
| Implement minimal legacy GUI (`LoadGUILayout` + `Render(GUILayout&)`) | **0.5–1 day** | Trivial code, but unreachable from `Window::Render` without reverting GLFW to the legacy loop (a Frame-architecture regression). Not recommended. |
| Implement full Frame port (`Execute` + text + mesh cache + GL context fixes) | **2–4 weeks** | ~400–500 LOC port + GLFW de-Vulkanization + desktop GL verification; doubles future Frame-command maintenance. |
| Deprecate (CMake warning + README + header comments; keep files) | **0.5–1 day** | Zero risk to the Vulkan build; deletion + Vulkan-mandatory migration is a separate follow-up. |

**Recommended next action:** take the deprecation path now — (1) mark
`OPENGL_ENABLED` deprecated in `CMakeLists.txt` with a warning message, (2) correct
the stale README backend table/section, (3) add top-of-file deprecation comments to
the four OpenGL files, (4) keep the files as the non-Vulkan fallback, and (5) file a
follow-up issue for the DirectX `Execute` break so all three backends' status is
explicit.

---

## Evidence index (file:line)

| Claim | Evidence |
|---|---|
| No OpenGL branch in `Renderer` | `src/include/Display/Renderer.hpp:8-12, 19-23` |
| `Renderer.cpp` calls `specializedRenderer.*` incl. `Execute` | `src/source/Display/Renderer.cpp:80` |
| GLAD missing; CMake warns only | `src/CMakeLists.txt:79-94`; `src/include/3rdparty/` (only `stb_image.h`) |
| GLAD include path wired | `CMakeLists.txt:58`; `OpenGLRenderer.hpp:15` |
| FetchContent glm/glfw | `CMakeLists.txt:36-47`; OpenGL link `src/CMakeLists.txt:164-168` |
| Frame-driven `Window::Render` | `src/source/Display/GLFW/Window.cpp:197-221` |
| `OpenGLRenderer : public IRenderer` only | `src/include/Display/OpenGLRenderer.hpp:22` |
| Vulkan implements `IFrameExecutor` | `src/include/Display/Vulkan/VulkanRenderer.hpp:17, 230-231`; `VulkanRenderer.cpp:1075-1185` |
| TODOs | `OpenGLRenderer.cpp:212, 309` |
| `Window` owns `Renderer` | `src/include/Display/Window.hpp:44`; `GLFW/Window.cpp:117` |
| App only drives WindowManager | `src/source/App/App.cpp:51-57` |
| Singleton sets `GLFW_NO_API` | `src/source/Display/GLFW/WindowManager.cpp:14-15` |
| `Window::Show` no context hints | `GLFW/Window.cpp:160`; renderer init `:178` |
| OpenGLRenderer creates its own window | `OpenGLRenderer.cpp:86-99`; `OpenGLHelper.cpp:23-44` |
| Vulkan renders into Window's handle | `VulkanRenderer.cpp:243` |
| Legacy callers are DX-only | `src/source/Display/Windows/Window.cpp:106-108`; `src/CMakeLists.txt:67-68` |
| GUI components are IRenderables | `GUILayer.hpp:14`, `IGUI.hpp:18`, `Geometry/Primitives.hpp:30` |
| Panel moved GUI to Frame | `src/ChiaApp/Panel.cpp:25-29` |
| DirectX lacks `Execute` | `src/include/Display/DirectX/DirectXRenderer.hpp` (no Execute) |
| OpenGL never modified since creation | `git log --oneline -- src/source/Display/OpenGLRenderer.cpp` (1 commit, `3ff016a`) |
| README stale backend note | `README.md:9-15, 32-42, 57-59` |
| Architecture: Frame = 唯一貨幣 | `docs/ARCHITECTURE.md:16-17, 164-213` |

*Investigated 2026-08-10 at commit `e968787` on branch `pa/issue-54-agent-pilot`.
Read-only — no source files modified, no commit/push.*
