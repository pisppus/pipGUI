#!/usr/bin/env python3
import argparse
import hashlib
import json
import urllib.request
from pathlib import Path

from _util import ensure_pynacl, manifest_sig_payload_v5, parse_version, version_to_build

def _read_manifest(src: str) -> dict:
    if src.startswith("http://") or src.startswith("https://"):
        raw = urllib.request.urlopen(src, timeout=10).read().decode("utf-8")
        return json.loads(raw)
    return json.loads(Path(src).read_text(encoding="utf-8"))


def _read_bytes(src: str) -> bytes:
    if src.startswith("http://") or src.startswith("https://"):
        return urllib.request.urlopen(src, timeout=30).read()
    return Path(src).read_bytes()

def main() -> int:
    ap = argparse.ArgumentParser(description="Verify OTA manifest.json against a firmware binary.")
    ap.add_argument("--manifest", required=True, help="Path or HTTPS URL to manifest.json")
    ap.add_argument("--bin", default=None, help="Path or HTTPS URL to firmware.bin (default: manifest.url)")
    ap.add_argument("--pubkey", default=None, help="Ed25519 public key hex (64 chars). If set, verifies sig_ed25519.")
    args = ap.parse_args()

    verbose = os.environ.get("PIPGUI_TOOLS_VERBOSE", "0").strip().lower() in ("1", "true", "yes", "on")

    m = _read_manifest(args.manifest)
    for k in ("title", "version", "build", "size", "sha256", "url", "desc"):
        if k not in m:
            raise SystemExit(f"Missing field '{k}' in manifest")

    v = str(m["version"]).strip()
    major, minor, patch = parse_version(v)
    expected_build = version_to_build(major, minor, patch)
    if int(m["build"]) != expected_build:
        raise SystemExit(f"Invalid build in manifest: {m['build']} (expected {expected_build})")

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
        ensure_pynacl(verbose)
        from nacl.signing import VerifyKey
        from nacl.exceptions import BadSignatureError

        pub = bytes.fromhex(args.pubkey.strip())
        sig_hex = m.get("sig_ed25519", "")
        if not sig_hex:
            print("[fail] sig_ed25519 missing")
            ok = False
        else:
            sig = bytes.fromhex(str(sig_hex))
            try:
                payload = manifest_sig_payload_v5(
                    str(m["title"]),
                    str(m["version"]),
                    int(m["build"]),
                    int(m["size"]),
                    str(m["sha256"]),
                    str(m["url"]),
                    str(m["desc"]),
                )
                VerifyKey(pub).verify(payload, sig)
                print("[ok] sig_ed25519 valid")
            except BadSignatureError:
                print("[fail] sig_ed25519 invalid (pubkey mismatch or manifest fields changed?)")
                ok = False

    print(f"[info] title={m['title']} version={m['version']} build={m['build']} url={m['url']}")
    return 0 if ok else 2


if __name__ == "__main__":
    raise SystemExit(main())
