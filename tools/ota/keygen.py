#!/usr/bin/env python3
import argparse
import os
import subprocess
import sys


def _ensure_pynacl(verbose: bool) -> bool:
    try:
        import nacl.signing  # type: ignore

        return True
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
            print(f"[ota] pip is not available and ensurepip failed: {e}", file=sys.stderr)
            return False

    print("[ota] python deps: installing pynacl")
    try:
        cmd = [sys.executable, "-m", "pip", "install", "pynacl"]
        if not verbose:
            cmd.append("-q")
        p = subprocess.run(cmd, capture_output=not verbose, text=True)
        if p.returncode != 0:
            if not verbose:
                err = (p.stderr or "").strip()
                if err:
                    print(f"[ota] pip failed: {err[:400]}", file=sys.stderr)
            return False
    except Exception as e:
        print(f"[ota] pip failed: {e}", file=sys.stderr)
        return False

    try:
        import nacl.signing  # type: ignore

        return True
    except Exception as e:
        print(f"[ota] pynacl import failed after install: {e}", file=sys.stderr)
        return False


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate an Ed25519 keypair for OTA signing.")
    ap.add_argument("--out", default="tools/ota/keys/ota_ed25519.key", help="Output private key file (hex, 32 bytes seed).")
    args = ap.parse_args()

    verbose = os.environ.get("PIPGUI_TOOLS_VERBOSE", "0").strip().lower() in ("1", "true", "yes", "on")

    if not _ensure_pynacl(verbose):
        print("Missing dependency: pynacl", file=sys.stderr)
        print("Tip: run with PlatformIO python: `pio system python tools/ota/keygen.py ...`", file=sys.stderr)
        return 2

    from nacl import signing  # type: ignore

    sk = signing.SigningKey.generate()
    seed = sk.encode()  # 32 bytes
    pk = sk.verify_key.encode()  # 32 bytes

    priv_hex = seed.hex()
    pub_hex = pk.hex()

    out_path = os.path.abspath(args.out)
    out_dir = os.path.dirname(out_path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    if os.path.exists(out_path):
        print(f"Refusing to overwrite existing file: {args.out}", file=sys.stderr)
        return 1

    with open(out_path, "w", encoding="utf-8") as f:
        f.write(priv_hex + "\n")

    print("Generated Ed25519 OTA signing keys:")
    print(f"- Private seed (saved): {args.out}")
    print(f"- Public key hex (put into firmware config): {pub_hex}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
