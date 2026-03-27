#!/usr/bin/env python3
import argparse
import hashlib
import json
import os
import sys
import urllib.request
from pathlib import Path

os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")
sys.dont_write_bytecode = True

TOOLS_DIR = Path(__file__).resolve().parents[1]
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

from menu import choose, choose_existing_path, prompt_text
from util import ensure_pynacl, manifest_sig_payload, parse_version, pubkey_from_seed_ed25519, read_priv_seed_hex, version_to_build

def _read_manifest(src: str) -> dict:
    if src.startswith("http://") or src.startswith("https://"):
        raw = urllib.request.urlopen(src, timeout=10).read().decode("utf-8")
        return json.loads(raw)
    return json.loads(Path(src).read_text(encoding="utf-8"))


def _read_bytes(src: str) -> bytes:
    if src.startswith("http://") or src.startswith("https://"):
        return urllib.request.urlopen(src, timeout=30).read()
    return Path(src).read_bytes()


def _find_manifests(repo_root: Path) -> list[Path]:
    return sorted(repo_root.glob("tools/ota/out/*/*/*/manifest.json"))


def _find_bins(repo_root: Path) -> list[Path]:
    return sorted(repo_root.glob("tools/ota/out/*/*/*/fw.bin"))


def _detect_pubkey(repo_root: Path) -> str | None:
    candidates = [
        repo_root / "tools" / "ota" / "keys" / "ota_ed25519.key",
        repo_root / "ota_ed25519.key",
        repo_root / "tools" / "ota" / "ota_ed25519.key",
    ]
    for candidate in candidates:
        if candidate.exists():
            return pubkey_from_seed_ed25519(read_priv_seed_hex(candidate)).hex()
    return None


def _interactive_inputs(repo_root: Path, args: argparse.Namespace) -> tuple[str, str | None]:
    manifest_menu: list[tuple[str, str | None]] = [(str(path), str(path)) for path in _find_manifests(repo_root)]
    manifest_menu.append(("Manual manifest path or URL", None))
    manifest_pick = choose("Manifest", manifest_menu, lambda item: item[0])
    manifest_src = manifest_pick[1] or prompt_text("Manifest path or URL")
    source_mode = choose(
        "Firmware source",
        ["Use firmware URL from manifest", "Choose local fw.bin", "Enter custom path or URL"],
    )
    bin_src = None
    if source_mode == "Choose local fw.bin":
        bin_src = str(choose_existing_path("Firmware binary", _find_bins(repo_root), custom_label="Manual firmware path"))
    elif source_mode == "Enter custom path or URL":
        bin_src = prompt_text("Firmware path or URL")

    auto_pubkey = _detect_pubkey(repo_root)
    sig_options = ["Skip signature check"]
    if auto_pubkey:
        sig_options.append("Use detected OTA key")
    sig_options.append("Enter pubkey manually")
    sig_mode = choose("Signature check", sig_options)
    if sig_mode == "Use detected OTA key":
        args.pubkey = auto_pubkey
    elif sig_mode == "Enter pubkey manually":
        args.pubkey = prompt_text("Public key hex")
    return manifest_src, bin_src

def main() -> int:
    ap = argparse.ArgumentParser(description="Verify OTA manifest.json against a firmware binary.")
    ap.add_argument("--pubkey", default=None, help="Ed25519 public key hex (64 chars). If set, verifies sig_ed25519.")
    ap.add_argument("--interactive", action="store_true", help="Choose values from interactive menus")
    args, extra = ap.parse_known_args()

    compact = [token[2:] if token.startswith("--") else token for token in extra]
    repo_root = Path(__file__).resolve().parents[2]
    interactive = args.interactive or not compact
    if interactive:
        manifest_src, bin_src_arg = _interactive_inputs(repo_root, args)
    else:
        if len(compact) not in (1, 2, 3):
            raise SystemExit(
                "Usage:\n"
                "  python tools/ota/verify.py\n"
                "  python tools/ota/verify.py --tools/ota/out/<project>/stable/1.2.3/manifest.json [--tools/ota/out/<project>/stable/1.2.3/fw.bin] [--<pubkey>]"
            )
        manifest_src = compact[0]
        bin_src_arg = compact[1] if len(compact) == 2 else None
        if len(compact) == 3:
            bin_src_arg = compact[1]
            if args.pubkey is None:
                args.pubkey = compact[2]


    verbose = os.environ.get("PIPGUI_TOOLS_VERBOSE", "0").strip().lower() in ("1", "true", "yes", "on")

    m = _read_manifest(manifest_src)
    for k in ("title", "version", "build", "size", "sha256", "url", "desc"):
        if k not in m:
            raise SystemExit(f"Missing field '{k}' in manifest")

    v = str(m["version"]).strip()
    major, minor, patch = parse_version(v)
    expected_build = version_to_build(major, minor, patch)
    if int(m["build"]) != expected_build:
        raise SystemExit(f"Invalid build in manifest: {m['build']} (expected {expected_build})")

    bin_src = bin_src_arg or str(m["url"])
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
                payload = manifest_sig_payload(
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
