# OTA updater

`pipGUI` OTA uses a small manifest file that the device downloads first.

## Manifest fields

- `version` (string): human version `X.Y.Z`.
- `size` (number): firmware binary size in bytes.
- `sha256` (string, hex): SHA-256 of the firmware binary.
- `sig_ed25519` (string, hex): Ed25519 signature of the **manifest payload** (v2).
- `url` (string): HTTPS URL to the firmware binary.

See `tools/ota/manifest.example.json`.

### Manifest signature payload (v2)

The signature is computed over a deterministic binary payload:

- ASCII prefix `pipgui-ota-manifest-v2` (no NUL)
- `version` ASCII `X.Y.Z` + `NUL` separator
- `size` u32 (big-endian)
- `sha256` raw bytes (32 bytes)
- `url` UTF-8 bytes (no trailing NUL)

## Recommended workflow (Vercel static)

1) Generate signing keys (once):
   - `python tools/ota/keygen.py` (default output: `tools/ota/keys/ota_ed25519.key`)
   - Put the printed public key into `include/config.hpp` as `PIPGUI_OTA_ED25519_PUBKEY_HEX`
   - Keep the private key file private (do not commit).

2) Build firmware to get `firmware.bin`.

3) Upload `firmware.bin` and `manifest.json` to your Vercel site:
   - Put them into your site repo, for example `fw/manifest.json` and `fw/<project>-<version>.bin`
   - Commit + push → Vercel deploys and serves `/fw/...` via HTTPS.
   - Recommended caching headers (in your site repo `vercel.json`):
     - no-cache for `manifest.json`
     - long cache for `*.bin`
     Example:
     ```json
     {
       "headers": [
         { "source": "/fw/manifest.json", "headers": [{ "key": "Cache-Control", "value": "no-store" }] },
         { "source": "/fw/(.*)\\.bin", "headers": [{ "key": "Cache-Control", "value": "public, max-age=31536000, immutable" }] }
       ]
     }
     ```

4) Stage a release into `tools/ota/out/` (recommended):
   - `python tools/ota/make_release.py --bin .pio/build/esp32-s3-devkitm-1/firmware.bin --version 1.3.8 --site-base https://your-domain.com/fw`

   This creates:
   - `tools/ota/out/manifest.json`
   - `tools/ota/out/<project>-1.3.8.bin` (project name auto-detected; override via `--project`)

   Then upload/copy these 2 files into your site repo `fw/` and deploy.

5) Or create only the manifest (advanced / if you upload the bin separately):
   - `python tools/ota/make_manifest.py --bin .pio/build/esp32-s3-devkitm-1/firmware.bin --version 1.3.8 --url https://your-domain.com/fw/pipgui-1.3.8.bin --sign-key tools/ota/keys/ota_ed25519.key --out tools/ota/out/manifest.json`

6) (Optional) Verify before deploying:
   - `python tools/ota/verify_manifest.py --manifest tools/ota/out/manifest.json --bin .pio/build/esp32-s3-devkitm-1/firmware.bin --pubkey <PIPGUI_OTA_ED25519_PUBKEY_HEX>`

## Troubleshooting

- Boot error like `esp_image: Checksum failed` after OTA usually means your OTA slot partition goes past the real flash size (partition table mismatch).
  - Fix by using a partition table that matches your hardware flash size (example: `partitions_ota_littlefs_8MB.csv` vs `partitions_ota_littlefs_16MB.csv`).
