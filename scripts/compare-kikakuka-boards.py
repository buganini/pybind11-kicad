#!/usr/bin/env python3
"""Compare Kikakuka-generated KiCad boards through the native KiCad model."""

from __future__ import annotations

import argparse
import math
import os
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any, Iterable

import pybind11_kicad as kk
from shapely.geometry import LineString, MultiLineString
from shapely.ops import unary_union


QUANTUM_IU = int(os.environ.get("PYBIND11_KICAD_KIKAKUKA_COMPARE_QUANTUM_IU", "100"))
LINE_TOLERANCE_IU = float(os.environ.get("PYBIND11_KICAD_KIKAKUKA_COMPARE_LINE_TOLERANCE_IU", "1000"))
LENGTH_TOLERANCE_IU = float(os.environ.get("PYBIND11_KICAD_KIKAKUKA_COMPARE_LENGTH_TOLERANCE_IU", "1000"))
REPORT_LIMIT = 8
MOUSEBITE_REFERENCE = re.compile(r"^KiKit_MB_\d+_\d+$")


def q(value: int | float) -> int:
    return int(round(int(value) / QUANTUM_IU)) * QUANTUM_IU


def point(point: Any) -> tuple[int, int]:
    return (q(point.x), q(point.y))


def mm_to_iu(value: float) -> int:
    return q(round(value * 1_000_000))


def mm_point(point: Any) -> tuple[int, int]:
    return (mm_to_iu(point.x_mm), mm_to_iu(point.y_mm))


def normalized_ring(points: Iterable[Any]) -> tuple[tuple[int, int], ...]:
    ring = [point(item) for item in points]

    if len(ring) > 1 and ring[0] == ring[-1]:
        ring = ring[:-1]

    if not ring:
        return tuple()

    rotations: list[tuple[tuple[int, int], ...]] = []

    for sequence in (ring, list(reversed(ring))):
        rotations.extend(tuple(sequence[index:] + sequence[:index]) for index in range(len(sequence)))

    return min(rotations)


def drawing_key(drawing: Any) -> tuple[Any, ...]:
    if drawing.shape == 3:
        return ("circle", drawing.layer, q(drawing.width), q(drawing.radius), drawing.filled, point(drawing.center))

    if drawing.shape == 2:
        return (
            "arc",
            drawing.layer,
            q(drawing.width),
            drawing.filled,
            point(drawing.start),
            point(drawing.mid),
            point(drawing.end),
        )

    if drawing.shape == 4:
        return (
            "polygon",
            drawing.layer,
            q(drawing.width),
            drawing.filled,
            normalized_ring(getattr(drawing, "polygon_points", [])),
        )

    return (
        "drawing",
        drawing.shape,
        drawing.layer,
        q(drawing.width),
        q(drawing.radius),
        drawing.filled,
        point(drawing.start),
        point(drawing.end),
        point(drawing.center),
        point(drawing.mid),
        normalized_ring(getattr(drawing, "polygon_points", [])),
    )


def non_line_drawings(board: kk.Board) -> Counter[tuple[Any, ...]]:
    return Counter(drawing_key(drawing) for drawing in board.drawings() if drawing.shape != 0)


def line_groups(board: kk.Board) -> dict[tuple[Any, ...], list[tuple[tuple[int, int], tuple[int, int]]]]:
    groups: dict[tuple[Any, ...], list[tuple[tuple[int, int], tuple[int, int]]]] = defaultdict(list)

    for drawing in board.drawings():
        if drawing.shape == 0:
            groups[(drawing.layer, q(drawing.width), drawing.filled)].append(
                ((drawing.start.x, drawing.start.y), (drawing.end.x, drawing.end.y))
            )

    return groups


def is_generated_mousebite(footprint: Any) -> bool:
    return (
        footprint.fpid == "NPTH"
        and footprint.value == "NPTH"
        and bool(MOUSEBITE_REFERENCE.match(footprint.reference))
    )


def field_key(field: Any, normalize_mousebite_reference: bool = False) -> tuple[Any, ...]:
    value = field.value
    if normalize_mousebite_reference and field.name == "Reference" and MOUSEBITE_REFERENCE.match(value):
        value = "KiKit_MB"

    if not field.visible:
        return ("hidden-field", field.name, value, field.layer)

    return (
        "visible-field",
        field.name,
        value,
        field.layer,
        point(field.position),
        point(field.size),
        q(field.thickness),
        round(field.angle_degrees, 3),
        field.h_justify,
        field.v_justify,
        field.mirrored,
        field.keep_upright,
    )


def pad_key(pad: Any) -> tuple[Any, ...]:
    return (
        pad.name,
        pad.net,
        pad.attribute,
        mm_point(pad.position),
        mm_point(pad.size),
        tuple(mm_to_iu(value) for value in pad.drill_size),
        pad.shape,
        pad.drill_shape,
        tuple(pad.layers),
        pad.has_local_solder_mask_margin,
        q(pad.local_solder_mask_margin),
        pad.has_local_clearance,
        q(pad.local_clearance),
    )


def footprint_key(footprint: Any) -> tuple[Any, ...]:
    generated_mousebite = is_generated_mousebite(footprint)
    reference = "KiKit_MB" if generated_mousebite else footprint.reference

    return (
        reference,
        footprint.value,
        footprint.fpid,
        mm_point(footprint.position),
        footprint.layer,
        round(footprint.orientation_degrees, 3),
        footprint.excluded_from_pos,
        footprint.excluded_from_bom,
        footprint.board_only,
        footprint.dnp,
        tuple(sorted(field_key(field, generated_mousebite) for field in footprint.fields())),
        tuple(sorted(pad_key(pad) for pad in footprint.pads())),
        tuple(sorted(drawing_key(drawing) for drawing in footprint.drawings())),
    )


def footprint_counter(board: kk.Board) -> Counter[tuple[Any, ...]]:
    return Counter(footprint_key(footprint) for footprint in board.footprints())


def mousebite_reference_key(footprint: Any) -> tuple[Any, ...] | None:
    if not is_generated_mousebite(footprint):
        return None

    return (
        footprint.reference,
        footprint.value,
        tuple(
            sorted(
                field_key(field)
                for field in footprint.fields()
                if field.name in {"Reference", "Value"}
            )
        ),
    )


def mousebite_reference_counter(board: kk.Board) -> Counter[tuple[Any, ...]]:
    result: Counter[tuple[Any, ...]] = Counter()

    for footprint in board.footprints():
        key = mousebite_reference_key(footprint)
        if key is not None:
            result[key] += 1

    return result


def track_key(track: Any) -> tuple[Any, ...]:
    return (
        track.net,
        track.layer,
        track.is_arc,
        point(track.start),
        point(track.mid),
        point(track.end),
        q(track.width),
    )


def via_key(via: Any) -> tuple[Any, ...]:
    return (
        via.net,
        via.via_type,
        point(via.position),
        q(via.drill),
        q(via.diameter),
        tuple(via.layers),
    )


def polygon_key(polygon: Any) -> tuple[Any, ...]:
    return (
        normalized_ring(polygon.outline),
        tuple(sorted(normalized_ring(hole) for hole in polygon.holes)),
    )


def zone_key(zone: Any) -> tuple[Any, ...]:
    fills = (
        (
            fill.layer,
            tuple(sorted(polygon_key(polygon) for polygon in fill.polygons)),
        )
        for fill in getattr(zone, "fills", [])
    )

    return (
        zone.net,
        tuple(zone.layers),
        zone.priority,
        zone.name,
        zone.fill_mode,
        zone.is_rule_area,
        getattr(zone, "is_filled", False),
        tuple(sorted(polygon_key(polygon) for polygon in zone.polygons)),
        tuple(sorted(fills)),
    )


def counter_report(label: str, golden: Counter[tuple[Any, ...]], local: Counter[tuple[Any, ...]]) -> list[str]:
    missing = golden - local
    extra = local - golden

    if not missing and not extra:
        return []

    lines = [f"{label} differ: missing={sum(missing.values())} extra={sum(extra.values())}"]

    for prefix, values in (("missing", missing), ("extra", extra)):
        for index, (item, count) in enumerate(values.items()):
            if index >= REPORT_LIMIT:
                lines.append(f"  {prefix}: ...")
                break
            lines.append(f"  {prefix} x{count}: {item!r}")

    return lines


def compare_lines(golden: kk.Board, local: kk.Board) -> list[str]:
    golden_groups = line_groups(golden)
    local_groups = line_groups(local)
    lines: list[str] = []

    for key in sorted(set(golden_groups) | set(local_groups)):
        golden_geometry = (
            unary_union([LineString(segment) for segment in golden_groups.get(key, [])])
            if golden_groups.get(key)
            else MultiLineString([])
        )
        local_geometry = (
            unary_union([LineString(segment) for segment in local_groups.get(key, [])])
            if local_groups.get(key)
            else MultiLineString([])
        )

        if golden_geometry.is_empty and local_geometry.is_empty:
            continue

        if golden_geometry.is_empty or local_geometry.is_empty:
            lines.append(f"lines differ for {key!r}: one side is empty")
            continue

        hausdorff = golden_geometry.hausdorff_distance(local_geometry)
        length_delta = math.fabs(golden_geometry.length - local_geometry.length)

        if hausdorff > LINE_TOLERANCE_IU or length_delta > LENGTH_TOLERANCE_IU:
            lines.append(
                f"lines differ for {key!r}: hausdorff={hausdorff:.3f}iu "
                f"length_delta={length_delta:.3f}iu "
                f"counts={len(golden_groups.get(key, []))}/{len(local_groups.get(key, []))}"
            )

    return lines


def compare_boards(golden_path: Path, local_path: Path) -> list[str]:
    golden = kk.Board.open(golden_path)
    local = kk.Board.open(local_path)
    lines: list[str] = []

    lines.extend(counter_report("board non-line drawings", non_line_drawings(golden), non_line_drawings(local)))
    lines.extend(compare_lines(golden, local))
    lines.extend(counter_report("footprints", footprint_counter(golden), footprint_counter(local)))
    lines.extend(counter_report("mousebite references", mousebite_reference_counter(golden), mousebite_reference_counter(local)))
    lines.extend(counter_report("tracks", Counter(track_key(track) for track in golden.tracks()), Counter(track_key(track) for track in local.tracks())))
    lines.extend(counter_report("vias", Counter(via_key(via) for via in golden.vias()), Counter(via_key(via) for via in local.vias())))
    lines.extend(counter_report("zones", Counter(zone_key(zone) for zone in golden.zones()), Counter(zone_key(zone) for zone in local.zones())))

    return lines


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("golden", type=Path)
    parser.add_argument("local", type=Path)
    args = parser.parse_args()

    differences = compare_boards(args.golden, args.local)

    if differences:
        print("\n".join(differences))
        return 1

    print("native board content matches")
    return 0


if __name__ == "__main__":
    sys.exit(main())
