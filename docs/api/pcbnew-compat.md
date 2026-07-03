# pcbnew Compatibility

Import the compatibility layer the same way existing scripts do:

```python
import pcbnew
```

The compatibility layer is implemented on top of `pybind11_kicad` and the same
native KiCad-backed board IO. It is not a separate parser, and it should fail
clearly when the native backend is unavailable.

## Current Compatibility Target

The current target is existing KiKit/Kikakuka-style code that expects the legacy
SWIG `pcbnew` module.

Current tested status:

* KiKit pinned unit tests are satisfied by the compatibility shim.
* All Kikakuka v6.6 test panels pass the golden comparison.
* The Kikakuka Gerber conversion sample passes the golden comparison.

## Common Entry Points

```python
board = pcbnew.LoadBoard("input.kicad_pcb")
board.Save("output.kicad_pcb")

new_board = pcbnew.NewBoard()
```

Supported metadata helpers include:

```python
pcbnew.GetBuildVersion()
pcbnew.GetMajorMinorVersion()
pcbnew.Version()
pcbnew.CompatibilityLevel()
pcbnew.RequireCompatibility("partial-pcbnew-v10")
```

## Units And Values

```python
pcbnew.FromMM(1.0)
pcbnew.ToMM(1_000_000)
pcbnew.FromMils(1)
pcbnew.ToMils(25_400)
pcbnew.PCB_IU_PER_MM
```

Important value types:

```python
pcbnew.VECTOR2I(x, y)
pcbnew.BOX2I(origin, size)
pcbnew.EDA_ANGLE(value, pcbnew.DEGREES_T)
pcbnew.EDA_ANGLE(value, pcbnew.RADIANS_T)
pcbnew.EDA_ANGLE(value, pcbnew.TENTHS_OF_A_DEGREE_T)
```

## Board Items

The shim supports constructible and mutable board items needed by the tested
KiKit/Kikakuka paths:

```python
pcbnew.PCB_SHAPE()
pcbnew.PCB_TEXT(board_or_parent)
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
board.GetDesignSettings()
```

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

The comparison test keeps generated `KiKit_MB_*` labels as metadata. It does
not require equivalent generated mousebite labels to attach to identical cut
geometry in the same order, because Shapely/Python version differences can
change generated cut ordering without changing the resulting board geometry or
metadata set.

## Unsupported GUI State

Editor and action-plugin APIs are intentionally unsupported in the headless
backend:

```python
pcbnew.GetBoard()
pcbnew.GetPcbFrame()
pcbnew.Refresh()
pcbnew.ActionPlugin
```

Calls that require KiCad editor state should raise a clear `NotImplementedError`
instead of silently fabricating GUI behavior.

## Boundaries

The compatibility layer should grow only when existing scripts or tests require
additional `pcbnew` behavior. New code should prefer `pybind11_kicad`.
