# OpenCode Task — Fix Issue #97: CI Windows job needs MSVC toolchain

## Goal
Fix the Windows CI job so it uses a real MSVC toolchain instead of MinGW, and the DirectX backend doesn't break the build.

## Context
- Repo: github.com/garychia/ChiaEngine (main branch)
- Workdir: /tmp/oc-ci (fresh clone of main)
- Issue: #97 — https://github.com/garychia/ChiaEngine/issues/97

## Problem
The Windows CI job compiles with MinGW and the DirectX backend fails. Need to:
1. Configure Windows job to use MSVC (not MinGW)
2. Ensure the DirectX compile break (issue #105) is addressed so Windows builds don't fail

## Acceptance criteria
1. Review ci.yml Windows matrix entry and steps for MSVC configuration
2. If DirectX guard from #105 is applied, confirm Windows job can at least reach configure step
3. Report: findings and any ci.yml changes needed

## Deliverable
A report on Windows CI configuration state and any changes made.