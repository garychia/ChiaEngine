# DirectX Backend Assessment (issues #53 / #63)

## Why

The DirectX backend (`src/source/Display/DirectX/DirectXRenderer.cpp`, 712 lines)
accumulated two independent problems:

- **#53** — ~52 stub markers (TODO / `return true` / `return false` /
  not-implemented patterns) in 712 lines: dead-code weight, misleading README,
  maintenance tax.
- **#63** — Frame-broken build: `Renderer::Execute(const Frame&)` calls
  `specializedRenderer.Execute(frame)` unconditionally, but `DirectXRenderer`
  implements only the legacy `IRenderer` interface — no `IFrameExecutor`, no
  `Execute`. A `DIRECTX_ENABLED` build fails to compile.

This assessment decides deprecate-vs-complete, mirroring the OpenGL decision
(#54, `docs/agents/opengl-backend-assessment.md`).

## Verified findings (with evidence)

### F1 — `DirectXRenderer` implements only the legacy `IRenderer` interface ✅

`src/include/Display/DirectX/DirectXRenderer.hpp:10`:
`class DirectXRenderer : public IRenderer` — no `IFrameExecutor`, no
`Execute(const Frame&)`. Contrast `VulkanRenderer.hpp:17`:
`public IRenderer, public IFrameExecutor`.

### F2 — `Renderer::Execute` cannot compile under `DIRECTX_ENABLED` ✅

`src/source/Display/Renderer.cpp:78-80`:
```cpp
bool Renderer::Execute(const Frame &frame)
{
    return specializedRenderer.Execute(frame);
}
```
`specializedRenderer` is bound to `DirectXRenderer` in a DIRECTX_ENABLED build
(`src/CMakeLists.txt:69-77` appends DirectX sources) → no `Execute` member →
compile error. Not build-verified (no Windows build env on this box), but the
static break is unambiguous.

### F3 — The Frame architecture is the only renderer contract since P2/P6/P7 ✅

The GLFW window path is fully Frame-driven (`GLFW/Window.cpp:197-221`). The
**only remaining legacy `IRenderer` callers** are the DirectX window path —
`Windows/Window.cpp:106-108` (`renderer.Render(*pScene)` /
`renderer.Render(*pGUILayout)`). Precedent: OpenGL was deprecated (#54,
ca689f9) for the same reason.

### F4 — DirectX is Windows-only with no CI coverage ✅

`src/CMakeLists.txt:17-20`: `cmake_dependent_option(DIRECTX_ENABLED ... ON "WIN32" OFF)`.
Windows-only (win32 WndProc in `Windows/Window.cpp`). CI runs Linux
(GitHub Actions, #51) — DirectX never compiles in CI, so the #63 break went
unnoticed.

### F5 — Stub density confirmed ✅

`DirectXRenderer.cpp`: ~52 stub markers (TODO / `return true` / `return false`
/ not-implemented patterns) across 712 lines — the backend does not implement a
usable render path even where it compiles.

## A. Reachability

- `DirectXRenderer` is constructed only in DIRECTX_ENABLED builds
  (`src/CMakeLists.txt:69-77`). Default CMake on non-Windows does not enable it.
- Nothing else references `DirectXRenderer` (mirrors OpenGL F5: the renderer
  lives in `Window`, and the DirectX path is self-contained in
  `source/Display/Windows/`).

## B. Compile feasibility — what is missing today

Exactly the `Execute(const Frame&)` contract (and any Frame commands it must
serve). Even after a compile guard, the backend does not implement the Frame
command set (BeginFrame / SetCamera / DrawRenderable / DrawGUILayout /
transform stack / DrawText), so it cannot render the v2 architecture.

## C. Full-implementation scope — what "complete DirectX" costs

### (a) MINIMAL legacy path — keep legacy `IRenderer` + fix compile

Wrap `Renderer::Execute` in `#ifdef VULKAN_ENABLED` (returns false otherwise),
keep the legacy `Render(Scene&)` / `Render(GUILayout&)` path alive. ~1 hour,
but leaves 52 stubs and the misleading "Windows ✅" story.

### (b) FRAME path — port `VulkanRenderer::Execute(Frame)` semantics to D3D11

Second full executor (~mirrors the OpenGL 2-4 week estimate): swapchain + D3D
device/context, shaders for the default pipeline, descriptor/uniform mapping,
text rendering via GlyphAtlas path, GUI layout. Big effort, Windows-only value,
no CI coverage on this box.

## D. Deprecation scope — concrete change plan (chosen)

1. `CMakeLists.txt`: DIRECTX_ENABLED option description → DEPRECATED, pointing
   at this assessment.
2. `src/source/Display/Renderer.cpp`: guard `Execute` with `#ifdef VULKAN_ENABLED`
   (legacy backends return false) → DIRECTX_ENABLED builds compile again.
3. `src/include/Display/DirectX/*` + `src/source/Display/DirectX/*`:
   deprecation header comments (mirror OpenGL).
4. `README.md`: backend table — DirectX ⚠️ deprecated, note updated.
5. Keep files (legacy reference), no default-build exclusion needed (already
   non-default on non-Windows).

## E. Recommendation

**Deprecate** (option 2 of #53, option 2 of #63). Vulkan is the strategic
backend (evidence: sole Frame executor, runs ChiaApp, CI-covered on Linux);
OpenGL was deprecated for the identical reason and that decision stuck. A
Windows port has no CI coverage on this box, so completing DirectX would carry
unverifiable weight and ongoing maintenance tax.

Resolves #53 and #63 together: #53 (deprecate-or-complete) and #63
(Frame-broken build — deprecation makes the compile guard permanent policy).

## Evidence index (file:line)

- `src/include/Display/DirectX/DirectXRenderer.hpp:10` — legacy IRenderer only
- `src/source/Display/Renderer.cpp:78-82` — unconditional `Execute` call
- `src/CMakeLists.txt:17-20` — DIRECTX_ENABLED Windows-only option
- `src/CMakeLists.txt:69-77` — DirectX source list
- `src/source/Display/Windows/Window.cpp:106-108` — legacy `Render` callers
- `src/source/Display/GLFW/Window.cpp:197-221` — Frame-driven reference path
- `docs/agents/opengl-backend-assessment.md` — OpenGL precedent (#54)