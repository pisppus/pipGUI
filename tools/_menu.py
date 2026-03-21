from __future__ import annotations

from pathlib import Path
from typing import Callable, Iterable, Sequence, TypeVar


T = TypeVar("T")


def _print_header(title: str) -> None:
    print()
    print(title)


def choose(
    title: str,
    options: Sequence[T],
    label: Callable[[T], str] | None = None,
    *,
    default_index: int | None = 0,
) -> T:
    if not options:
        raise SystemExit(f"{title}: no options available")

    render = label or (lambda item: str(item))
    _print_header(title)
    for idx, item in enumerate(options, start=1):
        suffix = " [default]" if default_index is not None and idx - 1 == default_index else ""
        print(f"{idx}. {render(item)}{suffix}")

    while True:
        raw = input("> ").strip()
        if not raw:
            if default_index is not None:
                return options[default_index]
            print("Enter a number.")
            continue
        if raw.isdigit():
            selected = int(raw)
            if 1 <= selected <= len(options):
                return options[selected - 1]
        print(f"Enter a number from 1 to {len(options)}.")


def prompt_text(
    title: str,
    *,
    default: str | None = None,
    allow_empty: bool = False,
    validator: Callable[[str], str | None] | None = None,
) -> str:
    hint = f" [{default}]" if default else ""
    while True:
        raw = input(f"{title}{hint}: ").strip()
        if not raw and default is not None:
            raw = default
        if not raw and not allow_empty:
            print("Value is required.")
            continue
        if validator is not None:
            problem = validator(raw)
            if problem:
                print(problem)
                continue
        return raw


def prompt_multiline(title: str, *, default: str = "") -> str:
    print()
    print(f"{title} (finish with single '.' line)")
    if default:
        print(f"Current default: {default}")
    lines: list[str] = []
    while True:
        line = input()
        if line == ".":
            break
        lines.append(line)
    value = "\n".join(lines).strip()
    return value if value else default


def choose_existing_path(
    title: str,
    paths: Iterable[Path],
    *,
    default_index: int | None = 0,
    custom_label: str = "Manual path",
) -> Path:
    items = [Path(p) for p in paths]
    menu_items: list[tuple[str, Path | None]] = [(str(path), path) for path in items]
    menu_items.append((custom_label, None))
    selected = choose(title, menu_items, lambda item: item[0], default_index=default_index)
    if selected[1] is not None:
        return selected[1]
    while True:
        raw = input("Path: ").strip().strip('"')
        if not raw:
            print("Path is required.")
            continue
        path = Path(raw)
        if path.exists():
            return path
        print("Path does not exist.")
