# KiKit/Kikakuka KiCad Native Backend Plan

## Goal

KiKit/Kikakuka needs to keep working after KiCad removes the legacy SWIG `pcbnew` Python API.

The preferred KiKit/Kikakuka architecture is to bundle a pinned KiCad engine and expose both:

1. a clean KiKit/Kikakuka-owned Python API, and
2. an optional `pcbnew` compatibility layer for old scripts.

This avoids depending on the user’s installed KiCad and avoids IPC for normal file creation/editing workflows.

## Background

KiCad’s legacy SWIG Python API allowed scripts to import `pcbnew`, load `.kicad_pcb` files, manipulate board objects, and save the result without running the KiCad GUI.

KiCad’s official replacement direction is IPC. The KiCad IPC API runs plugins as standalone processes communicating with a KiCad instance, and KiCad 11 adds headless IPC through `kicad-cli api-server`.

KiKit/Kikakuka’s requirement is different:

* no IPC for normal operation
* no dependency on the user-installed KiCad
* native KiCad file compatibility as much as possible
* FreeCAD/Python integration
* ability to read/write KiCad board files offline
* source compatibility with common old `pcbnew` scripts where practical

Therefore, KiKit/Kikakuka will treat KiCad as a bundled private engine.

## Proposed Architecture

```text
KiKit/Kikakuka Python / FreeCAD layer
        ↓
KiKit/Kikakuka stable Python API
        ↓
optional pcbnew compatibility shim
        ↓
pybind11 native module
        ↓
KiKit/Kikakuka C++ facade
        ↓
bundled pinned KiCad core
        ↓
.kicad_pcb / .kicad_mod / .kicad_sch / .kicad_sym
```

The pybind11 module must not expose KiCad’s raw C++ API directly. It should expose KiKit/Kikakuka-owned wrapper classes.

Good API shape:

```python
import kikit_kikakuka.kicad as kk

board = kk.Board.open("input.kicad_pcb")

for fp in board.footprints():
    print(fp.reference, fp.position, fp.layer)

board.add_track(
    net="GND",
    layer="F.Cu",
    start=(10.0, 10.0),
    end=(20.0, 10.0),
    width=0.25,
)

board.save("output.kicad_pcb")
```

Compatibility API shape:

```python
import pcbnew

board = pcbnew.LoadBoard("input.kicad_pcb")

for fp in board.GetFootprints():
    print(fp.GetReference())

board.Save("output.kicad_pcb")
```

Avoid exposing raw KiCad internals directly:

```python
# Avoid designing the new core API around this style.
import kikit_kikakuka_pcbnew as pcbnew

board = pcbnew.BOARD()
track = pcbnew.PCB_TRACK(board)
```

The goal is not to recreate KiCad’s entire removed SWIG API. The goal is to support the subset needed by KiKit/Kikakuka and common offline board automation scripts.

## Repository Layout

```text
kikit-kikakuka/
  python/
    pcbnew.py
      # optional top-level compatibility module

    kikit_kikakuka/
      __init__.py
      kicad.py

      compat/
        __init__.py
        pcbnew.py

  native/
    CMakeLists.txt

    include/
      kikit_kikakuka_kicad/
        board.hpp
        footprint.hpp
        pad.hpp
        track.hpp
        via.hpp
        zone.hpp
        drawing.hpp
        net.hpp
        layer.hpp
        types.hpp
        errors.hpp

    src/
      bindings.cpp
      board.cpp
      footprint.cpp
      pad.cpp
      track.cpp
      via.cpp
      zone.cpp
      drawing.cpp
      net.cpp
      layer.cpp
      kicad_init.cpp
      kicad_io.cpp
      error_map.cpp

  third_party/
    kicad/
      # pinned KiCad source tree or submodule

  tests/
    golden/
      simple_board.kicad_pcb
      footprints.kicad_pcb
      tracks_vias.kicad_pcb
      zones.kicad_pcb
      edge_cuts.kicad_pcb
      custom_properties.kicad_pcb

    test_open_save.py
    test_footprints.py
    test_tracks.py
    test_roundtrip.py
    test_pcbnew_compat.py
```

## Versioning Strategy

KiKit/Kikakuka should version the native backend by bundled KiCad major version.

```text
kikit-kikakuka-kicad-native-10
  uses bundled KiCad 10.x

kikit-kikakuka-kicad-native-11
  uses bundled KiCad 11.x

KiKit/Kikakuka public API
  should stay stable above both
```

Example:

```python
import kikit_kikakuka.kicad as kk

print(kk.backend_version())
# "kicad-11.0.0-kikit-kikakuka-0.1"
```

The bundled KiCad version should be pinned exactly:

```text
KiCad source commit: <commit hash>
KiCad version: 11.0.x
KiKit/Kikakuka native ABI: 0.x
```

Do not link against arbitrary user-installed KiCad libraries.

## API Layers

KiKit/Kikakuka should provide two public Python layers.

### 1. Clean KiKit/Kikakuka API

This is the preferred new API.

```python
import kikit_kikakuka.kicad as kk

board = kk.Board.open("input.kicad_pcb")
board.add_track(...)
board.save("output.kicad_pcb")
```

This API should be:

* stable
* typed where useful
* Pythonic
* independent of KiCad internal class names
* easier to maintain across KiCad versions

### 2. `pcbnew` Compatibility API

This is a migration layer for old scripts.

```python
import pcbnew

board = pcbnew.LoadBoard("input.kicad_pcb")
board.Save("output.kicad_pcb")
```

This API should be:

* best-effort source compatible
* intentionally partial
* clearly documented
* backed by the same bundled KiCad engine
* implemented on top of the KiKit/Kikakuka facade where possible

The compatibility layer should not become the main internal API.

## `pcbnew` Compatibility Layer

KiKit/Kikakuka provides an optional `pcbnew` compatibility layer for scripts that previously used KiCad’s SWIG Python API.

This layer is not the official KiCad `pcbnew` module. It is a KiKit/Kikakuka-maintained compatibility shim backed by the bundled KiCad engine.

The goal is to support common offline board file automation:

* open and save `.kicad_pcb`
* inspect footprints, pads, tracks, vias, zones, nets, and layers
* move footprints
* add or remove simple board items
* generate tracks, vias, and drawings
* run common KiKit/Kikakuka workflows that previously imported `pcbnew`

The goal is not to fully recreate KiCad’s removed SWIG API.

Unsupported or limited areas include:

* KiCad GUI/editor state
* action plugin registration
* current selection/highlight state
* live board editor frame access
* router integration
* DRC engine
* plot controller
* schematic-to-board update
* full project/library table behavior
* wx UI objects
* raw KiCad pointer ownership

## `pcbnew` Compatibility Tiers

### Tier 1: Common Script-Compatible API

These should be implemented first.

```python
pcbnew.LoadBoard(path)
pcbnew.SaveBoard(path, board)
pcbnew.GetBuildVersion()

board.Save(path)
board.GetFileName()
board.GetFootprints()
board.GetTracks()
board.GetDrawings()
board.GetZones()
board.GetNetsByName()
board.FindFootprintByReference("U1")
board.GetLayerName(layer_id)
board.GetLayerID("F.Cu")

fp.GetReference()
fp.SetReference("U1")
fp.GetValue()
fp.SetValue("STM32")
fp.GetPosition()
fp.SetPosition(pos)
fp.GetOrientation()
fp.SetOrientation(angle)
fp.Pads()

pad.GetName()
pad.GetNetname()
pad.GetPosition()
pad.GetSize()
pad.GetDrillSize()
pad.GetShape()

track.GetStart()
track.SetStart(pos)
track.GetEnd()
track.SetEnd(pos)
track.GetWidth()
track.SetWidth(width)
track.GetLayer()
track.SetLayer(layer)
track.GetNetname()
```

Also useful:

```python
pcbnew.VECTOR2I(x, y)
pcbnew.wxPoint(x, y)
pcbnew.FromMM(1.0)
pcbnew.ToMM(value)

pcbnew.PCB_TRACK(board)
pcbnew.PCB_VIA(board)
```

### Tier 2: Useful but Later

These can be implemented after the common flow works.

```python
board.Add(item)
board.Remove(item)
board.GetDesignSettings()
board.GetBoardEdgesBoundingBox()
board.GetPads()
board.GetAreaCount()
board.Zones()

fp.GetFPID()
fp.GetPath()
fp.GetProperties()
fp.SetProperty(key, value)
fp.GetProperty(key)

zone.GetNetname()
zone.GetLayerSet()
zone.GetAssignedPriority()
```

### Tier 3: Explicitly Unsupported or Stubbed

These should raise clear errors rather than silently doing the wrong thing.

```python
pcbnew.ZONE_FILLER
pcbnew.DRC
pcbnew.PLOT_CONTROLLER
pcbnew.ActionPlugin
pcbnew.GetBoard()
pcbnew.GetPcbFrame()
pcbnew.Refresh()
```

Example error:

```python
raise NotImplementedError(
    "pcbnew.ZONE_FILLER is not supported by KiKit/Kikakuka's bundled backend. "
    "Use KiCad CLI/GUI validation or the clean KiKit/Kikakuka API instead."
)
```

## Compatibility Metadata

The `pcbnew` shim should identify itself clearly.

```python
import pcbnew

print(pcbnew.GetBuildVersion())
# "KiKit/Kikakuka pcbnew compatibility layer, bundled KiCad 11.0.0"

print(pcbnew.CompatibilityLevel())
# "partial-pcbnew-v10"
```

Optional:

```python
pcbnew.RequireCompatibility("partial-pcbnew-v10")
```

This helps old scripts fail early when they rely on unsupported features.

## C++ Facade Design

The C++ facade should own KiCad objects internally and hide raw KiCad pointers from Python.

Example:

```cpp
class KkBoard {
public:
    static KkBoard open(const std::string& path);
    void save(const std::string& path);

    std::vector<KkFootprint> footprints() const;
    std::vector<KkTrack> tracks() const;
    std::vector<KkVia> vias() const;
    std::vector<KkZone> zones() const;

    void add_track(const KkTrackSpec& spec);
    void add_via(const KkViaSpec& spec);

private:
    std::unique_ptr<BOARD> board_;
};
```

Expose only simple value types to Python:

```cpp
struct KkPoint {
    double x_mm;
    double y_mm;
};

struct KkTrackSpec {
    std::string net;
    std::string layer;
    KkPoint start;
    KkPoint end;
    double width_mm;
};

struct KkViaSpec {
    std::string net;
    KkPoint position;
    double drill_mm;
    double diameter_mm;
};
```

Avoid exposing:

* raw `BOARD*`
* raw `EDA_ITEM*`
* KiCad object ownership rules
* wxWidgets UI objects
* editor frame objects
* plugin manager internals
* arbitrary KiCad enums without translation

## Native Object Lifetime Rules

The native layer should enforce simple ownership rules.

Recommended model:

```text
KkBoard owns the KiCad BOARD object.
KkFootprint, KkPad, KkTrack, KkVia, and KkZone are lightweight handles or snapshots.
Python never owns raw KiCad objects directly.
Items cannot outlive their parent KkBoard.
```

For mutable item handles, use validity checks:

```python
fp = board.find_footprint("U1")
board.remove_footprint("U1")

fp.GetReference()
# raises ReferenceError or RuntimeError
```

Prefer snapshot/value objects for read-only operations where possible.

## Units

KiCad internal units should not leak into the clean KiKit/Kikakuka API.

Clean API:

```python
board.add_track(
    start=(10.0, 10.0),  # mm
    end=(20.0, 10.0),    # mm
    width=0.25,          # mm
)
```

`pcbnew` compatibility API:

```python
track.SetWidth(pcbnew.FromMM(0.25))
```

The compatibility layer should preserve common `pcbnew` unit helpers:

```python
pcbnew.FromMM(mm)
pcbnew.ToMM(value)
```

## Initial Feature Scope

### Phase 1: Board File IO

Required:

* open `.kicad_pcb`
* save `.kicad_pcb`
* roundtrip simple board files
* preserve KiCad-compatible output
* initialize required KiCad runtime state without GUI

Public API:

```python
board = kk.Board.open(path)
board.save(path)
```

Compatibility API:

```python
board = pcbnew.LoadBoard(path)
board.Save(path)
```

### Phase 2: Read Board Content

Required:

* list footprints
* list pads
* list tracks
* list vias
* list zones
* list nets
* list layers
* read board outline graphics

Clean API:

```python
board.footprints()
board.tracks()
board.vias()
board.zones()
board.nets()
board.layers()
board.edge_cuts()
```

Compatibility API:

```python
board.GetFootprints()
board.GetTracks()
board.GetZones()
board.GetDrawings()
board.GetNetsByName()
```

### Phase 3: Modify Board Geometry

Required:

* move footprint
* add track
* add via
* add graphic line/arc/circle
* add board outline geometry
* remove generated items by KiKit/Kikakuka marker/property

Clean API:

```python
fp.move_to((x, y))
board.add_track(...)
board.add_via(...)
board.add_edge_line(...)
board.remove_items_by_tag("kikit-kikakuka-generated")
```

Compatibility API:

```python
track = pcbnew.PCB_TRACK(board)
track.SetStart(pcbnew.VECTOR2I(...))
track.SetEnd(pcbnew.VECTOR2I(...))
track.SetWidth(pcbnew.FromMM(0.25))
board.Add(track)
```

### Phase 4: Footprint and Library Support

Required:

* load `.kicad_mod`
* insert footprint into board
* inspect pads from footprint file
* optionally support KiCad library tables

Clean API:

```python
fp = kk.Footprint.open("foo.kicad_mod")
board.add_footprint(fp, at=(x, y), reference="U1")
```

Compatibility API:

```python
fp = pcbnew.FootprintLoad("/path/to/lib.pretty", "foo")
board.Add(fp)
```

### Phase 5: Optional Advanced KiCad Features

These are higher risk:

* zone refill
* DRC
* schematic-to-board update
* plotting/export
* full project path resolution
* library table resolution
* 3D model path resolution

These should be optional, not required for the first native backend.

## Runtime Initialization

The native module may need to initialize parts of KiCad that are normally initialized by the application.

Likely areas:

* locale handling
* settings paths
* environment variables
* KiCad resource paths
* plugin registry
* file IO plugin loading
* wxWidgets initialization if unavoidable
* project path resolver
* library table handling

Create one explicit initialization point:

```python
import kikit_kikakuka.kicad as kk

kk.initialize(
    resource_dir="/path/to/bundled/kicad/resources",
    config_dir="/path/to/kikit-kikakuka/config",
)
```

Or initialize lazily on first use.

Do not rely on the user’s KiCad config directory by default.

## Packaging Strategy

### Windows

Ship:

```text
kikit_kikakuka_kicad_native.pyd
bundled KiCad DLLs
required runtime DLLs
KiCad resources
```

Key issues:

* MSVC runtime
* DLL search paths
* vcpkg/MSYS2 dependency mismatch
* Python ABI per version
* large package size

### macOS

Ship:

```text
kikit_kikakuka_kicad_native.so
bundled dylibs
KiCad resources
```

Key issues:

* `@rpath`
* code signing
* notarization
* universal2 or separate x86_64/arm64 builds
* Python ABI per version

### Linux

Ship:

```text
kikit_kikakuka_kicad_native.so
bundled shared libraries where practical
KiCad resources
```

Key issues:

* manylinux compatibility may be difficult
* system wxWidgets/GTK conflicts
* glibc baseline
* OpenCascade and other large dependencies

For Linux, AppImage/Flatpak-style packaging may be easier than normal PyPI wheels.

## Build Strategy

Use CMake for the native module.

High-level targets:

```text
kicad_engine
  bundled KiCad subset or patched KiCad build

kikit_kikakuka_kicad_core
  C++ facade over KiCad internals

kikit_kikakuka_kicad_native
  pybind11 Python extension
```

Avoid spreading KiCad includes throughout KiKit/Kikakuka. Keep KiCad-specific code inside `native/src`.

Preferred layering:

```text
Python
  ↓
pybind11 bindings
  ↓
KiKit/Kikakuka C++ facade
  ↓
KiCad adapter implementation
  ↓
KiCad internals
```

This makes future KiCad upgrades less painful.

## Build Configuration

Recommended build options:

```text
KIKIT_KIKAKUKA_BUNDLED_KICAD_VERSION=11.0.x
KIKIT_KIKAKUKA_ENABLE_PCBNEW_COMPAT=ON
KIKIT_KIKAKUKA_ENABLE_IPC=OFF
KIKIT_KIKAKUKA_ENABLE_KICAD_CLI_VALIDATION=OPTIONAL
KIKIT_KIKAKUKA_ENABLE_GUI=OFF
```

The first proof of concept should avoid GUI/editor functionality.

## Testing Plan

### Golden Roundtrip Tests

For each fixture:

1. open board
2. save to temp file
3. open saved file in bundled KiCad backend
4. compare expected structural content
5. optionally compare normalized S-expression

Test files:

```text
simple_board.kicad_pcb
multi_layer_board.kicad_pcb
footprints.kicad_pcb
tracks_vias.kicad_pcb
zones.kicad_pcb
edge_cuts.kicad_pcb
custom_properties.kicad_pcb
```

### Compatibility Tests

For every bundled KiCad update:

* open files created by previous KiKit/Kikakuka versions
* open files saved by official KiCad
* save files and verify official KiCad can open them
* compare generated diffs
* verify no unwanted deletion of unknown fields

### Python API Tests

Test the stable KiKit/Kikakuka API, not KiCad internals.

Example:

```python
def test_add_track(tmp_path):
    board = kk.Board.open("tests/golden/simple_board.kicad_pcb")
    board.add_track(
        net="GND",
        layer="F.Cu",
        start=(10, 10),
        end=(20, 10),
        width=0.25,
    )
    out = tmp_path / "out.kicad_pcb"
    board.save(out)

    board2 = kk.Board.open(out)
    assert any(t.net == "GND" for t in board2.tracks())
```

### `pcbnew` Compatibility Tests

Test common old-style scripts.

Example:

```python
def test_pcbnew_load_save(tmp_path):
    import pcbnew

    board = pcbnew.LoadBoard("tests/golden/simple_board.kicad_pcb")
    assert board is not None

    out = tmp_path / "out.kicad_pcb"
    board.Save(str(out))

    board2 = pcbnew.LoadBoard(str(out))
    assert board2 is not None
```

Example:

```python
def test_pcbnew_footprint_reference():
    import pcbnew

    board = pcbnew.LoadBoard("tests/golden/footprints.kicad_pcb")
    refs = [fp.GetReference() for fp in board.GetFootprints()]

    assert "U1" in refs
```

Example:

```python
def test_pcbnew_units():
    import pcbnew

    assert pcbnew.ToMM(pcbnew.FromMM(1.0)) == 1.0
```

### Real KiCad Validation

Optional but useful:

* open output in official KiCad GUI
* run `kicad-cli` DRC if available
* export Gerbers
* compare with expected generated files

This can be CI optional because it may require a full KiCad runtime.

## Fallback Direct S-expression Backend

Even with a native KiCad backend, KiKit/Kikakuka should keep a simple direct S-expression backend for emergency use.

Purpose:

* inspect files without initializing KiCad
* simple patching
* debugging output
* preserving unknown fields
* fallback if native backend fails
* golden-file normalization

Architecture:

```text
KiKit/Kikakuka API
  ├─ native KiCad backend
  └─ direct S-expression backend
```

The direct S-expression backend does not need to emulate KiCad. It only needs to support narrow file editing/debugging tasks.

## Migration Strategy from Old `pcbnew`

Existing scripts should be migrated in stages.

### Stage 1: Run Through Compatibility Shim

Old script:

```python
import pcbnew

board = pcbnew.LoadBoard("input.kicad_pcb")
for fp in board.GetFootprints():
    print(fp.GetReference())
board.Save("output.kicad_pcb")
```

Should continue to work if it only uses supported compatibility APIs.

### Stage 2: Replace Unsupported Calls

Unsupported calls should raise clear errors.

Example:

```python
pcbnew.GetPcbFrame()
```

Error:

```text
NotImplementedError: pcbnew.GetPcbFrame is not supported by KiKit/Kikakuka's offline backend.
This API requires KiCad GUI/editor state.
```

### Stage 3: Move to Clean API

New code should prefer:

```python
import kikit_kikakuka.kicad as kk

board = kk.Board.open("input.kicad_pcb")
for fp in board.footprints():
    print(fp.reference)
board.save("output.kicad_pcb")
```

## Risk Register

### Risk: KiCad Internals Change

Mitigation:

* pin bundled KiCad version
* hide raw KiCad API behind KiKit/Kikakuka facade
* treat every KiCad major version as a porting task

### Risk: KiCad Is Not a Supported Library

Mitigation:

* keep native backend narrow
* avoid UI/editor objects
* focus on file IO and board object manipulation
* add direct S-expression fallback

### Risk: Build and Packaging Complexity

Mitigation:

* start with one platform first
* automate full native build
* keep exact dependency versions pinned
* avoid system KiCad dependencies

### Risk: GPL Licensing

Mitigation:

* treat native module linked with KiCad as GPL-compatible
* provide corresponding source when distributing binaries
* keep licensing clear in documentation

### Risk: Recreating Full `pcbnew`

Mitigation:

* do not mirror the entire KiCad SWIG API
* define compatibility tiers
* expose only commonly needed offline APIs first
* make unsupported calls fail clearly
* keep clean KiKit/Kikakuka API as the real core API

### Risk: Full KiCad Behavior Is Hard to Embed

Mitigation:

* do not require DRC/router/zone refill in phase 1
* use optional CLI/GUI validation for advanced features
* implement only needed subset first

## Non-goals

The native backend is not intended to:

* provide a full replacement for KiCad’s old `pcbnew` API
* support arbitrary user-installed KiCad versions
* expose KiCad internals directly to Python
* become a general KiCad SDK
* replace KiCad IPC for normal KiCad plugins
* reimplement the full KiCad editor
* support KiCad GUI action plugins in offline mode

## Recommended First Milestone

Build a minimal proof of concept:

```python
import kikit_kikakuka.kicad as kk

kk.initialize()

board = kk.Board.open("input.kicad_pcb")
print(board.footprints())
board.save("output.kicad_pcb")
```

Success criteria:

* native module imports in Python
* bundled KiCad code initializes without GUI
* `.kicad_pcb` can be opened
* footprints can be listed
* board can be saved
* saved board can be opened by official KiCad
* no dependency on user-installed KiCad

## Recommended Second Milestone

Add `pcbnew` compatibility for open/save/list footprints:

```python
import pcbnew

board = pcbnew.LoadBoard("input.kicad_pcb")

for fp in board.GetFootprints():
    print(fp.GetReference())

board.Save("output.kicad_pcb")
```

Success criteria:

* old-style import works
* common old scripts can open a board
* footprints can be inspected
* board can be saved
* unsupported GUI calls fail clearly

## Recommended Third Milestone

Add simple generated geometry:

Clean API:

```python
board.add_track(...)
board.add_via(...)
board.add_edge_line(...)
board.save(...)
```

Compatibility API:

```python
track = pcbnew.PCB_TRACK(board)
track.SetStart(...)
track.SetEnd(...)
track.SetWidth(...)
board.Add(track)
board.Save("output.kicad_pcb")
```

Success criteria:

* generated board opens in KiCad
* added items appear correctly
* unit conversion is correct
* nets/layers are resolved correctly
* tests pass on golden boards

## Decision Summary

KiKit/Kikakuka will use:

```text
pybind11 + bundled pinned KiCad + KiKit/Kikakuka-owned facade API
```

KiKit/Kikakuka will also provide:

```text
optional pcbnew compatibility shim for common old scripts
```

KiKit/Kikakuka will avoid:

```text
pybind11 directly exposing KiCad's raw C++ API
linking against user-installed KiCad
depending on IPC for normal offline file operations
trying to recreate the full old pcbnew API
using the compatibility shim as the primary internal API
```

This is not KiCad’s officially supported integration model, but it is a reasonable self-contained architecture because KiKit/Kikakuka controls and bundles the KiCad engine version.
