# ChiaEngine Determinism Manifesto

## Why this document exists

Unreal and Unity treat determinism as a concession: rollback netcode, frame-perfect replays, and lockstep simulation are bolted onto engines that were never designed for them. ChiaEngine is built from the ground up to be deterministic. This is not a feature flag — it is the architecture.

## Core principles

### 1. Fixed timestep is mandatory

`World` advances in discrete, equal ticks. No variable-dt interpolation. If you cannot express gameplay in fixed steps, you cannot replay it.

**Rule:** All simulation logic runs in `World::FixedUpdate`-driven module updates. `dt` is a compile-time constant per build configuration, never a runtime measurement from the OS clock.

### 2. SimInput is the only input source

`SimInput` (`src/include/System/Module/SimInput.hpp`) is trivially copyable by design. It is the single interface between the real world (keyboard, mouse, network) and the simulation.

**Rule:** No module reads raw GLFW, raw sockets, or raw OS events during `FixedUpdate`. All input flows through `SimRecorder` → `EngineContext::ResolveService<SimInput>()` → consumer modules.

**Rule:** `SimInput` must remain trivially copyable (`std::is_trivially_copyable_v<SimInput> == true`). Adding a `std::string`, `std::vector`, or pointer member violates this principle.

### 3. Hash is the source of truth for state equivalence

`World::Hash()` (`src/include/System/World/World.hpp`) and `ComponentPool::Hash()` (`src/include/System/World/ComponentPool.hpp`) use `HashComponent<T>` (`src/include/Data/Hash.hpp`), an ADL customization point. Two worlds with identical logical state must produce identical hashes on any compiler, any platform, any optimization level.

**Rule:** Every component type used in a replay-critical path must either:
- Be padding-free (no implicit padding bytes), verified by `static_assert` on `sizeof(T)` and member `offsetof` checks, **or**
- Provide a `HashComponent<T>` specialization that hashes only significant members and skips padding.

**Rule:** `ComponentPool::Add()` no longer guarantees zeroed padding (see `src/include/System/World/ComponentPool.hpp:62-77`). The old `memset`+assignment pattern cannot defeat whole-object copy on Clang. Rely on `HashComponent<T>` for determinism, not on padding hygiene.

### 4. Replay is a first-class workflow

`SimRecorder` (`src/include/System/Module/SimRecorder.hpp`) records `SimInput` and feeds it back on replay. Desync is detected by comparing `World::Hash()` at checkpoints.

**Rule:** Attachment order matters. `SimRecorder` must be attached to `EngineContext` before any module that reads `SimInput`. Add a debug assertion in `SimRecorder::Attach` if it detects late attachment.

**Rule:** Replay tests must run on every CI leg (gcc, clang, appleclang) with identical `SimInput` sequences and assert identical `World::Hash()` sequences.

### 5. No hidden state, no implicit magic

If a subsystem caches, buffers, or defers work, that state must be serializable into the replay stream or explicitly excluded from the deterministic core.

**Rule:** `AssetManager` (`src/include/System/Asset/AssetManager.hpp`) async loading is allowed, but asset *availability* must not affect `World::Hash()`. Async loads complete outside the simulation tick or are excluded from hash input.

**Rule:** Audio (`src/source/System/Audio/AudioSystem.cpp`) plays back on the real-time clock; `ma_sound` state is not part of `World::Hash()` and does not affect replay equivalence.

## What ChiaEngine is NOT

- **Not Unreal Engine.** No Blueprints, no visual scripting, no Slate UI framework, no 1M-line monolith. If you can't read the whole engine source in an afternoon, it's too big.
- **Not Unity.** No C#, no Mono/IL2CPP, no garbage collector, no asset store, no editor bloat. Pure C++17 RAII.
- **Not a general-purpose game engine.** It is a deterministic simulation engine with a Vulkan renderer attached. The simulation is the product; rendering is a view on top.

## Non-negotiables (enforced in CI)

| Check | Location | Enforcement |
|-------|----------|-------------|
| `SimInput` is trivially copyable | `src/include/System/Module/SimInput.hpp:10` | `static_assert` in CI build |
| Replay hash identical across compilers | `test/System/ReplayTest.hpp` | Run on gcc, clang, appleclang |
| `HashComponent<T>` for padded types | `src/include/Data/Hash.hpp:26` | Code review gate |
| `always()` on gating CI jobs | `.github/workflows/ci.yml` | YAML lint + gate |
| ASan/UBSan clean on main | `.github/workflows/ci.yml` sanitizers job | Must pass, not "expected during development" |

## Violations

If you add:
- A `std::string` to `SimInput` → revert.
- A `new` without matching `delete` in audio stop/shutdown → revert (see #126).
- A temp file path hardcoded to `/tmp/` → revert (see #130).
- DirectX or OpenGL backend code that is not gated behind a clear build flag → deprecate and remove.

## Signed

This manifesto is enforced by CI, not by good intentions. If the code contradicts this document, the code is wrong.
