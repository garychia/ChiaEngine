# OpenCode Task — Fix Issue #105: DirectX backend Frame-broken compile

## Goal
Fix the DirectX backend compile break under DIRECTX_ENABLED by guarding Renderer::Execute.

## Context
- Repo: github.com/garychia/ChiaEngine (main branch)
- Workdir: /tmp/oc-ci (fresh clone of main)
- Issue: #105 — https://github.com/garychia/ChiaEngine/issues/105
- Reference: docs/agents/directx-backend-assessment.md (deprecation recommendation)

## Problem
src/source/Display/Renderer.cpp:78-80 calls `specializedRenderer.Execute(frame)` unconditionally, but DirectXRenderer implements only legacy IRenderer — no IFrameExecutor, no Execute(). A DIRECTX_ENABLED (Windows) build fails to compile Renderer.cpp.

## Acceptance criteria
1. Guard `specializedRenderer.Execute(frame)` with `#ifdef VULKAN_ENABLED` so DirectX builds compile (legacy backends return false)
2. Alternatively, per the assessment recommendation, deprecate DirectX (CMake warning + header comments) — but the immediate goal is making DIRECTX_ENABLED builds compile
3. Verify the change compiles syntactically (at minimum, check braces/preprocessor balance)
4. Report: exact files and lines changed

## Deliverable
A patch/diff showing the Execute guard, plus a note on whether the project builds (if buildable in this environment).