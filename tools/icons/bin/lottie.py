import json
import math
import os
import subprocess
import sys

sys.dont_write_bytecode = True

_THIS_DIR = os.path.dirname(os.path.abspath(__file__))
_TOOLS_DIR = os.path.dirname(os.path.dirname(_THIS_DIR))
if _THIS_DIR not in sys.path:
    sys.path.insert(0, _THIS_DIR)
if _TOOLS_DIR not in sys.path:
    sys.path.insert(0, _TOOLS_DIR)

from common import msdf_transform_from_viewbox, source_files, source_key, stroke_to_path_svg
from psdf import (
    _best_grid,
    _camel_from_file,
    _file_stat_signature,
    _fmt_f,
    _hash_bytes,
    _hash_file,
    _read_json,
    _safe_ident,
    _write_if_changed,
    _write_png_gray8,
)


def _out_icons_dir(project_dir: str) -> str:
    return os.path.join(project_dir, "lib", "pipKit", "pipGUI", "Graphics", "Text", "Icons")


def _work_dir(project_dir: str) -> str:
    return os.path.join(project_dir, "tools", "icons", "PSDF", "animated")


def _project_stamp_path(project_dir: str) -> str:
    return os.path.join(_work_dir(project_dir), "project.stamp")


def _atlas_stamp_path(project_dir: str) -> str:
    return os.path.join(_work_dir(project_dir), "stamp.txt")


def _read_text(path: str) -> str | None:
    try:
        with open(path, "r", encoding="utf-8") as handle:
            return handle.read()
    except OSError:
        return None


def _write_text(path: str, content: str) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as handle:
        handle.write(content)


def outputs_exist(project_dir: str) -> bool:
    out_icons_dir = _out_icons_dir(project_dir)
    work_dir = _work_dir(project_dir)
    return (
        os.path.isfile(os.path.join(out_icons_dir, "AnimIcons.cpp"))
        and os.path.isfile(os.path.join(out_icons_dir, "AnimMetrics.hpp"))
        and os.path.isfile(os.path.join(work_dir, "atlas.bin"))
        and os.path.isfile(os.path.join(work_dir, "atlas.json"))
    )


def stamp_fragment(project_dir: str, json_files: list[str]) -> str:
    icons_sources_dir = os.path.join(project_dir, "tools", "icons", "sources")
    common_path = os.path.join(_THIS_DIR, "common.py")
    psdf_path = os.path.join(project_dir, "tools", "psdf.py")
    stroke_steps = os.environ.get("PIPGUI_STROKE2PATH_STEPS", "2048")
    parts = [
        "icons_animated_project_v2",
        _hash_file(os.path.abspath(__file__)),
        _hash_file(common_path),
        _hash_file(psdf_path),
        stroke_steps,
    ]
    for json_file in json_files:
        parts.append(_hash_file(os.path.join(icons_sources_dir, json_file)))
    return "\n".join(parts)


def _debug_artifacts_enabled() -> bool:
    return os.environ.get("PIPGUI_PSDF_DEBUG", "0").strip().lower() in ("1", "true", "yes", "on")


def _remove_if_exists(path: str) -> None:
    try:
        os.remove(path)
    except OSError:
        pass


def _mat_mul(a, b):
    return (
        a[0] * b[0] + a[1] * b[2],
        a[0] * b[1] + a[1] * b[3],
        a[2] * b[0] + a[3] * b[2],
        a[2] * b[1] + a[3] * b[3],
        a[0] * b[4] + a[1] * b[5] + a[4],
        a[2] * b[4] + a[3] * b[5] + a[5],
    )


def _mat_translate(tx: float, ty: float):
    return (1.0, 0.0, 0.0, 1.0, tx, ty)


def _mat_scale(sx: float, sy: float):
    return (sx, 0.0, 0.0, sy, 0.0, 0.0)


def _mat_rotate(deg: float):
    rad = math.radians(deg)
    c = math.cos(rad)
    s = math.sin(rad)
    return (c, -s, s, c, 0.0, 0.0)


def _mat_apply(mat, x: float, y: float):
    return (
        mat[0] * x + mat[1] * y + mat[4],
        mat[2] * x + mat[3] * y + mat[5],
    )


def _value_at(prop, frame: float, fallback):
    if not isinstance(prop, dict):
        return fallback
    if int(prop.get("a", 0)) == 0:
        return prop.get("k", fallback)

    keys = prop.get("k")
    if not isinstance(keys, list) or not keys:
        return fallback

    def as_value(key, field, default):
        if field in key:
            return key[field]
        return default

    if frame <= float(keys[0].get("t", 0.0)):
        return as_value(keys[0], "s", fallback)

    for index in range(len(keys) - 1):
        key0 = keys[index]
        key1 = keys[index + 1]
        t0 = float(key0.get("t", 0.0))
        t1 = float(key1.get("t", t0))
        if frame < t1 and t1 > t0:
            start = as_value(key0, "s", fallback)
            end = as_value(key0, "e", as_value(key1, "s", start))
            mix = (frame - t0) / (t1 - t0)
            if isinstance(start, list):
                return [
                    float(start[i]) + (float(end[i]) - float(start[i])) * mix
                    for i in range(min(len(start), len(end)))
                ]
            return float(start) + (float(end) - float(start)) * mix

    last = keys[-1]
    if "s" in last:
        return last["s"]
    if len(keys) >= 2 and "e" in keys[-2]:
        return keys[-2]["e"]
    return fallback


def _shape_to_path_d(shape, matrix) -> str:
    shape_value = shape.get("ks", {}).get("k")
    if isinstance(shape_value, dict):
        path_shape = shape_value
    elif isinstance(shape_value, list) and shape_value and isinstance(shape_value[0], dict):
        path_shape = shape_value[0].get("s", [None])[0]
    else:
        return ""

    vertices = path_shape.get("v", [])
    tangents_in = path_shape.get("i", [])
    tangents_out = path_shape.get("o", [])
    closed = bool(path_shape.get("c", False))
    count = min(len(vertices), len(tangents_in), len(tangents_out))
    if count == 0:
        return ""

    px, py = _mat_apply(matrix, float(vertices[0][0]), float(vertices[0][1]))
    commands = [f"M {_fmt_f(px)} {_fmt_f(py)}"]

    last_index = count if closed else count - 1
    for index in range(last_index):
        next_index = (index + 1) % count
        v0 = vertices[index]
        v1 = vertices[next_index]
        o0 = tangents_out[index]
        i1 = tangents_in[next_index]

        c1x, c1y = _mat_apply(matrix, float(v0[0]) + float(o0[0]), float(v0[1]) + float(o0[1]))
        c2x, c2y = _mat_apply(matrix, float(v1[0]) + float(i1[0]), float(v1[1]) + float(i1[1]))
        p1x, p1y = _mat_apply(matrix, float(v1[0]), float(v1[1]))
        commands.append(
            f"C {_fmt_f(c1x)} {_fmt_f(c1y)} {_fmt_f(c2x)} {_fmt_f(c2y)} {_fmt_f(p1x)} {_fmt_f(p1y)}"
        )

    if closed:
        commands.append("Z")
    return " ".join(commands)


def _group_transform(transform_item) -> tuple[float, float, float, float, float, float]:
    if not isinstance(transform_item, dict):
        return _mat_translate(0.0, 0.0)

    anchor = _value_at(transform_item.get("a", {}), 0.0, [0.0, 0.0, 0.0])
    position = _value_at(transform_item.get("p", {}), 0.0, [0.0, 0.0, 0.0])
    scale = _value_at(transform_item.get("s", {}), 0.0, [100.0, 100.0, 100.0])
    rotation = _value_at(transform_item.get("r", {}), 0.0, [0.0])

    anchor_x = float(anchor[0]) if isinstance(anchor, list) and anchor else 0.0
    anchor_y = float(anchor[1]) if isinstance(anchor, list) and len(anchor) > 1 else 0.0
    pos_x = float(position[0]) if isinstance(position, list) and position else 0.0
    pos_y = float(position[1]) if isinstance(position, list) and len(position) > 1 else 0.0
    scale_x = (float(scale[0]) if isinstance(scale, list) and scale else 100.0) * 0.01
    scale_y = (float(scale[1]) if isinstance(scale, list) and len(scale) > 1 else scale_x * 100.0) * 0.01
    rotation_deg = float(rotation[0]) if isinstance(rotation, list) and rotation else float(rotation)

    return _mat_mul(
        _mat_translate(pos_x, pos_y),
        _mat_mul(_mat_rotate(rotation_deg), _mat_mul(_mat_scale(scale_x, scale_y), _mat_translate(-anchor_x, -anchor_y))),
    )


def _emit_group_svg(items, parent_matrix, out_paths):
    if not isinstance(items, list):
        return

    transform = parent_matrix
    stroke_width = None
    has_stroke = False
    fill_enabled = False
    child_groups = []
    shapes = []

    for item in items:
        if not isinstance(item, dict):
            continue
        item_type = item.get("ty")
        if item_type == "tr":
            transform = _mat_mul(parent_matrix, _group_transform(item))
        elif item_type == "st":
            has_stroke = True
            width_value = _value_at(item.get("w", {}), 0.0, 1.0)
            if isinstance(width_value, list):
                width_value = width_value[0] if width_value else 1.0
            stroke_width = float(width_value)
        elif item_type == "fl":
            fill_enabled = True
        elif item_type == "sh":
            shapes.append(item)
        elif item_type == "gr":
            child_groups.append(item)

    for shape in shapes:
        path_d = _shape_to_path_d(shape, transform)
        if not path_d:
            continue
        attrs = [f'd="{path_d}"']
        if fill_enabled:
            attrs.append('fill="black"')
        else:
            attrs.append('fill="none"')
        if has_stroke:
            attrs.append('stroke="black"')
            attrs.append(f'stroke-width="{_fmt_f(stroke_width if stroke_width is not None else 1.0)}"')
            attrs.append('stroke-linecap="round"')
            attrs.append('stroke-linejoin="round"')
        else:
            attrs.append('stroke="none"')
        out_paths.append("<path " + " ".join(attrs) + "/>")

    for group in child_groups:
        _emit_group_svg(group.get("it", []), transform, out_paths)


def _layer_svg(comp_w: int, comp_h: int, layer: dict) -> str | None:
    out_paths = []
    _emit_group_svg(layer.get("shapes", []), _mat_translate(0.0, 0.0), out_paths)
    if not out_paths:
        return None
    body = "\n  ".join(out_paths)
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{comp_w}" height="{comp_h}" '
        f'viewBox="0 0 {comp_w} {comp_h}">\n  {body}\n</svg>\n'
    )


def _frame_value(prop, frame: float, default, index: int = 0) -> float:
    value = _value_at(prop, frame, default)
    if isinstance(value, list):
        if index < len(value):
            return float(value[index])
        if value:
            return float(value[0])
    return float(value)


def _sample_layer_frames(layer: dict, ip: int, frame_count: int, transform_scale: int) -> list[dict]:
    ks = layer.get("ks", {})
    frames = []
    for frame_index in range(frame_count + 1):
        frame = float(ip + frame_index)
        pos_x = _frame_value(ks.get("p", {}), frame, [0.0, 0.0, 0.0], 0)
        pos_y = _frame_value(ks.get("p", {}), frame, [0.0, 0.0, 0.0], 1)
        anchor_x = _frame_value(ks.get("a", {}), frame, [0.0, 0.0, 0.0], 0)
        anchor_y = _frame_value(ks.get("a", {}), frame, [0.0, 0.0, 0.0], 1)
        scale_x = _frame_value(ks.get("s", {}), frame, [100.0, 100.0, 100.0], 0)
        scale_y = _frame_value(ks.get("s", {}), frame, [100.0, 100.0, 100.0], 1)
        rotation = _frame_value(ks.get("r", {}), frame, [0.0], 0)
        opacity = _frame_value(ks.get("o", {}), frame, [100.0], 0)
        frames.append(
            {
                "posX": int(round(pos_x * transform_scale)),
                "posY": int(round(pos_y * transform_scale)),
                "anchorX": int(round(anchor_x * transform_scale)),
                "anchorY": int(round(anchor_y * transform_scale)),
                "scaleX": int(round(scale_x * transform_scale)),
                "scaleY": int(round(scale_y * transform_scale)),
                "rotation": int(round(rotation * transform_scale)),
                "opacity": max(0, min(255, int(round(opacity * 255.0 / 100.0)))),
            }
        )
    return frames


def _gen_anim_def_cpp(data: bytes) -> str:
    out = ['#include "AnimMetrics.hpp"\n\n', "const uint8_t animIcons[] = {\n"]
    for index in range(0, len(data), 16):
        chunk = data[index : index + 16]
        out.append("  " + ", ".join(f"0x{value:02x}" for value in chunk) + ",\n")
    out.append("};\n")
    return "".join(out)


def _gen_anim_metrics_hpp(animated_icons, glyph_boxes, frames, layers, atlas_w, atlas_h, nominal_px, pxrange) -> str:
    out = []
    out.append("#pragma once\n")
    out.append("#include <cstdint>\n")
    out.append("\nextern const uint8_t animIcons[];\n")
    out.append("\nnamespace pipgui\n{\nnamespace psdf_anim\n{")
    out.append(f"\ninline constexpr uint16_t AtlasWidth = {atlas_w};")
    out.append(f"\ninline constexpr uint16_t AtlasHeight = {atlas_h};")
    out.append(f"\ninline constexpr float DistanceRange = {_fmt_f(float(pxrange))}f;")
    out.append(f"\ninline constexpr float NominalSizePx = {_fmt_f(float(nominal_px))}f;")
    out.append("\ninline constexpr int16_t TransformScale = 64;\n")

    out.append("\nenum AnimatedIconId : uint16_t\n{")
    for index, icon in enumerate(animated_icons):
        comma = "," if index + 1 < len(animated_icons) else ""
        out.append(f"\n    Animated{icon['name']} = {index}{comma}")
    out.append("\n};\n")

    out.append("\nstruct Glyph { uint16_t x; uint16_t y; uint16_t w; uint16_t h; };")
    out.append("\nstruct FrameTransform { int16_t posX; int16_t posY; int16_t anchorX; int16_t anchorY; int16_t scaleX; int16_t scaleY; int16_t rotation; uint8_t opacity; };")
    out.append("\nstruct Layer { uint16_t glyphIndex; uint16_t frameOffset; };")
    out.append("\nstruct AnimatedIcon { uint16_t width; uint16_t height; uint16_t frameRateX100; uint16_t frameCount; uint16_t firstLayer; uint8_t layerCount; };\n")

    out.append(f"\ninline constexpr uint16_t GlyphCount = {len(glyph_boxes)};")
    out.append("\ninline constexpr Glyph Glyphs[GlyphCount] =\n{")
    for index, (x, y, w, h) in enumerate(glyph_boxes):
        comma = "," if index + 1 < len(glyph_boxes) else ""
        out.append(f"\n    {{{x}u, {y}u, {w}u, {h}u}}{comma}")
    out.append("\n};\n")

    out.append("\ninline constexpr FrameTransform Frames[] =\n{")
    for index, frame in enumerate(frames):
        comma = "," if index + 1 < len(frames) else ""
        out.append(
            f"\n    {{{frame['posX']}, {frame['posY']}, {frame['anchorX']}, {frame['anchorY']}, "
            f"{frame['scaleX']}, {frame['scaleY']}, {frame['rotation']}, {frame['opacity']}u}}{comma}"
        )
    out.append("\n};\n")

    out.append("\ninline constexpr Layer Layers[] =\n{")
    for index, layer in enumerate(layers):
        comma = "," if index + 1 < len(layers) else ""
        out.append(f"\n    {{{layer['glyphIndex']}u, {layer['frameOffset']}u}}{comma}")
    out.append("\n};\n")

    out.append(f"\ninline constexpr uint16_t AnimatedIconCount = {len(animated_icons)};")
    out.append("\ninline constexpr AnimatedIcon Icons[AnimatedIconCount] =\n{")
    for index, icon in enumerate(animated_icons):
        comma = "," if index + 1 < len(animated_icons) else ""
        out.append(
            f"\n    {{{icon['width']}u, {icon['height']}u, {icon['frameRateX100']}u, {icon['frameCount']}u, "
            f"{icon['firstLayer']}u, {icon['layerCount']}u}}{comma}"
        )
    out.append("\n};\n")
    out.append("}\n}\n")

    out.append("\nnamespace pipgui\n{\n")
    out.append("using AnimatedIconId = ::pipgui::psdf_anim::AnimatedIconId;\n")
    for icon in animated_icons:
        out.append(f"inline constexpr AnimatedIconId Animated{icon['name']} = ::pipgui::psdf_anim::Animated{icon['name']};\n")
    out.append("}\n\n")
    for icon in animated_icons:
        out.append(f"inline constexpr pipgui::AnimatedIconId {icon['alias']} = ::pipgui::psdf_anim::Animated{icon['name']};\n")
    return "".join(out)


def generate(project_dir: str, icons_sources_dir: str, icons_exe: str, icon_px: int, pxrange: int) -> bool:
    json_files = source_files(icons_sources_dir, (".json",))
    if not json_files:
        return True

    out_icons_dir = _out_icons_dir(project_dir)
    out_icons_cpp = os.path.join(out_icons_dir, "AnimIcons.cpp")
    out_metrics_hpp = os.path.join(out_icons_dir, "AnimMetrics.hpp")
    work_dir = _work_dir(project_dir)
    os.makedirs(work_dir, exist_ok=True)

    project_stamp = "\n".join(
        [
            "icons_animated_project_v2",
            stamp_fragment(project_dir, json_files),
            _file_stat_signature(icons_exe),
            json.dumps({"icon_px": icon_px, "pxrange": pxrange}, sort_keys=True),
        ]
    )
    if _read_text(_project_stamp_path(project_dir)) == project_stamp and outputs_exist(project_dir):
        return True

    glyph_bins = []
    glyph_hashes = []
    glyph_boxes = []
    frames = []
    layers = []
    animated_icons = []
    rendered_layers = 0
    reused_layers = 0
    debug_artifacts = _debug_artifacts_enabled()

    for json_name in json_files:
        json_path = os.path.join(icons_sources_dir, json_name)
        data = _read_json(json_path)
        base = source_key(json_name)
        base_camel = _camel_from_file(base)
        alias = _safe_ident(base.lower() + "_anim")
        comp_w = int(data.get("w", 0))
        comp_h = int(data.get("h", 0))
        ip = int(data.get("ip", 0))
        op = int(data.get("op", 0))
        frame_count = max(0, op - ip)
        fr = float(data.get("fr", 0.0))
        if comp_w <= 0 or comp_h <= 0:
            continue

        first_layer = len(layers)
        rendered_layer_count = 0
        icon_dir = os.path.join(work_dir, base)
        os.makedirs(icon_dir, exist_ok=True)

        for layer_index, layer in enumerate(data.get("layers", [])):
            if not isinstance(layer, dict) or int(layer.get("ty", -1)) != 4:
                continue

            layer_svg = _layer_svg(comp_w, comp_h, layer)
            if not layer_svg:
                continue

            raw_svg_path = os.path.join(icon_dir, f"layer{layer_index}.svg")
            stroked_svg_path = os.path.join(icon_dir, f"layer{layer_index}__stroked.svg")
            out_bin = os.path.join(icon_dir, f"layer{layer_index}_{icon_px}.bin")
            preview_png = os.path.splitext(out_bin)[0] + ".png"
            layer_stamp_path = os.path.join(icon_dir, f"layer{layer_index}_{icon_px}.stamp")

            frame_payload = _sample_layer_frames(layer, ip, frame_count, 64)
            layer_stamp = "\n".join(
                [
                    "icons_animated_layer_v3",
                    _hash_bytes(layer_svg.encode("utf-8")),
                    _hash_bytes(json.dumps(frame_payload, sort_keys=True).encode("utf-8")),
                    _file_stat_signature(icons_exe),
                    _hash_file(os.path.abspath(__file__)),
                    _hash_file(os.path.join(_THIS_DIR, "common.py")),
                    _hash_file(os.path.join(project_dir, "tools", "psdf.py")),
                    json.dumps(
                        {
                            "icon_px": icon_px,
                            "pxrange": pxrange,
                            "comp": [comp_w, comp_h],
                            "stroke_steps": os.environ.get("PIPGUI_STROKE2PATH_STEPS", "2048"),
                        },
                        sort_keys=True,
                    ),
                ]
            )

            layer_cache_ok = _read_text(layer_stamp_path) == layer_stamp and os.path.isfile(out_bin)
            if not layer_cache_ok:
                _write_if_changed(raw_svg_path, layer_svg)
                src_svg_path = raw_svg_path
                if stroke_to_path_svg(
                    raw_svg_path,
                    stroked_svg_path,
                    step_env="PIPGUI_STROKE2PATH_STEPS",
                    default_steps=2048,
                    min_steps=256,
                    max_steps=16384,
                    warn_prefix="[icons] animated stroke->path failed",
                    apply_path_transform=False,
                    copy_if_empty=True,
                ):
                    src_svg_path = stroked_svg_path
                xform = msdf_transform_from_viewbox((0.0, 0.0, float(comp_w), float(comp_h)), icon_px)
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
                    scale, tx, ty = xform
                    cmd.extend(["-scale", _fmt_f(float(scale))])
                    cmd.extend(["-translate", _fmt_f(float(tx)), _fmt_f(float(ty))])
                try:
                    subprocess.check_call(cmd, cwd=work_dir)
                except subprocess.CalledProcessError as exc:
                    print("[icons] animated msdfgen failed:", exc)
                    return False

                _write_text(layer_stamp_path, layer_stamp)
                rendered_layers += 1
            else:
                reused_layers += 1

            with open(out_bin, "rb") as handle:
                icon_bin = handle.read()
            if debug_artifacts and (not os.path.isfile(preview_png) or not layer_cache_ok):
                _write_png_gray8(preview_png, icon_px, icon_px, icon_bin)
            if not debug_artifacts:
                _remove_if_exists(preview_png)
                _remove_if_exists(raw_svg_path)
                _remove_if_exists(stroked_svg_path)

            glyph_index = len(glyph_bins)
            glyph_bins.append(icon_bin)
            glyph_hashes.append(_hash_bytes(icon_bin))
            layers.append({"glyphIndex": glyph_index, "frameOffset": len(frames)})
            frames.extend(frame_payload)
            rendered_layer_count += 1

        if rendered_layer_count == 0:
            continue

        animated_icons.append(
            {
                "name": _safe_ident(base_camel),
                "alias": alias,
                "width": comp_w,
                "height": comp_h,
                "frameRateX100": int(round(fr * 100.0)),
                "frameCount": frame_count,
                "firstLayer": first_layer,
                "layerCount": rendered_layer_count,
            }
        )

    if not animated_icons:
        return True

    cols, rows = _best_grid(len(glyph_bins))
    atlas_w = cols * icon_px
    atlas_h = rows * icon_px
    atlas = bytearray(atlas_w * atlas_h)

    for index, icon_bin in enumerate(glyph_bins):
        cx = index % cols
        cy = index // cols
        x0 = cx * icon_px
        y0 = cy * icon_px
        glyph_boxes.append((x0, y0, icon_px, icon_px))
        for yy in range(icon_px):
            src_row = icon_px - 1 - yy
            src_off = src_row * icon_px
            dst_off = (y0 + yy) * atlas_w + x0
            atlas[dst_off : dst_off + icon_px] = icon_bin[src_off : src_off + icon_px]

    atlas_bin_path = os.path.join(work_dir, "atlas.bin")
    atlas_json_path = os.path.join(work_dir, "atlas.json")
    atlas_png_path = os.path.join(work_dir, "atlas.png")
    atlas_stamp = "\n".join(
        [
            "icons_animated_atlas_v2",
            str(icon_px),
            str(pxrange),
            json.dumps(glyph_hashes, sort_keys=True),
            json.dumps(animated_icons, sort_keys=True),
        ]
    )
    atlas_cache_ok = (
        _read_text(_atlas_stamp_path(project_dir)) == atlas_stamp
        and os.path.isfile(atlas_bin_path)
        and os.path.isfile(atlas_json_path)
    )

    atlas_bytes = bytes(atlas)
    if not atlas_cache_ok:
        with open(atlas_bin_path, "wb") as handle:
            handle.write(atlas_bytes)
        with open(atlas_json_path, "w", encoding="utf-8") as handle:
            json.dump(
                {
                    "atlas": {"width": atlas_w, "height": atlas_h, "distanceRange": pxrange, "size": icon_px},
                    "glyphs": [{"x": x, "y": y, "w": w, "h": h} for x, y, w, h in glyph_boxes],
                    "icons": animated_icons,
                },
                handle,
                indent=2,
            )
        _write_png_gray8(atlas_png_path, atlas_w, atlas_h, atlas_bytes)
        _write_text(_atlas_stamp_path(project_dir), atlas_stamp)
    elif not os.path.isfile(atlas_png_path):
        _write_png_gray8(atlas_png_path, atlas_w, atlas_h, atlas_bytes)

    _write_if_changed(out_icons_cpp, _gen_anim_def_cpp(atlas_bytes))
    _write_if_changed(
        out_metrics_hpp,
        _gen_anim_metrics_hpp(animated_icons, glyph_boxes, frames, layers, atlas_w, atlas_h, icon_px, pxrange),
    )
    _write_text(_project_stamp_path(project_dir), project_stamp)

    print(f"[icons] animated generated {rendered_layers}, reused {reused_layers}")
    return True
