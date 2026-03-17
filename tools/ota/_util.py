from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path


def find_platformio_esptool() -> Path | None:
    override = os.environ.get("PIPGUI_ESPTOOL", "").strip()
    if override:
        p = Path(override)
        if p.exists():
            return p

    home = Path.home()
    candidates = [
        home / ".platformio" / "packages" / "tool-esptoolpy" / "esptool.py",
        home / ".platformio" / "penv" / ("Scripts" if os.name == "nt" else "bin") / "esptool.py",
        home / ".platformio" / "penv" / ("Scripts" if os.name == "nt" else "bin") / "esptool",
    ]
    for c in candidates:
        if c.exists():
            return c
    return None


def validate_esp_image(bin_path: Path, *, chip: str = "esp32s3", verbose: bool = False) -> None:
    data0 = bin_path.read_bytes()[:1]
    if not data0:
        raise SystemExit(f"[ota] firmware is empty: {bin_path}")
    if data0[0] != 0xE9:
        raise SystemExit(
            f"[ota] firmware does not look like an ESP app image (magic=0x{data0[0]:02x}, expected 0xE9): {bin_path}"
        )

    esptool = find_platformio_esptool()
    if not esptool:
        # Magic check is better than nothing; full verification requires esptool/IDF libs.
        if verbose:
            print("[ota] esptool not found; skipped deep image validation")
        return

    cmd = [sys.executable, str(esptool), "--chip", chip, "image_info", str(bin_path)]
    p = subprocess.run(cmd, capture_output=not verbose, text=True)
    if p.returncode != 0:
        msg = (p.stderr or p.stdout or "").strip()
        if not msg:
            msg = f"esptool returned {p.returncode}"
        raise SystemExit(f"[ota] invalid {chip} app image: {msg[:800]}")

