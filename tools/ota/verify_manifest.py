#!/usr/bin/env python3
import argparse
import hashlib
import json
import os
import subprocess
import sys
import urllib.request
from pathlib import Path


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


def _read_manifest(src: str) -> dict:
    if src.startswith("http://") or src.startswith("https://"):
        raw = urllib.request.urlopen(src, timeout=10).read().decode("utf-8")
        return json.loads(raw)
    return json.loads(Path(src).read_text(encoding="utf-8"))


def _read_bytes(src: str) -> bytes:
    if src.startswith("http://") or src.startswith("https://"):
        return urllib.request.urlopen(src, timeout=30).read()
    return Path(src).read_bytes()


def manifest_sig_payload_v2(version: str, size: int, sha256_hex: str, url: str) -> bytes:
    prefix = b"pipgui-ota-manifest-v2"
    v = version.strip()
    if "\x00" in v:
        raise SystemExit("Invalid version (contains NUL)")
    sha = bytes.fromhex(sha256_hex)
    if len(sha) != 32:
        raise SystemExit(f"Invalid sha256 hex (expected 32 bytes, got {len(sha)})")
    if "\x00" in url:
        raise SystemExit("Invalid url (contains NUL)")
    return prefix + v.encode("ascii") + b"\x00" + int(size).to_bytes(4, "big") + sha + url.encode("utf-8")

def main() -> int:
    ap = argparse.ArgumentParser(description="Verify OTA manifest.json against a firmware binary.")
    ap.add_argument("--manifest", required=True, help="Path or HTTPS URL to manifest.json")
    ap.add_argument("--bin", default=None, help="Path or HTTPS URL to firmware.bin (default: manifest.url)")
    ap.add_argument("--pubkey", default=None, help="Ed25519 public key hex (64 chars). If set, verifies sig_ed25519.")
    args = ap.parse_args()

    verbose = os.environ.get("PIPGUI_TOOLS_VERBOSE", "0").strip().lower() in ("1", "true", "yes", "on")

    m = _read_manifest(args.manifest)
    for k in ("version", "size", "sha256", "url"):
        if k not in m:
            raise SystemExit(f"Missing field '{k}' in manifest")

    v = str(m["version"]).strip()
    parts = v.split(".")
    if len(parts) != 3 or not all(p.isdigit() for p in parts):
        raise SystemExit(f"Invalid version in manifest: {v} (expected X.Y.Z)")

    bin_src = args.bin or str(m["url"])
    data = _read_bytes(bin_src)
    sha = hashlib.sha256(data).hexdigest()

    ok = True
    if len(data) != int(m["size"]):
        print(f"[fail] size: manifest={m['size']} actual={len(data)}")
        ok = False
    else:
        print(f"[ok] size={len(data)}")

    if sha != str(m["sha256"]):
        print(f"[fail] sha256: manifest={m['sha256']} actual={sha}")
        ok = False
    else:
        print(f"[ok] sha256={sha}")

    if args.pubkey is not None:
        _ensure_pynacl(verbose)
        from nacl.signing import VerifyKey  # type: ignore
        from nacl.exceptions import BadSignatureError  # type: ignore

        pub = bytes.fromhex(args.pubkey.strip())
        sig_hex = m.get("sig_ed25519", "")
        if not sig_hex:
            print("[fail] sig_ed25519 missing")
            ok = False
        else:
            sig = bytes.fromhex(str(sig_hex))
            try:
                payload = manifest_sig_payload_v2(str(m["version"]), int(m["size"]), str(m["sha256"]), str(m["url"]))
                VerifyKey(pub).verify(payload, sig)
                print("[ok] sig_ed25519 valid")
            except BadSignatureError:
                print("[fail] sig_ed25519 invalid (pubkey mismatch or manifest fields changed?)")
                ok = False

    print(f"[info] version={m['version']} url={m['url']}")
    return 0 if ok else 2


if __name__ == "__main__":
    raise SystemExit(main())
