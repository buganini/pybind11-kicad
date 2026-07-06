# Porting Notes

These are maintainer-facing notes for porting KiCad SWIG behavior into
pybind11-kicad. The goal is observable compatibility with KiCad 10 `pcbnew`,
not a new geometry model.

## Coordinate System

KiCad board geometry exposed through `pcbnew` behaves as x-right/y-down
coordinates. Positive rotations therefore look clockwise on screen, and the
compat layer must not use the usual y-up mathematical rotation formula.

The Python shim currently follows this rotation:

```text
x' = cx + x * cos(angle) + y * sin(angle)
y' = cy - x * sin(angle) + y * cos(angle)
```

where `x` and `y` are offsets from the rotation center. A useful sanity check:
rotating `(10, 0)` by `+90 deg` around `(0, 0)` produces `(0, -10)`.

Native KiCad angles passed through `EDA_ANGLE(..., DEGREES_T)` already use
KiCad semantics. Do not negate angles at the Python/native boundary unless a
SWIG comparison proves that a specific API does so.

## Angles And Orientation

The SWIG-compatible angle constants are:

```text
TENTHS_OF_A_DEGREE_T = 0
DEGREES_T = 1
RADIANS_T = 2
```

`EDA_ANGLE.AsDegrees()` returns degrees. `int(EDA_ANGLE(...))` follows the SWIG
integer representation in tenths of a degree.

Footprint and text orientation are stored as KiCad angles, but many Python API
calls pass or return degrees for compatibility with historical scripts. Keep
the unit conversion at the API edge and keep native specs explicit about their
unit.

`FOOTPRINT.SetOrientation()` is a delta operation for children: it changes the
footprint orientation and rotates pads, drawings, and fields around the
footprint position by the orientation delta. `FOOTPRINT.Rotate()` also rotates
the footprint position around the provided center, then applies the angle to
children.

`PCB_TEXT.Rotate()` moves the text anchor and adds the angle to the text angle.
Mirroring is a separate text flag. Do not infer mirroring from the layer unless
the corresponding KiCad/SWIG API does that.

## Arcs

SWIG exposes track arcs as `PCB_ARC`. The compatibility shim currently carries
arc track data through `PCB_TRACK` helper plumbing (`is_arc`, `mid`, and
`center`) while the public class surface is still being aligned.

For drawing arcs, `PCB_SHAPE.SetArcGeometry(start, mid, end)` is the important
compatibility point. It computes the center from the three points and preserves
start/mid/end behavior expected by KiCad scripts.

The helper functions `_point_on_circle()` and `_angle_degrees()` look like
ordinary mathematical helpers, but their current behavior is covered by SWIG
compatibility tests. Do not invert the y-axis or swap sweep direction there
without comparing against the KiCad 10 SWIG wrapper.

Full-circle handling is intentional: equal start and end angles represent a
360-degree circle, not a zero-length arc.

Before touching arc behavior, run the local value-type tests and the extracted
KiCad 10 SWIG track tests. The regression that showed up in practice was
mouse-bite output: geometry can look close while text, hole, or arc metadata is
wrong.

## Units

There are several unit layers:

* `pcbnew` object methods generally use KiCad internal units.
* SWIG helpers such as `FromMM()` and `ToMM()` convert millimeters.
* The cleaner pybind11-kicad API may expose explicit millimeter convenience
  fields for selected operations.
* Native backend specs should state whether a field is internal units or a
  convenience unit.

Do not change a field's unit without changing its name, type, or conversion
boundary. Silent unit changes are especially hard to debug in panelization code.

## Metadata And Ownership

SWIG exposes pointer-ownership details such as `thisown`; pybind11-kicad should
match the observable script behavior instead of copying that implementation
detail. For example, after `BOARD.Add(item)`, the board owns the item for
practical scripting purposes.

Preserve UUIDs, attributes, net names, layers, text flags, and other metadata
when native KiCad data is round-tripped. Test comparison should tolerate
ordering differences where KiCad serialization is not stable, but it should not
intentionally ignore meaningful attributes.

## Native Boundary

Prefer KiCad's native data when reading or writing real board files. Python
fallback code is useful for compatibility tests and pure-Python behavior, but it
should not recompute native board metadata when KiCad already provides the
answer.

When the target KiCad version changes, re-extract SWIG constants and rerun the
full compatibility set:

```sh
scripts/run.sh python -m unittest discover -s tests
scripts/run-pcbnewswig-tests.sh
scripts/run-kikit-tests.sh unit
scripts/run-kikit-tests.sh system
scripts/run-kikakuka-test.sh
```
