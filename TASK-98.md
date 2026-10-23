# OpenCode Task — Fix Issue #98: CI macOS job hangs in Setup dependencies

## Goal
Fix the macOS CI job that hangs during 'Setup dependencies (macOS)'.

## Context
- Repo: github.com/garychia/ChiaEngine (main branch)
- Workdir: /tmp/oc-ci (fresh clone of main)
- Issue: #98 — https://github.com/garychia/ChiaEngine/issues/98

## Problem
The macOS job hangs in 'Setup dependencies (macOS)' — suspected causes: vulkan-sdk cask install, clang-tidy formula, or brew operations timing out.

## Acceptance criteria
1. Review ci.yml macOS Setup dependencies step (lines 68-75)
2. Identify likely hang causes (vulkan-sdk cask, clang-tidy brew install, brew update timeout)
3. Propose and apply fixes (e.g., pin formulas, add timeouts, split into smaller steps)
4. Report: findings and changes made

## Deliverable
A report on macOS CI hang root cause and any ci.yml changes.