import importlib.util
import json
import os
import re
import subprocess
import sys

os.environ.setdefault("PYTHONDONTWRITEBYTECODE", "1")
sys.dont_write_bytecode = True

_THIS_DIR = os.path.dirname(os.path.abspath(__file__))
_TOOLS_DIR = os.path.dirname(os.path.dirname(_THIS_DIR))
if _THIS_DIR not in sys.path:
    sys.path.insert(0, _THIS_DIR)
if _TOOLS_DIR not in sys.path:
    sys.path.insert(0, _TOOLS_DIR)

from lottie import generate as generate_lottie, outputs_exist as lottie_outputs_exist, stamp_fragment as lottie_stamp_fragment
from common import msdf_transform_from_viewbox, source_files, source_key, stroke_to_path_svg, svg_viewbox
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


def _icons_sources_dir(project_dir: str) -> str:
    return os.path.join(project_dir, "tools", "icons", "sources")


def _project_stamp_path(project_dir: str) -> str:
    return os.path.join(project_dir, "tools", "icons", "PSDF", "project.stamp")


def _atlas_stamp_path(project_dir: str) -> str:
    return os.path.join(project_dir, "tools", "icons", "PSDF", "stamp.txt")


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


def _project_stamp(project_dir: str, svg_files: list[str], json_files: list[str], icons_exe: str, icon_px: int, pxrange: int) -> str:
    psdf_path = os.path.join(project_dir, "tools", "psdf.py")
    common_path = os.path.join(_THIS_DIR, "common.py")
    steps = os.environ.get("PIPGUI_STROKE2PATH_STEPS", "2048")
    icons_sources_dir = _icons_sources_dir(project_dir)

    try:
        svgpathtools_spec = importlib.util.find_spec("svgpathtools")
        svgpathtools_sig = _file_stat_signature(svgpathtools_spec.origin) if svgpathtools_spec and svgpathtools_spec.origin else "present"
    except Exception:
        svgpathtools_sig = "missing"

    parts = [
        "icons_project_cache_v5",
        _hash_file(os.path.abspath(__file__)),
        _hash_file(psdf_path),
        _hash_file(common_path),
        _file_stat_signature(icons_exe),
        json.dumps(
            {
                "icon_px": icon_px,
                "pxrange": pxrange,
                "stroke_steps": steps,
                "svgpathtools": svgpathtools_sig,
            },
            sort_keys=True,
        ),
    ]

    for svg_file in svg_files:
        svg_path = os.path.join(icons_sources_dir, svg_file)
        parts.append(json.dumps({"file": svg_file, "hash": _hash_file(svg_path)}, sort_keys=True))

    parts.append(lottie_stamp_fragment(project_dir, json_files))
    return "\n".join(parts)


def _project_outputs_exist(project_dir: str) -> bool:
    out_icons_dir = _out_icons_dir(project_dir)
    return (
        os.path.isfile(os.path.join(out_icons_dir, "Icons.cpp"))
        and os.path.isfile(os.path.join(out_icons_dir, "Metrics.hpp"))
        and lottie_outputs_exist(project_dir)
        and os.path.isfile(os.path.join(project_dir, "tools", "icons", "PSDF", "atlas.bin"))
        and os.path.isfile(os.path.join(project_dir, "tools", "icons", "PSDF", "atlas.json"))
    )


def _debug_artifacts_enabled() -> bool:
    return os.environ.get("PIPGUI_PSDF_DEBUG", "0").strip().lower() in ("1", "true", "yes", "on")


def _remove_if_exists(path: str) -> None:
    try:
        os.remove(path)
    except OSError:
        pass


def _assert_unique_source_keys(rel_paths: list[str], kind: str) -> None:
    seen: dict[str, str] = {}
    for rel_path in rel_paths:
        key = source_key(rel_path)
        prev = seen.get(key)
        if prev is not None and prev != rel_path:
            raise ValueError(f"[icons] {kind} source id collision: {prev} and {rel_path} both map to '{key}'")
        seen[key] = rel_path


def is_up_to_date(project_dir: str, announce: bool = False) -> bool:
    icons_sources_dir = _icons_sources_dir(project_dir)
    icons_exe = os.path.join(project_dir, "tools", "icons", "bin", "msdfgen.exe")
    if not os.path.isfile(icons_exe):
        icons_exe = os.path.join(project_dir, "tools", "icons", "msdfgen.exe")

    if not os.path.isdir(icons_sources_dir) or not os.path.isfile(icons_exe):
        return False

    svg_files = source_files(icons_sources_dir, (".svg",))
    json_files = source_files(icons_sources_dir, (".json",))
    if not svg_files and not json_files:
        return False
    try:
        _assert_unique_source_keys(svg_files, "static")
        _assert_unique_source_keys(json_files, "animated")
    except ValueError:
        return False

    prev_stamp = _read_text(_project_stamp_path(project_dir))
    if prev_stamp is None:
        return False

    current_stamp = _project_stamp(project_dir, svg_files, json_files, icons_exe, 48, 8)
    up_to_date = (prev_stamp == current_stamp) and _project_outputs_exist(project_dir)
    if up_to_date and announce:
        print("[icons] up-to-date")
    return up_to_date


def _svg_split_layers(svg_path: str):
    try:
        with open(svg_path, "r", encoding="utf-8") as handle:
            text = handle.read()
    except OSError:
        return None

    match = re.search(r"<svg\b[^>]*>", text, flags=re.IGNORECASE)
    if not match:
        return None
    svg_open = match.group(0)

    path_re = re.compile(r"<path\b[^>]*/\s*>|<path\b[^>]*>.*?</path>", flags=re.IGNORECASE | re.DOTALL)
    path_matches = list(path_re.finditer(text))
    if not path_matches:
        return None

    layers = []
    for index, path_match in enumerate(path_matches):
        path_tag = path_match.group(0).strip()
        layer_name = f"layer{index}"

        path_tag = re.sub(r"\s+(opacity|fill-opacity|stroke-opacity)\s*=\s*\"[^\"]*\"", "", path_tag, flags=re.IGNORECASE)
        path_tag = re.sub(r"\s+(opacity|fill-opacity|stroke-opacity)\s*=\s*'[^']*'", "", path_tag, flags=re.IGNORECASE)

        has_stroke = bool(re.search(r"\bstroke\s*=", path_tag, flags=re.IGNORECASE))
        fill_match = re.search(r"\bfill\s*=\s*(\"[^\"]*\"|'[^']*')", path_tag, flags=re.IGNORECASE)
        has_fill = bool(fill_match)
        stroke_only = bool(has_stroke and not has_fill)
        if stroke_only and os.environ.get("PIPGUI_ICONS_WARN_STROKE", "0").strip().lower() in ("1", "true", "yes", "on"):
            print(f"[icons] warn: stroke-only path in {os.path.basename(svg_path)} (layer {layer_name})")

        if has_fill:
            fill_raw = fill_match.group(1)[1:-1].strip().lower() if fill_match else ""
            if fill_raw != "none":
                path_tag = re.sub(r"\bfill\s*=\s*(\"[^\"]*\"|'[^']*')", 'fill="black"', path_tag, flags=re.IGNORECASE)
        if has_stroke:
            path_tag = re.sub(r"\bstroke\s*=\s*(\"[^\"]*\"|'[^']*')", 'stroke="black"', path_tag, flags=re.IGNORECASE)

        layers.append((layer_name, f"{svg_open}{path_tag}</svg>", has_stroke))
    return layers


def _svg_has_stroke(svg_path: str) -> bool:
    try:
        with open(svg_path, "r", encoding="utf-8") as handle:
            text = handle.read(256 * 1024)
    except OSError:
        return False
    return bool(re.search(r"\bstroke\s*=", text, flags=re.IGNORECASE) or re.search(r"\bstroke\s*:", text, flags=re.IGNORECASE))


def _gen_icons_def_cpp(data: bytes) -> str:
    out = ['#include "Metrics.hpp"\n\n', "const uint8_t icons[] = {\n"]
    for index in range(0, len(data), 16):
        chunk = data[index:index + 16]
        out.append("  " + ", ".join(f"0x{value:02x}" for value in chunk) + ",\n")
    out.append("};\n")
    return "".join(out)


def _gen_icons_metrics_hpp(icon_names, icon_aliases, icon_boxes, atlas_w, atlas_h, nominal_px, pxrange) -> str:
    out = []
    out.append("#pragma once\n")
    out.append("#include <cstdint>\n")
    out.append("\nextern const uint8_t icons[];\n")
    out.append("\nnamespace pipgui\n{")
    out.append("\nnamespace psdf_icons\n{")
    out.append(f"\ninline constexpr uint16_t AtlasWidth = {atlas_w};")
    out.append(f"\ninline constexpr uint16_t AtlasHeight = {atlas_h};")
    out.append(f"\ninline constexpr float DistanceRange = {_fmt_f(float(pxrange))}f;")
    out.append(f"\ninline constexpr float NominalSizePx = {_fmt_f(float(nominal_px))}f;\n")

    out.append("\nenum IconId : uint16_t\n{")
    for index, name in enumerate(icon_names):
        comma = "," if index + 1 < len(icon_names) else ""
        out.append(f"\n    Icon{name} = {index}{comma}")
    out.append("\n};\n")

    out.append("\nstruct Icon\n{")
    out.append("\n    uint16_t x;")
    out.append("\n    uint16_t y;")
    out.append("\n    uint16_t w;")
    out.append("\n    uint16_t h;")
    out.append("\n};\n")

    out.append(f"\ninline constexpr uint16_t IconCount = {len(icon_names)};\n")
    out.append("\ninline constexpr Icon Icons[IconCount] =\n{")
    for index, (x, y, w, h) in enumerate(icon_boxes):
        comma = "," if index + 1 < len(icon_boxes) else ""
        out.append(f"\n    {{{x}u, {y}u, {w}u, {h}u}}{comma}")
    out.append("\n};\n")

    out.append("\n}\n}")
    out.append("\n\nnamespace pipgui\n{\n")
    out.append("using IconId = ::pipgui::psdf_icons::IconId;\n")
    for name in icon_names:
        out.append(f"inline constexpr IconId Icon{name} = ::pipgui::psdf_icons::Icon{name};\n")
    out.append("}\n")
    out.append("\n")

    for name, alias in zip(icon_names, icon_aliases):
        out.append(f"inline constexpr pipgui::IconId {alias} = ::pipgui::psdf_icons::Icon{name};\n")

    return "".join(out)


def _layer_stamp_path(icon_dir: str, base: str, layer_name: str, icon_px: int) -> str:
    return os.path.join(icon_dir, f"{base}__{layer_name}_{icon_px}.stamp")


def _render_static_icons(
    project_dir: str,
    svg_files: list[str],
    icons_sources_dir: str,
    icons_work_dir: str,
    icons_exe: str,
    icon_px: int,
    pxrange: int,
):
    common_path = os.path.join(_THIS_DIR, "common.py")
    common_hash = _hash_file(common_path)
    generator_hash = _hash_file(os.path.abspath(__file__))
    psdf_hash = _hash_file(os.path.join(project_dir, "tools", "psdf.py"))
    stroke_steps = os.environ.get("PIPGUI_STROKE2PATH_STEPS", "2048")
    debug_artifacts = _debug_artifacts_enabled()

    per_icon_bins: list[bytes] = []
    per_icon_hashes: list[str] = []
    icon_names: list[str] = []
    icon_aliases: list[str] = []
    rendered_layers = 0
    reused_layers = 0

    for svg_name in svg_files:
        svg_path = os.path.join(icons_sources_dir, svg_name)
        base = source_key(svg_name)
        icon_dir = os.path.join(icons_work_dir, base)
        os.makedirs(icon_dir, exist_ok=True)

        viewbox = svg_viewbox(svg_path)
        xform = msdf_transform_from_viewbox(viewbox, icon_px)
        layers = _svg_split_layers(svg_path) or [("layer0", None, False)]
        multi_layer = len(layers) > 1
        base_camel = _camel_from_file(base)

        for layer_name, layer_svg, has_stroke in layers:
            name_camel = base_camel
            alias_base = base.lower()
            if multi_layer:
                layer_index = 0
                match = re.fullmatch(r"layer(\d+)", layer_name, flags=re.IGNORECASE)
                if match:
                    layer_index = int(match.group(1))
                name_camel = f"{base_camel}L{layer_index}"
                alias_base = f"{alias_base}_l{layer_index}"

            icon_names.append(_safe_ident(name_camel))
            icon_aliases.append(_safe_ident(alias_base))

            out_bin = os.path.join(icon_dir, f"{base}__{layer_name}_{icon_px}.bin")
            preview_png = os.path.splitext(out_bin)[0] + ".png"
            raw_svg_path = os.path.join(icon_dir, f"{base}__{layer_name}.svg")
            stroked_svg_path = os.path.join(icon_dir, f"{base}__{layer_name}__stroked.svg")
            layer_stamp_path = _layer_stamp_path(icon_dir, base, layer_name, icon_px)
            stroke_mode = bool(has_stroke or (layer_svg is None and _svg_has_stroke(svg_path)))
            layer_input_hash = _hash_bytes(layer_svg.encode("utf-8")) if layer_svg is not None else _hash_file(svg_path)
            layer_stamp = "\n".join(
                [
                    "icons_static_layer_v3",
                    layer_input_hash,
                    _file_stat_signature(icons_exe),
                    generator_hash,
                    psdf_hash,
                    common_hash if stroke_mode else "",
                    json.dumps(
                        {
                            "base": base,
                            "layer": layer_name,
                            "icon_px": icon_px,
                            "pxrange": pxrange,
                            "stroke": stroke_mode,
                            "stroke_steps": stroke_steps if stroke_mode else "",
                            "transform": tuple(_fmt_f(float(value)) for value in xform) if xform else None,
                        },
                        sort_keys=True,
                    ),
                ]
            )

            layer_cache_ok = _read_text(layer_stamp_path) == layer_stamp and os.path.isfile(out_bin)
            if not layer_cache_ok:
                if layer_svg is not None:
                    _write_if_changed(raw_svg_path, layer_svg)
                    src_svg_path = raw_svg_path
                    if stroke_mode:
                        if stroke_to_path_svg(
                            raw_svg_path,
                            stroked_svg_path,
                            step_env="PIPGUI_STROKE2PATH_STEPS",
                            default_steps=2048,
                            min_steps=64,
                            max_steps=16384,
                            warn_prefix="[icons] warn: vector stroke-to-path failed",
                            apply_path_transform=False,
                            copy_if_empty=True,
                        ):
                            src_svg_path = stroked_svg_path
                        else:
                            print(f"[icons] warn: stroke-to-path failed for {svg_name} {layer_name}")
                else:
                    src_svg_path = svg_path
                    if stroke_mode:
                        if stroke_to_path_svg(
                            svg_path,
                            stroked_svg_path,
                            step_env="PIPGUI_STROKE2PATH_STEPS",
                            default_steps=2048,
                            min_steps=64,
                            max_steps=16384,
                            warn_prefix="[icons] warn: vector stroke-to-path failed",
                            apply_path_transform=False,
                            copy_if_empty=True,
                        ):
                            src_svg_path = stroked_svg_path
                        else:
                            print(f"[icons] warn: stroke-to-path failed for {svg_name}")

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

                subprocess.check_call(cmd, cwd=icons_work_dir)
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
            per_icon_bins.append(icon_bin)
            per_icon_hashes.append(_hash_bytes(icon_bin))

    return {
        "bins": per_icon_bins,
        "hashes": per_icon_hashes,
        "names": icon_names,
        "aliases": icon_aliases,
        "rendered_layers": rendered_layers,
        "reused_layers": reused_layers,
    }


def generate_all(project_dir: str) -> bool:
    icons_sources_dir = _icons_sources_dir(project_dir)
    icons_exe = os.path.join(project_dir, "tools", "icons", "bin", "msdfgen.exe")
    if not os.path.isfile(icons_exe):
        icons_exe = os.path.join(project_dir, "tools", "icons", "msdfgen.exe")

    if not os.path.isdir(icons_sources_dir):
        print("[icons] sources dir not found:", icons_sources_dir)
        return False
    if not os.path.isfile(icons_exe):
        print("[icons] msdfgen.exe not found:", icons_exe)
        return False

    svg_files = source_files(icons_sources_dir, (".svg",))
    json_files = source_files(icons_sources_dir, (".json",))
    if not svg_files and not json_files:
        print("[icons] No icon sources found")
        return False
    if not svg_files:
        print("[icons] No static SVG icon sources found")
        return False
    try:
        _assert_unique_source_keys(svg_files, "static")
        _assert_unique_source_keys(json_files, "animated")
    except ValueError as exc:
        print(exc)
        return False

    out_icons_dir = _out_icons_dir(project_dir)
    out_icons_cpp = os.path.join(out_icons_dir, "Icons.cpp")
    out_metrics_hpp = os.path.join(out_icons_dir, "Metrics.hpp")
    icons_work_dir = os.path.join(project_dir, "tools", "icons", "PSDF")
    os.makedirs(icons_work_dir, exist_ok=True)

    icon_px = 48
    pxrange = 8
    project_stamp = _project_stamp(project_dir, svg_files, json_files, icons_exe, icon_px, pxrange)
    project_stamp_path = _project_stamp_path(project_dir)
    if _read_text(project_stamp_path) == project_stamp and _project_outputs_exist(project_dir):
        print("[icons] up-to-date")
        return True

    try:
        rendered = _render_static_icons(project_dir, svg_files, icons_sources_dir, icons_work_dir, icons_exe, icon_px, pxrange)
    except subprocess.CalledProcessError as exc:
        print("[icons] msdfgen failed:", exc)
        return False

    per_icon_bins = rendered["bins"]
    per_icon_hashes = rendered["hashes"]
    icon_names = rendered["names"]
    icon_aliases = rendered["aliases"]

    atlas_bin_path = os.path.join(icons_work_dir, "atlas.bin")
    atlas_json_path = os.path.join(icons_work_dir, "atlas.json")
    atlas_png_path = os.path.join(icons_work_dir, "atlas.png")
    debug_artifacts = _debug_artifacts_enabled()
    atlas_stamp = "\n".join(
        [
            "icons_atlas_v7",
            str(icon_px),
            str(pxrange),
            json.dumps(icon_names, sort_keys=True),
            json.dumps(icon_aliases, sort_keys=True),
            json.dumps(per_icon_hashes, sort_keys=True),
        ]
    )
    atlas_cache_ok = (
        _read_text(_atlas_stamp_path(project_dir)) == atlas_stamp
        and os.path.isfile(atlas_bin_path)
        and os.path.isfile(atlas_json_path)
    )

    if not atlas_cache_ok:
        cols, rows = _best_grid(len(per_icon_bins))
        atlas_w = cols * icon_px
        atlas_h = rows * icon_px
        atlas = bytearray(atlas_w * atlas_h)
        boxes = []

        for index, icon_bin in enumerate(per_icon_bins):
            cx = index % cols
            cy = index // cols
            x0 = cx * icon_px
            y0 = cy * icon_px
            boxes.append((x0, y0, icon_px, icon_px))
            for yy in range(icon_px):
                src_row = icon_px - 1 - yy
                src_off = src_row * icon_px
                dst_off = (y0 + yy) * atlas_w + x0
                atlas[dst_off:dst_off + icon_px] = icon_bin[src_off:src_off + icon_px]

        atlas_dict = {
            "atlas": {
                "width": atlas_w,
                "height": atlas_h,
                "distanceRange": pxrange,
                "size": icon_px,
            },
            "icons": [
                {
                    "name": icon_names[index],
                    "x": boxes[index][0],
                    "y": boxes[index][1],
                    "w": boxes[index][2],
                    "h": boxes[index][3],
                }
                for index in range(len(icon_names))
            ],
        }

        with open(atlas_bin_path, "wb") as handle:
            handle.write(bytes(atlas))
        with open(atlas_json_path, "w", encoding="utf-8") as handle:
            json.dump(atlas_dict, handle, indent=2)
        _write_png_gray8(atlas_png_path, atlas_w, atlas_h, bytes(atlas))
        _write_text(_atlas_stamp_path(project_dir), atlas_stamp)

    atlas = _read_json(atlas_json_path)
    atlas_info = atlas.get("atlas", {})
    atlas_w = int(atlas_info.get("width", 0))
    atlas_h = int(atlas_info.get("height", 0))
    icon_boxes = [
        (int(icon.get("x", 0)), int(icon.get("y", 0)), int(icon.get("w", icon_px)), int(icon.get("h", icon_px)))
        for icon in atlas.get("icons", [])
    ]
    if len(icon_boxes) != len(icon_names):
        cols, rows = _best_grid(len(icon_names))
        atlas_w = max(atlas_w, cols * icon_px)
        atlas_h = max(atlas_h, rows * icon_px)
        icon_boxes = [(index % cols * icon_px, index // cols * icon_px, icon_px, icon_px) for index in range(len(icon_names))]

    with open(atlas_bin_path, "rb") as handle:
        atlas_data = handle.read()
    if not os.path.isfile(atlas_png_path):
        _write_png_gray8(atlas_png_path, atlas_w, atlas_h, atlas_data)

    _write_if_changed(out_icons_cpp, _gen_icons_def_cpp(atlas_data))
    _write_if_changed(
        out_metrics_hpp,
        _gen_icons_metrics_hpp(
            icon_names,
            icon_aliases,
            icon_boxes,
            atlas_w,
            atlas_h,
            icon_px,
            pxrange,
        ),
    )

    if not generate_lottie(project_dir, icons_sources_dir, icons_exe, icon_px, pxrange):
        return False

    _write_text(project_stamp_path, project_stamp)

    if rendered["rendered_layers"] == 0 and atlas_cache_ok:
        print("[icons] up-to-date")
    else:
        print(f"[icons] generated {rendered['rendered_layers']}, reused {rendered['reused_layers']}")
    return True


if __name__ == "__main__":
    try:
        Import("env")
        project_dir = env["PROJECT_DIR"]
    except Exception:
        project_dir = os.path.abspath(os.path.join(_THIS_DIR, "..", "..", ".."))

    generate_all(project_dir)
