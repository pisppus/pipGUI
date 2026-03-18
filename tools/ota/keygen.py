import argparse
import os
import sys

from _util import ensure_pynacl

def main() -> int:
    ap = argparse.ArgumentParser(description="Generate an Ed25519 keypair for OTA signing.")
    ap.add_argument("--out", default="tools/ota/keys/ota_ed25519.key", help="Output private key file (hex, 32 bytes seed).")
    args = ap.parse_args()

    verbose = os.environ.get("PIPGUI_TOOLS_VERBOSE", "0").strip().lower() in ("1", "true", "yes", "on")
    ensure_pynacl(verbose)

    from nacl import signing

    sk = signing.SigningKey.generate()
    seed = sk.encode()
    pk = sk.verify_key.encode()

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
