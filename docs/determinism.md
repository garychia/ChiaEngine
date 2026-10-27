# ChiaEngine Determinism Manifesto

## Why this document exists

Unreal and Unity treat determinism as a concession: rollback netcode, frame-perfect replays, and lockstep simulation are bolted onto engines that were never designed for them. ChiaEngine is built from the ground up to be deterministic. This is not a feature flag — it is the architecture.

## Core principles

### 1. Fixed timestep is mandatory

`World` advances in discrete, equal ticks. No variable-dt interpolation. If you cannot express gameplay in fixed steps, you cannot replay it.

**Rule:** All simulation logic runs in `FixedUpdate`-driven module updates, and every tick is driven with the *same* `dt` value. `Engine::Tick(double)` currently takes that value as an argument, so the contract lives with the caller: a fixed constant per build configuration, never a measurement from the OS clock.

**Gap:** nothing enforces that yet. Either `Tick()` should stop accepting a `dt`, or a debug assertion should check it against the configured constant. Listed as a gap, not as a rule.

### 2. SimInput is the only input source

`SimInput` (`src/include/System/Module/SimInput.hpp`) is the single interface between the real world (keyboard, mouse, network) and the simulation.

**Rule:** No module reads raw GLFW, raw sockets, or raw OS events during `FixedUpdate`. All input flows through `SimRecorder` → `EngineContext::ResolveService<SimInput>()` → consumer modules.

**Rule:** `SimInput` stays byte-copyable and byte-stable, enforced by two `static_assert`s in its own header: `std::is_trivially_copyable_v<SimInput>` (SimRecorder copies it bytewise) and `CHIA_PADDING_FREE` (no implicit padding — see principle 3). Adding a `std::string`, `std::vector` or pointer member breaks the first at compile time.

### 3. The world hash is the desync oracle

`World::Hash()` and `ComponentPool::Hash()` (`src/include/System/World/`) produce a FNV-1a 64 fingerprint of the simulation state. It is an oracle, not the state: it can prove that two runs *diverged*, never that two states are equal in every respect. A hash value means nothing without the recipe that produced it.

**Rule — what the hash covers:** entity identity (the alive set, each entity's generation, and the walk order of the dense array) plus every component pool's bytes and owner indices, pools folded in registration order together with their ordinal. Nothing else. In particular a runtime type id is *not* an input: `TypeId<T>()` is a first-request counter, so folding it in made the hash depend on unrelated service registration.

**Rule — component types must be padding-free, enforced at compile time.** The hash reads `sizeof(T)` bytes, and an implicit padding byte has an unspecified value, so a padded type hashes differently per compiler (measured: clang keeps the garbage, gcc zeroes it, and the resulting pool hashes disagreed). Every component type declares `CHIA_PADDING_FREE(Type, &Type::members...)` (`src/include/Data/Padding.hpp`) next to its definition. The check compares `sizeof(T)` against the sum of the listed members *and* requires every nested member type to carry the same mark, so a padded type or an unmarked nested one is a compile error rather than a convention. `ColliderComponent` was the one offender (32 bytes of storage, 29 meaningful) and is dense now. A type that cannot be made dense does not belong in a hashed component pool.

**Rule — floating point is pinned.** GCC and Clang both default to `-ffp-contract=fast` for C++, which lets `a*b+c` become an FMA and round differently. The build passes `-ffp-contract=off`, and `CMakeLists.txt` refuses to configure at all if `-ffast-math`, `-Ofast` or `-march=native` appears in any flag set. The corollary, stated plainly: hashes are comparable **within** one compiler/ABI/FP configuration. The draft of this document claimed "identical on any compiler, any platform, any optimization level" — that was not true, and is not claimed anymore. Cross-leg agreement is measured (see the tables below), not assumed.

**Rule — the type set is audited, not hashed.** A stable type identity cannot be hashed portably (Itanium mangling on gcc/clang/appleclang, a different scheme on MSVC), so the digest deliberately carries no type names. `World::BuildHashRecord()` / `World::WriteHashRecord()` (`src/include/System/World/WorldHashRecord.hpp`) records the hash, the recipe version, and the registered component type list in registration order. A recorded hash without that record cannot be interpreted.

**Rule — changing the inputs means bumping the recipe.** `CHIA_WORLD_HASH_RECIPE_VERSION` is part of the record, and any change to what the hash folds in must bump it. Old records do not degrade gracefully: they silently stop meaning what they meant.

### 4. Replay is a first-class workflow

`SimRecorder` (`src/include/System/Module/SimRecorder.hpp`) records `SimInput` and feeds it back on replay. Desync is detected by comparing `World::Hash()` at checkpoints.

**Rule:** Attachment order matters. `SimRecorder` is attached to `EngineContext` before any module that reads `SimInput`; its `OnAttach` is what registers the `SimInput` service.

**Gap:** there is no late-attachment assertion. A usable check needs `EngineContext` to freeze its service table once attachment is finished.

**Rule:** Replay tests run on every CI leg with the same scripted input, and the hash sequences are compared *across* legs, not just within each one: each leg writes its per-tick sequence and the `determinism-compare` job diffs them, failing outright if a leg is missing rather than treating absence as agreement. Measured on the current scripted replay: gcc, clang, appleclang and msvc produce identical sequences.

### 5. No hidden state, no implicit magic

If a subsystem caches, buffers, or defers work, that state must be serializable into the replay stream or explicitly excluded from the deterministic core.

**Rule:** `AssetManager` async loading is allowed, but asset *availability* must not affect `World::Hash()`. Async loads complete outside the simulation tick or are excluded from hash input.

**Rule:** Audio (`src/source/System/Audio/`) plays back on the real-time clock; audio state is not part of `World::Hash()` and does not affect replay equivalence.

## What ChiaEngine is NOT

- **Not Unreal Engine.** No Blueprints, no visual scripting, no Slate UI framework, no 1M-line monolith. If you can't read the whole engine source in an afternoon, it's too big.
- **Not Unity.** No C#, no Mono/IL2CPP, no garbage collector, no asset store, no editor bloat. Pure C++17 RAII.
- **Not a general-purpose game engine.** It is a deterministic simulation engine with a Vulkan renderer attached. The simulation is the product; rendering is a view on top.

## Non-negotiables: automated

A violation turns a check red.

| Check | Where | Enforced by |
|-------|-------|-------------|
| `SimInput` byte-copyable and padding-free | `src/include/System/Module/SimInput.hpp` | `static_assert` in every build |
| Component types padding-free, nested types marked | each component header + `src/include/Data/Padding.hpp` | compile error (`CHIA_PADDING_FREE`) |
| Replay hash sequence identical across compiler legs | `test/System/ReplayTest.hpp` + `.github/workflows/ci.yml` | `determinism-compare` job (gcc, clang, appleclang, msvc) |
| FP configuration pinned | `CMakeLists.txt` | configure-time `FATAL_ERROR` |
| Gating CI jobs run even when a dependency failed | `.github/workflows/ci.yml` | `always()` present in the workflow |
| ASan/UBSan clean | `.github/workflows/ci.yml` | `ASan/UBSan` job: Debug + `-fsanitize=address,undefined` + `halt_on_error=1` |

## Non-negotiables: kept by review, not yet automated

Do not read these as checks.

| Rule | Why it is not a check yet |
|------|---------------------------|
| Late attachment of `SimRecorder` | needs a service-table freeze in `EngineContext` |
| `dt` fixed per build configuration | `Engine::Tick(double)` still accepts any value |
| Recipe version bumped when hash inputs change | review rule; the record makes the drift visible (a value with a stale version is wrong, not merely old) |
| clang-tidy clean | `.clang-tidy` defines the check set, but the code-quality step reports findings and never fails — turning it into a gate is its own decision, not a config edit |
| `clang-format` clean | advisory by design: the step reports, it never fails |

## Violations

If you add:
- A `std::string` to `SimInput` → revert (compile error).
- A component type with implicit padding, or a nested member type that is not marked → revert (compile error).
- `-ffast-math`, `-Ofast` or `-march=native` → revert (configure error).
- A `new` without matching `delete` in audio stop/shutdown → revert (see #126).
- A temp file path hardcoded to `/tmp/` → revert (see #130).
- DirectX or OpenGL backend code that is not gated behind a clear build flag → deprecate and remove.

## Signed

This manifesto is enforced by CI where the tables say so, and by review where they say that instead. If the code contradicts this document, the code is wrong. And if this document contradicts the code, correcting the document belongs in the same pull request that changed the code — a claim that is not in the tree is not a claim.
