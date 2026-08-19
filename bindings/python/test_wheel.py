#!/usr/bin/env python3
"""Smoke test run against an installed pytriskel wheel.

cibuildwheel installs the freshly built wheel into an isolated test
environment before running this script.
"""

from __future__ import annotations

import math
import sys
import tempfile
from pathlib import Path

# Drop the script's own directory from sys.path: it contains the source
# pytriskel/ package (just an __init__.py), which would otherwise shadow the
# installed wheel under test.
_here = Path(__file__).resolve().parent
sys.path = [p for p in sys.path if Path(p or ".").resolve() != _here]

import pytriskel
from pytriskel.pytriskel import EdgeType, make_layout_builder, make_png_renderer, make_svg_renderer


def check_package_files() -> None:
    package_dir = Path(pytriskel.__file__).parent
    for name in ("py.typed", "pytriskel.pyi"):
        path = package_dir / name
        assert path.is_file(), f"missing {path}"
    stub = (package_dir / "pytriskel.pyi").read_text(encoding="utf-8")
    for symbol in ("class CFGLayout", "class LayoutBuilder", "def make_layout_builder"):
        assert symbol in stub, f"stub is missing {symbol!r}"


def check_layout() -> None:
    builder = make_layout_builder()

    n1 = builder.make_node("Hello")
    n2 = builder.make_node("World")
    n3 = builder.make_node("!")
    e1 = builder.make_edge(n1, n2, EdgeType.T)
    builder.make_edge(n1, n3, EdgeType.F)
    builder.make_edge(n2, n3)

    png_renderer = make_png_renderer()
    svg_renderer = make_svg_renderer()
    builder.measure_nodes(png_renderer)

    layout = builder.build()

    assert layout.get_width() > 0
    assert layout.get_height() > 0

    coords = layout.get_coords(n1)
    assert math.isfinite(coords.x) and math.isfinite(coords.y)
    assert len(layout.get_waypoints(e1)) >= 2

    with tempfile.TemporaryDirectory() as tmp:
        for renderer, suffix in ((png_renderer, "png"), (svg_renderer, "svg")):
            out = Path(tmp) / f"out.{suffix}"
            layout.save(renderer, str(out))
            assert out.is_file() and out.stat().st_size > 0, f"empty {out}"


def main() -> int:
    print(f"pytriskel module: {pytriskel.__file__}")
    check_package_files()
    check_layout()
    print("OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
