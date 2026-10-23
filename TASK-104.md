# OpenCode Task — Fix Issue #104: Release job ships Linux artifacts only

## Goal
Add Windows zip and macOS artifacts to the release job so tags produce cross-platform releases.

## Context
- Repo: github.com/garychia/ChiaEngine (main branch, PR #96 already merged)
- Workdir: /tmp/oc-ci (fresh clone of main)
- Issue: #104 — https://github.com/garychia/ChiaEngine/issues/104

## Problem
ci.yml release job (lines 309-322) only creates `chiaengine-<tag>-linux.tar.gz`. The Windows zip is commented out (line 321-322) and there are no macOS artifacts.

## Acceptance criteria
1. Uncomment and implement the Windows zip step (zip -r for Windows builds)
2. Add macOS artifact step (tar.gz or zip)
3. Update release body to list all platform assets
4. YAML remains valid
5. Report: exact lines changed, before/after

## Deliverable
A patch/diff showing the added release artifact steps, plus confirmation YAML is valid.