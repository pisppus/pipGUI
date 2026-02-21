"""
Font generation: TTF -> PSDF atlas for embedded rendering.
"""
import json
import os
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from psdf import (
    _write_png_gray8,
    _fmt_f,
    _read_json,
    _write_if_changed,
    _hash_file,
    _safe_ident,
    _snake_to_camel,
    _default_charset_text,
)


def _gen_atlas_decl_hpp(var_name: str) -> str:
    out = []
    out.append("#pragma once\n\n")
    out.append("#include <cstdint>\n\n")
    out.append(f"extern const uint8_t {var_name}[];\n")
    return "".join(out)


def _gen_atlas_def_cpp(hpp_filename: str, var_name: str, data: bytes) -> str:
    out = []
    out.append(f"#include \"{hpp_filename}\"\n\n")
    out.append(f"const uint8_t {var_name}[] = {{\n")

    for i in range(0, len(data), 16):
        chunk = data[i : i + 16]
        line = ", ".join(f"0x{b:02x}" for b in chunk)
        out.append(f"  {line},\n")

    out.append("};\n")
    return "".join(out)


def _gen_metrics_header(atlas: dict, font_ident: str) -> str:
    ns_name = "psdf_" + font_ident.lower().replace("made", "").replace("display", "").replace("one", "")
    if ns_name == "psdf_wix":
        ns_name = "psdf"

    atlas_info = atlas.get("atlas", {})
    metrics = atlas.get("metrics", {})
    glyphs = atlas.get("glyphs", [])

    width = int(atlas_info.get("width", 0))
    height = int(atlas_info.get("height", 0))
    y_origin = str(atlas_info.get("yOrigin", "top")).lower()
    distance_range = float(atlas_info.get("distanceRange", 0))
    nominal_size = float(atlas_info.get("size", 0))

    ascender = float(metrics.get("ascender", 0))
    descender = float(metrics.get("descender", 0))
    line_height = float(metrics.get("lineHeight", 0))

    out = []
    out.append("#pragma once\n")
    out.append("#include <cstdint>\n")
    out.append("\n")
    out.append("namespace pipgui\n{")
    out.append(f"\nnamespace {ns_name}\n{{")
    out.append(f"\nstatic constexpr uint16_t AtlasWidth = {width};")
    out.append(f"\nstatic constexpr uint16_t AtlasHeight = {height};")
    out.append(f"\nstatic constexpr float DistanceRange = {_fmt_f(distance_range)}f;")
    out.append(f"\nstatic constexpr float NominalSizePx = {_fmt_f(nominal_size)}f;")
    out.append(f"\nstatic constexpr float Ascender = {_fmt_f(ascender)}f;")
    out.append(f"\nstatic constexpr float Descender = {_fmt_f(descender)}f;")
    out.append(f"\nstatic constexpr float LineHeight = {_fmt_f(line_height)}f;\n")

    out.append("\nstruct Glyph\n{")
    out.append("\n    uint32_t codepoint;")
    out.append("\n    float advance;")
    out.append("\n    float pl, pb, pr, pt;")
    out.append("\n    float al, ab, ar, at;")
    out.append("\n};\n")

    items = []
    for g in glyphs:
        if "unicode" not in g:
            continue
        code = int(g.get("unicode"))
        adv = float(g.get("advance", 0))
        pb = g.get("planeBounds")
        ab = g.get("atlasBounds")
        if not pb or not ab:
            continue
        item = {
            "code": code,
            "adv": adv,
            "pl": float(pb.get("left", 0)),
            "pb": float(pb.get("bottom", 0)),
            "pr": float(pb.get("right", 0)),
            "pt": float(pb.get("top", 0)),
            "al": float(ab.get("left", 0)),
            "ab": float(ab.get("bottom", 0)),
            "ar": float(ab.get("right", 0)),
            "at": float(ab.get("top", 0)),
        }

        if y_origin == "bottom" and height > 0:
            old_ab = item["ab"]
            old_at = item["at"]
            item["ab"] = float(height) - old_at
            item["at"] = float(height) - old_ab

        items.append(item)

    items.sort(key=lambda x: x["code"])

    out.append(f"\nstatic constexpr uint16_t GlyphCount = {len(items)};\n")
    out.append("\nstatic const Glyph Glyphs[GlyphCount] =\n{")
    for it in items:
        out.append(
            "\n    {"
            f"{it['code']}u, {_fmt_f(it['adv'])}f, "
            f"{_fmt_f(it['pl'])}f, {_fmt_f(it['pb'])}f, {_fmt_f(it['pr'])}f, {_fmt_f(it['pt'])}f, "
            f"{_fmt_f(it['al'])}f, {_fmt_f(it['ab'])}f, {_fmt_f(it['ar'])}f, {_fmt_f(it['at'])}f"
            "},"
        )
    out.append("\n};\n")
    out.append("\n}\n}")
    out.append("\n")
    return "".join(out)


def find_msdf_atlas_gen(project_dir: str) -> str | None:
    msdf_exe = shutil.which("msdf-atlas-gen") or shutil.which("msdf-atlas-gen.exe")
    if not msdf_exe:
        local_exe = os.path.join(project_dir, "tools", "fonts", "msdf-atlas-gen.exe")
        if os.path.isfile(local_exe):
            msdf_exe = local_exe
    if not msdf_exe:
        local_exe = os.path.join(project_dir, "tools", "msdf-atlas-gen", "msdf-atlas-gen.exe")
        if os.path.isfile(local_exe):
            msdf_exe = local_exe
    return msdf_exe


def generate_all(project_dir: str) -> bool:
    tools_fonts_dir = os.path.join(project_dir, "tools", "fonts")
    ttf_dir = os.path.join(tools_fonts_dir, "TTF")
    
    if not os.path.isdir(ttf_dir):
        print("[fonts] TTF dir not found:", ttf_dir)
        return False

    msdf_exe = find_msdf_atlas_gen(project_dir)
    if not msdf_exe:
        print("[fonts] msdf-atlas-gen.exe not found (PATH / tools/fonts / tools/msdf-atlas-gen)")
        return False

    print("[fonts] msdf-atlas-gen:", msdf_exe)

    ttf_files = [f for f in os.listdir(ttf_dir) if f.lower().endswith(".ttf")]
    ttf_files.sort()

    for ttf_file in ttf_files:
        ttf_path = os.path.join(ttf_dir, ttf_file)
        if not os.path.isfile(ttf_path):
            continue

        font_base = os.path.splitext(os.path.basename(ttf_path))[0]
        font_folder = _snake_to_camel(font_base)
        if font_base.lower() == "wixmadefordisplay":
            font_folder = "WixMadeForDisplay"
        font_ident = _safe_ident(font_folder)

        out_fonts_dir = os.path.join(project_dir, "lib", "pipSystem", "pipGUI", "fonts", font_folder)
        out_atlas_hpp = os.path.join(out_fonts_dir, f"{font_folder}.hpp")
        out_atlas_cpp = os.path.join(out_fonts_dir, f"{font_folder}.cpp")
        out_metrics_hpp = os.path.join(out_fonts_dir, "metrics.hpp")

        work_dir = os.path.join(tools_fonts_dir, "PSDF", font_folder)
        os.makedirs(work_dir, exist_ok=True)

        charset_path = os.path.join(work_dir, "charset.txt")
        _write_if_changed(charset_path, _default_charset_text())

        json_path = os.path.join(work_dir, "atlas.json")
        bin_path = os.path.join(work_dir, "atlas.bin")
        stamp_path = os.path.join(work_dir, "stamp.txt")

        params = {
            "font": os.path.abspath(ttf_path),
            "type": "psdf",
            "size": "48",
            "pxrange": "8",
            "potr": "1",
            "charset": os.path.abspath(charset_path),
        }

        stamp_in = "\n".join([_hash_file(ttf_path), json.dumps(params, sort_keys=True)])

        prev_stamp = None
        try:
            with open(stamp_path, "r", encoding="utf-8") as f:
                prev_stamp = f.read()
        except OSError:
            prev_stamp = None

        if prev_stamp == stamp_in and os.path.isfile(json_path) and os.path.isfile(bin_path):
            atlas = _read_json(json_path)
            metrics_h = _gen_metrics_header(atlas, font_ident)
            _write_if_changed(out_metrics_hpp, metrics_h)

            with open(bin_path, "rb") as f:
                data = f.read()

            atlas_info = atlas.get("atlas", {})
            w = int(atlas_info.get("width", 0))
            h = int(atlas_info.get("height", 0))
            y_origin = str(atlas_info.get("yOrigin", "top")).lower()
            if y_origin == "bottom" and w > 0 and h > 0 and len(data) == w * h:
                row = w
                flipped = bytearray(len(data))
                for yy in range(h):
                    src = (h - 1 - yy) * row
                    dst = yy * row
                    flipped[dst : dst + row] = data[src : src + row]
                data = bytes(flipped)

            _write_png_gray8(os.path.join(work_dir, "atlas.png"), w, h, data)

            hpp_filename = os.path.basename(out_atlas_hpp)
            _write_if_changed(out_atlas_hpp, _gen_atlas_decl_hpp(font_folder))
            _write_if_changed(out_atlas_cpp, _gen_atlas_def_cpp(hpp_filename, font_folder, data))
            continue

        cmd = [
            msdf_exe,
            "-font", os.path.abspath(ttf_path),
            "-type", "psdf",
            "-format", "bin",
            "-imageout", os.path.abspath(bin_path),
            "-json", os.path.abspath(json_path),
            "-size", "48",
            "-pxrange", "8",
            "-charset", os.path.abspath(charset_path),
            "-fontname", font_folder,
        ]

        try:
            import subprocess
            subprocess.check_call(cmd, cwd=work_dir)
        except subprocess.CalledProcessError as e:
            print("[fonts] msdf-atlas-gen failed:", e)
            print("[fonts] cmd:", " ".join(cmd))
            raise

        atlas = _read_json(json_path)
        metrics_h = _gen_metrics_header(atlas, font_ident)
        _write_if_changed(out_metrics_hpp, metrics_h)

        with open(bin_path, "rb") as f:
            data = f.read()

        atlas_info = atlas.get("atlas", {})
        w = int(atlas_info.get("width", 0))
        h = int(atlas_info.get("height", 0))
        y_origin = str(atlas_info.get("yOrigin", "top")).lower()
        if y_origin == "bottom" and w > 0 and h > 0 and len(data) == w * h:
            row = w
            flipped = bytearray(len(data))
            for yy in range(h):
                src = (h - 1 - yy) * row
                dst = yy * row
                flipped[dst : dst + row] = data[src : src + row]
            data = bytes(flipped)

        _write_png_gray8(os.path.join(work_dir, "atlas.png"), w, h, data)

        hpp_filename = os.path.basename(out_atlas_hpp)
        _write_if_changed(out_atlas_hpp, _gen_atlas_decl_hpp(font_folder))
        _write_if_changed(out_atlas_cpp, _gen_atlas_def_cpp(hpp_filename, font_folder, data))

        with open(stamp_path, "w", encoding="utf-8") as f:
            f.write(stamp_in)

    return True


if __name__ == "__main__":
    try:
        Import("env")
        project_dir = env["PROJECT_DIR"]
    except Exception:
        project_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    
    generate_all(project_dir)
