#!/usr/bin/env python3
import argparse
import hashlib
import json
import os
import shutil
import sys
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1]
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

from _menu import choose, choose_existing_path, prompt_multiline, prompt_text
from _util import (
    ensure_pynacl,
    manifest_sig_payload,
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


def _find_firmware_bins(repo_root: Path) -> list[Path]:
    return sorted(repo_root.glob(".pio/build/*/firmware.bin"))


def _detect_project_name(repo_root: Path, explicit: str | None) -> str:
    if explicit:
        return str(explicit).strip()

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
                    in_platformio = s.lower() == "[platformio]"
                    continue
                if in_platformio and s.lower().startswith("name"):
                    parts = s.split("=", 1)
                    if len(parts) == 2 and parts[1].strip():
                        return parts[1].strip()
        except Exception:
            pass
    return repo_root.name


def _detect_sign_key(repo_root: Path, explicit: str | None) -> Path | None:
    if explicit:
        return Path(explicit)
    candidates = [
        repo_root / "tools" / "ota" / "keys" / "ota_ed25519.key",
        repo_root / "ota_ed25519.key",
        repo_root / "tools" / "ota" / "ota_ed25519.key",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return None


def _interactive_inputs(repo_root: Path, args: argparse.Namespace) -> tuple[str, str, str, str, str, str]:
    bin_path = choose_existing_path(
        "Firmware binary",
        _find_firmware_bins(repo_root),
        custom_label="Manual firmware path",
    )
    channel = choose("Channel", ["stable", "beta"])
    project = _detect_project_name(repo_root, args.project)
    title = prompt_text("Release title", default=project)
    version = prompt_text("Version", default="1.0.0")
    site_base = prompt_text(
        "Site base",
        default="https://example.com/fw",
        validator=lambda value: None if value.startswith("https://") else "Must start with https://",
    )
    desc_mode = choose(
        "Release notes",
        ["Empty", "Type notes", "Load from file"],
    )
    desc = ""
    if desc_mode == "Type notes":
        desc = prompt_multiline("Release notes")
    elif desc_mode == "Load from file":
        desc_path = choose_existing_path(
            "Release notes file",
            sorted(repo_root.glob("*.txt")),
            custom_label="Manual notes path",
        )
        desc = desc_path.read_text(encoding="utf-8", errors="replace").strip()

    args.project = project
    return str(bin_path), channel, title, version, site_base, desc


def main() -> int:
    ap = argparse.ArgumentParser(description="Create an OTA release folder + update index.json files.")
    ap.add_argument("--stable", action="store_true", help="Create a stable release")
    ap.add_argument("--beta", action="store_true", help="Create a beta release")
    ap.add_argument("--project", default=None, help="Project slug used on the site (default: auto-detect)")
    ap.add_argument("--out-dir", default="tools/ota/out", help="Output directory root (will create <project>/...)")
    ap.add_argument("--desc-file", default=None, help="Path to a UTF-8 text file with release notes")
    ap.add_argument("--sign-key", default=None, help="Path to Ed25519 seed (hex) created by keygen.py")
    ap.add_argument("--interactive", action="store_true", help="Choose values from interactive menus")
    args, extra = ap.parse_known_args()

    verbose = os.environ.get("PIPGUI_TOOLS_VERBOSE", "0").strip().lower() in ("1", "true", "yes", "on")
    repo_root = Path(__file__).resolve().parents[2]

    compact = [token[2:] if token.startswith("--") else token for token in extra]
    interactive = args.interactive or (not compact and not args.stable and not args.beta)
    if interactive:
        bin_arg, channel, title_arg, version_arg, site_base_arg, desc_arg = _interactive_inputs(repo_root, args)
        args.stable = channel == "stable"
        args.beta = channel == "beta"
    else:
        if args.stable == args.beta:
            raise SystemExit("Choose exactly one channel flag: --stable or --beta")
        if len(compact) not in (4, 5):
            raise SystemExit(
                "Usage:\n"
                "  python tools/ota/make_release.py\n"
                "  python tools/ota/make_release.py --.pio/build/<env>/firmware.bin --stable --\"Title\" --1.2.3 --https://<domain>/fw [--\"Notes\"]\n"
                "  python tools/ota/make_release.py --beta --interactive"
            )
        bin_arg, title_arg, version_arg, site_base_arg = compact[:4]
        desc_arg = compact[4] if len(compact) == 5 else ""

    bin_path = Path(bin_arg)
    if not bin_path.exists():
        raise SystemExit(f"Missing file: {bin_path}")

    validate_esp_image(bin_path, verbose=verbose)

    out_dir = Path(args.out_dir)
    project = _detect_project_name(repo_root, args.project)
    if not project:
        raise SystemExit("Failed to detect project name; pass --project")

    channel = "stable" if args.stable else "beta"

    title = str(title_arg).strip()
    if not title:
        raise SystemExit("--title must not be empty")
    if "\x00" in title:
        raise SystemExit("--title contains NUL byte")
    if len(title.encode("utf-8")) > 64:
        raise SystemExit("--title too long (max 64 UTF-8 bytes)")

    version = str(version_arg).strip()
    major, minor, patch = parse_version(version)
    build = version_to_build(major, minor, patch)

    site_base = str(site_base_arg).rstrip("/")
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

    desc = str(desc_arg)
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
    seed_path = _detect_sign_key(repo_root, args.sign_key)
    if seed_path is None:
        raise SystemExit(
            "Signing key not found. Generate one with:\n"
            "  python tools/ota/keygen.py\n"
            "or pass --sign-key <path-to-ota_ed25519.key>"
        )
    seed = read_priv_seed_hex(seed_path)
    pub = pubkey_from_seed_ed25519(seed)
    payload = manifest_sig_payload(
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
