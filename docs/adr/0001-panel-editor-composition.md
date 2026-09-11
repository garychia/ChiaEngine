# ADR-0001: Panel — Editor Composition & Layout

- Status: Proposed
- Date: 2026-08-28
- Area: `src/ChiaApp/` (Panel, SceneWindow, PanelLayout), `src/include/Display/GUI/`
- Supersedes: — (first ADR)
- Related issues: #60 (hierarchy step 1), #68 (inspector step 2), #64 (input dispatch), #67 (GUI NDC clip)

## Context

`Panel` is the engine's editor window. As of #60/#68 it composes a top toolbar, a left
hierarchy sidebar, and a right inspector. The architecture already forces one non-obvious
decision: **the viewport is a separate OS window**, not a GUI region. This ADR records the
composition model and the abstractions needed to grow the editor without duplicating state
or fighting the Frame/swapchain design.

### Grounded current state (read before changing anything)

- `Panel : Window` — top-level editor window. Owns a `PanelLayout : GUILayout`.
  `Window::Render` (GLFW) serializes the layout to a `Frame` via
  `frame.DrawGUILayout(*pGUILayout)` + `GUIFrameProjector::ProjectLabels` (P6/P7e — Frame is
  the only render currency; labels become `DrawText`).
- `SceneWindow : Window` — a **separate GLFW child window**, constructed via
  `WindowManager::ConstructChildWindow<SceneWindow>` and positioned centered inside `Panel`
  (`SetPosition((w-vw)/2, TopBarHeight)`, `SetSize(vw, h-TopBarHeight)`). It owns its own
  `Renderer` + swapchain and draws the 3D cube demo. Its input (F5/F6 replay, WASD look)
  is recorded into `SimRecorder`.
- `PanelLayout` holds three layers: `TopPanelBar` (30px), `pHierarchyLayer` (180px left,
  dark), and `pInspector` (200px right, created in #68). Hierarchy rows are rebuilt from
  `SceneSystem::GetHierarchy(nodes, depths)`; clicking a row fires `HierarchyRow::rowClicked`
  → `Panel::OnHierarchyRowClicked`.
- Selection is **duplicated**: `Panel::selectedEntity` (canonical `Entity`) and
  `InspectorLayer::selectedEntityIndex` (mirror `uint32`). Both updated on row click.
- Inspector edits write directly into `SceneSystem.world` `TransformComponent` via
  `EditTransformComponent` — fully headless-testable (`InspectorTest`). The edit path
  **bypasses `SimRecorder`** (it is editor-time, not sim-time).

### Why this matters

The "panel" framing in #60 ("right side of the Panel") assumed GUI regions *inside one
window*. The implementation diverged to **multi-window**. That divergence is not a bug — it
is the architecturally-aligned choice (see D1). But the split introduced two design debts
(duplicated selection, a god-object layout) that block clean growth to dockable panes,
gizmos, and undo/redo. This ADR settles both.

## Decisions

### D1 — Keep multi-window composition (viewport = child `Window`, not a GUI region)

**Decision.** The 3D viewport remains a separate `SceneWindow` child window centered on top
of `Panel`. GUI panels (toolbar, hierarchy, inspector) live in the `Panel` window and occupy
the gutters around the centered viewport. No in-viewport GUI overlay (gizmos/debug text) in
this ADR's scope.

**Rationale.**
- Each `Window` owns exactly one `Renderer` + one swapchain + one `Frame`. The Vulkan
  renderer's `DrawGUILayout` path already clips GUI-to-NDC at the swapchain level (#67). A
  single-window design would force the 3D scene and the GUI to share one `Frame`/swapchain
  and require correct draw-order + NDC clipping of GUI over 3D — i.e. it *re-opens* #67.
- Multi-window gives correct layering for free: the child 3D window is simply drawn above the
  parent's gutter GUI by the window composer. No per-pixel GUI/3D blend.
- Input routing is already per-window (`OnMouseInputReceived` per `Window`); the model fits.

**Alternatives considered.**
- *Single window, viewport as a `GUILayer` region.* Rejected now: requires solving #67
  properly (GUI NDC clip over 3D) and merging two `Frame` streams into one swapchain.
  Revisit only if gizmo-over-viewport or drag-drop-from-hierarchy becomes required (D6).
- *Single window, 3D rendered to a texture, GUI draws it in a region.* Rejected: adds an
  offscreen render target + blit path for no current benefit.

### D2 — Single selection source of truth: a `Selection` model owned by `Panel`

**Decision.** Introduce `Selection { uint32_t entityIndex; bool hasSelection; }` (extendable
later with a focused component/property). `Panel` owns the canonical `Selection`.
`InspectorLayer` receives a `const Selection*` (or a getter) and reads it; it no longer
stores `selectedEntityIndex`. `Panel::OnHierarchyRowClicked` updates only `Selection`.

**Rationale.** Two copies of selection (D1's current state) violate "one source of truth"
and already drift: `InspectorLayer::SelectEntity` must be called in lockstep with
`Panel::selectedEntity =`. A single model removes the mirror and the sync bug class.

**Migration.** `InspectorLayer::SelectEntity(idx)` becomes `InspectorLayer::SetSelection(const
Selection*)`. `GetFieldRows()` / `ApplyEdit` unchanged.

### D3 — `PanelLayout` becomes a dock manager; introduce `PanelPane : GUILayer`

**Decision.** `PanelLayout` stops being a god-object. It owns **named dock regions**
(`TopBar`, `LeftDock`, `RightDock`, `CenterViewport`) and a resize/region contract
(D5). Each visible surface (toolbar, hierarchy, inspector, future console/asset browser) is a
`PanelPane`.

`PanelPane : GUILayer` adds: a title bar, collapse state, and a dock-region assignment. It is
the unit of layout, not `GUILayer` directly. Hierarchy/Inspector/Toolbar become `PanelPane`s.

**Rationale.** Steps 3–4 of #60 (camera controls, undo/redo) and later panels (console,
assets, play-mode toolbar) all need the same chrome (title, collapse, resize). Encoding that
once in `PanelPane` removes per-panel duplication and gives a uniform resize path.

**Migration.** `PanelLayout::BuildHierarchy` / `CreateInspector` construct `PanelPane`s into
`LeftDock` / `RightDock`. `GetHierarchyRows` / `GetInspector` stay as accessors.

### D4 — `EditorSession`: window-agnostic editor state

**Decision.** Extract editor state from `Panel` into an `EditorSession` (owns `SceneSystem*`,
`CameraController*`, `SimRecorder*`, `Selection`, and the `PanelPane` registry). `Panel` is
the *View*: it constructs `EditorSession` and forwards input/resize to it. `EditorSession`
contains no GLFW/`Window` types.

**Rationale.** The engine's strength is headless testing (InspectorTest proves it). Keeping
editor logic (selection, pane registry, command stack D5, inspector edits) free of `Window`
means the whole editor can be unit-tested without a GPU/GLFW context — consistent with the
Sim/View split already used by `SceneSystem` + `InspectorLayer`.

**Migration.** `Panel::Initialize` builds `EditorSession`; `Panel::Render` calls
`session.Update()` then `Window::Render`. `OnHierarchyRowClicked` becomes
`session.Select(entity)` + highlight refresh.

### D5 — Undo/redo via an `EditorCommand` stack, **separate** from `SimRecorder`

**Decision.** Inspector edits (and future property edits) are recorded as reversible
`EditorCommand`s on a per-session undo stack (`Command<EditorState>` with `Apply`/`Revert`).
`SimRecorder` stays purely for *sim/gameplay* replay (deterministic input replay, F5/F6). The
two are not merged.

**Rationale.** Inspector mutations are editor-time, not sim-time. They write directly into
`world` and bypass `SimRecorder` by design. Forcing them through `SimRecorder` would pollute
gameplay replay with editor actions. A dedicated command stack (command pattern) is the
standard, correct separation and reuses the engine's existing `Event`/`SharedPtr` idioms.

**Migration.** `InspectorButton::OnClicked` → `session.Execute(TransformEditCommand(...))`
instead of calling `EditTransformComponent` directly. `EditTransformComponent` becomes the
command's `Apply` body.

### D6 — Out of scope (explicit non-decisions for this ADR)

- Transform gizmos / in-viewport overlays (needs D1 single-window or a texture-blit path).
- Dock drag-and-drop / floating panes (D3's `PanelPane` is the prerequisite, not the impl).
- Multi-select, custom component inspectors (#68 out-of-scope list).
- Asset browser, console, play-mode transition UI.

These are sequenced *after* D1–D5 land.

## Layout regions & resize contract (D3/D5)

`PanelLayout` computes a `PanelRegions` struct from window size (replace today's magic
numbers 30/180/200):

```
TopBar:    rect(0, 0, W, TopBarHeight)
LeftDock:  rect(0, TopBarHeight, SidebarWidth, H - TopBarHeight)
RightDock: rect(W - InspectorWidth, TopBarHeight, InspectorWidth, H - TopBarHeight)
CenterViewport: rect((W - vw)/2, TopBarHeight, vw, H - TopBarHeight)   // SceneWindow child
```
`Panel::OnWindowResized` recomputes `PanelRegions`, calls `layout.SetRegions(...)` (which
repositions docks + `SceneWindow`), and `RefreshDepths()` (the #67 fix already requires this
after dynamic panel changes). All region constants live in `PanelLayout` as named statics.

## Consequences

- **Positive.** Single selection model (D2) removes a sync bug class; `PanelPane` (D3) makes
  new panels a 10-line addition; `EditorSession` (D4) keeps editor logic headless-testable;
  undo/redo (D5) gets a clean home without corrupting sim replay.
- **Negative / cost.** D3–D5 are refactors of working #60/#68 code; they should land behind
  the existing headless tests (extend `InspectorTest`, add `EditorSessionTest`) so behavior is
  preserved. D1 is *no change* — zero risk.
- **Risk.** If gizmo-over-viewport is later required, D1 must be revisited (D6). Acceptable:
  D3/D4 do not preclude that future work.

## Follow-up issues to file

1. Extract `Selection` + remove `InspectorLayer::selectedEntityIndex` (D2).
2. Add `PanelPane`, refactor `PanelLayout` to dock manager (D3).
3. Extract `EditorSession`; move `Panel` editor logic into it; add `EditorSessionTest` (D4).
4. Add `EditorCommand` stack; route inspector edits through it; add undo/redo keys (D5).
5. Promote region magic numbers to `PanelRegions` + `SetRegions` (D5 resize contract).
