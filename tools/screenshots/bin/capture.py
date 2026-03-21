import argparse
import os
import struct
import sys
import time
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[2]
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

from _menu import choose, choose_existing_path, prompt_text


MAGIC = b"PSCR"
LOG_PREFIX = "[screenshots]"


def log(msg: str):
    print(f"{LOG_PREFIX} {msg}")


def hash_565(px: int) -> int:
    r = (px >> 11) & 31
    g = (px >> 5) & 63
    b = px & 31
    return (r * 3 + g * 5 + b * 7) & 63


def rgb_from_565(px: int) -> tuple[int, int, int]:
    r5 = (px >> 11) & 31
    g6 = (px >> 5) & 63
    b5 = px & 31
    return (
        ((r5 << 3) | (r5 >> 2)) & 0xFF,
        ((g6 << 2) | (g6 >> 4)) & 0xFF,
        ((b5 << 3) | (b5 >> 2)) & 0xFF,
    )


def packed565_decode_to_rgb(w: int, h: int, read_byte) -> bytes:
    pixels_total = w * h
    out = bytearray(pixels_total * 3)
    index = [0] * 64
    prev = 0
    o = 0
    pos = 0
    while pos < pixels_total:
        b1 = read_byte()

        if b1 < 0x40:
            px = index[b1 & 0x3F]
            run = 1
        elif b1 < 0x80:
            px = prev
            run = (b1 & 0x3F) + 1
        elif b1 < 0xC0:
            pr = (prev >> 11) & 31
            pg = (prev >> 5) & 63
            pb = prev & 31
            dr = ((b1 >> 4) & 0x03) - 2
            dg = ((b1 >> 2) & 0x03) - 2
            db = (b1 & 0x03) - 2
            px = (((pr + dr) << 11) | ((pg + dg) << 5) | (pb + db)) & 0xFFFF
            run = 1
        elif b1 < 0xE0:
            b2 = read_byte()
            pr = (prev >> 11) & 31
            pg = (prev >> 5) & 63
            pb = prev & 31
            dg = (b1 & 0x1F) - 16
            dr = dg + ((b2 >> 4) & 0x0F) - 8
            db = dg + (b2 & 0x0F) - 8
            px = (((pr + dr) << 11) | ((pg + dg) << 5) | (pb + db)) & 0xFFFF
            run = 1
        else:
            run = (b1 & 0x1F) + 1
            for _ in range(run):
                hi = read_byte()
                lo = read_byte()
                px = (hi << 8) | lo
                r, g, b = rgb_from_565(px)
                out[o] = r
                out[o + 1] = g
                out[o + 2] = b
                o += 3
                pos += 1
                index[hash_565(px)] = px
                prev = px
            continue

        for _ in range(run):
            r, g, b = rgb_from_565(px)
            out[o] = r
            out[o + 1] = g
            out[o + 2] = b
            o += 3
            pos += 1
            index[hash_565(px)] = px
            prev = px

    return bytes(out)


class SerialByteReader:
    def __init__(self, ser):
        self.ser = ser
        self.buf = bytearray()
        self.off = 0

    def read_byte(self) -> int:
        while self.off >= len(self.buf):
            chunk = self.ser.read(4096)
            if chunk:
                self.buf = bytearray(chunk)
                self.off = 0
                break
            raise EOFError("serial read timeout")
        b = self.buf[self.off]
        self.off += 1
        return b


def open_serial(port: str, baud: int):
    try:
        import serial
        import serial.tools.list_ports
    except Exception:
        raise RuntimeError("pyserial is required: python -m pip install pyserial")

    if port == "auto":
        ports = list(serial.tools.list_ports.comports())
        if not ports:
            raise RuntimeError("no serial ports found")

        ports_sorted = sorted(
            ports,
            key=lambda p: (
                0
                if (
                    "USB" in (p.description or "")
                    or "CP210" in (p.description or "")
                    or "CH34" in (p.description or "")
                )
                else 1,
                p.device,
            ),
        )
        port = ports_sorted[0].device
        log(f"using port: {port} ({ports_sorted[0].description})")

    ser = serial.Serial(port, baudrate=baud, timeout=1)
    ser.reset_input_buffer()
    return ser


def list_serial_ports() -> list[tuple[str, str]]:
    try:
        import serial.tools.list_ports
    except Exception:
        return []
    ports = []
    for port in serial.tools.list_ports.comports():
        desc = port.description or "Unknown device"
        ports.append((port.device, desc))
    ports.sort(key=lambda item: item[0])
    return ports


def interactive_args(default_out: str) -> tuple[str, int, str]:
    port_options = [("Auto detect", "auto")]
    port_options.extend((f"{device} ({desc})", device) for device, desc in list_serial_ports())
    selected_port = choose("Serial port", port_options, lambda item: item[0])[1]

    baud = choose("Baud rate", [1000000, 921600, 460800, 230400], lambda item: str(item))

    out_mode = choose(
        "Output directory",
        [
            ("Default captures folder", default_out),
            ("Choose existing folder", None),
            ("Enter custom folder", ""),
        ],
        lambda item: item[0],
    )
    if out_mode[1] is None:
        existing_dirs = sorted(path for path in Path(default_out).parent.glob("*") if path.is_dir())
        out_dir = str(choose_existing_path("Capture folder", existing_dirs, custom_label="Manual folder path"))
    elif out_mode[1] == "":
        out_dir = prompt_text("Capture folder", default=default_out)
    else:
        out_dir = out_mode[1]
    return selected_port, int(baud), out_dir


def read_exact(ser, n: int) -> bytes:
    chunks = []
    got = 0
    while got < n:
        b = ser.read(n - got)
        if not b:
            continue
        chunks.append(b)
        got += len(b)
    return b"".join(chunks)


def save_rgb_images(out_dir: str, base: str, w: int, h: int, rgb: bytes):
    try:
        from PIL import Image
    except Exception:
        raise RuntimeError("Pillow is required: python -m pip install pillow")

    img = Image.frombytes("RGB", (w, h), rgb)
    img = img.resize((w * 4, h * 4), resample=Image.BICUBIC)
    png_path = os.path.join(out_dir, base + ".png")
    img.save(png_path)
    log(f"saved: {png_path}")


def main():
    ap = argparse.ArgumentParser(description="Capture pipGUI screenshots from serial (PSCR)")
    ap.add_argument("--port", default="auto", help="serial port (e.g. COM9) or 'auto'")
    ap.add_argument("--baud", type=int, default=1000000)
    ap.add_argument("--out", default=None, help="output directory")
    ap.add_argument("--interactive", action="store_true", help="Choose values from interactive menus")
    args = ap.parse_args()

    base_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    default_out = os.path.join(base_dir, "captures")
    if args.interactive or len(sys.argv) == 1:
        args.port, args.baud, out_dir = interactive_args(default_out)
    else:
        out_dir = args.out or default_out
    os.makedirs(out_dir, exist_ok=True)

    try:
        ser = open_serial(args.port, args.baud)
    except Exception as e:
        log("failed to open serial port. Close Serial Monitor/other apps and try again.")
        raise

    log(f"listening @ {args.baud} on {args.port}")
    log(f"output: {out_dir}")
    log("waiting for PSCR header...")

    window = bytearray()
    while True:
        b = ser.read(1)
        if not b:
            continue
        window += b
        if len(window) > 4:
            del window[:-4]
        if bytes(window) != MAGIC:
            continue

        rest = read_exact(ser, 2 + 2 + 4)
        w, h, payload_size = struct.unpack("<HHI", rest)

        if w == 0 or h == 0:
            log(f"invalid header: w={w} h={h} size={payload_size}")
            continue

        log(f"capture {w}x{h} packed565 bytes={payload_size}")

        payload = read_exact(ser, payload_size) if payload_size else b""

        ts = time.strftime("%Y%m%d_%H%M%S")
        base = ts
        n = 0
        while True:
            png_path = os.path.join(out_dir, base + ".png")
            if not os.path.exists(png_path):
                break
            n += 1
            base = f"{ts}_{n:02d}"

        try:
            if payload:
                p = 0

                def read_byte():
                    nonlocal p
                    if p >= len(payload):
                        raise EOFError("packed565 payload too small")
                    b = payload[p]
                    p += 1
                    return b

                rgb = packed565_decode_to_rgb(w, h, read_byte)
            else:
                reader = SerialByteReader(ser)
                rgb = packed565_decode_to_rgb(w, h, reader.read_byte)
        except Exception as e:
            log(f"decode failed: {e}")
            continue

        try:
            save_rgb_images(out_dir, base, w, h, rgb)
        except Exception as e:
            log(f"save failed: {e}")
            continue


if __name__ == "__main__":
    main()
