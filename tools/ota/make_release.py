#!/usr/bin/env python3
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

from _util import validate_esp_image


def _ensure_pynacl(verbose: bool) -> None:
    try:
        import nacl.signing  # type: ignore

        return
    except Exception:
        pass

    def _ensure_pip_available() -> bool:
        try:
            p = subprocess.run([sys.executable, "-m", "pip", "--version"], capture_output=True, text=True)
            return p.returncode == 0
        except Exception:
            return False

    if not _ensure_pip_available():
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
        import nacl.signing  # type: ignore
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
    from nacl import signing  # type: ignore

    sk = signing.SigningKey(priv_seed)
    return sk.verify_key.encode()


def sign_ed25519(priv_seed: bytes, message: bytes) -> bytes:
    from nacl import signing  # type: ignore

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


def manifest_sig_payload_v2(version: str, size: int, sha256_hex: str, url: str) -> bytes:
    prefix = b"pipgui-ota-manifest-v2"
    v = version.strip()
    if "\x00" in v:
        raise SystemExit("Version must not contain NUL bytes")
    if "\x00" in url:
        raise SystemExit("URL must not contain NUL bytes")
    sha = bytes.fromhex(sha256_hex)
    if len(sha) != 32:
        raise SystemExit(f"Expected sha256 to be 32 bytes, got {len(sha)}")
    return prefix + v.encode("ascii") + b"\x00" + int(size).to_bytes(4, "big") + sha + url.encode("utf-8")


def main() -> int:
    ap = argparse.ArgumentParser(description="Stage a firmware + manifest into tools/ota/out for easy deployment.")
    ap.add_argument("--bin", required=True, help="Path to PlatformIO firmware.bin (e.g. .pio/build/<env>/firmware.bin)")
    ap.add_argument("--version", required=True, help="Human version X.Y.Z (required)")
    ap.add_argument("--site-base", required=True, help="Base HTTPS URL to your /fw directory, e.g. https://example.com/fw")
    ap.add_argument("--out-dir", default="tools/ota/out", help="Output directory (manifest + staged bin)")
    ap.add_argument("--project", default=None, help="Project name used in default firmware file name (default: auto-detect)")
    ap.add_argument("--name", default=None, help="Firmware file name (default: <project>-<version>.bin)")
    ap.add_argument("--sign-key", default=None, help="Path to Ed25519 seed (hex) created by keygen.py (default: auto-detect)")
    args = ap.parse_args()

    verbose = os.environ.get("PIPGUI_TOOLS_VERBOSE", "0").strip().lower() in ("1", "true", "yes", "on")

    bin_path = Path(args.bin)
    if not bin_path.exists():
        raise SystemExit(f"Missing file: {bin_path}")

    validate_esp_image(bin_path, verbose=verbose)

    repo_root = Path(__file__).resolve().parents[2]

    def detect_project_name() -> str:
        if args.project:
            return str(args.project).strip()

        ini = repo_root / "platformio.ini"
        if ini.exists():
            try:
                text = ini.read_text(encoding="utf-8", errors="ignore").splitlines()
                in_platformio = False
                for line in text:
                    s = line.strip()
                    if not s or s.startswith(";") or s.startswith("#"):
                        continue
                    if s.startswith("[") and s.endswith("]"):
                        in_platformio = (s.lower() == "[platformio]")
                        continue
                    if in_platformio and s.lower().startswith("name"):
                        parts = s.split("=", 1)
                        if len(parts) == 2 and parts[1].strip():
                            return parts[1].strip()
            except Exception:
                pass
        return repo_root.name

    def detect_sign_key() -> Path | None:
        if args.sign_key:
            return Path(args.sign_key)
        candidates = [
            repo_root / "tools" / "ota" / "keys" / "ota_ed25519.key",
            repo_root / "ota_ed25519.key",
            repo_root / "tools" / "ota" / "ota_ed25519.key",
        ]
        for c in candidates:
            if c.exists():
                return c
        return None

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    project = detect_project_name()
    if not project:
        raise SystemExit("Failed to detect project name; pass --project")

    version = str(args.version).strip()
    parse_version(version)  # validate

    if args.name:
        fw_name = str(args.name)
    else:
        fw_name = f"{project}-{version}.bin"
    if "/" in fw_name or "\\" in fw_name:
        raise SystemExit("--name must be a file name (no slashes)")

    site_base = str(args.site_base).rstrip("/")
    if not site_base.startswith("https://"):
        raise SystemExit("--site-base must start with https://")

    staged_bin = out_dir / fw_name
    shutil.copyfile(bin_path, staged_bin)

    data = staged_bin.read_bytes()
    sha_hex = hashlib.sha256(data).hexdigest()
    size = len(data)
    url = f"{site_base}/{fw_name}"

    manifest: dict[str, object] = {
        "version": version,
        "size": int(size),
        "sha256": sha_hex,
        "url": url,
    }

    _ensure_pynacl(verbose)
    seed_path = detect_sign_key()
    if seed_path is None:
        raise SystemExit(
            "Signing key not found. Generate one with:\n"
            "  python tools/ota/keygen.py\n"
            "or pass --sign-key <path-to-ota_ed25519.key>"
        )
    seed = read_priv_seed_hex(seed_path)
    pub = pubkey_from_seed_ed25519(seed)
    payload = manifest_sig_payload_v2(manifest["version"], manifest["size"], manifest["sha256"], manifest["url"])
    sig = sign_ed25519(seed, payload)
    manifest["sig_ed25519"] = sig.hex()

    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

    print("[ok] staged:")
    print(f"- {staged_bin}")
    print(f"- {out_dir / 'manifest.json'}")
    print(f"[ota] pubkey_ed25519={pub.hex()}")
    print(f"[info] deploy these files to your site: /fw/{fw_name} and /fw/manifest.json")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
