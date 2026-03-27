import hashlib
import json
import os
import re
import struct
import zlib


def _write_png_gray8(path: str, w: int, h: int, data: bytes) -> bool:
    if w <= 0 or h <= 0:
        return False
    if len(data) != w * h:
        return False

    def _chunk(tag: bytes, payload: bytes) -> bytes:
        return struct.pack(">I", len(payload)) + tag + payload + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF)

    stride = w
    raw = bytearray((stride + 1) * h)
    for y in range(h):
        raw[y * (stride + 1)] = 0
        src0 = y * stride
        dst0 = y * (stride + 1) + 1
        raw[dst0 : dst0 + stride] = data[src0 : src0 + stride]

    ihdr = struct.pack(">IIBBBBB", w, h, 8, 0, 0, 0, 0)
    idat = zlib.compress(bytes(raw), level=9)
    png = b"\x89PNG\r\n\x1a\n" + _chunk(b"IHDR", ihdr) + _chunk(b"IDAT", idat) + _chunk(b"IEND", b"")

    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(png)
    return True


def _fmt_f(v: float) -> str:
    if v == 0.0:
        return "0.0"
    s = f"{v:.9g}"
    if s == "-0":
        return "0.0"
    if ("e" in s) or ("E" in s) or ("." in s):
        return s
    return s + ".0"


def _read_json(path: str):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def _write_if_changed(path: str, content: str) -> None:
    prev = None
    try:
        with open(path, "r", encoding="utf-8") as f:
            prev = f.read()
    except OSError:
        prev = None

    if prev == content:
        return

    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write(content)


def _hash_file(path: str) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while True:
            b = f.read(1024 * 1024)
            if not b:
                break
            h.update(b)
    return h.hexdigest()


def _hash_bytes(data: bytes) -> str:
    h = hashlib.sha256()
    h.update(data)
    return h.hexdigest()


def _file_stat_signature(path: str) -> str:
    norm_path = os.path.normcase(os.path.abspath(path))
    try:
        st = os.stat(path)
    except OSError:
        return f"missing:{norm_path}"
    return json.dumps(
        {
            "path": norm_path,
            "size": st.st_size,
            "mtime_ns": st.st_mtime_ns,
        },
        sort_keys=True,
    )


def _safe_ident(s: str) -> str:
    s2 = re.sub(r"[^A-Za-z0-9_]", "_", s)
    if not s2:
        return "Font"
    if s2[0].isdigit():
        s2 = "F_" + s2
    return s2


def _snake_to_camel(s: str) -> str:
    parts = re.split(r"[_\-\s]+", s)
    out = "".join(p[:1].upper() + p[1:] for p in parts if p)
    return out if out else "Font"


def _best_grid(n: int):
    best_cols = 1
    best_rows = n
    best_area = best_cols * best_rows
    best_aspect = abs(best_cols - best_rows)
    for cols in range(1, n + 1):
        rows = int(__import__('math').ceil(n / cols))
        area = cols * rows
        aspect = abs(cols - rows)
        if area < best_area or (area == best_area and aspect < best_aspect):
            best_area = area
            best_aspect = aspect
            best_cols = cols
            best_rows = rows
    return best_cols, best_rows


def _camel_from_file(base: str) -> str:
    s = _snake_to_camel(base)
    if not s:
        return "Icon"
    if s[0].isdigit():
        s = "I" + s
    return s


def _default_charset_text() -> str:
    parts = []
    parts.append("[0x20, 0x7e]")
    parts.append("[0x0410, 0x044f]")
    parts.append("\"\u0401\u0451\u2116\u20bd\u00b0\"")
    return "\n".join(parts) + "\n"

