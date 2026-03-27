import os
import re
import shutil
import sys
import xml.etree.ElementTree as ET

_THIS_DIR = os.path.dirname(os.path.abspath(__file__))
_TOOLS_DIR = os.path.dirname(os.path.dirname(_THIS_DIR))
if _TOOLS_DIR not in sys.path:
    sys.path.insert(0, _TOOLS_DIR)

from psdf import _fmt_f


def source_files(root_dir: str, suffixes: tuple[str, ...]) -> list[str]:
    if not os.path.isdir(root_dir):
        return []

    suffixes = tuple(suffix.lower() for suffix in suffixes)
    out: list[str] = []
    for current_root, dirnames, filenames in os.walk(root_dir):
        dirnames[:] = sorted(name for name in dirnames if name != "__pycache__")
        for filename in sorted(filenames):
            if not filename.lower().endswith(suffixes):
                continue
            full_path = os.path.join(current_root, filename)
            rel_path = os.path.relpath(full_path, root_dir).replace("\\", "/")
            out.append(rel_path)
    out.sort()
    return out


def source_stem(rel_path: str) -> str:
    normalized = rel_path.replace("\\", "/").strip("/")
    stem = os.path.splitext(normalized)[0]
    parts = [part for part in stem.split("/") if part and part not in (".", "..")]
    return "/".join(parts)


def source_key(rel_path: str) -> str:
    stem = source_stem(rel_path)
    if not stem:
        return "icon"

    parts: list[str] = []
    for part in stem.split("/"):
        cleaned = re.sub(r"[^A-Za-z0-9._-]+", "_", part).strip("._-")
        parts.append(cleaned or "item")
    return "__".join(parts)


def svg_viewbox(svg_path: str):
    try:
        with open(svg_path, "r", encoding="utf-8") as handle:
            text = handle.read(256 * 1024)
    except OSError:
        return None

    match = re.search(r"\bviewBox\s*=\s*\"([^\"]+)\"", text, flags=re.IGNORECASE)
    if not match:
        match = re.search(r"\bviewBox\s*=\s*'([^']+)'", text, flags=re.IGNORECASE)
    if not match:
        return None

    parts = re.split(r"[\s,]+", match.group(1).strip())
    if len(parts) != 4:
        return None
    try:
        vx, vy, vw, vh = (float(parts[0]), float(parts[1]), float(parts[2]), float(parts[3]))
    except Exception:
        return None
    if vw <= 0.0 or vh <= 0.0:
        return None
    return vx, vy, vw, vh


def msdf_transform_from_viewbox(viewbox, out_px: int):
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


def _poly_area(points):
    if not points:
        return 0.0
    area = 0.0
    count = len(points)
    for index in range(count):
        p0 = points[index]
        p1 = points[(index + 1) % count]
        area += (p0.real * p1.imag) - (p1.real * p0.imag)
    return 0.5 * area


def _cross(a: complex, b: complex) -> float:
    return (a.real * b.imag) - (a.imag * b.real)


def _dot(a: complex, b: complex) -> float:
    return (a.real * b.real) + (a.imag * b.imag)


def _is_straight_segment(segment) -> bool:
    start = getattr(segment, "start", None)
    end = getattr(segment, "end", None)
    if start is None or end is None:
        return False

    delta = end - start
    length_sq = _dot(delta, delta)
    if length_sq <= 1e-18:
        return False

    if type(segment).__name__ == "Line":
        return True

    controls = []
    if hasattr(segment, "control1"):
        controls.append(segment.control1)
    if hasattr(segment, "control2"):
        controls.append(segment.control2)
    if hasattr(segment, "control"):
        controls.append(segment.control)
    if not controls:
        return False

    eps_cross = max(1e-9, abs(delta) * 1e-7)
    eps_proj = 1e-7
    last_proj = -eps_proj
    for point in controls:
        rel = point - start
        if abs(_cross(delta, rel)) > eps_cross:
            return False
        proj = _dot(rel, delta) / length_sq
        if proj < -eps_proj or proj > 1.0 + eps_proj or proj + eps_proj < last_proj:
            return False
        last_proj = proj
    return True


def _offset_line_segment(segment, offset_distance: float):
    delta = segment.end - segment.start
    length = abs(delta)
    if length <= 1e-12:
        return None
    normal = complex(-delta.imag / length, delta.real / length)
    offset = offset_distance * normal
    return [segment.start + offset, segment.end + offset]


def _append_segment_points(points: list[complex], segment_points: list[complex]) -> None:
    if not segment_points:
        return
    if points and abs(points[-1] - segment_points[0]) <= 1e-12:
        points.extend(segment_points[1:])
        return
    points.extend(segment_points)


def _simplify_collinear(points: list[complex]) -> list[complex]:
    if len(points) < 3:
        return points

    simplified: list[complex] = [points[0]]
    for point in points[1:]:
        simplified.append(point)
        while len(simplified) >= 3:
            a = simplified[-3]
            b = simplified[-2]
            c = simplified[-1]
            ab = b - a
            bc = c - b
            if abs(ab) <= 1e-12 or abs(bc) <= 1e-12:
                simplified.pop(-2)
                continue
            eps = max(1e-12, max(abs(ab), abs(bc)) * 1e-9)
            if abs(_cross(ab, bc)) > eps or _dot(ab, bc) < 0.0:
                break
            simplified.pop(-2)

    if len(simplified) >= 3:
        while True:
            a = simplified[-2]
            b = simplified[-1]
            c = simplified[0]
            ab = b - a
            bc = c - b
            if abs(ab) <= 1e-12 or abs(bc) <= 1e-12:
                simplified.pop()
                if len(simplified) < 3:
                    break
                continue
            eps = max(1e-12, max(abs(ab), abs(bc)) * 1e-9)
            if abs(_cross(ab, bc)) > eps or _dot(ab, bc) < 0.0:
                break
            simplified.pop()
            if len(simplified) < 3:
                break

    return simplified


def _offset_curve_points(path, offset_distance: float, steps: int):
    points: list[complex] = []
    for segment in path:
        if _is_straight_segment(segment):
            line_points = _offset_line_segment(segment, offset_distance)
            if not line_points:
                continue
            _append_segment_points(points, line_points)
            continue

        segment_points: list[complex] = []
        for step in range(steps):
            t = step / steps
            try:
                normal = segment.normal(t)
                point = segment.point(t)
            except Exception:
                return None
            segment_points.append(point + offset_distance * normal)
        _append_segment_points(points, segment_points)
    return _simplify_collinear(points)


def _points_to_d(points):
    if not points:
        return ""
    commands = [f"M {_fmt_f(points[0].real)} {_fmt_f(points[0].imag)}"]
    for point in points[1:]:
        commands.append(f"L {_fmt_f(point.real)} {_fmt_f(point.imag)}")
    commands.append("Z")
    return " ".join(commands)


def _stroke_outline_d(path, stroke_width, *, step_env: str, default_steps: int, min_steps: int, max_steps: int):
    if not getattr(path, "isclosed", lambda: False)():
        return None

    half = float(stroke_width) * 0.5
    if half <= 0.0:
        return None

    steps = int(os.environ.get(step_env, str(default_steps)))
    if steps < min_steps:
        steps = min_steps
    if steps > max_steps:
        steps = max_steps

    outer = _offset_curve_points(path, +half, steps)
    inner = _offset_curve_points(path, -half, steps)
    if not outer or not inner:
        return None

    area_outer = _poly_area(outer)
    area_inner = _poly_area(inner)
    if abs(area_inner) > abs(area_outer):
        outer, inner = inner, outer
        area_outer, area_inner = area_inner, area_outer

    if (area_outer >= 0.0) == (area_inner >= 0.0):
        inner = list(reversed(inner))

    if outer[0] != outer[-1]:
        outer.append(outer[0])
    if inner[0] != inner[-1]:
        inner.append(inner[0])

    return f"{_points_to_d(outer)} {_points_to_d(inner)}"


def _apply_svg_transform(path_obj, transform_text: str):
    if not transform_text:
        return path_obj

    ops = re.findall(r"([a-zA-Z]+)\s*\(([^)]*)\)", transform_text)
    if not ops:
        return path_obj

    result = path_obj
    for name, params_text in reversed(ops):
        values = [float(value) for value in re.split(r"[,\s]+", params_text.strip()) if value]
        op = name.lower()
        if op == "translate":
            tx = values[0] if values else 0.0
            ty = values[1] if len(values) > 1 else 0.0
            result = result.translated(complex(tx, ty))
        elif op == "scale":
            sx = values[0] if values else 1.0
            sy = values[1] if len(values) > 1 else sx
            result = result.scaled(sx, sy)
        elif op == "rotate":
            angle = values[0] if values else 0.0
            if len(values) >= 3:
                result = result.rotated(angle, origin=complex(values[1], values[2]))
            else:
                result = result.rotated(angle)
    return result


def stroke_to_path_svg(
    in_svg_path: str,
    out_svg_path: str,
    *,
    step_env: str,
    default_steps: int,
    min_steps: int,
    max_steps: int,
    warn_prefix: str,
    apply_path_transform: bool,
    copy_if_empty: bool,
) -> bool:
    try:
        from svgpathtools import svg2paths
    except ImportError:
        return False

    try:
        tree = ET.parse(in_svg_path)
        root = tree.getroot()

        viewbox = root.get("viewBox", f"0 0 {root.get('width', '100')} {root.get('height', '100')}")
        width = root.get("width", "100")
        height = root.get("height", "100")

        paths, attributes = svg2paths(in_svg_path)
        d_list: list[str] = []

        for path, attr in zip(paths, attributes):
            if apply_path_transform:
                path = _apply_svg_transform(path, attr.get("transform", "").strip())

            stroke = attr.get("stroke", "").lower()
            if stroke and stroke != "none":
                stroke_width = float(attr.get("stroke-width", "1").replace("px", "").replace("pt", ""))
                d = _stroke_outline_d(
                    path,
                    stroke_width,
                    step_env=step_env,
                    default_steps=default_steps,
                    min_steps=min_steps,
                    max_steps=max_steps,
                )
                if d:
                    d_list.append(d)
                continue

            fill = attr.get("fill", "").lower()
            if fill and fill != "none":
                d_list.append(path.d())

        if not d_list:
            if copy_if_empty:
                shutil.copy(in_svg_path, out_svg_path)
                return True
            return False

        svg_ns = "http://www.w3.org/2000/svg"
        ET.register_namespace("", svg_ns)
        out_root = ET.Element("{http://www.w3.org/2000/svg}svg")
        out_root.set("width", str(width))
        out_root.set("height", str(height))
        out_root.set("viewBox", str(viewbox))

        for d in d_list:
            element = ET.SubElement(out_root, "{http://www.w3.org/2000/svg}path")
            element.set("d", d)
            element.set("fill", "black")
            element.set("stroke", "none")

        ET.ElementTree(out_root).write(out_svg_path, encoding="utf-8", xml_declaration=False)
        return True
    except Exception as exc:
        print(f"{warn_prefix}: {exc}")
        return False
