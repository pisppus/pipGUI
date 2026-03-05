import json
import math
import os
import re
import shutil
import subprocess
import sys
import struct
import zlib

sys.dont_write_bytecode = True

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from psdf import (
    _write_png_gray8,
    _fmt_f,
    _read_json,
    _write_if_changed,
    _hash_bytes,
    _safe_ident,
    _snake_to_camel,
    _camel_from_file,
    _best_grid,
)

def _stroke_to_path_svg(project_dir: str, in_svg_path: str, out_svg_path: str) -> bool:
    try:
        from svgpathtools import svg2paths
        import xml.etree.ElementTree as ET
    except ImportError:
        return False
    
    try:
        tree = ET.parse(in_svg_path)
        root = tree.getroot()
        
        viewbox = root.get('viewBox', f"0 0 {root.get('width', '100')} {root.get('height', '100')}")
        width = root.get('width', '100')
        height = root.get('height', '100')
        
        paths, attributes = svg2paths(in_svg_path)
        
        d_list = []
        
        for path, attr in zip(paths, attributes):
            stroke = attr.get('stroke', '').lower()
            has_stroke = stroke and stroke != 'none'
            
            if has_stroke:
                stroke_width = float(attr.get('stroke-width', '1').replace('px', '').replace('pt', ''))
                d = _stroke_outline_d(path, stroke_width)
                if d:
                    d_list.append(d)
            else:
                fill = attr.get('fill', '').lower()
                if fill and fill != 'none':
                    d_list.append(path.d())
        
        if d_list:
            svg_ns = "http://www.w3.org/2000/svg"
            ET.register_namespace("", svg_ns)

            out_root = ET.Element("{http://www.w3.org/2000/svg}svg")
            out_root.set("width", str(width))
            out_root.set("height", str(height))
            out_root.set("viewBox", str(viewbox))

            for d in d_list:
                el = ET.SubElement(out_root, "{http://www.w3.org/2000/svg}path")
                el.set("d", d)
                el.set("fill", "black")
                el.set("stroke", "none")

            ET.ElementTree(out_root).write(out_svg_path, encoding="utf-8", xml_declaration=False)
            return True
        else:
            import shutil
            shutil.copy(in_svg_path, out_svg_path)
            return True
            
    except Exception as e:
        print(f"[icons] warn: vector stroke-to-path failed: {e}")
        return False


def _stroke_outline_d(path, stroke_width):
    if not getattr(path, "isclosed", lambda: False)():
        return None

    half = float(stroke_width) * 0.5
    if half <= 0.0:
        return None

    steps = int(os.environ.get("PIPGUI_STROKE2PATH_STEPS", "2048"))
    if steps < 64:
        steps = 64
    if steps > 16384:
        steps = 16384

    outer = _offset_curve_points(path, +half, steps)
    inner = _offset_curve_points(path, -half, steps)
    if not outer or not inner:
        return None

    a_outer = _poly_area(outer)
    a_inner = _poly_area(inner)
    if abs(a_inner) > abs(a_outer):
        outer, inner = inner, outer
        a_outer, a_inner = a_inner, a_outer

    if (a_outer >= 0.0) == (a_inner >= 0.0):
        inner = list(reversed(inner))
        a_inner = -a_inner

    if outer[0] != outer[-1]:
        outer.append(outer[0])
    if inner[0] != inner[-1]:
        inner.append(inner[0])

    return f"{_points_to_d(outer)} {_points_to_d(inner)}"


def _poly_area(pts):
    if not pts:
        return 0.0
    a = 0.0
    n = len(pts)
    for i in range(n):
        p0 = pts[i]
        p1 = pts[(i + 1) % n]
        a += (p0.real * p1.imag) - (p1.real * p0.imag)
    return 0.5 * a


def _offset_curve_points(path, offset_distance: float, steps: int):
    pts = []
    for seg in path:
        for k in range(steps):
            t = k / steps
            try:
                n = seg.normal(t)
                p = seg.point(t)
            except Exception:
                return None
            pts.append(p + offset_distance * n)
    return pts


def _points_to_d(pts):
    if not pts:
        return ""
    s = [f"M {_fmt_f(pts[0].real)} {_fmt_f(pts[0].imag)}"]
    for p in pts[1:]:
        s.append(f"L {_fmt_f(p.real)} {_fmt_f(p.imag)}")
    s.append("Z")
    return " ".join(s)


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

        path_tag = re.sub(r"\s+(opacity|fill-opacity|stroke-opacity)\s*=\s*\"[^\"]*\"", "", path_tag, flags=re.IGNORECASE)
        path_tag = re.sub(r"\s+(opacity|fill-opacity|stroke-opacity)\s*=\s*'[^']*'", "", path_tag, flags=re.IGNORECASE)

        has_stroke = bool(re.search(r"\bstroke\s*=", path_tag, flags=re.IGNORECASE))
        m_fill = re.search(r"\bfill\s*=\s*(\"[^\"]*\"|'[^']*')", path_tag, flags=re.IGNORECASE)
        has_fill = bool(m_fill)
        stroke_only = bool(has_stroke and not has_fill)
        stroke_warn = os.environ.get("PIPGUI_ICONS_WARN_STROKE", "0").strip().lower() in ("1", "true", "yes", "on")
        if stroke_only and stroke_warn:
            print(f"[icons] warn: stroke-only path in {os.path.basename(svg_path)} (layer {layer_name})")

        if has_fill:
            fill_raw = m_fill.group(1)[1:-1].strip().lower() if m_fill else ""
            if fill_raw != "none":
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


def _msdf_transform_from_viewbox(viewbox, out_px: int):
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
    ty = (-vy) + off_y
    return scale, tx, ty


def _gen_icons_decl_hpp() -> str:
    out = []
    out.append("#pragma once\n\n")
    out.append("#include <cstdint>\n\n")
    out.append("extern const uint8_t icons[];\n")
    return "".join(out)


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


def _gen_icons_metrics_hpp(icon_names, icon_aliases, icon_boxes, atlas_w, atlas_h, nominal_px, pxrange) -> str:
    out = []
    out.append("#pragma once\n")
    out.append("#include <cstdint>\n")
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
    out.append("}\n")
    out.append("\n")
    
    for name, alias in zip(icon_names, icon_aliases):
        out.append(f"constexpr pipgui::IconId {name} = ::pipgui::psdf_icons::Icon{name};\n")
        if alias != name:
            out.append(f"constexpr pipgui::IconId {alias} = ::pipgui::psdf_icons::Icon{name};\n")
    
    return "".join(out)

def generate_all(project_dir: str) -> bool:
    icons_svg_dir = os.path.join(project_dir, "tools", "icons", "SVG")
    icons_exe = os.path.join(project_dir, "tools", "icons", "bin", "msdfgen.exe")
    if not os.path.isfile(icons_exe):
        icons_exe = os.path.join(project_dir, "tools", "icons", "msdfgen.exe")

    if not os.path.isdir(icons_svg_dir):
        print("[icons] SVG dir not found:", icons_svg_dir)
        return False
    
    if not os.path.isfile(icons_exe):
        print("[icons] msdfgen.exe not found:", icons_exe)
        return False

    svg_files = [f for f in os.listdir(icons_svg_dir) if f.lower().endswith(".svg")]
    svg_files.sort()

    if not svg_files:
        print("[icons] No SVG files found")
        return False

    out_icons_dir = os.path.join(project_dir, "lib", "pipKit", "pipGUI", "icons")
    out_icons_hpp = os.path.join(out_icons_dir, "icons.hpp")
    out_icons_cpp = os.path.join(out_icons_dir, "icons.cpp")
    out_metrics_hpp = os.path.join(out_icons_dir, "metrics.hpp")

    icons_work_dir = os.path.join(project_dir, "tools", "icons", "bin", "PSDF")
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
        icon_dir = os.path.join(icons_work_dir, base)
        os.makedirs(icon_dir, exist_ok=True)
        viewbox = _svg_viewbox(svg_path)
        xform = _msdf_transform_from_viewbox(viewbox, icon_px)
        layers = _svg_split_layers(svg_path)
        if not layers:
            layers = [("layer0", None, False)]

        base_camel = _camel_from_file(base)

        for layer_name, layer_svg, has_stroke in layers:
            if layer_svg is not None:
                tmp_svg_path = os.path.join(icon_dir, f"{base}__{layer_name}.svg")
                _write_if_changed(tmp_svg_path, layer_svg)

                src_svg_path = tmp_svg_path
                if has_stroke:
                    out_svg_path = os.path.join(icon_dir, f"{base}__{layer_name}__stroked.svg")
                    ok = _stroke_to_path_svg(project_dir, tmp_svg_path, out_svg_path)
                    if ok:
                        src_svg_path = out_svg_path
                    else:
                        print(f"[icons] warn: stroke-to-path failed for {os.path.basename(svg_path)} {layer_name}")
            else:
                src_svg_path = svg_path
                if _svg_has_stroke(svg_path):
                    out_svg_path = os.path.join(icon_dir, f"{base}__{layer_name}__stroked.svg")
                    ok = _stroke_to_path_svg(project_dir, svg_path, out_svg_path)
                    if ok:
                        src_svg_path = out_svg_path
                    else:
                        print(f"[icons] warn: stroke-to-path failed for {os.path.basename(svg_path)}")

            name_camel = base_camel
            alias_base = base.lower()
            if layer_name != "layer0" or layer_svg is not None:
                name_camel = base_camel + _snake_to_camel("_" + layer_name)
                alias_base = f"{alias_base}_{layer_name.lower()}"

            icon_names.append(_safe_ident(name_camel))
            icon_aliases.append(_safe_ident(alias_base))

            out_bin = os.path.join(icon_dir, f"{base}__{layer_name}_{icon_px}.bin")

            cmd = [
                os.path.abspath(icons_exe),
                "psdf",
                "-svg", os.path.abspath(src_svg_path),
                "-dimensions", str(icon_px), str(icon_px),
                "-pxrange", str(pxrange),
                "-format", "bin",
                "-o", os.path.abspath(out_bin),
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

            _write_png_gray8(os.path.splitext(out_bin)[0] + ".png", icon_px, icon_px, b)

    stamp_path = os.path.join(icons_work_dir, "stamp.txt")
    atlas_bin_path = os.path.join(icons_work_dir, "atlas.bin")
    atlas_json_path = os.path.join(icons_work_dir, "atlas.json")
    atlas_png_path = os.path.join(icons_work_dir, "atlas.png")

    try:
        __import__("svgpathtools")
        has_svgpathtools = True
    except Exception:
        has_svgpathtools = False
    
    stamp_in = "\n".join([
        "icons_psdf_v5_vectorstroke_v2",
        str(icon_px),
        str(pxrange),
        f"svgpathtools={'1' if has_svgpathtools else '0'}",
        json.dumps(svg_files, sort_keys=True),
        json.dumps(per_icon_hashes, sort_keys=True),
    ])

    prev_stamp = None
    try:
        with open(stamp_path, "r", encoding="utf-8") as f:
            prev_stamp = f.read()
    except OSError:
        prev_stamp = None

    cache_ok = (
        prev_stamp == stamp_in
        and os.path.isfile(atlas_bin_path)
        and os.path.isfile(atlas_json_path)
        and os.path.isfile(out_icons_hpp)
        and os.path.isfile(out_icons_cpp)
        and os.path.isfile(out_metrics_hpp)
    )

    if cache_ok:
        print("[icons] up-to-date")
        return True

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
                src_row = icon_px - 1 - yy
                src_off = src_row * icon_px
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
        _write_png_gray8(atlas_png_path, atlas_w, atlas_h, bytes(atlas))
        with open(stamp_path, "w", encoding="utf-8") as f:
            f.write(stamp_in)

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

    print("[icons] generated")

    return True


if __name__ == "__main__":
    try:
        Import("env")
        project_dir = env["PROJECT_DIR"]
    except Exception:
        project_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
    
    generate_all(project_dir)
