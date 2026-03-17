#!/usr/bin/env python3
import argparse
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path

from _util import validate_esp_image


def read_priv_seed_hex(path: Path) -> bytes:
    seed_hex = path.read_text(encoding="utf-8").strip()
    try:
        seed = bytes.fromhex(seed_hex)
    except ValueError as e:
        raise SystemExit(f"Invalid hex in {path}: {e}")
    if len(seed) != 32:
        raise SystemExit(f"Expected 32-byte seed in {path}, got {len(seed)} bytes")
    return seed


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


def sign_ed25519(priv_seed: bytes, message: bytes) -> bytes:
    from nacl import signing  # type: ignore

    sk = signing.SigningKey(priv_seed)
    sig = sk.sign(message).signature
    if len(sig) != 64:
        raise SystemExit(f"Unexpected signature size: {len(sig)}")
    return sig


def pubkey_from_seed_ed25519(priv_seed: bytes) -> bytes:
    from nacl import signing  # type: ignore

    sk = signing.SigningKey(priv_seed)
    return sk.verify_key.encode()


def verify_sig_ed25519(pub: bytes, message: bytes, sig64: bytes) -> bool:
    from nacl.signing import VerifyKey  # type: ignore
    from nacl.exceptions import BadSignatureError  # type: ignore

    try:
        VerifyKey(pub).verify(message, sig64)
        return True
    except BadSignatureError:
        return False


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
    # Must match the firmware verifier (Systems/Update/Ota.cpp).
    # Layout:
    #   prefix (ascii, no NUL)
    #   version (ascii, no NUL) e.g. "1.3.8"
    #   0 byte separator
    #   size (u32, big-endian)
    #   sha256 (32 bytes)
    #   url (utf-8 bytes, no trailing NUL)
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
    ap = argparse.ArgumentParser(description="Create OTA manifest.json for pipGUI OTA updater.")
    ap.add_argument("--bin", required=True, help="Path to firmware.bin")
    ap.add_argument("--version", required=True, help="Human version X.Y.Z (required)")
    ap.add_argument("--url", required=True, help="Public HTTPS URL to the firmware.bin")
    ap.add_argument("--out", default="tools/ota/out/manifest.json", help="Output manifest path")
    ap.add_argument("--sign-key", default=None, help="Path to Ed25519 seed (hex) created by keygen.py")
    args = ap.parse_args()

    verbose = os.environ.get("PIPGUI_TOOLS_VERBOSE", "0").strip().lower() in ("1", "true", "yes", "on")

    bin_path = Path(args.bin)
    if not bin_path.exists():
        print(f"Missing file: {bin_path}", file=sys.stderr)
        return 2

    validate_esp_image(bin_path, verbose=verbose)

    data = bin_path.read_bytes()
    sha = hashlib.sha256(data).digest()
    size = len(data)

    version = str(args.version).strip()
    parse_version(version)  # validate

    manifest: dict[str, object] = {
        "version": version,
        "size": int(size),
        "sha256": sha.hex(),
        "url": str(args.url),
    }

    if args.sign_key:
        _ensure_pynacl(verbose)
        seed = read_priv_seed_hex(Path(args.sign_key))
        pub = pubkey_from_seed_ed25519(seed)
        payload = manifest_sig_payload_v2(manifest["version"], manifest["size"], manifest["sha256"], manifest["url"])
        sig = sign_ed25519(seed, payload)
        # Keep signature semantics simple: sign the manifest payload directly (not JSON),
        # and self-check using the same payload.
        if not verify_sig_ed25519(pub, payload, sig):
            raise SystemExit("[ota] internal error: signature self-check failed (wrong seed?)")
        manifest["sig_ed25519"] = sig.hex()
        print(f"[ota] pubkey_ed25519={pub.hex()}")

    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote: {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
