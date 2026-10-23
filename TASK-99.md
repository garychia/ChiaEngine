# OpenCode Task — Fix Issue #99: Deprecate and remove broken OpenGL backend

## Goal
Deprecate the OpenGL backend per the assessment in docs/agents/opengl-backend-assessment.md.

## Context
- Repo: github.com/garychia/ChiaEngine (main branch)
- Workdir: /tmp/oc-ci (fresh clone of main)
- Issue: #99 — https://github.com/garychia/ChiaEngine/issues/99
- Reference: docs/agents/opengl-backend-assessment.md

## Problem
OpenGL backend is not wired into the Frame/IFrameExecutor architecture, GLAD is not vendored, Renderer has no OpenGL branch, and the backend cannot compile or run. The assessment recommends deprecation (CMake warning + README update + header comments, keep files as non-Vulkan fallback).

## Acceptance criteria
1. Mark OPENGL_ENABLED as DEPRECATED in CMakeLists.txt with a message(WARNING ...)
2. Add deprecation header comments to OpenGLRenderer.hpp, OpenGLRenderer.cpp, OpenGLHelper.hpp, OpenGLHelper.cpp
3. Update README.md backend table/section to mark OpenGL deprecated
4. Verify the Vulkan build still configures (cmake -B build -GNinja on Linux if deps available)
5. Report: exact files/lines changed

## Deliverable
A patch/diff showing deprecation changes, plus build verification result.