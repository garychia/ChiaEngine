# Visual Verification for ChiaApp (screenshot → VLM)

## Why
VUID counting in the run log proves the Vulkan pipeline executed without
validation errors, but it cannot prove what is **on screen**. The
`scripts/vlm_verify.py` tool closes that blind spot: it grabs the live
ChiaApp GLFW window pixels and asks a vision-language model to describe the
scene.

## Verified capture pipeline (2026-08-09, Gary's box)
1. ChiaApp must be running on the **logged-in desktop session**:
   ```bash
   export DISPLAY=:0
   export XAUTHORITY=/run/user/1000/.mutter-Xwaylandauth.<rand>   # find with: ls /run/user/1000/ | grep -i xwayland
   export WAYLAND_DISPLAY=wayland-0
   ~/github/ChiaEngine/build/bin/ChiaApp &> /tmp/chiaapp_run.log &
   ```
2. Capture:
   ```bash
   python3 scripts/vlm_verify.py --out /tmp/chia_shot.png
   ```
3. Optionally describe with an NVIDIA NIM VLM (needs `NVIDIA_API_KEY`):
   ```bash
   python3 scripts/vlm_verify.py --describe
   python3 scripts/vlm_verify.py --all-models     # probe all known-good models
   ```

## Capture paths — what works and what does NOT
| method | result |
|---|---|
| **python-xlib `XGetImage` on the GLFW child window** (`class=GLFW-Application`, e.g. `0xc00021`) | ✅ the only reliable path |
| cua-driver `get_window_state` | ⚠️ works but returns a scaled-down copy (334×304 vs real 1029×771) — ok for humans, weak for VLM |
| ffmpeg `x11grab` of the window rect | ❌ **all black** (Xwayland does not composite the GLFW surface into the X root grab) |
| `xwd`/`import` | ❌ not installed on this box (needs sudo) |

## VLM model status on NVIDIA NIM (probed 2026-08-09)
- **All `qwen/*` vision models are EOL** (retired 2026-07-20; API returns
  404/410). Docs pages remain but the endpoints are dead.
- LIVE and tested on the ChiaEngine scene (cube textured with an orange-cat
  photo on dark blue-gray background — confirmed by eyeball 2026-08-09):

| model | description of the actual scene | verdict |
|---|---|---|
| `nvidia/nemotron-3-nano-omni-30b-a3b-reasoning` | "cube with orange tabby cats on its visible faces" | ✅ most accurate |
| `thinkingmachines/inkling` | "Solid dark blue-gray background. One central 3D object" | ✅ correct but vague |
| `nvidia/ising-calibration-1.5-31b` | "stepped pyramid with wing-like structures" | ❌ hallucinates |
| Hermes aux vision (`meta/llama-3.2-90b-vision-instruct`) | "dark blue background, one central light-yellow object" | ⚠️ misses the cat texture |

**GOTCHA (we got this wrong first):** an earlier pixel analysis read the
object region mean color (165,138,108) as a "warm-tan cube" and used that as
ground truth to mark nemotron's "orange tabby cats" as a hallucination. The
warm tan IS the cat's fur — nemotron was right. **Never trust a pixel-color
interpretation over an actual human eyeball.** When a VLM names a concrete
recognizable subject (animal, logo, text) and another says only "object",
trust the concrete one and verify by eye.

**Rule:** treat any single VLM's detailed claims (textures, animals, extra
objects) as unverified. Cross-check with pixel analysis
(`object region mean color ≈ (165,138,108)` for the standard cube) or the
saved PNG.

## Practical assertions you can script
- Window exists with the right size: `xwininfo -root -tree | grep GLFW-Application`
- Scene is not black: mean luminance of the captured PNG > threshold
- Cube present: a bright (>100 lum) pixel cluster near the window center
  with warm hue (R>G>B)
- VLM sanity: prefer `inkling`, and only accept object-presence claims
  (number of objects, background color) — not textures.

## Requirements
- `python3 -m pip install python-xlib Pillow` (user-level, no sudo needed)
- `xwininfo` (x11-utils) — usually present on desktop installs
- DISPLAY + XAUTHORITY of the logged-in GNOME session (Wayland: Xwayland
  auth file under `/run/user/1000/`)
