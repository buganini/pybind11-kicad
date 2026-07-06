# pcbnew Compatibility

Import the compatibility layer the same way existing scripts do:

```python
import pcbnew
```

The compatibility layer is implemented on top of `pybind11_kicad` and the same
native KiCad-backed board IO. It is not a separate parser, and it should fail
clearly when the native backend is unavailable.

## Current Compatibility Target

The primary target is the legacy SWIG `pcbnew` behavior covered by KiCad 10's
SWIG interface files and the vendored KiCad 10 test suite. KiKit and Kikakuka
are real-world consumer targets layered on top of that contract.

This section is a compatibility map, not an independent API specification. For
any name documented as supported, the intended semantics are the KiCad 10 SWIG
semantics unless this section explicitly marks the behavior as partial,
project-only, or unsupported. See
[SWIG Alignment](compatibility.md#swig-alignment) for the comparison rule
and current broad surface gap.

Current configured status is summarized in
[Compatibility](compatibility.md). KiKit's upstream stencil tests
are out of scope because they exercise OpenSCAD stencil generation rather than
`pcbnew` compatibility.

## Common Entry Points

```python
board = pcbnew.LoadBoard("input.kicad_pcb")
board.Save("output.kicad_pcb")
pcbnew.SaveBoard("output.kicad_pcb", board)

new_board = pcbnew.NewBoard()
```

`board.Save(filename)` follows SWIG's Python helper shape: it is the instance
form of `pcbnew.SaveBoard(filename, board)`. `pcbnew.NewBoard()` mirrors SWIG's
empty-board constructor path.

Supported metadata helpers include:

```python
pcbnew.GetBuildVersion()
pcbnew.GetMajorMinorVersion()
pcbnew.Version()
pcbnew.GetSettingsManager()
```

Project-only diagnostics are available, but they are not KiCad SWIG API:

```python
pcbnew.CompatibilityLevel()
pcbnew.RequireCompatibility("partial-pcbnew-v10")
```

## Units And Values

The module exports all plain top-level KiCad 10 SWIG constants with their pinned
SWIG values. This includes enum-like layer, unit, plotting, shape, pad, via,
zone, color, DRC, and board-setting constants.

```python
pcbnew.FromMM(1.0)
pcbnew.ToMM(1_000_000)
pcbnew.FromMils(1)
pcbnew.ToMils(25_400)
pcbnew.PCB_IU_PER_MM
```

The implemented unit helpers cover scalar values. `ToMM()` also accepts the
currently implemented vector-like values used by the tests. SWIG also accepts
`wxPoint`, `wxSize`, and `VECTOR2L`; those wider conversions are not part of the
current implemented subset.

Important value types:

```python
pcbnew.KIID()
pcbnew.LIB_ID("Resistor_SMD:R_0603_1608Metric")
pcbnew.VECTOR2I(x, y)
pcbnew.VECTOR2I_MM(x_mm, y_mm)
pcbnew.BOX2I(origin, size)
pcbnew.LSET([pcbnew.F_Cu, pcbnew.B_Cu])
pcbnew.TITLE_BLOCK()
pcbnew.EDA_ANGLE(value, pcbnew.DEGREES_T)
pcbnew.EDA_ANGLE(value, pcbnew.RADIANS_T)
pcbnew.EDA_ANGLE(value, pcbnew.TENTHS_OF_A_DEGREE_T)
```

`VECTOR2I` exposes `x` and `y`, iteration, indexing, and arithmetic operators in
the implemented subset. SWIG's explicit `VECTOR2I.Get()` and `VECTOR2I.Set()`
helpers are not implemented yet.

`LSET` supports the SWIG layer mutation aliases `AddLayer`, `addLayer`,
`RemoveLayer`, `removeLayer`, `AddLayerSet`, `addLayerSet`, `RemoveLayerSet`,
and `removeLayerSet`.

## Board Items

The shim supports constructible and mutable board items needed by the tested
KiKit/Kikakuka paths:

```python
pcbnew.PCB_SHAPE()
pcbnew.PCB_TEXT(board_or_parent)
pcbnew.PCB_DIM_ORTHOGONAL(board)
pcbnew.PCB_VIA(board)
pcbnew.ZONE(board)
pcbnew.FOOTPRINT(native_footprint_or_none)
pcbnew.PAD(native_pad_or_none)
pcbnew.FootprintLoad(lib_path, footprint_name)
```

Board methods include:

```python
board.Add(item)
board.Remove(item)
board.GetDrawings()
board.GetFootprints()
board.GetTracks()
board.GetPads()
board.Zones()
board.GetNetInfo()
board.GetNetsByName()
board.GetNetcodeFromNetname(name)
board.TracksInNet(net_code)
board.GetConnectivity()
board.GetDesignSettings()
board.GetTitleBlock()
board.SetTitleBlock(title_block)
board.GetCopperLayerCount()
board.SetCopperLayerCount(count)
board.GetEnabledLayers()
board.SetEnabledLayers(layers)
board.GetLayerName(layer_id)
board.GetLayerID(name)
board.SetLayerName(layer_id, name)
board.FindFootprintByReference(reference)
board.ComputeBoundingBox()
board.ResolveItem(kiid)
```

SWIG defines `GetFootprints()`, `GetDrawings()`, and `GetTracks()` as Python
helpers that return lists around the iterable board containers. The shim follows
that list-returning behavior for the implemented methods.

SWIG defines `Add()` and `Remove()` as ownership-managing Python wrappers around
native container calls. The pybind11 shim exposes the same supported names and
observable add/remove behavior, but raw SWIG `thisown` pointer ownership is not
a pybind11 concept.

## Footprints And Mousebites

Generated NPTH mousebite footprints are written through the normal full
footprint path. There is no special output-side mousebite writer.

The compatibility path preserves:

* hidden Reference and Value text
* pad attribute `PAD_ATTRIB_NPTH`
* pad layers, drill, size, shape, and drill shape
* footprint flags such as board-only, exclude-from-position-files, and
  exclude-from-BOM
* user fields and footprint metadata covered by the native model
* UUIDs when copying existing native objects

Implemented SWIG field helpers include:

```python
footprint.GetFieldsText()
footprint.GetFieldText(name)
footprint.SetField(name, value)
```

SWIG's `GetFieldText(name)` raises `KeyError` when the field is missing. The
method exists in the shim, but matching that missing-field exception is still a
current gap.

Example:

```python
hole = pcbnew.FootprintLoad("", "NPTH")
hole.SetReference("KiKit_MB_1_1")
hole.SetPosition(pcbnew.VECTOR2I(pcbnew.FromMM(1), pcbnew.FromMM(2)))
hole.SetBoardOnly(True)
hole.SetExcludedFromPosFiles(True)
hole.SetExcludedFromBOM(True)
board.Add(hole)
```

Generated `KiKit_MB_*` labels are treated as normal footprint/text metadata and
are preserved through the standard footprint, field, and pad APIs.

For pads, SWIG's `GetName()` is a compatibility alias for the pad number. The
shim uses `GetName()` with the same pad-number meaning for the implemented
subset. The wider SWIG aliases `GetPadName`, `SetPadName`, `GetNumber`, and
`SetNumber` are not implemented yet.

## Tracks, Vias, Zones, And Dimensions

The shim supports the mutable item surface exercised by the SWIG tests, KiKit,
and Kikakuka:

```python
track = pcbnew.PCB_TRACK(board)
track.SetStart(pcbnew.VECTOR2I(0, 0))
track.SetEnd(pcbnew.VECTOR2I(1_000_000, 0))
track.SetWidth(pcbnew.FromMM(0.25))
board.Add(track)

via = pcbnew.PCB_VIA(board)
via.SetPosition(pcbnew.VECTOR2I(500_000, 500_000))
via.SetDrill(pcbnew.FromMM(0.3))
via.SetWidth(pcbnew.FromMM(0.6))
board.Add(via)
```

`ZONE` preserves outline polygons, holes, layer sets, rule-area flags, fill
state, and filled polygons. `PCB_DIM_ORTHOGONAL` is represented as board
drawing geometry for the currently tested render-dimension path.

Current gap: KiCad SWIG exposes arc tracks through `PCB_ARC`. The shim currently
carries arc-track data through `PCB_TRACK` helper plumbing for native round
trips, but that is not final SWIG-compatible surface semantics.

## Plot, Drill, And DRC Facades

KiKit uses a small part of KiCad's plotting, drill, and DRC APIs. The shim
implements enough of these APIs for the configured tests:

```python
pcbnew.PCB_PLOT_PARAMS()
pcbnew.PLOT_CONTROLLER(board)
pcbnew.GERBER_JOBFILE_WRITER(board)
pcbnew.EXCELLON_WRITER(board)
pcbnew.WriteDRCReport(board, path, units, strict)
```

Current implementation status: these are partial compatibility facades. They
create deterministic placeholder plot/drill/job/report files sufficient for the
configured consumer tests, but they do not yet match the full KiCad SWIG
plotting, drill, and DRC semantics. Full SWIG-equivalent behavior remains a gap.

## Unsupported GUI State

Editor and action-plugin APIs are intentionally unsupported in the headless
backend:

```python
pcbnew.GetBoard()
pcbnew.Refresh()
pcbnew.ActionPlugin
```

Calls that require KiCad editor state should raise a clear `NotImplementedError`
instead of silently fabricating GUI behavior.

## Boundaries

The compatibility layer should grow only when existing scripts or tests require
additional `pcbnew` behavior, and supported behavior should follow SWIG
semantics. New project-owned code may use `pybind11_kicad`, but that does not
change the `pcbnew` parity target.
`ZONE_FILLER.Fill()` is currently an empty-zone no-op; non-empty zone refills are
not implemented by the compatibility shim.
