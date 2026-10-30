# Replay File Format v1

*   **Author:** Gary Cheng (@garychia)
*   **Status:** Draft — v3, post-review
*   **Date:** 2026-10-29
*   **Tracks:** issue #159 (split out of #150, closed as delivered)

> **Revision history.** v1 was reviewed by seven independent reviewers, v2 by
> four more. Four classes of defect were found, and each is fixed *in what the
> design claims*, not only in wording:
>
> 1. **A misattributed citation.** The type-name policy quote is at
>    `ci.yml:137`, not `ci.yml:164`. It is cited twice below as the authority for
>    masking `name`; both citations are corrected.
> 2. **A claim about this repository's CI that the default branch contradicts.**
>    v2 asserted there is no `ASan/UBSan` job on `main` and no checked-in
>    `.clang-tidy`. `main` has both (`ci.yml:228-229`; `.clang-tidy` tracked).
>    The verification plan is restated from the tree below.
> 3. **Five under-specifications** that made the reader unimplementable as
>    written: the tick index's base contradicted the field table;
>    `HashRecordPool::typeName` is a `std::string_view` with nowhere for the
>    owner to live; the recipe check was unassigned between the two readers;
>    `ReplayFileError::line` could not be computed across the delegation
>    boundary; and the `<unknown>` fallback could not both survive and match a
>    round trip. Each is decided in Proposed Architecture.
> 4. **A justification that did not hold.** The container was defended on
>    cohesion alone after the review questioned whether anything needed it. The
>    consumer is issue #159 and `docs/determinism.md` principle 4 — a replay file
>    format was named there as the reason #150 stayed open.
>
> v3 also adds the build-identity line #159 asks for, and drops the per-tick
> index (see Decisions taken in this revision).

--------------------------------------------------------------------------------

## Overview

A replay in ChiaEngine exists only in memory. `SimRecorder`
(`src/include/System/Module/SimRecorder.hpp`) holds a `DynamicArray<SimInput>`
named `log` (line 24); `LoadLog(const DynamicArray<SimInput> &)` (line 73)
assigns into it from an array in the same process, and `GetLog()` (line 57)
exposes it. Nothing writes `log` to disk or reads it back, so a recorded input
trace cannot survive the process that recorded it, be attached to a CI run, or
be handed to another machine.

The hash record is in a better but incomplete state.
`World::BuildHashRecord(WorldHashRecord &)` (`World.hpp:166`) fills a record; the
free `WriteWorldHashRecord(std::ostream &, const WorldHashRecord &)`
(`WorldHashRecord.hpp:45`) writes it as text; `World::WriteHashRecord(std::ostream &)`
(`World.hpp:183`) is a wrapper that builds a record and delegates. That text is
already written to a file and consumed: `test/System/HashRecordTest.hpp:130-142`
writes it to the path in `CHIA_HASH_RECORD_DUMP`, `ci.yml:135-139` sets that
variable per leg, the `Upload world hash record` step (`ci.yml:161`) publishes
`hash-record-*.txt` as an artifact, and the `Check the recorded recipe agrees
across legs` step in `determinism-compare` downloads them and compares the recipe
line.

What is missing is the inverse direction: **nothing parses that text back into a
`WorldHashRecord`.** Today the consumer greps the first line. So the record has a
writer, no reader, and no way to travel bound to the input trace it describes.

This design defines **Replay File Format v1**: one line-oriented ASCII file
carrying a build identity, a hash record and the tick-by-tick input trace, plus
the reader that makes the record's text form round-trippable for the first time.
It is the on-disk form of principle 4 of `docs/determinism.md` ("Replay is a
first-class workflow") and it answers issue #159.

## Problem Statement

Principle 3 of `docs/determinism.md` ("The world hash is the desync oracle", line
25) fixes why a bare hash is useless: "A hash value means nothing without the
recipe that produced it." The same argument applies to a bare input trace, which
is worse off: replaying a trace against a binary that judges state with a
different recipe yields a mismatch whose cause is not in the file, and the file
cannot say so.

Issue #150 asked for replay records to carry the recipe version and the
registered component type list, and **that ask is delivered** — #154 landed the
record, #156 publishes it and checks its recipe across legs, and #150 is closed.
Two things remain unmet, and they are what this document is for:

*   The record's text has no reader, so it cannot be re-loaded into the structure
    it came from. CI greps a line; it does not parse the block.
*   The input trace has neither a writer nor a reader, and no artifact binds a
    trace to the recipe that judged it. Shipping one without the other is the
    failure this format prevents.

#150's own closing comment records that "embedding the record in a replay file"
is "its own piece of work"; that work is issue #159.

## Goals

**G1 — Exact round trip.** Write then read yields a log bit-identical to the
original, floats included.

**G2 — Self-describing.** The file states the recipe version, the registered
component type list, and the build identity that makes a hash comparable at all
(compiler/ABI and the FP policy). The record block reuses the bytes
`WriteWorldHashRecord` emits, and the matching reader (`ReadWorldHashRecord`) is
added, so that text becomes round-trippable. No second serializer for the same
information.

**G3 — A stale file is rejected, not guessed at.** A file whose recipe differs
from `CHIA_WORLD_HASH_RECIPE_VERSION` fails to load with a result carrying both
version numbers and the offending line. Nothing is replayed, because the caller
has no log to replay.

**G4 — The file's bytes are stable across the four CI legs.** The file's diffable
projection (defined below) is byte-identical on gcc, clang, appleclang and msvc.
Stated honestly, this is **not** a second determinism measurement and **not** a
pure serializer check:

*   The projection excludes only the columns that are compiler-specific *by
    design* (`name`, `typeid`, `build`). It keeps the hash, the counts and every
    tick payload, so a simulation divergence fails this job exactly as it fails
    the per-tick sequence comparison. The per-tick sequence (`determinism.md:47`,
    and the `Compare sequences` step of `determinism-compare`) remains the
    primary measurement, and the scenario is the same scripted replay, so nothing
    new is being *assumed* — `determinism.md:47` records that this replay's
    sequence is measured identical on all four legs.
*   What this job adds is the file format's own bytes: a non-deterministic
    serializer (padding, locale-dependent formatting, line endings) fails here
    and nowhere else.

**G5 — No new dependencies.** C++17 standard library only, per file listed in the
implementation plan.

**G6 — Corruption is an error, never a plausible replay.** A truncated or
malformed file fails to load. This matters because `SimRecorder::FixedUpdate`
substitutes empty input when the replay cursor passes the end of the log
(`SimRecorder.hpp:99-100`) — correct for a log deliberately shorter than the run,
but it would silently absorb a file truncated on disk.

## Non-Goals

*   **Not a savegame.** The file carries the input trace and a hash record, not
    the initial world state. Reproducing a run still requires the construction
    code that built the world before tick 0, and the file does not record which
    construction code that was. A run cannot be reproduced from this file alone;
    see Open Question 1 and Alternative A.
*   **Not a cross-platform guarantee for simulation state.** `determinism.md:33`
    states the limit: "hashes are comparable **within** one compiler/ABI/FP
    configuration … Cross-leg agreement is measured (see the tables below), not
    assumed." G4 claims byte-stability of the file under that measured agreement.
*   **Not a replacement for the per-tick hash sequence artifact.** That already
    exists (`CHIA_REPLAY_HASH_DUMP` → `replay-hashes-*.txt`, `ci.yml:131-134`) and
    is already compared across legs. v1 does not duplicate it: the file carries
    one final hash, not a sequence.
*   **No binary format.** See Alternative B.
*   **No compression, delta encoding, or network streaming.**
*   **No backward-compatibility layer.** No v0 file exists anywhere; the record
    is the only thing on disk today.
*   **Not a desync localizer.** v1 can say a run diverged, not which tick
    diverged it. Per-tick hashes in the file are Future Improvements.
*   **Not a hostile-input sandbox.** The format is an internal artifact. The
    reader must not read past the end of a stream or allocate from an unverified
    count (see Risks & Mitigations), but it is not hardened against a deliberate
    adversary.

## Critical User Journeys

### CUJ 1: Record a session and write it out

*   **Given**: an engine with `SimRecorder` attached before the modules that read
    `SimInput`, and a scripted input sequence. **(G1, G2)**
*   **When**: the session runs N ticks and the caller asks for a file.
*   **Then**: the file contains, in order, a container line, the hash record
    block, a build-identity line, a declared tick count, exactly N tick lines, and
    the terminator.

### CUJ 2: Replay a file written elsewhere

*   **Given**: a file written by a different compiler leg at the same recipe
    version, **and the same construction code** — a precondition this file does
    not record and cannot verify.
*   **When**: it is loaded and replayed.
*   **Then**: the loaded log is bit-identical to the writer's log, and the
    replay's final `World::Hash()` equals the hash recorded in the file. **(G1)**

### CUJ 3: Reject a stale file instead of guessing

*   **Given**: a file whose `chia-world-hash` line names recipe 1, and a binary at
    recipe 2.
*   **When**: the file is loaded.
*   **Then**: the call returns `RecipeMismatch`, reporting both version numbers
    and the line, and the caller's array is unmodified. No log is produced, so no
    caller can proceed by accident. **(G3)**

### CUJ 4: Prove the file's bytes are stable in CI

*   **Given**: the four `Build and Test` legs, each writing its replay file for
    **the existing scripted replay** in `test/System/ReplayTest.hpp` — the same
    replay whose per-tick sequence is already in the measured set — under a new
    `CHIA_REPLAY_FILE_DUMP` knob modelled on the `CHIA_REPLAY_HASH_DUMP` dump
    directly above it in that file.
*   **When**: each file is uploaded as an artifact and `determinism-compare`
    downloads all four.
*   **Then**: the job fails if any leg's file is missing, takes the
    lexicographically first of the four as the reference (as the `Compare
    sequences` step does), and compares each file's diffable projection against
    the reference's with `diff -u --strip-trailing-cr`, failing with the same
    `::error file=…` shape the existing steps use. **(G4)**

## Proposed Architecture

The file is ASCII, one record per line, with three blocks in this order: a
container line, the hash record block, a build-identity line, the trace block, and
a terminator.

```text
chia-replay v1
chia-world-hash v2
hash 4b1e9b7f0c9a5d31
entities 1
pools 2
pool 0 typeid 1 elements 1 name replaytest::Position
pool 1 typeid 2 elements 1 name replaytest::Velocity
build gcc 13.3.0 ffp-contract=off
ticks 3
tick 00000001 3f800000 00000000
tick 00000001 40000000 00000000
tick 00000002 bf800000 80000000
end
```

### Field table

Every token below is separated by **exactly one ASCII space (0x20)**. Tabs, runs
of spaces, and trailing whitespace are `Malformed`; a line's final token must be
followed by the line ending, nothing else. Decimal means `[0-9]`, unpadded and
without a leading `+`. Hex means `[0-9a-f]` — **lowercase only**; `A-F` is
`Malformed`.

| Line | Tokens | Encoding |
| --- | --- | --- |
| 1 | `chia-replay` `v1` | literals; only `v1` is accepted |
| 2 | `chia-world-hash` `vN` | literal; `N` decimal, the writer's recipe version |
| 3 | `hash` `<v>` | literal; `v` hex, 1 to 16 digits, no `0x` |
| 4 | `entities` `<n>` | literal; `n` decimal uint32 |
| 5 | `pools` `<n>` | literal; `n` decimal uint32, must equal the pool lines present |
| 6.. | `pool` `<i>` `typeid` `<t>` `elements` `<e>` `name` `<s>` | literals + decimal uint32, uint32, uint32; `i` must equal the line's 0-based position in this block; `s` is everything after the literal `name ` up to the line end, at least one character, **not trimmed, may contain spaces** |
| next | `build` `<compiler>` `<version>` `<fp-policy>` | three non-empty tokens, no spaces within them (see Build identity) |
| next | `ticks` `<n>` | literal; `n` decimal uint32 |
| next | `tick` `<b>` `<x>` `<y>` | literal; three tokens, each **exactly 8 lowercase hex digits**, zero-padded |
| last | `end` | literal, alone on its line |

`hash` inherits the writer's formatting exactly: `WorldHashRecord.hpp:48` uses
`std::hex` with no `showbase`, no `setw` and no `setfill`, so the value is
unprefixed, lowercase and variable length — a hash of 255 prints as `ff`. The
example shows 16 digits because the value is large, not because the field is
padded. Parsing accepts 1 to 16 digits.

`name` is the one field that overrides the global "a line's final token must be
followed by the line ending, nothing else" rule: the field-specific rule wins.
Everything after the literal `name ` up to the line end *is* the name, so spaces
inside it — trailing ones included — are preserved rather than rejected, and it
must be at least one character long. The separator and the tokens before `name`
still obey the single-space rule.

### The record block is the writer's bytes, and its shape is a contract

Lines 2-5 plus the pool lines are exactly what `WriteWorldHashRecord` emits,
reused verbatim so the record cannot drift between its own artifact and this one.
That shape is a formula, stated once: after the `chia-world-hash vN` line come
`hash`, `entities`, `pools`, then exactly `pools` pool lines — **3 + N lines**
after the version line. `ReadReplayFile` delegates the whole block to
`ReadWorldHashRecord`; there is no second parser for it.

Two consequences are worth writing down:

*   `WriteWorldHashRecord` prints `record.poolCount` on the `pools` line but
    bounds its loop by `record.pools.GetNElements()` (`WorldHashRecord.hpp:50-51`).
    `World::BuildHashRecord` keeps the two equal (`World.hpp:171`), so this is
    latent, not a live bug. **Contract:** `poolCount` must equal
    `pools.GetNElements()`; the reader assigns `outRecord.poolCount` from the
    parsed line and reads that many pool lines. A record whose two values
    disagree is writable but not readable, and that is a writer-side contract
    (see Risks & Mitigations).
*   The version line is *inside* the delegated block, so the recipe version the
    file carries is the record's own field, not a duplicate.

### Float encoding: fixed-width bit patterns

Each `SimInput` field is written as its own 32-bit pattern in 8 lowercase hex
digits. `SimInput` is `{uint32_t actionBits; float axisX; float axisY;}`
(`SimInput.hpp:12-17`), asserted trivially copyable and free of implicit padding
(`SimInput.hpp:21-22`), so each field is exactly one 32-bit word: 1.0f is
`3f800000`, -1.0f is `bf800000`, -0.0f is `80000000`.

Why this encoding:

*   **Exact by construction.** The value written is the bit pattern, so no
    rounding step exists between the log and the file. Round-tripping needs no
    proof beyond copying the word back.
*   **No float parser and no float formatter.** The reader never parses a float:
    it parses an integer and copies the word into the field. Neither locale
    sensitivity nor library float-formatting differences can affect either
    direction, which is what G4 needs. Two library constraints make the text
    alternative unattractive on some legs: floating-point `std::to_chars` landed
    in libstdc++ only in GCC 11 (so the gcc and clang legs) and in libc++ in 14
    (the appleclang leg); MSVC has had it since VS 2019 16.4. A design that
    depends on it would be portable only by accident of toolchain version.
*   **Layout-independent.** Writing the fields individually, rather than
    `memcpy`ing the whole 12-byte struct, keeps the file independent of struct
    layout and padding — the same class of hazard #142 removed by making padding
    a compile error in hashed pools.

The cost, stated rather than hidden: axis values are not human-readable. The
counts, the tick count and the build line remain readable, and an IEEE-754 single
is decodable by hand, but a reader cannot see that `3f800000` is 1.0 without
decoding it. That is the price of exactness without a float formatter, and it is
accepted.

### Build identity

`build <compiler> <version> <fp-policy>` records what makes a hash comparable at
all, which `determinism.md:33` defines as compiler, ABI and FP configuration. The
values come from compiler macros, defined once in `ReplayFile.hpp` as constexpr
string views so writer and reader cannot disagree:

*   `compiler` — `clang` if `__clang__`, else `gcc` if `__GNUC__`, else `msvc` if
    `_MSC_VER`, else `unknown`. That precedence order matters: clang defines
    `__GNUC__` too, so the clang test must come first.
*   `version` — `__clang_version__` (minus anything after a space) for clang;
    `__GNUC__.__GNUC_MINOR__.__GNUC_PATCHLEVEL__` joined with `.` for gcc;
    `_MSC_VER` in decimal for msvc; `unknown` otherwise.
*   `fp-policy` — the literal the build pins: `ffp-contract=off`, which
    `CMakeLists.txt` enforces by refusing to configure if `-ffast-math`,
    `-Ofast` or `-march=native` appears (`determinism.md:33`, and the "FP
    configuration pinned" row of the automated table). A build that loosens it
    fails at configure time, so the reader may treat the literal as fixed.

It is **audit-only**, treated exactly like `name` and `typeid`: written, parsed into
the result, never diffed across legs (it differs per leg by construction), never
used to fail a load. The three tokens are returned in `ReplayBuildIdentity
outBuild`, filled on `Ok` exactly as the record and the log are — so a caller can
see that a file came from a different build than itself and decide whether that
is an error; the format does not decide for it. One consequence: build identity
lives in the container, not in the record block, because the block is frozen for
compatibility with the artifact `ci.yml:161` already publishes.

### Diffable projection

Three fields are **audit-only** and excluded from any cross-leg comparison:

*   `name` — derived from `__PRETTY_FUNCTION__` / `__FUNCSIG__` via
    `Data/TypeName.hpp`. `ci.yml:137` states the policy for this column: "the
    type-name column is compiler-specific, so it must never be diffed the way the
    value-only hash sequence above is."
*   `typeid` — `TypeId<T>()` is a first-request atomic counter
    (`src/include/System/Module/TypeId.hpp:24-28`, over `NextTypeId()` at
    lines 17-22); `WorldHashRecord.hpp:25` annotates it "執行期 key:只在同一個
    process 內有意義", and `determinism.md:31` records it is deliberately not a
    hash input. Written for audit, parsed on read, never compared across builds.
*   `build` — per leg by construction, as above.

The **diffable projection** is the file with those three masked, anchored to the
pool-line and build-line shapes so a literal `typeid` or `name` inside a type
name cannot be clipped by accident:

```text
sed -E 's/^pool ([0-9]+) typeid [0-9]+ elements ([0-9]+) name .*$/pool \1 elements \2/; s/^build .*$/build <build-identity>/' replay.txt
```

The projection is defined for files this writer produced. Everything outside it —
the container line, the recipe line, the hash, the counts, the tick block, the
terminator — must be byte-identical across the four legs.

Line endings are part of the format: the writer emits `\n` only, and the reader
strips a single trailing `\r`, so a Windows leg's file loads unchanged. The
writer is a function over `std::ostream`, so "binary mode" is the *caller's*
responsibility, not the codec's: the dump in `test/System/ReplayTest.hpp` opens
its `std::ofstream` the way the existing dump above it does. The comparison side
already handles CRLF — the `Compare sequences` step diffs with
`--strip-trailing-cr`.

### Strictness and total error mapping

The reader is **strict**: an unrecognized line is `Malformed`. Strictness is only
safe because the container version exists: a file that adds a line or changes a
line's meaning increments `chia-replay` to `v2`, and a v1 reader rejects it with
`BadContainer` rather than half-understanding it.

The mapping is total and exclusive — every deviation lands on exactly one code:

| Input | Result |
| --- | --- |
| empty stream (no first line) | `Truncated`, line 1 |
| first line not exactly `chia-replay v1` | `BadContainer`, line 1 |
| `chia-world-hash` version ≠ `CHIA_WORLD_HASH_RECIPE_VERSION` | `RecipeMismatch`, both versions, that line |
| any line whose leading literal is not the one expected next | `Malformed` |
| a numeric token with characters left over (`0x41`, `1x`, `12abc`), or an uppercase hex digit | `Malformed` |
| a token whose value overflows its type (`std::from_chars` reports it) | `Malformed` |
| `ticks N` declared but the stream ends before N tick lines | `Truncated` |
| more tick lines than declared, or pool lines beyond the declared count | `Malformed` |
| `pool` ordinal ≠ its position, or a tick line with a wrong token count | `Malformed` |
| `end` missing at the point it is expected | `Truncated` |
| anything after `end`, including text on the `end` line | `Malformed` |
| a stream that fails for a reason other than EOF where the file is otherwise complete | `Truncated` |

The final line ending is not significant: a complete `end` with no trailing
newline is `Ok`, because EOF immediately after it is not a failure. Every other
line is read by `std::getline`, so a missing newline before `end` is likewise
just the line ending.

Every integer is parsed with `std::from_chars` and must consume the **entire**
token; `from_chars` reports overflow as an error, so an out-of-range count is
`Malformed` rather than a wrapped value. Numbers are never parsed with
locale-aware `operator>>` or `std::stoi`. Structural position, not line counts,
decides `Malformed` vs `Truncated`: a wrong or surplus line is `Malformed`, and
only a stream that ends where a line was required is `Truncated`.

### Reader algorithm

Line-oriented, one pass, in order. The reader commits to its output parameters
only after the `end` line is seen.

1.  Line 1: `chia-replay v1`, else `BadContainer`. Absent stream → `Truncated`.
2.  Delegate the record block to `ReadWorldHashRecord(in, outRecord, line)`
    starting at line 2 (see Decisions for the line counter). Its `Malformed` /
    `Truncated` map to the file-level results unchanged. This function does **not**
    validate the recipe version; it reports the parsed value in
    `outRecord.recipeVersion`.
3.  Line 6 + N: `build …`. Three non-empty tokens, no validation of their
    values; they are copied into `outBuild`.
4.  Next: `ticks N`, then exactly N `tick` lines in order.
5.  `end`. Absent → `Truncated`. Content after it → `Malformed`. EOF immediately
    after a complete `end` (no trailing newline) is `Ok`; a stream failure that
    is not EOF is `Truncated`.
6.  Only now: compare `outRecord.recipeVersion` against
    `CHIA_WORLD_HASH_RECIPE_VERSION`; differ → `RecipeMismatch`, filling
    `fileRecipe` and `binaryRecipe`, and leaving all three output parameters
    untouched.

### Decisions taken in this revision

*   **The tick index is gone.** A reader that requires `index == position` gives
    the field zero information content, and a diff already localizes by line
    number, so the column was redundant data defended by a redundant rationale —
    and it was the source of the base contradiction between the field table
    (whole line "hex") and the example (decimal). Removing it resolves both.
*   **`HashRecordPool::typeName` becomes `std::string`.** It is
    `std::string_view` today (`WorldHashRecord.hpp:26`), pointing at a static
    string for the writer's lifetime (`Data/TypeName.hpp:7`), which is fine for a
    writer and impossible for a reader: the parsed characters need an owner, and
    the declared signature has nowhere to put one. The container supports it —
    `Array<T>` allocates with `new T[…]{}` and releases with `delete[]`
    (`Data/Array.hpp:25`, `:17`), so element destructors run. Existing users
    still compile: `World.hpp:176` assigns from `GetTypeName()` (a
    `string_view`), the writer's fallback becomes `std::string("<unknown>")`, and
    `HashRecordTest.hpp:64-66,78,124-125` already construct `std::string` from
    it.
*   **The recipe check belongs to `ReadReplayFile`, not `ReadWorldHashRecord`.**
    The record reader accepts any version and reports it; the file reader compares
    and produces `RecipeMismatch`, filling `ReplayFileError.fileRecipe` and
    `binaryRecipe`. Otherwise `RecipeMismatch` would be unreachable (the record
    result enum has no such code) and G3 and CUJ 3 could not pass.
*   **The line counter crosses the delegation boundary explicitly.**
    `ReadWorldHashRecord(std::istream &in, WorldHashRecord &outRecord, uint32_t
    &inOutLineNumber)`: 1-based, entering the call equal to the line of the
    `chia-world-hash` line, incremented after each successfully consumed line, and
    **left at the offending line on failure**. `ReadReplayFile` owns the counter
    and copies it into `ReplayFileError.line`, which is therefore always the line
    the load failed on (1 at minimum) — no "0 if none" case, since a failure
    always has a line.
*   **`<unknown>` maps back to empty.** The writer prints the literal when
    `typeName` is empty (`WorldHashRecord.hpp:55`), so the reader maps that literal
    back to an empty `typeName` and a round trip is exact. A component type
    literally named `<unknown>` is therefore not representable — unreachable in
    practice, because names come from `__PRETTY_FUNCTION__` / `__FUNCSIG__` and
    always carry a qualified form (a compiler that yields nothing gives an empty
    name, which is the fallback's own case).
*   **The codec does not live on `SimRecorder`.** `SimRecorder` is an `IModule`
    with a scheduling job; a codec that also schedules is harder to test. The codec
    is a free function pair over `std::istream` / `std::ostream`, so it needs no
    `<fstream>` and no engine knowledge.
*   **No `SimRecorder::SaveToFile` / `LoadFromFile` in v1.** Their only consumer
    is one test, which opens its own stream in three lines. Adding `<fstream>` to
    a module header every input consumer includes is not worth it; `LoadLog`
    already exists for the last step.
*   **`MissingRecipe` stays removed.** The "no record block at all" case is a
    wrong line (`Malformed`) or an ended stream (`Truncated`). Four failure codes,
    each with a defined trigger, replace five with one undefined.
*   **The writer never formats a message.** G3 needs two version numbers and a
    line, which the result struct carries as values. No string channel, no logging
    dependency; a caller that wants text formats it.

## Alternatives Considered

### Alternative A: Full state snapshot

Record the entity set and each component's bytes, so the file alone reconstructs
the run.

*   **Pros**: one file reproduces everything; no dependence on construction code.
*   **Cons**: needs type-erased serialization over arbitrary component types,
    which does not exist — `World::Hash()` reads `sizeof(T)` bytes per element
    (`ComponentPool.hpp:140`) but has no notion of writing a `T` back. It
    duplicates the hash's byte layout in a second place, so the two can disagree.
*   **Rejected for v1**: strictly larger, adds a new failure mode (snapshot and
    hash disagreeing), and does not address the record-reader gap that motivates
    this design.

### Alternative B: Binary format

A header struct followed by `sizeof(SimInput)` bytes per tick.

*   **Pros**: smaller; the float question disappears because bytes are copied.
*   **Cons**: endianness and struct layout become format rules that must be pinned
    and tested; the file cannot be read in a CI log; a reader is needed
    regardless, so it saves no code.
*   **Rejected**: pays a portability cost for a size win that does not matter, and
    abandons the readability the existing record text was built on.

### Alternative C: Raw `SimInput` byte dump

`memcpy` each tick's `SimInput` and write `sizeof(SimInput)` bytes.

*   **Pros**: one line of code; reproduces the in-memory log exactly.
*   **Cons**: not line-oriented, not diffable, and it reintroduces a dependency on
    the type's memory layout — the class of dependency #142 removed.
*   **Rejected**: reintroduces a hazard the project already paid to remove.

### Alternative D: Extend the existing record artifact instead of a container

`CHIA_HASH_RECORD_DUMP` already produces a per-leg file that
`determinism-compare` already downloads and reads (`ci.yml:139`, the `Upload world
hash record` step, and the `Check the recorded recipe agrees across legs` step).
This alternative appends the trace block to that file and adds no container.

*   **Pros**: no new format, no new artifact, no new download step; reuses
    plumbing already proven green. It is genuinely less work — the difference from
    the main design is the container line and the build line.
*   **Cons**: the record file currently ends after the pool lines, and a consumer
    outside this repository could reasonably treat "the record file" as exactly
    that block; extending it is a change to an artifact that already has a
    published shape. The container line is what versions *that* change, and what a
    future v2 can raise. Folding the trace in also makes the artifact that CI
    diffs for the recipe check the same artifact that carries megabytes of tick
    data.
*   **Verdict**: **the closest call in this document, and the one to revisit
    first.** It is rejected because two artifacts with separate, published shapes
    are safer to evolve than one whose shape means different things to different
    consumers — not because cohesion is a value in itself, which is what an
    earlier draft of this section argued.

### Alternative E: No file at all — keep replays in memory

*   **Pros**: zero cost; nothing to maintain; the per-tick hash sequence already
    proves cross-leg simulation agreement.
*   **Cons**: a recorded trace still cannot outlive its process, and the record's
    text stays unparseable.
*   **Rejected**: it does not answer issue #159, whose stated deliverable is
    exactly the file this document specifies, and it leaves principle 4 of
    `docs/determinism.md` without an on-disk form. It remains the right answer
    only if that issue is withdrawn.

## Risks & Mitigations

| Risk | Mitigation |
| --- | --- |
| **A truncated file replays as a plausible short run.** `FixedUpdate` pads a short log with empty input (`SimRecorder.hpp:99-100`), so a file cut on disk behaves like a deliberately short replay. | The declared `ticks` count must equal the tick lines read, and `end` must be present. Both failures are `Truncated`, and the reader commits nothing. |
| **A corrupt count drives allocation.** A declared `ticks` of 4 billion in a 40-line file. | The reader never pre-sizes from a declared count — it appends as it reads, so memory is bounded by the file's real length (a tick line is ~30 bytes: 3 × 8 hex digits plus the literal and separators), not by the declared number. The same rule covers `pools`: the declared value is a cross-check against the lines present, never an allocation size. `from_chars` rejects an overflowing count. |
| **Scalability ceiling.** The format cannot express a run longer than a `uint32` tick count. | Counts and the tick count are `uint32` (≈4.29e9 ticks ≈ 2.3 years at 60 Hz). Past that, the read side's `from_chars` reports overflow and the load is `Malformed`. Raising the ceiling is a `chia-replay v2` change. |
| **`poolCount` and `pools` disagree in a record.** The writer prints the field but loops the array (`WorldHashRecord.hpp:50-51`), so such a record is writable but not readable. | Stated as a writer-side contract in Proposed Architecture: `poolCount` must equal `pools.GetNElements()`, which `World::BuildHashRecord` maintains (`World.hpp:171`). The reader trusts the parsed count and reads that many pool lines; a disagreement surfaces as `Malformed`/`Truncated` on load rather than a wrong record. |
| **A stale file is replayed against a newer recipe.** The values would be silently meaningless (`determinism.md:33`). | `RecipeMismatch` is a load failure, not a warning, and the API returns no log, so a caller cannot proceed without ignoring a result. CUJ 3 tests it. |
| **The type-name column makes the file uncompariable across legs.** | `name`, `typeid` and `build` are audit-only and masked by the projection, following the policy `ci.yml:137` already states for the name column. |
| **A type name contains a newline**, breaking the line format. | Names come from `__PRETTY_FUNCTION__` / `__FUNCSIG__`, which cannot contain a newline; the reader consumes everything up to the first `\n` as the name either way. |
| **A future writer adds a float-formatted field**, reintroducing locale and library-version dependence. | The format rule forbids text floats (every numeric field is an integer, decimal or hex), and the reader is strict, so such a line is not silently accepted — it is `Malformed`, or a new container version. |
| **A future writer adds a field and an old reader half-understands it.** | The reader is strict: an unrecognized line is `Malformed`. Evolution goes through the container version, and a v1 reader rejects a v2 file with `BadContainer`. |
| **The new CI comparison is read as a determinism guarantee it is not.** | G4 states what the projection does and does not cover, that the per-tick sequence remains the primary measurement, and that the scenario is the replay whose agreement `determinism.md:47` already records as measured. |

There is no confidentiality, authentication or authorization surface: the file is
a local artifact with no secrets, and the format has no extensibility hook a
foreign payload could ride on. The remaining limit is the trust boundary — the
reader is robust, not a sandbox (see Non-Goals).

## Non-Functional Requirements

*   **Determinism / locale**: the encoding and the parse must not read the process
    locale. Guaranteed structurally: no float formatting or parsing at all, and
    `std::from_chars` for integers. `\n` on write, `\r` stripped on read, so line
    endings are canonical.
*   **Performance**: O(lines) in both directions. The read path allocates the
    output log, the record's `pools` array (`WorldHashRecord.hpp:41`) and one
    `std::string` per pool name; the caller's subsequent
    `SimRecorder::LoadLog` copy-assigns (`SimRecorder.hpp:73-76`), so two logs
    exist briefly. The correct claim is "one pass, one allocation per output
    container plus one per pool name, no per-tick allocation" — not "no allocation
    beyond the log".
*   **Rollout / rollback**: no engine behavior changes. Two files are created
    (`src/include/System/Module/ReplayFile.hpp`,
    `test/System/ReplayFileTest.hpp`); five are modified
    (`src/include/System/World/WorldHashRecord.hpp`,
    `test/System/SystemModule.cpp`, `test/System/ReplayTest.hpp`,
    `.github/workflows/ci.yml`, `docs/determinism.md`). Rollback is reverting those
    seven: remove the two new files, and revert the reader, the member type
    change, the two-line registration, the env-gated dump block, the CI additions
    and the documentation edit.

## Open Questions

**Q1 — should the file record what construction code built the world?** Without
it, a reader on another machine has the inputs and no way to know which scenario
to build them against, so CUJ 2's equality holds only when the same construction
code happens to be present. A `source <name> <commit>` line would close this, at
the cost of a field that must be kept accurate and a rule for what a reader does
when it is absent or wrong. It is not in v1 because #159 does not ask for it and
the file is already self-describing about *how* the state was judged. This is the
one genuinely undecided item, and it is the owner's call.

Resolved since v1 and recorded above: the float encoding (Proposed
Architecture), the codec's location and the absence of `SimRecorder` wrappers,
reader strictness and the total error mapping, build identity (added in v3), the
tick index (removed), `typeName`'s ownership, the recipe check's owner, the line
counter, and the `<unknown>` mapping.

## Detailed Implementation Plan

### Target Files to Create/Modify

#### 1. `src/include/System/World/WorldHashRecord.hpp` (Modify) — G2

*   **Type / Scope**: C++ header, data serialization and parsing.
*   **Proposed Changes**: Change `HashRecordPool::typeName` (line 26) from
    `std::string_view` to `std::string`, and update the writer's fallback (line 55)
    accordingly. Add the inverse of the writer:

    ```cpp
    enum class HashRecordReadResult : uint8_t { Ok, Malformed, Truncated };
    inline HashRecordReadResult ReadWorldHashRecord(std::istream &in, WorldHashRecord &outRecord,
                                                   uint32_t &inOutLineNumber);
    ```

    It consumes exactly the block the writer emits — the `chia-world-hash vN`
    line, then `hash`, `entities`, `pools`, then `pools` pool lines — with the
    field table's encoding, the exact-token rule, the pool ordinal == position
    rule, `name` as the untrimmed remainder of the line, and the literal
    `<unknown>` mapped back to an empty name. It does **not** validate the recipe
    version. It increments `inOutLineNumber` after each consumed line and leaves
    it at the offending line on failure. It writes into locals and commits to
    `outRecord` only on `Ok`. New includes: `<istream>`, `<string>`;
    `<string_view>` is no longer needed here.
*   **Rationale**: The record's text form has a writer and no reader, so it cannot
    round-trip. Keeping the reader beside the writer is what stops the two
    drifting, and the member change is what makes the parsed names ownable.

#### 2. `src/include/System/Module/ReplayFile.hpp` (Create) — G1, G2, G3, G4, G6

*   **Type / Scope**: C++ header, file-format codec. Includes
    `System/World/WorldHashRecord.hpp`, `System/Module/SimInput.hpp`,
    `Data/DynamicArray.hpp`, `<istream>`, `<ostream>`, `<string>`, `<string_view>`,
    `<cstddef>`, `<cstdint>`, `<charconv>`, `<cstring>`.
*   **Proposed Changes**: the build-identity constants (from the compiler macros,
    in the precedence order given above), then:

    ```cpp
    enum class ReplayFileResult : uint8_t { Ok, BadContainer, RecipeMismatch, Malformed, Truncated };

    struct ReplayFileError
    {
        ReplayFileResult result = ReplayFileResult::Ok;
        uint32_t containerVersion = 0; // parsed from line 1; 0 if line 1 was not a container line
        uint32_t fileRecipe = 0;       // RecipeMismatch only
        uint32_t binaryRecipe = 0;     // RecipeMismatch only; CHIA_WORLD_HASH_RECIPE_VERSION
        uint32_t line = 1;             // 1-based line the load failed on
    };

    struct ReplayBuildIdentity
    {
        std::string compiler;
        std::string version;
        std::string fpPolicy;
    };

    bool WriteReplayFile(std::ostream &out, const WorldHashRecord &record,
                         const DynamicArray<SimInput> &log);

    ReplayFileError ReadReplayFile(std::istream &in, WorldHashRecord &outRecord,
                                   DynamicArray<SimInput> &outLog,
                                   ReplayBuildIdentity &outBuild);
    ```

    `WriteReplayFile` emits the container line, delegates the record block to
    `WriteWorldHashRecord`, writes the build line, then `ticks N`, one `tick` line
    per entry (each field `memcpy`'d into a `uint32_t` and printed as 8 hex
    digits), then `end`. It returns `false` only on a stream failure and does not
    validate the record. `ReadReplayFile` implements the algorithm above, owns the
    line counter, and writes all three output parameters — the record, the log and
    the parsed build identity — only on `Ok`.
*   **Rationale**: Keeps serialization out of `SimRecorder`, needs no `<fstream>`
    anywhere, and puts the format in one place a test can drive with in-memory
    streams.

#### 3. `test/System/ReplayFileTest.hpp` (Create) — G1, G2, G3, G6

*   **Type / Scope**: Unit test, namespace `replayfiletest`, suite
    `Test("SystemReplayFile")`, matching `Test("SystemReplay")` /
    `Test("SystemHashRecord")`.
*   **Proposed Changes**: cases driven by `std::stringstream`, so nothing touches
    the filesystem (the CI artifact is written by the dump in file 5, not here):
    *   **(a) Round trip — G1.** A log with values chosen to break a naive
        encoding: `0.1f`, `1e-7f`, `-0.0f`, a subnormal, and an `actionBits` with
        the high bit set — and enough entries that `ticks` reaches two digits.
        Compare field-wise via `memcpy` into `uint32_t`, **not** with `==`, since
        `-0.0f == 0.0f` is true and an equality check would pass even if the
        writer dropped the sign bit. `SimInput` has no `operator==`, so the
        comparison is written explicitly.
    *   **(b) Recipe mismatch — G3.** A hand-written header naming a different
        recipe: assert `RecipeMismatch`, that `fileRecipe` and `binaryRecipe`
        carry both numbers, that `line` names the recipe line, and that all three
        output parameters are untouched.
    *   **(c) Truncation — G6.** No `end` line; fewer tick lines than declared; a
        stream cut inside the record block. Assert `Truncated`, the reported line,
        and untouched outputs.
    *   **(d) Malformed — G6.** An unrecognized line; a surplus tick line; a pool
        ordinal not matching its position; `0x41` and `12abc` where a number is
        expected; an uppercase hex digit in a payload; trailing text after `end`.
        Assert `Malformed` and the reported line for each.
    *   **(e) Record round trip — G2.** Write a record, read it back with
        `ReadWorldHashRecord`, and assert recipe version, hash, counts and pool
        fields match; assert the `<unknown>` fallback round-trips to an empty
        name; assert the line counter has advanced `4 + pools` lines in total (the
        `chia-world-hash` line, the 3 header lines, one line per pool).
    *   **(f) Build line — G2.** Read a written file back through `ReadReplayFile`
        and assert `outBuild`'s three tokens equal the `CHIA_REPLAY_BUILD_*`
        constants — the assertion that catches a reader which parses the line and
        discards it; then assert the line is not validated (a file claiming
        `msvc 19.0 unknown` still loads, with those tokens returned unchanged),
        and that the projection sed masks it.
*   **Rationale**: (a) is the only case that catches an encoding mistake, and its
    comparison method has to be exact; (b) and (c) are the two failure modes the
    format exists to prevent; (d) pins the error mapping that v2 left ambiguous.

#### 4. `test/System/SystemModule.cpp` (Modify)

*   **Type / Scope**: Test registration.
*   **Proposed Changes**: add `#include "ReplayFileTest.hpp"` to the include block
    (lines 2-27 — without it the translation unit does not compile), and
    `AddTest<replayfiletest::ReplayFileTest>();` next to the existing
    `AddTest<hashrecordtest::HashRecordTest>();` (line 40) and
    `AddTest<replaytest::ReplayTest>();` (line 49). `AddTest` takes the type alone
    unless the suite needs constructor arguments, as `AddTest<IOTest>(ioTestPath)`
    does; this suite needs none.
*   **Rationale**: A test that is not registered does not run. Test headers are
    included by `SystemModule.cpp` rather than listed in
    `test/System/CMakeLists.txt`, which is how `ReplayTest.hpp` works.

#### 5. `test/System/ReplayTest.hpp` (Modify) — G4

*   **Type / Scope**: Test, the scripted replay the CI artifacts come from.
*   **Proposed Changes**: beside the existing env-gated hash-sequence dump
    (lines 17-23: `std::getenv` + `std::ofstream`, active only when the variable
    is set and non-empty), add the same for `CHIA_REPLAY_FILE_DUMP`, calling
    `WriteReplayFile` with the record the test already builds and the log it
    already has. This is the only test that writes the file, and it is
    deliberately the *existing* scripted replay: that is the run whose cross-leg
    sequence agreement `determinism.md:47` records as measured, which is what lets
    CUJ 4 claim anything at all.
*   **Rationale**: G4's artifact had no writer in v2 (the new suite is
    filesystem-free by design), so the CI comparison would have failed its
    missing-leg check on the first run. The dump belongs where the existing dump is.

#### 6. `.github/workflows/ci.yml` (Modify) — G4

*   **Type / Scope**: CI workflow.
*   **Proposed Changes**, each modelled on the step directly above it:
    *   `Run unit tests` env (lines 131-139): add
        `CHIA_REPLAY_FILE_DUMP: replay-file-${{ matrix.compiler }}.txt`.
    *   New upload step after `Upload world hash record` (line 161), named
        `replay-files-${{ matrix.os }}-${{ matrix.compiler }}-${{ github.sha }}`,
        `if: always()`, `if-no-files-found: warn`, path
        `replay-file-${{ matrix.compiler }}.txt`.
    *   In `determinism-compare`: a download step
        (`pattern: replay-files-*-${{ github.sha }}`, `path: replayfiles`), then a
        comparison step copying the `Compare sequences` shape — `mapfile` +
        `find | sort`, the same `< 4` guard with `::error::`, `ref` = first file,
        and per file `diff -u --strip-trailing-cr <(sed -E '…' "$ref") <(sed -E '…' "$f")`
        with the same `::error file=…` message. The projection is applied to
        **both** sides.
*   **Rationale**: G4 is a claim until something checks it. The mechanism, the
    artifact shape and the missing-leg failure all already exist in this file.

#### 7. `docs/determinism.md` (Modify) — G3, G6

*   **Type / Scope**: Documentation, the normative statement of the contract.
*   **Proposed Changes**: in principle 4 (line 39), after the existing rule at
    line 47, add a bolded `**Rule:**` line in the file's own convention: the
    on-disk form exists as Replay File Format v1
    (`docs/designs/replay-file-format.md`), it carries the hash record in the form
    principle 3 defines plus the build identity, loading a file whose recipe
    differs is an error rather than a warning, and a truncated file is an error
    rather than a shorter replay. Add the matching row to the automated table
    (lines 63-74), which is where the file records what CI enforces and where the
    per-tick sequence row already lives. Do not make this edit if the design is
    not implemented.
*   **Rationale**: The manifesto is normative and self-consistent about it:
    `docs/determinism.md:100` — "a claim that is not in the tree is not a claim."

### Verification Plan

| Goal | Evidence |
| --- | --- |
| G1 | Test (a), which fails if any field's bits change across the round trip. |
| G2 | Tests (e) and (f), plus the record block being the writer's own bytes. |
| G3 | Test (b). |
| G4 | The four uploaded files, diffed via their projection in `determinism-compare`; fails if a leg is missing. |
| G5 | No new dependency: the include lists in the plan are the complete set for each file. |
| G6 | Tests (c) and (d), which pin the total error mapping. |

*   **Build**: `cmake --build build --parallel`, the per-leg command the workflow
    already runs (`ci.yml:141` shows where the binary lands).
*   **Unit tests**: `./build/bin/TestMain` (`.exe` on Windows, under `xvfb-run` on
    Linux — `ci.yml:140-148`), with the new suite green on all four legs.
*   **CI**: the four `Build and Test` legs, the `determinism-compare` job with its
    new comparison, and the `ASan/UBSan` leg (`ci.yml:228-229`), which will
    exercise the new parsing code.
*   **clang-tidy**: `.clang-tidy` is checked in and defines the check set
    (`determinism.md:85`); the code-quality step reports findings and does not
    fail. This design does not change that, and does not claim a clean run as
    evidence.
*   **Manual**: open a written file and confirm a human can read the container,
    recipe, build identity, counts and tick count.

## Additional Resources

*   `docs/determinism.md` — principle 3 (lines 25-37: the hash is an oracle, a
    value means nothing without its recipe, and line 33's measured-not-assumed
    limit), principle 4 (lines 39-48: replay is first-class, and the measured
    cross-leg sequence), the automated table (63-74) and line 100.
*   Issue #159 — this work; issue #150 — what came before it and what of it is
    delivered.
*   `src/include/System/World/WorldHashRecord.hpp` — the recipe constant, the
    record, and the writer being reused.
*   `src/include/System/Module/SimRecorder.hpp` — the log this format persists
    (`log` at 24, `GetLog` at 57, `LoadLog` at 73, the short-log substitution at
    99-100).
*   `src/include/System/Module/SimInput.hpp` — the three fields and the two
    `static_assert`s the bit-pattern encoding relies on.
*   `src/include/Data/TypeName.hpp` — how type names are extracted per compiler,
    and the lifetime note (line 7) that made the member change necessary.
*   `src/include/Data/Array.hpp` / `Data/DynamicArray.hpp` — the storage the
    record's pool list uses, and its `new T[…]{}` / `delete[]` semantics.
*   `test/System/ReplayTest.hpp` — the scripted replay, and the env-gated dump
    pattern the new artifact follows.
*   `.github/workflows/ci.yml` — the two existing dumps (131-139), their upload
    steps (150, 161), the `ASan/UBSan` leg (224-229), and `determinism-compare`
    (284 onward).

## Future Improvements

*   **Per-tick hashes in the file** — turns it into a desync localizer; costs a
    second source of truth that must stay consistent with the final hash, and
    duplicates an artifact that already exists.
*   **Full state snapshot** (Alternative A) — worth doing only when a real need to
    reproduce a run without its construction code appears.
*   **A `source` line** (Open Question 1) — which scenario and commit produced the
    file, so a foreign reader knows what to build.
*   **A `chia-replay` CLI** — replays a file and prints the verdict, so the format
    is usable outside the test binary.

--------------------------------------------------------------------------------

<!-- Provenance: Indicates initial structural drafting via EGM. -->

*Drafted with EGM; revised across two adversarial review rounds (seven reviewers,
then four), with every blocker and major re-verified against the tree before
being acted on.*
