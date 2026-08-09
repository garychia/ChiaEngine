#!/usr/bin/env python3
"""ChiaEngine visual verification: capture the live ChiaApp GLFW window and
describe it with a vision-language model (VLM).

WHY THIS EXISTS
---------------
VUID counting in the run log proves the Vulkan pipeline executed without
validation errors, but it cannot prove what is *on screen*. This tool closes
that blind spot: it grabs the actual rendered window pixels and asks a VLM
to describe the scene, giving an automated (if approximate) visual check.

PIPELINE (verified 2026-08-09 on Gary's box):
  DISPLAY=:0 XAUTHORITY=<session auth>  →  find GLFW window by class
  →  XGetImage (python-xlib)  →  PNG  →  NVIDIA NIM VLM chat/completions

PITFALLS LEARNED (do not regress):
  - ffmpeg x11grab of the window rect returns ALL BLACK (Xwayland does not
    composite the GLFW surface into the X root grab).
  - cua-driver get_window_state returns the window but at a scaled-down
    size (334x304 here vs real 1029x771) — fine for humans, bad for VLM.
  - python-xlib XGetImage on the GLFW child window (class GLFW-Application,
    the *second* "Chia Engine" frame) is the ONLY reliable capture path.
  - NVIDIA VLM models hallucinate on complex screenshots (ising-calibration
    described "wooden cube with orange dragon heads" for a plain warm-tan
    cube). Use Hermes aux vision (llama-3.2-90b-vision-instruct) or eyeball
    the saved PNG for ground truth; treat NVIDIA VLM output as a hint.
  - All qwen/* vision models are EOL on NIM (2026-07-20); use the models
    probed live here instead.

USAGE
-----
  # capture only (no VLM call, no NVIDIA key needed)
  python3 scripts/vlm_verify.py --out /tmp/chia_shot.png

  # capture + NVIDIA VLM description (default model)
  python3 scripts/vlm_verify.py --describe

  # specific model / custom NVIDIA key
  python3 scripts/vlm_verify.py --describe --model nvidia/ising-calibration-1.5-31b
  NVIDIA_API_KEY=... python3 scripts/vlm_verify.py --describe

REQUIRES
  python3 -m pip install python-xlib Pillow   (user-level)
  DISPLAY and XAUTHORITY of the logged-in session (see .github/CI or docs)
"""
import argparse
import base64
import json
import os
import subprocess
import sys
import urllib.error
import urllib.request

PROBE_MODELS = [
    "nvidia/nemotron-3-nano-omni-30b-a3b-reasoning",  # most accurate on cube scene (probed 2026-08-09)
    "thinkingmachines/inkling",           # multimodal MoE reasoning, correct but vague
    "nvidia/ising-calibration-1.5-31b",   # Gemma 4 31B VLM, hallucinates detail
]

DESCRIBE_PROMPT = (
    "This is a screenshot of a 3D game engine scene viewport. "
    "Describe exactly what you see in the main scene area: any 3D objects "
    "(shape, color), background color. Is there a GUI toolbar with buttons "
    "or text? Be concise, no guessing."
)


def find_glfw_window():
    """Return the X11 window id of the ChiaApp GLFW surface (class
    GLFW-Application). Uses xwininfo -root -tree; fails loudly if missing."""
    out = subprocess.run(
        ["xwininfo", "-root", "-tree"], capture_output=True, text=True
    ).stdout
    for line in out.splitlines():
        if "GLFW-Application" in line or "glfw-application" in line:
            # format: 0xc00021 (has no name): ("glfw-application" ...) 1029x771+75+186
            parts = line.split()
            for p in parts:
                if p.startswith("0x"):
                    return int(p, 16)
    raise RuntimeError(
        "ChiaApp GLFW window not found. Is ChiaApp running on DISPLAY=:0 "
        "with the session XAUTHORITY? See docs/agents/visual-verification.md"
    )


def capture_window(win_id, out_path):
    """XGetImage the window contents to a PNG. The ONLY reliable path
    (ffmpeg x11grab returns black on Xwayland)."""
    from Xlib import X, display
    from PIL import Image

    d = display.Display()
    win = d.create_resource_object("window", win_id)
    geom = win.get_geometry()
    raw = win.get_image(0, 0, geom.width, geom.height, X.ZPixmap, 0xFFFFFFFF)
    img = Image.frombytes("RGB", (geom.width, geom.height), raw.data, "raw", "BGRX")
    img.save(out_path)
    return out_path, geom.width, geom.height


def get_nvidia_key():
    """NVIDIA_API_KEY env, else ~/.hermes/.env NVIDIA_API_KEY=..."""
    k = os.environ.get("NVIDIA_API_KEY")
    if k:
        return k
    for line in open(os.path.expanduser("~/.hermes/.env")):
        m = line.strip().split("=", 1)
        if len(m) == 2 and m[0] == "NVIDIA_API_KEY":
            return m[1]
    raise RuntimeError("NVIDIA_API_KEY not found (env or ~/.hermes/.env)")


def describe_with_vlm(image_path, model, key, timeout=90):
    b64 = base64.b64encode(open(image_path, "rb").read()).decode()
    body = {
        "model": model,
        "messages": [{
            "role": "user",
            "content": [
                {"type": "text", "text": DESCRIBE_PROMPT},
                {"type": "image_url",
                 "image_url": {"url": "data:image/png;base64," + b64}},
            ],
        }],
        "max_tokens": 200,
    }
    req = urllib.request.Request(
        "https://integrate.api.nvidia.com/v1/chat/completions",
        data=json.dumps(body).encode(),
        headers={"Authorization": "Bearer " + key, "Content-Type": "application/json"},
    )
    try:
        r = urllib.request.urlopen(req, timeout=timeout)
        d = json.loads(r.read())
        return d["choices"][0]["message"]["content"]
    except urllib.error.HTTPError as e:
        return f"[HTTP {e.code}] {e.read().decode()[:200]}"
    except Exception as e:
        return f"[ERROR] {e}"


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", default="/tmp/chia_shot.png", help="PNG output path")
    ap.add_argument("--describe", action="store_true",
                    help="also send the capture to an NVIDIA VLM")
    ap.add_argument("--model", default=PROBE_MODELS[0],
                    help="VLM model id (default: %(default)s)")
    ap.add_argument("--all-models", action="store_true",
                    help="describe with every probe model")
    args = ap.parse_args()

    win_id = find_glfw_window()
    path, w, h = capture_window(win_id, args.out)
    print(f"captured {path} ({w}x{h}, window 0x{win_id:x})")

    if args.describe or args.all_models:
        key = get_nvidia_key()
        models = PROBE_MODELS if args.all_models else [args.model]
        for m in models:
            print(f"\n=== {m} ===")
            print(describe_with_vlm(path, m, key))
    print("\nNOTE: NVIDIA VLM descriptions are hints; verify with the saved PNG"
          " or Hermes aux vision (llama-3.2-90b-vision-instruct).")


if __name__ == "__main__":
    main()
