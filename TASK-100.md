# OpenCode Task — Fix Issue #100: Pong ball tunnels through left paddle

## Goal
Fix the Pong ball-paddle collision bug where the ball tunnels through the left paddle and contact collapses/inverts ball speed.

## Context
- Repo: github.com/garychia/ChiaEngine (main branch)
- Workdir: /tmp/oc-ci (fresh clone of main)
- Issue: #100 — https://github.com/garychia/ChiaEngine/issues/100
- Related: PR #95 fixed camera yaw (#94); this is the ReflectBall physics bug

## Problem
Pong ball tunnels through the left paddle; paddle contact collapses/inverts ball speed in ReflectBall. The collision response in PongSystem is broken.

## Acceptance criteria
1. Locate ReflectBall / paddle collision code in src/ (likely src/ChiaApp/PongSystem.cpp or similar)
2. Fix the collision response so the ball bounces correctly off both paddles
3. Verify TestMain passes (PongSystem tests included)
4. Report: exact files/lines changed, test results

## Deliverable
A patch/diff showing the collision fix, plus TestMain run results (pass/fail counts).