# OpenCode Task — Fix Issue #103: $(nproc) and other Linux-only shell commands in CI

## Goal
Replace Linux-only shell commands in .github/workflows/ci.yml with cross-platform equivalents.

## Context
- Repo: github.com/garychia/ChiaEngine (main branch, PR #96 already merged)
- Workdir: /tmp/oc-ci (fresh clone of main)
- Issue: #103 — https://github.com/garychia/ChiaEngine/issues/103

## Problem
Linux-only shell commands in ci.yml:
- `$(nproc)` at lines 113, 125, 187, 189, 307, 392 — GNU/Linux-only, not available on macOS/Windows without coreutils
- `./build/bin/TestMain` at lines 119, 121, 393 — Windows produces `.exe`, not a bare binary
- `tar -czf` at line 320 — Linux-only (Windows zip is commented out, tracked in #104)

## Acceptance criteria
1. Replace `$(nproc)` with a cross-platform core count (e.g., a shell fallback or CMake-computed value)
2. Make `./build/bin/TestMain` invocation work on Windows (append `.exe` when runner.os == 'Windows')
3. YAML remains valid
4. No unrelated changes
5. Report: exact lines changed, before/after for each

## Deliverable
A patch/diff showing the cross-platform replacements, plus confirmation YAML is valid.