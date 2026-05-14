#!/usr/bin/env python3
"""
qmk-keymap-format.py

Format, lightly lint, and generate QMK keymap.c files using keyboard.json/info.json.

Expected QMK shape:

  keyboard_root/
    keyboard.json or info.json
    keymaps/default/keymap.c

Defaults are tuned for your Cosmotyl layout:
  - layout array order defines keymap argument order
  - text rows start when x wraps left
  - bottom/thumb rows align to a split grid
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


LAYOUT_CALL_RE = re.compile(r"(?P<name>LAYOUT(?:_[A-Za-z0-9_]+)?)\s*\(")
TOKEN_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*(?:\s*\(.*\))?$")

BUILTIN_PREFIXES = (
    "KC_",
    "QK_",
    "RGB_",
    "BL_",
    "AU_",
    "MU_",
    "DT_",
    "EE_",
    "MAGIC_",
    "SAFE_RANGE",
    "_______",
    "XXXXXXX",
)

BUILTIN_MACROS = {
    "MO",
    "TG",
    "TO",
    "TT",
    "DF",
    "OSL",
    "LT",
    "LM",
    "MT",
    "OSM",
    "LCTL",
    "LSFT",
    "LALT",
    "LGUI",
    "RCTL",
    "RSFT",
    "RALT",
    "RGUI",
    "C",
    "S",
    "A",
    "G",
    "HYPR",
    "MEH",
    "CTL_T",
    "SFT_T",
    "ALT_T",
    "GUI_T",
    "C_S_T",
    "ALL_T",
    "LCA",
    "LSA",
    "LAG",
    "SGUI",
    "LCAG",
    "RCAG",
    "SEND_STRING",
    "SS_TAP",
    "SS_DELAY",
}


@dataclass(frozen=True)
class LayoutKey:
    matrix: tuple[int, int]
    x: float
    y: float
    raw: dict


@dataclass(frozen=True)
class Finding:
    level: str
    message: str


class FormatError(Exception):
    pass


def find_info_file(root: Path) -> Path:
    for filename in ("keyboard.json", "info.json"):
        path = root / filename
        if path.is_file():
            return path
    raise FormatError(f"Could not find keyboard.json or info.json in {root}")


def load_layout(info_path: Path, layout_name: str) -> list[LayoutKey]:
    data = json.loads(info_path.read_text())
    try:
        raw_keys = data["layouts"][layout_name]["layout"]
    except KeyError as exc:
        raise FormatError(
            f"Could not find layouts.{layout_name}.layout in {info_path}"
        ) from exc

    keys: list[LayoutKey] = []
    for idx, raw in enumerate(raw_keys):
        try:
            row, col = raw["matrix"]
            keys.append(
                LayoutKey(
                    matrix=(int(row), int(col)),
                    x=float(raw["x"]),
                    y=float(raw["y"]),
                    raw=raw,
                )
            )
        except (KeyError, TypeError, ValueError) as exc:
            raise FormatError(f"Invalid layout key at index {idx}: {raw!r}") from exc
    return keys


def layout_order_rows(keys: list[LayoutKey], x_wrap_epsilon: float) -> list[list[int]]:
    if not keys:
        return []

    rows: list[list[int]] = [[0]]
    previous_x = keys[0].x

    for idx, key in enumerate(keys[1:], start=1):
        if key.x < previous_x - x_wrap_epsilon:
            rows.append([])
        rows[-1].append(idx)
        previous_x = key.x

    return rows


def visual_rows(keys: list[LayoutKey], y_epsilon: float) -> list[list[int]]:
    indexed = sorted(enumerate(keys), key=lambda item: (item[1].y, item[1].x))
    rows: list[list[tuple[int, LayoutKey]]] = []

    for idx, key in indexed:
        best_row = None
        best_dist = None
        for row_i, row in enumerate(rows):
            avg_y = sum(k.y for _i, k in row) / len(row)
            dist = abs(avg_y - key.y)
            if dist <= y_epsilon and (best_dist is None or dist < best_dist):
                best_row = row_i
                best_dist = dist

        if best_row is None:
            rows.append([(idx, key)])
        else:
            rows[best_row].append((idx, key))

    rows.sort(key=lambda row: sum(k.y for _i, k in row) / len(row))
    return [
        [idx for idx, _key in sorted(row, key=lambda item: item[1].x)] for row in rows
    ]


def build_rows(
    keys: list[LayoutKey], row_mode: str, y_epsilon: float, x_wrap_epsilon: float
) -> list[list[int]]:
    if row_mode == "layout-order":
        return layout_order_rows(keys, x_wrap_epsilon)
    if row_mode == "visual":
        return visual_rows(keys, y_epsilon)
    raise FormatError(f"Unknown row mode: {row_mode}")


def cosmotyl_grid_column(key: LayoutKey) -> int | None:
    row, col = key.matrix

    if row in (0, 1, 2, 3):
        return col + 1

    # Right main matrix, mirrored matrix columns, shifted right for a visual gutter.
    if row in (8, 9, 10, 11):
        return 16 - col

    # Fifth visual row.
    if row == 4 and col in (2, 3, 4):
        return col + 1
    if row == 12 and col in (2, 3, 4):
        return 16 - col

    # Curved thumbs.
    if row == 5 and col == 4:
        return 5
    if row == 13 and col == 4:
        return 12
    if row == 6 and col == 4:
        return 6
    if row == 14 and col == 4:
        return 11
    if row == 7 and col == 4:
        return 7
    if row == 15 and col == 4:
        return 10

    return None


def strip_c_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    text = re.sub(r"//.*", "", text)
    return text


def split_top_level_commas(text: str) -> list[str]:
    tokens: list[str] = []
    start = 0
    paren = bracket = brace = 0
    in_string = False
    in_char = False
    escape = False

    for i, ch in enumerate(text):
        if escape:
            escape = False
            continue
        if ch == "\\":
            escape = True
            continue
        if in_string:
            if ch == '"':
                in_string = False
            continue
        if in_char:
            if ch == "'":
                in_char = False
            continue
        if ch == '"':
            in_string = True
            continue
        if ch == "'":
            in_char = True
            continue

        if ch == "(":
            paren += 1
        elif ch == ")":
            paren -= 1
        elif ch == "[":
            bracket += 1
        elif ch == "]":
            bracket -= 1
        elif ch == "{":
            brace += 1
        elif ch == "}":
            brace -= 1
        elif ch == "," and paren == 0 and bracket == 0 and brace == 0:
            token = text[start:i].strip()
            if token:
                tokens.append(token)
            start = i + 1

        if paren < 0 or bracket < 0 or brace < 0:
            raise FormatError("Unbalanced punctuation inside LAYOUT(...) block")

    tail = text[start:].strip()
    if tail:
        tokens.append(tail)

    if paren or bracket or brace or in_string or in_char:
        raise FormatError("Unbalanced punctuation inside LAYOUT(...) block")

    return tokens


def find_matching_paren(text: str, open_index: int) -> int:
    depth = 0
    in_string = False
    in_char = False
    escape = False

    for i in range(open_index, len(text)):
        ch = text[i]
        if escape:
            escape = False
            continue
        if ch == "\\":
            escape = True
            continue
        if in_string:
            if ch == '"':
                in_string = False
            continue
        if in_char:
            if ch == "'":
                in_char = False
            continue
        if ch == '"':
            in_string = True
            continue
        if ch == "'":
            in_char = True
            continue
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                return i

    raise FormatError("Could not find closing ')' for LAYOUT(...) call")


def indentation_before(text: str, index: int) -> str:
    line_start = text.rfind("\n", 0, index) + 1
    return re.match(r"\s*", text[line_start:index]).group(0)


def token_base_name(token: str) -> str:
    match = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)", token.strip())
    return match.group(1) if match else token.strip()


def is_probably_builtin(token: str) -> bool:
    compact = " ".join(token.split())
    return (
        compact.startswith(BUILTIN_PREFIXES)
        or token_base_name(compact) in BUILTIN_MACROS
    )


def lint_tokens(
    tokens: list[str], expected_count: int, layout_call: str
) -> list[Finding]:
    findings: list[Finding] = []

    if len(tokens) != expected_count:
        findings.append(
            Finding(
                "error",
                f"{layout_call} has {len(tokens)} keycodes, but layout has {expected_count} keys",
            )
        )

    for token in tokens:
        compact = " ".join(token.split())
        if not TOKEN_RE.match(compact):
            findings.append(
                Finding("warning", f"Suspicious keycode expression: {compact!r}")
            )
        elif not is_probably_builtin(compact):
            findings.append(Finding("warning", f"Custom-looking keycode: {compact}"))

    return findings


def format_row(
    row: list[int],
    normalized: list[str],
    keys: list[LayoutKey],
    max_width: int,
    body_indent: str,
    visual_spacing: bool,
    thumb_grid: str,
) -> str:
    cell_width = max_width + 2

    def cell(layout_index: int) -> str:
        return normalized[layout_index].ljust(max_width)

    if visual_spacing and thumb_grid != "none":
        placed: list[tuple[int, str]] = []
        for layout_index in row:
            col = (
                cosmotyl_grid_column(keys[layout_index])
                if thumb_grid == "cosmotyl"
                else None
            )
            if col is None:
                col = len(placed) + 1
            placed.append((col, cell(layout_index)))

        line = body_indent
        current_col = 1
        first = True
        for col, text in sorted(placed, key=lambda item: item[0]):
            gap = max(0, col - current_col)
            if first:
                line += " " * (gap * cell_width)
                line += text
                first = False
            else:
                line += ", "
                line += " " * (gap * cell_width)
                line += text
            current_col = col + 1
        return line.rstrip()

    parts: list[str] = []
    previous_x: float | None = None
    for layout_index in row:
        key = keys[layout_index]
        if visual_spacing and previous_x is not None:
            gap_units = max(0, round(key.x - previous_x - 1))
            if gap_units:
                parts.append(" " * (gap_units * cell_width))
        parts.append(cell(layout_index))
        previous_x = key.x

    return body_indent + ", ".join(parts).rstrip()


def format_layout_call(
    layout_call: str,
    tokens: list[str],
    rows: list[list[int]],
    keys: list[LayoutKey],
    base_indent: str,
    indent_step: str,
    visual_spacing: bool,
    thumb_grid: str,
) -> str:
    normalized = [" ".join(token.split()) for token in tokens]
    max_width = max((len(token) for token in normalized), default=0)
    body_indent = base_indent + indent_step

    usable_rows = [[idx for idx in row if idx < len(normalized)] for row in rows]
    usable_rows = [row for row in usable_rows if row]

    lines = [f"{layout_call}("]
    for row_i, row in enumerate(usable_rows):
        line = format_row(
            row=row,
            normalized=normalized,
            keys=keys,
            max_width=max_width,
            body_indent=body_indent,
            visual_spacing=visual_spacing,
            thumb_grid=thumb_grid,
        )
        if row_i != len(usable_rows) - 1:
            line += ","
        lines.append(line)

    lines.append(base_indent + ")")
    return "\n".join(lines)


def rewrite_keymap(
    text: str,
    rows: list[list[int]],
    keys: list[LayoutKey],
    indent_step: str,
    visual_spacing: bool,
    thumb_grid: str,
) -> tuple[str, list[Finding]]:
    findings: list[Finding] = []
    output: list[str] = []
    pos = 0

    while True:
        match = LAYOUT_CALL_RE.search(text, pos)
        if not match:
            output.append(text[pos:])
            break

        call_name = match.group("name")
        open_paren = text.find("(", match.start())
        close_paren = find_matching_paren(text, open_paren)

        output.append(text[pos : match.start()])
        body = text[open_paren + 1 : close_paren]
        tokens = split_top_level_commas(strip_c_comments(body))
        findings.extend(lint_tokens(tokens, len(keys), call_name))

        output.append(
            format_layout_call(
                layout_call=call_name,
                tokens=tokens,
                rows=rows,
                keys=keys,
                base_indent=indentation_before(text, match.start()),
                indent_step=indent_step,
                visual_spacing=visual_spacing,
                thumb_grid=thumb_grid,
            )
        )

        pos = close_paren + 1

    return "".join(output), findings


def generate_blank_keymap(
    layout_name: str,
    rows: list[list[int]],
    keys: list[LayoutKey],
    indent_step: str,
    layer_name: str,
    fill: str,
    visual_spacing: bool,
    thumb_grid: str,
) -> str:
    layout_call = format_layout_call(
        layout_call=layout_name,
        tokens=[fill] * len(keys),
        rows=rows,
        keys=keys,
        base_indent="    ",
        indent_step=indent_step,
        visual_spacing=visual_spacing,
        thumb_grid=thumb_grid,
    )

    return f"""// Copyright 2026
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

enum layers {{
    {layer_name},
}};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {{
    [{layer_name}] = {layout_call},
}};
"""


def print_findings(path: Path, findings: Iterable[Finding]) -> int:
    exit_code = 0
    for finding in findings:
        stream = sys.stderr if finding.level == "error" else sys.stdout
        print(f"{path}: {finding.level}: {finding.message}", file=stream)
        if finding.level == "error":
            exit_code = 1
    return exit_code


def find_keymaps(root: Path, keymap_name: str | None) -> list[Path]:
    if keymap_name:
        path = root / "keymaps" / keymap_name / "keymap.c"
        if not path.is_file():
            raise FormatError(f"Could not find {path}")
        return [path]
    return sorted(root.glob("keymaps/*/keymap.c"))


def process_existing_file(
    keymap_path: Path,
    rows: list[list[int]],
    keys: list[LayoutKey],
    indent_step: str,
    visual_spacing: bool,
    thumb_grid: str,
    write: bool,
    check: bool,
    backup: bool,
) -> int:
    original = keymap_path.read_text()
    formatted, findings = rewrite_keymap(
        text=original,
        rows=rows,
        keys=keys,
        indent_step=indent_step,
        visual_spacing=visual_spacing,
        thumb_grid=thumb_grid,
    )

    exit_code = print_findings(keymap_path, findings)
    changed = formatted != original

    if check and changed:
        print(f"{keymap_path}: error: file is not formatted", file=sys.stderr)
        exit_code = 1

    if write:
        if changed:
            if backup:
                shutil.copy2(
                    keymap_path, keymap_path.with_suffix(keymap_path.suffix + ".bak")
                )
            keymap_path.write_text(formatted)
            print(f"formatted: {keymap_path}")
        else:
            print(f"unchanged: {keymap_path}")
    elif not check:
        sys.stdout.write(formatted)

    return exit_code


def generate_file(
    root: Path,
    output: Path | None,
    keymap_name: str,
    layout_name: str,
    rows: list[list[int]],
    keys: list[LayoutKey],
    indent_step: str,
    layer_name: str,
    fill: str,
    visual_spacing: bool,
    thumb_grid: str,
    force: bool,
) -> int:
    target = (
        output if output else root / "keymaps" / keymap_name / "keymap.c"
    ).resolve()
    if target.exists() and not force:
        print(
            f"error: {target} already exists; use --force to overwrite", file=sys.stderr
        )
        return 1

    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(
        generate_blank_keymap(
            layout_name=layout_name,
            rows=rows,
            keys=keys,
            indent_step=indent_step,
            layer_name=layer_name,
            fill=fill,
            visual_spacing=visual_spacing,
            thumb_grid=thumb_grid,
        )
    )
    print(f"generated: {target}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Format, lint, and generate QMK keymap.c files."
    )
    parser.add_argument(
        "keymap",
        nargs="?",
        type=Path,
        help="Optional path to keymap.c or keymap directory",
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=Path.cwd(),
        help="Keyboard root, default: current directory",
    )
    parser.add_argument(
        "--info",
        type=Path,
        help="Path to keyboard.json/info.json, default: found in root",
    )
    parser.add_argument(
        "--layout", default="LAYOUT", help="Layout name under layouts, default: LAYOUT"
    )
    parser.add_argument(
        "--keymap-name",
        default="default",
        help="Keymap name under keymaps/<name>, default: default",
    )
    parser.add_argument(
        "--all", action="store_true", help="Format all keymaps under keymaps/*/keymap.c"
    )
    parser.add_argument(
        "--write", action="store_true", help="Rewrite keymap.c in place"
    )
    parser.add_argument(
        "--check", action="store_true", help="Exit non-zero if formatting would change"
    )
    parser.add_argument(
        "--backup", action="store_true", help="Create keymap.c.bak before rewriting"
    )
    parser.add_argument(
        "--indent",
        default="    ",
        help="Indent inside LAYOUT blocks, default: 4 spaces",
    )
    parser.add_argument(
        "--row-mode", choices=("layout-order", "visual"), default="layout-order"
    )
    parser.add_argument("--x-wrap-epsilon", type=float, default=0.25)
    parser.add_argument("--y-epsilon", type=float, default=0.60)
    parser.add_argument("--no-visual-spacing", action="store_true")
    parser.add_argument(
        "--thumb-grid", choices=("none", "cosmotyl"), default="cosmotyl"
    )
    parser.add_argument(
        "--generate", action="store_true", help="Generate blank starter keymap.c"
    )
    parser.add_argument("--output", type=Path, help="Output path for --generate")
    parser.add_argument("--layer-name", default="_BASE")
    parser.add_argument("--fill", default="KC_NO")
    parser.add_argument("--force", action="store_true")
    args = parser.parse_args()

    root = args.root.resolve()
    visual_spacing = not args.no_visual_spacing

    try:
        info_path = args.info.resolve() if args.info else find_info_file(root)
        keys = load_layout(info_path, args.layout)
        rows = build_rows(keys, args.row_mode, args.y_epsilon, args.x_wrap_epsilon)

        if args.generate:
            return generate_file(
                root=root,
                output=args.output,
                keymap_name=args.keymap_name,
                layout_name=args.layout,
                rows=rows,
                keys=keys,
                indent_step=args.indent,
                layer_name=args.layer_name,
                fill=args.fill,
                visual_spacing=visual_spacing,
                thumb_grid=args.thumb_grid,
                force=args.force,
            )

        if args.keymap:
            keymap_path = args.keymap.resolve()
            if keymap_path.is_dir():
                keymap_path = keymap_path / "keymap.c"
            keymaps = [keymap_path]
        elif args.all:
            keymaps = find_keymaps(root, None)
            if not keymaps:
                raise FormatError(f"No keymaps found under {root / 'keymaps'}")
        else:
            keymaps = find_keymaps(root, args.keymap_name)

        if len(keymaps) > 1 and not args.write and not args.check:
            raise FormatError(
                "Refusing to print multiple keymaps to stdout; use --write or --check"
            )

        exit_code = 0
        for keymap_path in keymaps:
            if not keymap_path.is_file():
                raise FormatError(f"Could not find {keymap_path}")
            exit_code |= process_existing_file(
                keymap_path=keymap_path,
                rows=rows,
                keys=keys,
                indent_step=args.indent,
                visual_spacing=visual_spacing,
                thumb_grid=args.thumb_grid,
                write=args.write,
                check=args.check,
                backup=args.backup,
            )
        return exit_code

    except (OSError, json.JSONDecodeError, FormatError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
