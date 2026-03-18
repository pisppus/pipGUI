#!/usr/bin/env python3
import argparse
import hashlib
import json
import os
import shutil
from pathlib import Path

from _util import (
    ensure_pynacl,
    manifest_sig_payload_v5,
    parse_version,
    pubkey_from_seed_ed25519,
    read_priv_seed_hex,
    sign_ed25519,
    validate_esp_image,
    version_to_build,
)


def _read_json(path: Path) -> dict | None:
    try:
        if not path.exists():
            return None
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return None


def _write_json(path: Path, obj: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(obj, indent=2) + "\n", encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser(description="Create an OTA release folder + update index.json files.")
    ap.add_argument("--bin", required=True, help="Path to PlatformIO firmware.bin (e.g. .pio/build/<env>/firmware.bin)")
    ap.add_argument("--project", default=None, help="Project slug used on the site (default: auto-detect)")
    ap.add_argument("--channel", required=True, choices=["stable", "beta"], help="Release channel")
    ap.add_argument("--title", required=True, help="Firmware title shown in UI (e.g. 'Pipboy OS')")
    ap.add_argument("--version", required=True, help="Human version X.Y.Z (required)")
    ap.add_argument("--site-base", required=True, help="Base HTTPS URL to your /fw directory, e.g. https://example.com/fw")
    ap.add_argument("--out-dir", default="tools/ota/out", help="Output directory root (will create <project>/...)")
    ap.add_argument("--desc", default="", help="Release notes text (optional). Empty means no description.")
    ap.add_argument("--desc-file", default=None, help="Path to a text file for --desc (UTF-8). Overrides --desc.")
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
    project = detect_project_name()
    if not project:
        raise SystemExit("Failed to detect project name; pass --project")

    channel = str(args.channel).strip().lower()
    if channel not in ("stable", "beta"):
        raise SystemExit("--channel must be stable or beta")

    title = str(args.title).strip()
    if not title:
        raise SystemExit("--title must not be empty")
    if "\x00" in title:
        raise SystemExit("--title contains NUL byte")
    if len(title.encode("utf-8")) > 64:
        raise SystemExit("--title too long (max 64 UTF-8 bytes)")

    version = str(args.version).strip()
    major, minor, patch = parse_version(version)
    build = version_to_build(major, minor, patch)

    site_base = str(args.site_base).rstrip("/")
    if not site_base.startswith("https://"):
        raise SystemExit("--site-base must start with https://")

    release_dir = out_dir / project / channel / version
    release_dir.mkdir(parents=True, exist_ok=True)

    staged_bin = release_dir / "fw.bin"
    shutil.copyfile(bin_path, staged_bin)

    data = staged_bin.read_bytes()
    sha_hex = hashlib.sha256(data).hexdigest()
    size = len(data)
    url = f"{site_base}/{project}/{channel}/{version}/fw.bin"

    desc = str(args.desc)
    if args.desc_file:
        desc = Path(args.desc_file).read_text(encoding="utf-8", errors="replace")
    desc = desc.strip()
    if "\x00" in desc:
        raise SystemExit("--desc contains NUL byte")
    if len(desc.encode("utf-8")) > 255:
        raise SystemExit("--desc too long (max 255 UTF-8 bytes)")

    manifest: dict[str, object] = {
        "title": title,
        "version": version,
        "build": int(build),
        "size": int(size),
        "sha256": sha_hex,
        "url": url,
        "desc": desc,
    }

    ensure_pynacl(verbose)
    seed_path = detect_sign_key()
    if seed_path is None:
        raise SystemExit(
            "Signing key not found. Generate one with:\n"
            "  python tools/ota/keygen.py\n"
            "or pass --sign-key <path-to-ota_ed25519.key>"
        )
    seed = read_priv_seed_hex(seed_path)
    pub = pubkey_from_seed_ed25519(seed)
    payload = manifest_sig_payload_v5(
        str(manifest["title"]),
        str(manifest["version"]),
        int(manifest["build"]),
        int(manifest["size"]),
        str(manifest["sha256"]),
        str(manifest["url"]),
        str(manifest["desc"]),
    )
    sig = sign_ed25519(seed, payload)
    manifest["sig_ed25519"] = sig.hex()

    _write_json(release_dir / "manifest.json", manifest)

    # Update channel index: /<project>/<channel>/index.json
    chan_index_path = out_dir / project / channel / "index.json"
    chan_index = _read_json(chan_index_path) or {}
    releases = list(chan_index.get("releases", [])) if isinstance(chan_index.get("releases", []), list) else []

    rel_manifest_path = f"{channel}/{version}/manifest.json"
    new_entry = {"version": version, "build": int(build), "manifest": rel_manifest_path}

    seen_versions: set[str] = set()
    merged: list[dict] = []
    merged.append(new_entry)
    seen_versions.add(version)
    for e in releases:
        if not isinstance(e, dict):
            continue
        v = str(e.get("version", "")).strip()
        if not v or v in seen_versions:
            continue
        b = int(e.get("build", 0))
        mp = str(e.get("manifest", "")).strip()
        if not mp:
            mp = f"{channel}/{v}/manifest.json"
        merged.append({"version": v, "build": b, "manifest": mp})
        seen_versions.add(v)

    merged.sort(key=lambda x: int(x.get("build", 0)), reverse=True)

    chan_index_out: dict[str, object] = {
        "project": project,
        "channel": channel,
        "title": title,
        "releases": merged,
    }
    _write_json(chan_index_path, chan_index_out)

    # Update project index: /<project>/index.json
    proj_index_path = out_dir / project / "index.json"
    proj_index = _read_json(proj_index_path) or {}

    def _pick_latest(channel_key: str) -> dict | None:
        p = out_dir / project / channel_key / "index.json"
        j = _read_json(p) or {}
        rr = j.get("releases", [])
        if not isinstance(rr, list) or not rr:
            return None
        best = None
        for e in rr:
            if not isinstance(e, dict):
                continue
            if "version" not in e or "build" not in e or "manifest" not in e:
                continue
            if best is None or int(e.get("build", 0)) > int(best.get("build", 0)):
                best = e
        if best is None:
            return None
        return {"version": str(best["version"]), "build": int(best["build"]), "manifest": str(best["manifest"])}

    stable_latest = _pick_latest("stable")
    beta_latest = _pick_latest("beta")

    proj_index_out: dict[str, object] = {
        "project": project,
        "title": title,
        "stable": stable_latest or {},
        "beta": beta_latest or {},
    }
    _write_json(proj_index_path, proj_index_out)

    print("[ok] staged:")
    print(f"- {release_dir / 'fw.bin'}")
    print(f"- {release_dir / 'manifest.json'}")
    print(f"- {proj_index_path}")
    print(f"- {chan_index_path}")
    print(f"[ota] pubkey_ed25519={pub.hex()}")
    print("[info] deploy to:")
    print(f"- {site_base}/{project}/index.json")
    print(f"- {site_base}/{project}/{channel}/index.json")
    print(f"- {site_base}/{project}/{channel}/{version}/manifest.json")
    print(f"- {url}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
