Import("env")

import json
import os


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


def _gen_header(atlas: dict) -> str:
    atlas_info = atlas.get("atlas", {})
    metrics = atlas.get("metrics", {})
    glyphs = atlas.get("glyphs", [])

    width = int(atlas_info.get("width", 0))
    height = int(atlas_info.get("height", 0))
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
    out.append("\nnamespace psdf\n{")
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


def _main():
    project_dir = env["PROJECT_DIR"]
    atlas_path = os.path.join(project_dir, "lib", "pipSystem", "pipGUI", "fonts", "atlas.json")
    out_path = os.path.join(project_dir, "lib", "pipSystem", "pipGUI", "fonts", "WixMadeForDisplay_psdf_metrics.h")

    data = _read_json(atlas_path)
    content = _gen_header(data)
    _write_if_changed(out_path, content)


_main()
