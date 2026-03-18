import argparse
import os
import struct
import time


MAGIC = b"PSCR"
FMT_QOI_RGB = 3
LOG_PREFIX = "[screenshots]"


def log(msg: str):
    print(f"{LOG_PREFIX} {msg}")


def qoi_decode_to_rgb(w: int, h: int, read_byte) -> bytes:
    pixels_total = w * h
    out = bytearray(pixels_total * 3)

    index = [0] * 64
    px = 0x000000FF  # RGBA packed, initial: 0,0,0,255
    run = 0

    def qoi_hash(p: int) -> int:
        r = (p >> 24) & 0xFF
        g = (p >> 16) & 0xFF
        b = (p >> 8) & 0xFF
        a = p & 0xFF
        return (r * 3 + g * 5 + b * 7 + a * 11) & 63

    o = 0
    for _ in range(pixels_total):
        if run:
            run -= 1
        else:
            b1 = read_byte()

            if b1 == 0xFE:  # QOI_OP_RGB
                r = read_byte()
                g = read_byte()
                b = read_byte()
                px = (r << 24) | (g << 16) | (b << 8) | (px & 0xFF)
            elif b1 == 0xFF:  # QOI_OP_RGBA
                r = read_byte()
                g = read_byte()
                b = read_byte()
                a = read_byte()
                px = (r << 24) | (g << 16) | (b << 8) | a
            else:
                tag = b1 & 0xC0
                if tag == 0x00:  # QOI_OP_INDEX
                    px = index[b1 & 63]
                elif tag == 0x40:  # QOI_OP_DIFF
                    dr = ((b1 >> 4) & 3) - 2
                    dg = ((b1 >> 2) & 3) - 2
                    db = (b1 & 3) - 2
                    r = ((px >> 24) + dr) & 0xFF
                    g = ((px >> 16) + dg) & 0xFF
                    b = ((px >> 8) + db) & 0xFF
                    px = (r << 24) | (g << 16) | (b << 8) | (px & 0xFF)
                elif tag == 0x80:  # QOI_OP_LUMA
                    b2 = read_byte()
                    dg = (b1 & 63) - 32
                    dr_dg = ((b2 >> 4) & 15) - 8
                    db_dg = (b2 & 15) - 8
                    r = ((px >> 24) + dg + dr_dg) & 0xFF
                    g = ((px >> 16) + dg) & 0xFF
                    b = ((px >> 8) + dg + db_dg) & 0xFF
                    px = (r << 24) | (g << 16) | (b << 8) | (px & 0xFF)
                elif tag == 0xC0:  # QOI_OP_RUN
                    run = b1 & 63

        index[qoi_hash(px)] = px
        out[o] = (px >> 24) & 0xFF
        out[o + 1] = (px >> 16) & 0xFF
        out[o + 2] = (px >> 8) & 0xFF
        o += 3

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
    args = ap.parse_args()

    base_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    out_dir = args.out or os.path.join(base_dir, "captures")
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

        rest = read_exact(ser, 2 + 2 + 1 + 4)
        w, h, fmt, payload_size = struct.unpack("<HHBI", rest)

        if w == 0 or h == 0:
            log(f"invalid header: w={w} h={h} fmt={fmt} size={payload_size}")
            continue

        if fmt != FMT_QOI_RGB:
            log(f"unsupported fmt: {fmt} (expected {FMT_QOI_RGB})")
            continue

        log(f"capture {w}x{h} qoi bytes={payload_size}")

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
                        raise EOFError("qoi payload too small")
                    b = payload[p]
                    p += 1
                    return b

                rgb = qoi_decode_to_rgb(w, h, read_byte)
            else:
                reader = SerialByteReader(ser)
                rgb = qoi_decode_to_rgb(w, h, reader.read_byte)
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
