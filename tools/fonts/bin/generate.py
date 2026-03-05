import hashlib
import json
import os
import shutil
import subprocess
import sys

sys.dont_write_bytecode = True

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from psdf import (
    _write_png_gray8,
    _fmt_f,
    _read_json,
    _write_if_changed,
    _hash_file,
    _safe_ident,
    _snake_to_camel,
)


def _load_config(project_dir: str):
    """Load configuration from tools/fonts/config.py (required)."""
    cfg_path = os.path.join(project_dir, "tools", "fonts", "config.py")
    if not os.path.isfile(cfg_path):
        raise FileNotFoundError(cfg_path)

    try:
        import importlib.util
        spec = importlib.util.spec_from_file_location("pipgui_fonts_config", cfg_path)
        cfg = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cfg)
        return cfg, cfg_path
    except Exception as e:
        raise RuntimeError(f"failed to load config.py: {e}")


def _require_font_entry(cfg, ttf_base: str) -> dict:
    if not hasattr(cfg, "FONTS") or not isinstance(cfg.FONTS, dict):
        raise RuntimeError("config.py must define FONTS dict")
    if ttf_base not in cfg.FONTS:
        raise KeyError(ttf_base)
    ent = cfg.FONTS[ttf_base]
    if not isinstance(ent, dict):
        raise RuntimeError(f"FONTS['{ttf_base}'] must be a dict")
    for k in ("size", "distanceRange", "glyphs"):
        if k not in ent:
            raise RuntimeError(f"FONTS['{ttf_base}'] missing '{k}'")
    if not isinstance(ent["glyphs"], list):
        raise RuntimeError(f"FONTS['{ttf_base}']['glyphs'] must be a list")
    return ent
def _build_charset_from_glyphs(glyphs: list) -> str:
    """Build charset.txt content from a list of ranges/singles."""
    lines = []
    for entry in glyphs:
        if isinstance(entry, list):
            if len(entry) == 2:
                lines.append(f"[{hex(entry[0])}, {hex(entry[1])}]")
            elif len(entry) > 0:
                chars = "".join(chr(c) for c in entry if isinstance(c, int) and 0 <= c <= 0x10FFFF)
                if chars:
                    lines.append(f'"{chars}"')
    return "\n".join(lines) + "\n"


def _hash_text(s: str) -> str:
    return hashlib.sha256(s.encode("utf-8")).hexdigest()


def _gen_atlas_decl_hpp(var_name: str) -> str:
    out = []
    out.append("#pragma once\n\n")
    out.append("#include <cstdint>\n\n")
    out.append(f"extern const uint8_t {var_name}[];\n")
    return "".join(out)


def _gen_atlas_def_cpp(hpp_filename: str, var_name: str, data: bytes, w: int | None = None, h: int | None = None) -> str:
    out = []
    out.append(f"#include \"{hpp_filename}\"\n\n")
    out.append(f"const uint8_t {var_name}[] = {{\n")

    for i in range(0, len(data), 16):
        chunk = data[i : i + 16]
        line = ", ".join(f"0x{b:02x}" for b in chunk)
        out.append(f"  {line},\n")

    out.append("};\n")
    return "".join(out)


def _gen_metrics_header(atlas: dict, font_ident: str, font_folder: str) -> str:
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
    out.append("\n    uint16_t advance;")
    out.append("\n    int8_t padLeft;")
    out.append("\n    int8_t padBottom;")
    out.append("\n    int8_t padRight;")
    out.append("\n    int8_t padTop;")
    out.append("\n    uint16_t atlasLeft;")
    out.append("\n    uint16_t atlasBottom;")
    out.append("\n    uint16_t atlasRight;")
    out.append("\n    uint16_t atlasTop;")
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
        adv_u16 = max(0, min(65535, int(it['adv'] * 256.0 + 0.5)))
        pl_i8 = max(-128, min(127, int(it['pl'] * 128.0 + (0.5 if it['pl'] >= 0 else -0.5))))
        pb_i8 = max(-128, min(127, int(it['pb'] * 128.0 + (0.5 if it['pb'] >= 0 else -0.5))))
        pr_i8 = max(-128, min(127, int(it['pr'] * 128.0 + (0.5 if it['pr'] >= 0 else -0.5))))
        pt_i8 = max(-128, min(127, int(it['pt'] * 128.0 + (0.5 if it['pt'] >= 0 else -0.5))))

        al_u16 = max(0, min(65535, int(it['al'] + 0.5)))
        ab_u16 = max(0, min(65535, int(it['ab'] + 0.5)))
        ar_u16 = max(0, min(65535, int(it['ar'] + 0.5)))
        at_u16 = max(0, min(65535, int(it['at'] + 0.5)))
        
        out.append(
            "\n    {"
            f"{it['code']}u, {adv_u16}, "
            f"{pl_i8}, {pb_i8}, {pr_i8}, {pt_i8}, "
            f"{al_u16}, {ab_u16}, {ar_u16}, {at_u16}"
            "},"
        )
    out.append("\n};\n")
    out.append("\n}\n}")
    out.append("\n\n#include <pipGUI/core/api/pipGUI.hpp>")
    out.append("\nstatic inline pipgui::FontId registerFont_" + font_folder + "(pipgui::GUI &gui)")
    out.append("{\n")
    out.append("    return gui.registerFont(")
    out.append(f"\n        ::{font_folder},")
    out.append(f"\n        pipgui::{ns_name}::AtlasWidth,")
    out.append(f"\n        pipgui::{ns_name}::AtlasHeight,")
    out.append(f"\n        pipgui::{ns_name}::DistanceRange,")
    out.append(f"\n        pipgui::{ns_name}::NominalSizePx,")
    out.append(f"\n        pipgui::{ns_name}::Ascender,")
    out.append(f"\n        pipgui::{ns_name}::Descender,")
    out.append(f"\n        pipgui::{ns_name}::LineHeight,")
    out.append(f"\n        pipgui::{ns_name}::Glyphs,")
    out.append(f"\n        pipgui::{ns_name}::GlyphCount);")
    out.append("\n}")
    out.append("\n")
    
    return "".join(out)


def _gen_fonts_metrics_hpp(font_names: list, font_idents: list) -> str:
    out = []
    out.append("#pragma once\n")
    out.append("#include <cstdint>\n\n")
    
    out.append("namespace pipgui\n{\n")
    out.append("namespace psdf_fonts\n{\n")
    out.append("enum FontId : uint16_t\n{\n")
    for i, name in enumerate(font_idents):
        comma = "," if i + 1 < len(font_idents) else ""
        out.append(f"    Font{name} = {i}{comma}\n")
    out.append("};\n")
    out.append("\nstatic constexpr uint16_t FontCount = ")
    out.append(f"{len(font_idents)};\n")
    out.append("}\n}\n\n")
    
    out.append("namespace pipgui\n{\n")
    out.append("using FontId = ::pipgui::psdf_fonts::FontId;\n")
    for name in font_idents:
        out.append(f"static constexpr FontId Font{name} = ::pipgui::psdf_fonts::Font{name};\n")
    out.append("}\n\n")
    
    for name in font_idents:
        out.append(f"constexpr pipgui::FontId {name} = ::pipgui::psdf_fonts::Font{name};\n")
    
    return "".join(out)


def find_msdf_atlas_gen(project_dir: str) -> str | None:
    msdf_exe = shutil.which("msdf-atlas-gen") or shutil.which("msdf-atlas-gen.exe")
    if not msdf_exe:
        local_exe = os.path.join(project_dir, "tools", "fonts", "bin", "msdf-atlas-gen.exe")
        if os.path.isfile(local_exe):
            msdf_exe = local_exe
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

    try:
        cfg, cfg_path = _load_config(project_dir)
    except Exception as e:
        print(f"[fonts] error: {e}")
        return False
    
    if not os.path.isdir(ttf_dir):
        print("[fonts] TTF dir not found:", ttf_dir)
        return False

    msdf_exe = find_msdf_atlas_gen(project_dir)
    if not msdf_exe:
        print("[fonts] msdf-atlas-gen.exe not found (PATH / tools/fonts / tools/msdf-atlas-gen)")
        return False

    generated = 0
    uptodate = 0

    ttf_files = [f for f in os.listdir(ttf_dir) if f.lower().endswith(".ttf")]
    ttf_files.sort()
    
    font_names = []
    font_idents = []

    for ttf_file in ttf_files:
        ttf_path = os.path.join(ttf_dir, ttf_file)
        if not os.path.isfile(ttf_path):
            continue

        font_base = os.path.splitext(os.path.basename(ttf_path))[0]
        font_folder = _snake_to_camel(font_base)
        if font_base.lower() == "wixmadefordisplay":
            font_folder = "WixMadeForDisplay"
        font_ident = _safe_ident(font_folder)
        
        font_names.append(font_folder)
        font_idents.append(font_ident)

        out_fonts_dir = os.path.join(project_dir, "lib", "pipKit", "pipGUI", "Fonts", font_folder)
        out_atlas_hpp = os.path.join(out_fonts_dir, f"{font_folder}.hpp")
        out_atlas_cpp = os.path.join(out_fonts_dir, f"{font_folder}.cpp")
        out_metrics_hpp = os.path.join(out_fonts_dir, "metrics.hpp")

        work_dir = os.path.join(tools_fonts_dir, "PSDF", font_folder)
        os.makedirs(work_dir, exist_ok=True)

        try:
            font_ent = _require_font_entry(cfg, font_base)
        except KeyError:
            print(f"[fonts] error: config.py missing FONTS['{font_base}']")
            return False
        except Exception as e:
            print(f"[fonts] error: {e}")
            return False

        font_size = font_ent["size"]
        pxrange = font_ent["distanceRange"]
        charset_content = _build_charset_from_glyphs(font_ent["glyphs"])
        if not charset_content.strip():
            print(f"[fonts] error: FONTS['{font_base}']['glyphs'] produced empty charset")
            return False

        charset_path = os.path.join(work_dir, "charset.txt")
        _write_if_changed(charset_path, charset_content)

        charset_preview = " | ".join([ln.strip() for ln in charset_content.splitlines() if ln.strip()][:3])
        print(f"[fonts] {font_folder}: {charset_preview}")

        json_path = os.path.join(work_dir, "atlas.json")
        bin_path = os.path.join(work_dir, "atlas.bin")
        stamp_path = os.path.join(work_dir, "stamp.txt")

        params = {
            "font": os.path.abspath(ttf_path),
            "type": "psdf",
            "size": str(font_size),
            "pxrange": str(pxrange),
            "potr": "1",
            "charset": os.path.abspath(charset_path),
        }

        stamp_in = "\n".join([
            "fonts_psdf_cfg_v3",
            _hash_file(ttf_path),
            _hash_text(charset_content),
            _hash_file(cfg_path),
            json.dumps(params, sort_keys=True),
        ])

        prev_stamp = None
        try:
            with open(stamp_path, "r", encoding="utf-8") as f:
                prev_stamp = f.read()
        except OSError:
            prev_stamp = None

        cache_ok = (
            prev_stamp == stamp_in
            and os.path.isfile(json_path)
            and os.path.isfile(bin_path)
            and os.path.isfile(out_atlas_hpp)
            and os.path.isfile(out_atlas_cpp)
            and os.path.isfile(out_metrics_hpp)
        )

        if cache_ok:
            uptodate += 1
            continue

        have_intermediate = os.path.isfile(json_path) and os.path.isfile(bin_path) and (prev_stamp == stamp_in)
        if not have_intermediate:
            cmd = [
                msdf_exe,
                "-font", os.path.abspath(ttf_path),
                "-type", "psdf",
                "-format", "bin",
                "-imageout", os.path.abspath(bin_path),
                "-json", os.path.abspath(json_path),
                "-size", str(font_size),
                "-pxrange", str(pxrange),
                "-charset", os.path.abspath(charset_path),
                "-fontname", font_folder,
            ]

            try:
                subprocess.check_call(cmd, cwd=work_dir)
            except subprocess.CalledProcessError as e:
                print("[fonts] msdf-atlas-gen failed:", e)
                return False

            with open(stamp_path, "w", encoding="utf-8") as f:
                f.write(stamp_in)

        atlas = _read_json(json_path)
        metrics_h = _gen_metrics_header(atlas, font_ident, font_folder)
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

        if not os.path.isfile(os.path.join(work_dir, "atlas.png")):
            _write_png_gray8(os.path.join(work_dir, "atlas.png"), w, h, data)

        hpp_filename = os.path.basename(out_atlas_hpp)
        _write_if_changed(out_atlas_hpp, _gen_atlas_decl_hpp(font_folder))
        _write_if_changed(out_atlas_cpp, _gen_atlas_def_cpp(hpp_filename, font_folder, data, w, h))
        generated += 1

    if font_idents:
        fonts_metrics_path = os.path.join(project_dir, "lib", "pipKit", "pipGUI", "Fonts", "metrics.hpp")
        os.makedirs(os.path.dirname(fonts_metrics_path), exist_ok=True)
        central_metrics = _gen_fonts_metrics_hpp(font_names, font_idents)
        _write_if_changed(fonts_metrics_path, central_metrics)

    if generated == 0 and uptodate > 0:
        print(f"[fonts] up-to-date ({uptodate})")
    else:
        print(f"[fonts] generated {generated}, up-to-date {uptodate}")

    return True


if __name__ == "__main__":
    try:
        Import("env")
        project_dir = env["PROJECT_DIR"]
    except Exception:
        project_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
    
    generate_all(project_dir)
