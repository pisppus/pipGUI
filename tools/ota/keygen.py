import argparse
import os
import sys
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1]
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

from _menu import choose, prompt_text
from _util import ensure_pynacl

def main() -> int:
    ap = argparse.ArgumentParser(description="Generate an Ed25519 keypair for OTA signing.")
    ap.add_argument("--out", default="tools/ota/keys/ota_ed25519.key", help="Output private key file (hex, 32 bytes seed).")
    ap.add_argument("--interactive", action="store_true", help="Choose output path from interactive menus")
    args = ap.parse_args()

    if args.interactive or len(sys.argv) == 1:
        choice = choose(
            "Key output",
            [
                "Default: tools/ota/keys/ota_ed25519.key",
                "Custom path",
            ],
        )
        if choice == "Custom path":
            args.out = prompt_text("Output path", default=args.out)

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
