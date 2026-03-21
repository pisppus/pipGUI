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


def ensure_pynacl(verbose: bool) -> None:
    try:
        import nacl.signing

        return
    except Exception:
        pass

    def _pip_ok() -> bool:
        try:
            p = subprocess.run([sys.executable, "-m", "pip", "--version"], capture_output=True, text=True)
            return p.returncode == 0
        except Exception:
            return False

    if not _pip_ok():
        try:
            import ensurepip

            if verbose:
                print("[ota] bootstrapping pip")
            ensurepip.bootstrap(upgrade=True)
        except Exception as e:
            raise SystemExit(f"[ota] pip is not available and ensurepip failed: {e}")

    print("[ota] python deps: installing pynacl")
    cmd = [sys.executable, "-m", "pip", "install", "pynacl"]
    if not verbose:
        cmd.append("-q")
    p = subprocess.run(cmd, capture_output=not verbose, text=True)
    if p.returncode != 0:
        if not verbose:
            err = (p.stderr or "").strip()
            if err:
                raise SystemExit(f"[ota] pip failed: {err[:400]}")
        raise SystemExit("[ota] pip failed")

    try:
        import nacl.signing
    except Exception as e:
        raise SystemExit(f"[ota] pynacl import failed after install: {e}")


def read_priv_seed_hex(path: Path) -> bytes:
    seed_hex = path.read_text(encoding="utf-8").strip()
    try:
        seed = bytes.fromhex(seed_hex)
    except ValueError as e:
        raise SystemExit(f"Invalid hex in {path}: {e}")
    if len(seed) != 32:
        raise SystemExit(f"Expected 32-byte seed in {path}, got {len(seed)} bytes")
    return seed


def pubkey_from_seed_ed25519(priv_seed: bytes) -> bytes:
    from nacl import signing

    sk = signing.SigningKey(priv_seed)
    return sk.verify_key.encode()


def sign_ed25519(priv_seed: bytes, message: bytes) -> bytes:
    from nacl import signing

    sk = signing.SigningKey(priv_seed)
    sig = sk.sign(message).signature
    if len(sig) != 64:
        raise SystemExit(f"Unexpected signature size: {len(sig)}")
    return sig


def parse_version(version: str) -> tuple[int, int, int]:
    v = version.strip()
    parts = v.split(".")
    if len(parts) != 3:
        raise SystemExit("--version must be in form X.Y.Z")
    try:
        major = int(parts[0])
        minor = int(parts[1])
        patch = int(parts[2])
    except ValueError:
        raise SystemExit("--version must be in form X.Y.Z (numbers only)")
    if major < 0 or minor < 0 or patch < 0:
        raise SystemExit("--version must be non-negative")
    if minor >= 1000 or patch >= 1000 or major >= 10000:
        raise SystemExit("--version too large (max: major<10000 minor<1000 patch<1000)")
    return major, minor, patch


def version_to_build(major: int, minor: int, patch: int) -> int:
    return (major * 1_000_000) + (minor * 1_000) + patch


def manifest_sig_payload(title: str, version: str, build: int, size: int, sha256_hex: str, url: str, desc: str) -> bytes:
    prefix = b"pipgui-ota-manifest"
    t = title.strip()
    v = version.strip()
    if not t:
        raise SystemExit("Title must not be empty")
    if "\x00" in t:
        raise SystemExit("Title must not contain NUL bytes")
    if "\x00" in v:
        raise SystemExit("Version must not contain NUL bytes")
    if "\x00" in url:
        raise SystemExit("URL must not contain NUL bytes")
    if "\x00" in desc:
        raise SystemExit("Description must not contain NUL bytes")
    if int(build) < 0:
        raise SystemExit("Build must be non-negative")
    sha = bytes.fromhex(sha256_hex)
    if len(sha) != 32:
        raise SystemExit(f"Expected sha256 to be 32 bytes, got {len(sha)}")
    return (
        prefix
        + t.encode("utf-8")
        + b"\x00"
        + v.encode("ascii")
        + b"\x00"
        + int(build).to_bytes(8, "big")
        + int(size).to_bytes(4, "big")
        + sha
        + url.encode("utf-8")
        + b"\x00"
        + desc.encode("utf-8")
    )
