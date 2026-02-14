import os

try:
    Import("env")
except Exception:
    env = {"PROJECT_DIR": os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))}

import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET
import math


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


def _gen_atlas_decl_hpp(var_name: str) -> str:
    out = []
    out.append("#pragma once\n\n")
    out.append("#include <stdint.h>\n\n")
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
    out.append("#include <stdint.h>\n")
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

        # Normalize atlas coordinates to top-left origin.
        # msdf-atlas-gen typically emits yOrigin=bottom.
        # After normalization, both atlas pixel buffer and atlasBounds are top-left.
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


def _gen_atlas_header(font_ident: str, data: bytes) -> str:
    out = []
    out.append("#pragma once\n\n")
    out.append("#include <stdint.h>\n\n")
    out.append(f"inline static const uint8_t {font_ident}[] PROGMEM  = {{\n")

    for i in range(0, len(data), 16):
        chunk = data[i : i + 16]
        line = ", ".join(f"0x{b:02x}" for b in chunk)
        out.append(f"  {line},\n")

    out.append("};\n")
    return "".join(out)


def _default_charset_text() -> str:
    parts = []
    # Basic Latin: space, digits, uppercase, lowercase, essential punctuation
    parts.append("[0x20, 0x7e]")  # !"#$%&'()*+,-./0-9:;<=>?@A-Z[\]^_`a-z{|}~
    # Russian Cyrillic only (not the whole 0x0400-0x052f block)
    parts.append("[0x0410, 0x044f]")  # А-Я, а-я
    parts.append("\"Ёё№₽°\"")  # Additional Russian: Ё, ё, №, ₽, degree
    return "\n".join(parts) + "\n"


def _best_grid(n: int):
    best_cols = 1
    best_rows = n
    best_area = best_cols * best_rows
    best_aspect = abs(best_cols - best_rows)
    for cols in range(1, n + 1):
        rows = int(math.ceil(n / cols))
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


def _gen_icons_metrics_hpp(icon_names, icon_aliases, icon_boxes, atlas_w, atlas_h, nominal_px, pxrange) -> str:
    out = []
    out.append("#pragma once\n")
    out.append("#include <stdint.h>\n")
    out.append("\n")
    out.append("namespace pipgui\n{")
    out.append("\nnamespace psdf_icons\n{")
    out.append(f"\nstatic constexpr uint16_t AtlasWidth = {atlas_w};")
    out.append(f"\nstatic constexpr uint16_t AtlasHeight = {atlas_h};")
    out.append(f"\nstatic constexpr float DistanceRange = {_fmt_f(float(pxrange))}f;")
    out.append(f"\nstatic constexpr float NominalSizePx = {_fmt_f(float(nominal_px))}f;\n")

    out.append("\nenum IconId : uint16_t\n{")
    for i, name in enumerate(icon_names):
        comma = "," if i + 1 < len(icon_names) else ""
        out.append(f"\n    Icon{name} = {i}{comma}")
    out.append("\n};\n")

    out.append("\nstruct Icon\n{")
    out.append("\n    uint16_t x;")
    out.append("\n    uint16_t y;")
    out.append("\n    uint16_t w;")
    out.append("\n    uint16_t h;")
    out.append("\n};\n")

    out.append(f"\nstatic constexpr uint16_t IconCount = {len(icon_names)};\n")
    out.append("\nstatic constexpr Icon Icons[IconCount] =\n{")
    for i, (x, y, w, h) in enumerate(icon_boxes):
        comma = "," if i + 1 < len(icon_boxes) else ""
        out.append(f"\n    {{{x}u, {y}u, {w}u, {h}u}}{comma}")
    out.append("\n};\n")

    out.append("\n}\n}")
    out.append("\n\nnamespace pipgui\n{\n")
    out.append("using IconId = ::pipgui::psdf_icons::IconId;\n")
    for name, alias in zip(icon_names, icon_aliases):
        out.append(f"static constexpr IconId Icon{name} = ::pipgui::psdf_icons::Icon{name};\n")
        out.append(f"static constexpr IconId {name} = ::pipgui::psdf_icons::Icon{name};\n")
        out.append(f"static constexpr IconId {alias} = ::pipgui::psdf_icons::Icon{name};\n")
    out.append("}\n")
    out.append("\n")
    return "".join(out)


def _gen_icons_decl_hpp() -> str:
    out = []
    out.append("#pragma once\n\n")
    out.append("#include <stdint.h>\n\n")
    out.append("extern const uint8_t icons[];\n")
    return "".join(out)


def _svg_tag_local(tag: str) -> str:
    if not tag:
        return ""
    if "}" in tag:
        return tag.split("}", 1)[1]
    return tag


def _svg_split_layers(svg_path: str):
    try:
        with open(svg_path, "r", encoding="utf-8") as f:
            s = f.read()
    except OSError:
        return None

    m = re.search(r"<svg\b[^>]*>", s, flags=re.IGNORECASE)
    if not m:
        return None
    svg_open = m.group(0)

    path_re = re.compile(r"<path\b[^>]*/\s*>|<path\b[^>]*>.*?</path>", flags=re.IGNORECASE | re.DOTALL)
    path_matches = list(path_re.finditer(s))
    if not path_matches:
        return None

    layers = []
    for i, pm in enumerate(path_matches):
        path_tag = pm.group(0).strip()
        layer_name = f"layer{i}"

        # Normalize: remove transparency and force solid colors to avoid confusion.
        # Note: SDF generation uses geometry; color is irrelevant, but some loaders treat missing attributes differently.
        path_tag = re.sub(r"\s+(opacity|fill-opacity|stroke-opacity)\s*=\s*\"[^\"]*\"", "", path_tag, flags=re.IGNORECASE)
        path_tag = re.sub(r"\s+(opacity|fill-opacity|stroke-opacity)\s*=\s*'[^']*'", "", path_tag, flags=re.IGNORECASE)

        has_stroke = bool(re.search(r"\bstroke\s*=", path_tag, flags=re.IGNORECASE))
        has_fill = bool(re.search(r"\bfill\s*=", path_tag, flags=re.IGNORECASE))
        stroke_only = bool(has_stroke and not has_fill)
        if stroke_only:
            print(f"[psdf] warn: stroke-only path in {os.path.basename(svg_path)} (layer {layer_name}) - stroke may be ignored; consider expanding stroke to path")

        # Force explicit solid colors for determinism
        if has_fill:
            path_tag = re.sub(r"\bfill\s*=\s*(\"[^\"]*\"|'[^']*')", 'fill="black"', path_tag, flags=re.IGNORECASE)
        if has_stroke:
            path_tag = re.sub(r"\bstroke\s*=\s*(\"[^\"]*\"|'[^']*')", 'stroke="black"', path_tag, flags=re.IGNORECASE)

        layer_svg = f"{svg_open}{path_tag}</svg>"
        layers.append((layer_name, layer_svg, has_stroke))

    return layers


def _svg_has_stroke(svg_path: str) -> bool:
    try:
        with open(svg_path, "r", encoding="utf-8") as f:
            s = f.read(256 * 1024)
    except OSError:
        return False

    if re.search(r"\bstroke\s*=", s, flags=re.IGNORECASE):
        return True
    if re.search(r"\bstroke\s*:", s, flags=re.IGNORECASE):
        return True
    return False


def _svg_viewbox(svg_path: str):
    try:
        with open(svg_path, "r", encoding="utf-8") as f:
            s = f.read(256 * 1024)
    except OSError:
        return None

    m = re.search(r"\bviewBox\s*=\s*\"([^\"]+)\"", s, flags=re.IGNORECASE)
    if not m:
        m = re.search(r"\bviewBox\s*=\s*'([^']+)'", s, flags=re.IGNORECASE)
    if not m:
        return None

    parts = re.split(r"[\s,]+", m.group(1).strip())
    if len(parts) != 4:
        return None
    try:
        vx, vy, vw, vh = (float(parts[0]), float(parts[1]), float(parts[2]), float(parts[3]))
    except Exception:
        return None
    if vw <= 0.0 or vh <= 0.0:
        return None
    return vx, vy, vw, vh


def _msdf_transform_from_viewbox(viewbox, out_px: int, flip_y: bool = True):
    if not viewbox:
        return None
    vx, vy, vw, vh = viewbox
    if vw <= 0.0 or vh <= 0.0:
        return None

    scale = min(float(out_px) / vw, float(out_px) / vh)
    extra_x_px = float(out_px) - vw * scale
    extra_y_px = float(out_px) - vh * scale
    off_x = (extra_x_px * 0.5) / scale
    off_y = (extra_y_px * 0.5) / scale

    tx = (-vx) + off_x
    if flip_y:
        # Flip Y: SVG Y-down -> msdfgen Y-up
        # ty = vh - ((-vy) + off_y) = vh + vy - off_y
        ty = vh + vy - off_y
    else:
        ty = (-vy) + off_y
    return scale, tx, ty


def _find_inkscape_exe(project_dir: str):
    exe = shutil.which("inkscape") or shutil.which("inkscape.exe")
    if exe:
        return exe

    local = os.path.join(project_dir, "tools", "icons", "script", "inkscape", "bin", "inkscape.exe")
    if os.path.isfile(local):
        return local

    local = os.path.join(project_dir, "tools", "icons", "script", "inkscape", "inkscape", "bin", "inkscape.exe")
    if os.path.isfile(local):
        return local

    local = os.path.join(project_dir, "tools", "icons", "script", "inkscape", "inkscape.exe")
    if os.path.isfile(local):
        return local

    return None


def _stroke_to_path_svg(project_dir: str, inkscape_exe: str, in_svg_path: str, out_svg_path: str) -> bool:
    cmd = [
        os.path.abspath(inkscape_exe),
        os.path.abspath(in_svg_path),
        "--export-plain-svg",
        "--export-overwrite",
        f"--export-filename={os.path.abspath(out_svg_path)}",
        "--actions=select-all;object-to-path;object-stroke-to-path",
    ]
    try:
        subprocess.check_call(cmd)
        return os.path.isfile(out_svg_path)
    except Exception:
        return False


def _gen_icons_def_cpp(data: bytes) -> str:
    out = []
    out.append('#include "icons.hpp"\n\n')
    out.append("const uint8_t icons[] = {\n")
    for i in range(0, len(data), 16):
        chunk = data[i : i + 16]
        line = ", ".join(f"0x{b:02x}" for b in chunk)
        out.append(f"  {line},\n")
    out.append("};\n")
    return "".join(out)


def _main():
    project_dir = env["PROJECT_DIR"]

    tools_fonts_dir = os.path.join(project_dir, "tools", "fonts")
    script_dir = os.path.join(tools_fonts_dir, "script")

    ttf_dir = os.path.join(tools_fonts_dir, "TTF")
    if not os.path.isdir(ttf_dir):
        print("[psdf] TTF dir not found:", ttf_dir)
        return

    msdf_exe = shutil.which("msdf-atlas-gen") or shutil.which("msdf-atlas-gen.exe")
    if not msdf_exe:
        local_exe = os.path.join(script_dir, "msdf-atlas-gen.exe")
        if os.path.isfile(local_exe):
            msdf_exe = local_exe

    if not msdf_exe:
        local_exe = os.path.join(project_dir, "tools", "msdf-atlas-gen", "msdf-atlas-gen.exe")
        if os.path.isfile(local_exe):
            msdf_exe = local_exe

    if not msdf_exe:
        local_exe = os.path.join(script_dir, "msdf-atlas-gen", "msdf-atlas-gen.exe")
        if os.path.isfile(local_exe):
            msdf_exe = local_exe

    if not msdf_exe:
        print("[psdf] msdf-atlas-gen.exe not found (PATH / tools/msdf-atlas-gen / tools/script)")
        return

    print("[psdf] msdf-atlas-gen:", msdf_exe)

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

        stamp_in = "\n".join(
            [
                _hash_file(ttf_path),
                json.dumps(params, sort_keys=True),
            ]
        )

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

            # Normalize atlas pixel buffer to top-left origin if needed
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

            hpp_filename = os.path.basename(out_atlas_hpp)
            _write_if_changed(out_atlas_hpp, _gen_atlas_decl_hpp(font_folder))
            _write_if_changed(out_atlas_cpp, _gen_atlas_def_cpp(hpp_filename, font_folder, data))
            continue

        cmd = [
            msdf_exe,
            "-font",
            os.path.abspath(ttf_path),
            "-type",
            "psdf",
            "-format",
            "bin",
            "-imageout",
            os.path.abspath(bin_path),
            "-json",
            os.path.abspath(json_path),
            "-potr",
            "-size",
            "48",
            "-pxrange",
            "8",
            "-charset",
            os.path.abspath(charset_path),
            "-fontname",
            font_folder,
        ]

        try:
            subprocess.check_call(cmd, cwd=work_dir)
        except subprocess.CalledProcessError as e:
            print("[psdf] msdf-atlas-gen failed:", e)
            print("[psdf] cmd:", " ".join(cmd))
            raise

        atlas = _read_json(json_path)
        metrics_h = _gen_metrics_header(atlas, font_ident)
        _write_if_changed(out_metrics_hpp, metrics_h)

        with open(bin_path, "rb") as f:
            data = f.read()

        # Normalize atlas pixel buffer to top-left origin if needed
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

        hpp_filename = os.path.basename(out_atlas_hpp)
        _write_if_changed(out_atlas_hpp, _gen_atlas_decl_hpp(font_folder))
        _write_if_changed(out_atlas_cpp, _gen_atlas_def_cpp(hpp_filename, font_folder, data))

        with open(stamp_path, "w", encoding="utf-8") as f:
            f.write(stamp_in)


    # Icons (SVG -> PSDF)
    icons_svg_dir = os.path.join(project_dir, "tools", "icons", "SVG")
    icons_exe = os.path.join(project_dir, "tools", "icons", "script", "msdfgen.exe")

    if os.path.isdir(icons_svg_dir) and os.path.isfile(icons_exe):
        svg_files = [f for f in os.listdir(icons_svg_dir) if f.lower().endswith(".svg")]
        svg_files.sort()

        if svg_files:
            icons_work_dir = os.path.join(project_dir, "tools", "icons", "PSDF")
            os.makedirs(icons_work_dir, exist_ok=True)

            icon_px = 48
            pxrange = 8

            per_icon_bins = []
            per_icon_hashes = []
            icon_names = []
            icon_aliases = []

            for fsvg in svg_files:
                svg_path = os.path.join(icons_svg_dir, fsvg)
                base = os.path.splitext(os.path.basename(svg_path))[0]
                viewbox = _svg_viewbox(svg_path)
                xform = _msdf_transform_from_viewbox(viewbox, icon_px)
                layers = _svg_split_layers(svg_path)
                if not layers:
                    layers = [("layer0", None, False)]

                base_camel = _camel_from_file(base)

                inkscape_exe = _find_inkscape_exe(project_dir)

                for layer_name, layer_svg, has_stroke in layers:
                    if layer_svg is not None:
                        tmp_svg_path = os.path.join(icons_work_dir, f"{base}__{layer_name}.svg")
                        _write_if_changed(tmp_svg_path, layer_svg)

                        src_svg_path = tmp_svg_path
                        if has_stroke:
                            if inkscape_exe:
                                out_svg_path = os.path.join(icons_work_dir, f"{base}__{layer_name}__stroked.svg")
                                ok = _stroke_to_path_svg(project_dir, inkscape_exe, tmp_svg_path, out_svg_path)
                                if ok:
                                    src_svg_path = out_svg_path
                                else:
                                    print(f"[psdf] warn: stroke-to-path failed for {os.path.basename(svg_path)} {layer_name}")
                            else:
                                print(f"[psdf] warn: inkscape not found, cannot stroke-to-path for {os.path.basename(svg_path)} {layer_name}")
                    else:
                        src_svg_path = svg_path
                        if inkscape_exe and _svg_has_stroke(svg_path):
                            out_svg_path = os.path.join(icons_work_dir, f"{base}__{layer_name}__stroked.svg")
                            ok = _stroke_to_path_svg(project_dir, inkscape_exe, svg_path, out_svg_path)
                            if ok:
                                src_svg_path = out_svg_path
                            else:
                                print(f"[psdf] warn: stroke-to-path failed for {os.path.basename(svg_path)}")

                    name_camel = base_camel
                    alias_base = base.lower()
                    if layer_name != "layer0" or layer_svg is not None:
                        name_camel = base_camel + _snake_to_camel("_" + layer_name)
                        alias_base = f"{alias_base}_{layer_name.lower()}"

                    icon_names.append(_safe_ident(name_camel))
                    icon_aliases.append(_safe_ident(alias_base))

                    out_bin = os.path.join(icons_work_dir, f"{base}__{layer_name}_{icon_px}.bin")

                    cmd = [
                        os.path.abspath(icons_exe),
                        "psdf",
                        "-svg",
                        os.path.abspath(src_svg_path),
                        "-dimensions",
                        str(icon_px),
                        str(icon_px),
                        "-pxrange",
                        str(pxrange),
                        "-format",
                        "bin",
                        "-o",
                        os.path.abspath(out_bin),
                    ]

                    if xform:
                        s, tx, ty = xform
                        cmd.extend(["-scale", _fmt_f(float(s))])
                        cmd.extend(["-translate", _fmt_f(float(tx)), _fmt_f(float(ty))])

                    subprocess.check_call(cmd, cwd=icons_work_dir)

                    with open(out_bin, "rb") as fb:
                        b = fb.read()
                    per_icon_bins.append(b)
                    per_icon_hashes.append(_hash_bytes(b))

            stamp_path = os.path.join(icons_work_dir, "stamp.txt")
            atlas_bin_path = os.path.join(icons_work_dir, "atlas.bin")
            atlas_json_path = os.path.join(icons_work_dir, "atlas.json")

            stamp_in = "\n".join([
                "icons_psdf_v2_layers",
                str(icon_px),
                str(pxrange),
                json.dumps(svg_files, sort_keys=True),
                json.dumps(per_icon_hashes, sort_keys=True),
            ])

            prev_stamp = None
            try:
                with open(stamp_path, "r", encoding="utf-8") as f:
                    prev_stamp = f.read()
            except OSError:
                prev_stamp = None

            if prev_stamp != stamp_in or (not os.path.isfile(atlas_bin_path)):
                n = len(per_icon_bins)
                cols, rows = _best_grid(n)
                atlas_w = cols * icon_px
                atlas_h = rows * icon_px
                atlas = bytearray(atlas_w * atlas_h)

                boxes = []
                for i, b in enumerate(per_icon_bins):
                    cx = i % cols
                    cy = i // cols
                    x0 = cx * icon_px
                    y0 = cy * icon_px
                    boxes.append((x0, y0, icon_px, icon_px))
                    for yy in range(icon_px):
                        src_off = yy * icon_px
                        dst_off = (y0 + yy) * atlas_w + x0
                        atlas[dst_off : dst_off + icon_px] = b[src_off : src_off + icon_px]

                atlas_dict = {
                    "atlas": {
                        "width": atlas_w,
                        "height": atlas_h,
                        "distanceRange": pxrange,
                        "size": icon_px,
                    },
                    "icons": [
                        {
                            "name": icon_names[i],
                            "x": boxes[i][0],
                            "y": boxes[i][1],
                            "w": boxes[i][2],
                            "h": boxes[i][3],
                        }
                        for i in range(len(icon_names))
                    ],
                }

                with open(atlas_bin_path, "wb") as f:
                    f.write(bytes(atlas))
                with open(atlas_json_path, "w", encoding="utf-8") as f:
                    json.dump(atlas_dict, f, indent=2)
                with open(stamp_path, "w", encoding="utf-8") as f:
                    f.write(stamp_in)

            # Generate lib outputs
            out_icons_dir = os.path.join(project_dir, "lib", "pipSystem", "pipGUI", "icons")
            out_icons_hpp = os.path.join(out_icons_dir, "icons.hpp")
            out_icons_cpp = os.path.join(out_icons_dir, "icons.cpp")
            out_metrics_hpp = os.path.join(out_icons_dir, "metrics.hpp")

            atlas = _read_json(atlas_json_path)
            atlas_info = atlas.get("atlas", {})
            atlas_w = int(atlas_info.get("width", 0))
            atlas_h = int(atlas_info.get("height", 0))
            icon_boxes = []
            for ic in atlas.get("icons", []):
                icon_boxes.append((int(ic.get("x", 0)), int(ic.get("y", 0)), int(ic.get("w", icon_px)), int(ic.get("h", icon_px))))

            if len(icon_boxes) != len(icon_names):
                n = len(icon_names)
                cols, rows = _best_grid(n)
                atlas_w = max(atlas_w, cols * icon_px)
                atlas_h = max(atlas_h, rows * icon_px)
                icon_boxes = []
                for i in range(n):
                    cx = i % cols
                    cy = i // cols
                    icon_boxes.append((cx * icon_px, cy * icon_px, icon_px, icon_px))

            with open(atlas_bin_path, "rb") as f:
                atlas_data = f.read()

            _write_if_changed(out_icons_hpp, _gen_icons_decl_hpp())
            _write_if_changed(out_icons_cpp, _gen_icons_def_cpp(atlas_data))
            _write_if_changed(out_metrics_hpp, _gen_icons_metrics_hpp(icon_names, icon_aliases, icon_boxes, atlas_w, atlas_h, icon_px, pxrange))


_main()
