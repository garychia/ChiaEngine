# OpenCode Task — Fix Issue #101: ccache guard in code-quality/release/nightly jobs

## Goal
Make ccache launcher conditional in all CI jobs that reference it, matching the pattern already applied in build-and-test (PR #96).

## Context
- Repo: github.com/garychia/ChiaEngine (main branch, PR #96 already merged)
- Workdir: /tmp/oc-ci (fresh clone of main)
- Issue: #101 — https://github.com/garychia/ChiaEngine/issues/101

## Problem
PR #96 made `build-and-test` conditional on `command -v ccache`, but these jobs still pass `-DCMAKE_C_COMPILER_LAUNCHER=ccache` unconditionally:
- `code-quality` job, ci.yml:186 — `cmake .. -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache`
- `release` job, ci.yml:297-303 — same ccache launcher flags
- `nightly` job, ci.yml:382-389 — same ccache launcher flags

ccache is NOT preinstalled on ubuntu-24.04 runner (verified). It's installed via apt in the Linux Setup dependencies step, but code-quality/release/nightly don't install ccache via apt — they only restore from cache. If the cache misses, ccache is missing and CMake fails.

## Acceptance criteria
1. In `code-quality`, `release`, and `nightly` jobs, wrap ccache launcher args in a `command -v ccache` check (same pattern as build-and-test lines 97-103)
2. YAML remains valid (parse with python3 -c "import yaml" or similar)
3. No unrelated changes to ci.yml
4. Report: exact lines changed, before/after

## Deliverable
A patch/diff showing the ccache guard added to code-quality, release, and nightly jobs, plus confirmation YAML is valid.