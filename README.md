# pybind11-kicad Headless KiCad Library Plan

## Experimental Scope

This is an experimental project. It is not the official replacement for KiCad's
legacy SWIG `pcbnew` Python API.

KiCad's official replacement direction for the SWIG API is the KiCad IPC API.
For official headless operation, use `kicad-cli api-server` and communicate
through that IPC API.

pybind11-kicad explores a different architecture: a pinned, self-built,
KiCad-linked native Python backend for offline library-style board workflows and
`pcbnew` compatibility experiments.

## Current Repository Status

This repository now contains the first executable proof-of-concept slice:

* `python/pybind11_kicad/__init__.py` exposes the clean headless KiCad API shape.
* `python/pcbnew.py` exposes a top-level partial `pcbnew` compatibility shim.
* `native/` contains the C++ facade, pybind11 module scaffold, pinned KiCad
  source resolver, KiCad-backed board IO adapter, and CMake glue for injecting
  the adapter into a KiCad build graph.
* `scripts/build.sh` is the reproducible entry point for building pinned
  KiCad and the KiCad-linked native backend.
* `tests/` verifies the Python import surface, compatibility metadata, unit
  helpers, clear native-backend failure for scaffold builds without board IO,
  and real board open/save behavior when the KiCad-backed backend is linked.
* the current target is pinned to [KiCad 10.0.4](#versioning-strategy), and the
  distribution/backend major-version name is `pybind11-kicad-native-10`.
* the current target Python version is [Python 3.14](#versioning-strategy).

See [docs/api/index.md](docs/api/index.md) for the current API documentation
entry point.

## Project Process

This project is human-directed and AI-coded. The human maintainer sets the
goals, technical direction, acceptance criteria, and final review decisions;
AI systems perform much of the implementation and documentation work under
that direction.

## License

pybind11-kicad is licensed under GPL-3.0-or-later, following KiCad's license.
See `LICENSE`.

## Build And Test

Use this path for normal development. With no subcommand,
`scripts/build.sh` builds pinned KiCad and the pybind11-kicad native module:

```sh
scripts/build.sh
scripts/test-kicad.sh
```

To build the pieces explicitly, use the same pinned KiCad build tree and choose
the named subcommand.

Build KiCad:

```sh
scripts/build.sh kicad
```

This fetches or reuses `tmp/kicad`, verifies the pinned KiCad commit,
configures KiCad, and builds the KiCad executable set, including
`tmp/pybind11-kicad-10-build/kicad/kicad-cli`.

Build pybind11-kicad:

```sh
scripts/build.sh pybind11-kicad
```

This builds the native extension at
`tmp/pybind11-kicad-10-build/pybind11-kicad-native/pybind11_kicad_native.so`.
For checkout-based development, the Python package itself is used directly from
`python/`. The pybind11-kicad targets are explicit build targets, so a plain
KiCad build does not accidentally build the Python extension.

Run Python with the built backend:

```sh
scripts/build.sh python
```

Example smoke check:

```sh
scripts/build.sh python -c 'import pybind11_kicad as kc; print(kc.backend_version()); print(kc.native_available())'
```

The `python` subcommand uses `PYBIND11_KICAD_PYTHON` or `python3.14`, then
creates or reuses the ignored project virtual environment at `$PWD/env`. It
execs `$PWD/env/bin/python`, prepends `$PWD/env/bin` to `PATH`, and sets
`PYTHONPATH` to include this checkout's `python/` directory and the native
module directory from the pinned build tree. Pass `-m`, `-c`, or a script path
after `python` to run commands in that environment. `scripts/test-kicad.sh`
uses the same subcommand before running the repository test suite.

`scripts/build.sh` is the reproducible build entry point. It:

* fetches or reuses the pinned shallow KiCad 10.0.4 source checkout
* verifies commit `f7414d419cae5df2d00e7eaacb16fc0e803799bc`
* requires the active interpreter to be Python 3.14
* configures KiCad as the top-level CMake project
* adds `native/` at the end of that KiCad configure pass with
  `CMAKE_PROJECT_TOP_LEVEL_INCLUDES` and `cmake_language(DEFER)`
* builds KiCad from the pinned source tree, including the command-line
  executable
* builds `pybind11_kicad_native` against KiCad's internal headless board/common
  IO targets instead of the full `pcbnew` editor kiface
* runs Python with the checkout package and built native module importable when
  invoked as `scripts/build.sh python`

The linked KiCad libraries come from the build tree created from the pinned
source checkout, not from a user-installed or prebuilt KiCad application.

The default paths are:

```text
KiCad source:       $PWD/tmp/kicad
KiCad build:        $PWD/tmp/pybind11-kicad-10-build
Native module dir:  $PWD/tmp/pybind11-kicad-10-build/pybind11-kicad-native
KiCad CLI:          $PWD/tmp/pybind11-kicad-10-build/kicad/kicad-cli
Python env:         $PWD/env
```

The pinned inputs are:

```text
KiCad version: 10.0.4
KiCad major:   10
KiCad tag:     10.0.4
KiCad commit:  f7414d419cae5df2d00e7eaacb16fc0e803799bc
Python:        3.14
```

Prerequisites:

* Python 3.14
* Git and network access, unless `tmp/kicad` already contains the pinned KiCad
  checkout
* CMake 3.20 or newer
* a C++20 compiler compatible with both the self-built KiCad artifacts and the
  CPython 3.14 extension ABI
* platform dependencies from the generated scripts in `deps/kicad-10.0.4/`

Use a single compiler/toolchain setup that is ABI-compatible with the exact
self-built KiCad tree and CPython 3.14 interpreter used for this backend.

Platform guidance:

* Windows: use MSVC, preferably Visual Studio 2022 for the KiCad 10 target, and
  build against the same CPython 3.14 ABI family.
* macOS: use the Apple Clang/Xcode toolchain family and match the Python
  architecture, such as arm64, x86_64, or universal2.
* Linux: use the distro-compatible GCC/libstdc++ toolchain and the matching
  Python 3.14 development package.

The dependency helpers are generated from the pinned KiCad source:

* `deps/kicad-10.0.4/install-linux-apt.sh`
* `deps/kicad-10.0.4/install-windows-vcpkg.ps1`
* `deps/kicad-10.0.4/ci-ubuntu20.04-build.sh`
* `deps/kicad-10.0.4/ci-fedora-build.sh`
* `deps/kicad-10.0.4/ci-macos-build.sh`

Refresh them after changing the KiCad target:

```sh
scripts/update-kicad-deps.sh
```

Common overrides:

```sh
PYBIND11_KICAD_PYTHON=/opt/homebrew/bin/python3.14 \
PYBIND11_KICAD_BUILD_DIR=$PWD/tmp/pybind11-kicad-10-build \
  scripts/build.sh
```

Use configure-only mode when checking CMake changes:

```sh
scripts/build.sh configure
```

For a metadata-only native scaffold without KiCad-linked board IO:

```sh
cmake -S native -B $PWD/tmp/pybind11-kicad-native-10-check \
  -DPYBIND11_KICAD_KICAD_SOURCE_DIR=$PWD/tmp/kicad
cmake --build $PWD/tmp/pybind11-kicad-native-10-check \
  --target pybind11_kicad_native
```

Run KiKit's upstream unit tests from the pinned compatibility source:

```sh
python3.14 -m venv $PWD/tmp/pybind11-kicad-kikit-test-venv
$PWD/tmp/pybind11-kicad-kikit-test-venv/bin/python -m pip install -r compat/kikit-test-requirements.txt
PATH=$PWD/tmp/pybind11-kicad-kikit-test-venv/bin:$PATH scripts/run-kikit-tests.sh unit
```

The wrapper reads `compat/kikit.lock`, fetches that exact KiKit commit into
`${PYBIND11_KICAD_COMPAT_DIR}` or the default ignored cache at
`.cache/kikit`, and reuses the cached checkout on later runs. It exports
this checkout's `python/` directory and the pinned KiKit source on `PYTHONPATH`,
and then calls KiKit's own test targets. The current compatibility shim
satisfies KiKit's unit tests.

Use `KIKIT_SOURCE=/path/to/KiKit` to run against an existing checkout, but it
must already be at the pinned commit in `compat/kikit.lock`.

`compat/kikit-test-requirements.txt` includes `wxPython` because KiKit's CLI
imports `wx` even for headless command execution. That dependency is acceptable
for KiKit compatibility testing; it does not change the production board-file
decision that real board IO must be handled by the native KiCad-backed backend.

`scripts/run-kikit-tests.sh system` and `scripts/run-kikit-tests.sh all` also
run KiKit's Bats-based system tests. Those tests require `bats` on `PATH`, the
KiCad-backed board IO build, and real board files. The wrapper creates temporary
`kikit` and `kikit-info` launchers bound to the active Python interpreter so the
system tests do not accidentally pick up a machine-global KiCad Python wrapper.
Set `PYBIND11_KICAD_KIKIT_PYTHON` to force a specific Python interpreter. The
system tests remain separate from the quick scaffold tests because they exercise
real file IO and a broader `pcbnew` compatibility surface.

Run the Kikakuka panel smoke test against a source checkout with variable
paths:

```sh
PYBIND11_KICAD_KIKAKUKA_VENV=$PWD/tmp/pybind11-kicad-kikit-test-venv \
PYBIND11_KICAD_NATIVE_BUILD_DIR=$PWD/tmp/pybind11-kicad-10-build/pybind11-kicad-native \
PYBIND11_KICAD_KIKAKUKA_SOURCE=../kikakuka \
PYBIND11_KICAD_KIKAKUKA_OUTPUT=$PWD/tmp/out.kicad_pcb \
  scripts/run-kikakuka-test.sh
```

The script makes this repository's `python/` directory importable so Kikakuka
can import the local `pcbnew` compatibility module. Set
`PYBIND11_KICAD_NATIVE_BUILD_DIR` to the native module directory produced by the
board IO build instead of an installed `pybind11_kicad_native` extension. A
scaffold native build is enough to test imports and clear failure behavior, but
Kikakuka immediately calls `pcbnew.LoadBoard`, so real Kikakuka panel workflows
require the KiCad-backed board IO adapter and any additional `pcbnew` API they
touch.

Current Kikakuka smoke status: the `L7.kikit_pnl` smoke path runs through
`pcbnew.LoadBoard()`, panel construction, native text/drawing creation, and
`out.kicad_pcb` generation with the KiCad-backed backend. The current run still
prints nonfatal Kikakuka/Shapely tab-substrate diagnostics for that sample, so
future compatibility work should distinguish sample tab-placement warnings from
hard missing-API failures.

For local comparison, generate a SWIG-wrapper reference output with the same
Kikakuka sample and keep it as an ignored workspace artifact, not a committed
fixture.

## Technical Decision: Single Native Backend

pybind11-kicad will have exactly one production board-file backend: the
pybind11 module backed by pinned supported target KiCad source/build artifacts.

The Python package may provide:

* public import paths
* thin API wrappers
* the partial `pcbnew` compatibility shim
* unit helpers and compatibility metadata

The Python package must not provide:

* a second `.kicad_pcb` parser
* a separate board object model
* direct board read/write behavior independent of the native module
* silent degradation when the native backend is missing or not initialized

If the native extension is unavailable, board IO should fail immediately with a
clear error. This keeps failures attributable to one implementation and avoids
creating a second behavior surface that has to be maintained, tested, and
debugged whenever KiCad file semantics change.

Consequences:

* Early tests can verify imports, wrappers, compatibility metadata, and clear
  failure modes, but real board roundtrip tests must wait for the installed
  target KiCad adapter.
* Any test-only file normalization or fixture inspection tooling must stay
  outside the public runtime API.
* The `pcbnew` compatibility layer is a shim over the same native API, not an
  independent implementation.

## Source Audit: Required `pcbnew` Surface

This audit is based on pinned source references:

* KiKit: `compat/kikit.lock`, pinned commit `036ca08`
* Kikakuka: `/Users/buganini/repo/buganini/kikakuka`, commit `4c37246`

Generated or vendored Kikakuka paths such as `env/`, `build/`, and `dist/`
were excluded from the API search.

The result is that full KiKit/Kikakuka compatibility requires a broad
board-editing object model. It is not enough to expose only
`LoadBoard`, `Save`, and `GetFootprints`.

### Core Bring-up API

These are needed before meaningful KiKit/Kikakuka workflows can run:

```python
pcbnew.LoadBoard(path)
pcbnew.NewBoard(path)
pcbnew.BOARD()
board.Save(path)
board.Add(item)
board.Remove(item)
board.GetFileName()
board.GetDrawings()
board.GetFootprints()
board.GetTracks()
board.GetPads()
board.Zones()
board.GetNetInfo()
board.GetDesignSettings()
board.GetCopperLayerCount()
board.SetCopperLayerCount(count)
board.GetEnabledLayers()
board.GetLayerName(layer)
```

Required value types and unit helpers:

```python
pcbnew.VECTOR2I(x, y)
pcbnew.BOX2I(origin, size)
pcbnew.EDA_ANGLE(value, unit)
pcbnew.DEGREES_T
pcbnew.RADIANS_T
pcbnew.TENTHS_OF_A_DEGREE_T
pcbnew.FromMM(mm)
pcbnew.ToMM(value)
pcbnew.ToMils(value)
pcbnew.PCB_IU_PER_MM
pcbnew.wxPoint
```

Required layer constants include at least:

```python
F_Cu, B_Cu, In1_Cu ... In30_Cu
F_SilkS, B_SilkS
F_Mask, B_Mask
F_Paste, B_Paste
F_CrtYd, B_CrtYd
Edge_Cuts
Margin
```

### Geometry and Board Items

Panelization, substrate extraction, Gerber import, and stencil generation need
constructible and mutable board items:

```python
pcbnew.PCB_SHAPE()
pcbnew.PCB_TEXT(board_or_parent)
pcbnew.PCB_VIA(board)
pcbnew.FootprintLoad(lib_path, footprint_name)
pcbnew.NETINFO_ITEM(board, name)
pcbnew.ZONE(board)
pcbnew.ZONES()
pcbnew.SHAPE_POLY_SET()
pcbnew.SHAPE_LINE_CHAIN()
```

Shape constants and via/pad constants used by KiKit/Kikakuka:

```python
SHAPE_T_SEGMENT
SHAPE_T_ARC
SHAPE_T_CIRCLE
SHAPE_T_RECTANGLE
SHAPE_T_POLY
S_SEGMENT
PAD_ATTRIB_SMD
PAD_SHAPE_OVAL
PAD_DRILL_SHAPE_OBLONG
VIATYPE_THROUGH
ZONE_FILL_MODE_HATCH_PATTERN
FP_EXCLUDE_FROM_POS_FILES
```

Common item methods used across the source:

```python
item.Duplicate()
item.Cast()
item.Move(vector)
item.Rotate(origin, angle)
item.Flip(origin, flip_left_right)
item.GetBoundingBox()
item.GetLayer()
item.SetLayer(layer)
item.GetWidth()
item.SetWidth(width)
item.m_Uuid.AsString()
```

`PCB_SHAPE` needs line, arc, circle, rectangle, and polygon operations:

```python
shape.GetShape()
shape.SetShape(shape_type)
shape.GetStart()
shape.SetStart(point)
shape.GetEnd()
shape.SetEnd(point)
shape.GetStartX()
shape.GetStartY()
shape.GetCenter()
shape.SetCenter(point)
shape.GetRadius()
shape.SetRadius(radius)
shape.SetArcGeometry(start, mid, end)
shape.SetArcAngleAndEnd(angle)
shape.GetPolyShape()
shape.SetFilled(bool)
```

`SHAPE_POLY_SET` / outline support needs:

```python
poly.NewOutline()
poly.Append(x, y, outline)
poly.AddOutline(line_chain)
poly.AddHole(line_chain)
poly.RemoveAllContours()
poly.Outline(index)
poly.OutlineCount()
line_chain.PointCount()
line_chain.CPoint(index)
```

### Footprints, Pads, Text, and Fields

Full compatibility needs footprint duplication, placement, metadata, pads, and
reference/value text manipulation:

```python
footprint.GetReference()
footprint.SetReference(text)
footprint.GetValue()
footprint.SetValue(text)
footprint.Reference()
footprint.Value()
footprint.GetPosition()
footprint.SetPosition(point)
footprint.GetX()
footprint.GetY()
footprint.GetOrientation()
footprint.SetOrientation(angle)
footprint.GetLayer()
footprint.SetLayer(layer)
footprint.GetFPID()
footprint.GetFPIDAsString()
footprint.SetFPIDAsString(text)
footprint.GetAttributes()
footprint.SetExcludedFromPosFiles(bool)
footprint.IsExcludedFromPosFiles()
footprint.SetExcludedFromBOM(bool)
footprint.Pads()
footprint.GraphicalItems()
footprint.Zones()
footprint.GetFieldByName(name)
footprint.SetField(name, value)
footprint.Remove(item)
```

Pad support must cover:

```python
pad.GetAttribute()
pad.SetShape(shape)
pad.SetDrillShape(shape)
pad.SetSize(vector)
pad.SetSizeX(value)
pad.SetSizeY(value)
pad.SetDrillSize(vector)
pad.SetDrillSizeX(value)
pad.SetDrillSizeY(value)
```

Text and field support must cover both standalone text and footprint fields:

```python
text.GetText()
text.SetText(text)
text.GetShownText()
text.GetTextPos()
text.SetTextX(x)
text.SetTextY(y)
text.GetTextSize()
text.SetTextSize(vector)
text.GetTextThickness()
text.SetTextThickness(value)
text.GetTextAngle()
text.SetTextAngle(angle)
text.GetHorizJustify()
text.SetHorizJustify(value)
text.GetVertJustify()
text.SetVertJustify(value)
text.IsMirrored()
text.SetMirrored(bool)
text.SetVisible(bool)
field.IsKeepUpright()
field.SetKeepUpright(bool)
field.GetDrawRotation()
```

### Nets, Layers, Zones, and Settings

Panelization remaps nets, copies settings, and optionally refills zones:

```python
netinfo.NetsByName()
netinfo.NetsByNetcode()
netinfo.GetNetItem(name_or_code)
net.GetNetCode()
item.GetNetname()
item.SetNetCode(code)
board.RemoveNative(net_item)
```

Layer and layer-set support:

```python
pcbnew.LSET()
pcbnew.LSET.AllLayersMask()
pcbnew.LSET.AllCuMask(copper_layer_count)
lset.Seq()
lset.AddLayer(layer)
board.GetEnabledLayers()
board.SetEnabledLayers(lset)
```

Zone support:

```python
zone.Outline()
zone.GetZoneName()
zone.SetZoneName(name)
zone.GetLayerSet()
zone.SetLayerSet(lset)
zone.GetAssignedPriority()
zone.SetAssignedPriority(priority)
zone.SetFillMode(mode)
zone.SetHatchGap(value)
zone.SetHatchOrientation(angle)
zone.SetHatchThickness(value)
zone.SetLocalClearance(value)
zone.SetLocalSolderMaskMargin(value)
zone.SetIsRuleArea(bool)
zone.SetDoNotAllowTracks(bool)
zone.SetDoNotAllowVias(bool)
```

Board settings and metadata:

```python
board.GetProperties()
board.SetProperties(properties)
board.GetPageSettings()
board.SetPageSettings(settings)
board.GetTitleBlock()
board.SetTitleBlock(title_block)
design_settings.GetBoardThickness()
design_settings.SetBoardThickness(value)
design_settings.GetAuxOrigin()
design_settings.SetAuxOrigin(point)
design_settings.CloneFrom(other)
```

### Advanced or Optional Surface

These APIs appear in KiKit/Kikakuka, but should be treated as explicit optional
features or delegated to `kicad-cli` where practical:

```python
pcbnew.ZONE_FILLER(board).Fill(zones)
pcbnew.GetSettingsManager().LoadProject(path)
pcbnew.WriteDRCReport(board, output_path, units, strict)
pcbnew.PLOT_CONTROLLER(board)
pcbnew.EXCELLON_WRITER(board)
pcbnew.GERBER_JOBFILE_WRITER(board)
```

GUI/action-plugin APIs are only needed for KiCad editor plugins and should
remain unsupported in the offline native backend unless a separate GUI mode is
explicitly added:

```python
pcbnew.ActionPlugin
pcbnew.GetBoard()
pcbnew.Refresh()
pcbnew.__file__
pcbnew.kiface
```

### Implementation Implication

The compatibility shim should be implemented in terms of native wrapper objects
that mirror the subset above. It should still avoid exposing raw KiCad pointer
ownership to Python, but it cannot be only a Pythonic facade if the goal is to
run existing KiKit/Kikakuka code unchanged.

## Goal

pybind11-kicad aims to be a headless KiCad Python library with a maintained
pybind11 native module backed by pinned target KiCad source/build artifacts.

KiKit and Kikakuka are important compatibility targets, but they are not the
only intended consumers. The library should be usable by normal Python tools,
FreeCAD integrations, CI automation, board-generation workflows, and migration
code that previously imported `pcbnew`.

The preferred architecture is to expose both:

1. a clean pybind11-kicad-owned Python API, and
2. an optional `pcbnew` compatibility layer for existing scripts.

This separates two different integration roles:

* KiKit compatibility is KiCad-hosted: when KiKit is used as a KiCad plugin, it
  runs inside the KiCad process that hosts it.
* Kikakuka is Kikakuka-controlled: it should drive KiCad board functionality
  from Kikakuka, FreeCAD, or normal Python without requiring the KiCad GUI or
  KiCad's bundled Python interpreter.

For headless library use, including Kikakuka-controlled mode, the backend
should use the self-built KiCad backend produced from pinned supported target
source/build artifacts and avoid IPC for normal file creation/editing workflows.
It must not discover a user-installed KiCad package as a substitute link target.

## Background

Existing `pcbnew`-based automation expects to import `pcbnew`, load
`.kicad_pcb` files, manipulate board objects, and save the result without
running the KiCad GUI.

KiCad's official replacement direction for the legacy SWIG API is the KiCad IPC
API. The official headless entry point is `kicad-cli api-server`, which exposes
KiCad functionality through that IPC API.

This project’s requirement is different:

* no IPC for normal operation
* headless library mode must require supported target KiCad source/build
  artifacts, but not KiCad's bundled Python
* KiKit's KiCad-hosted plugin mode may still run inside the user's KiCad
  process when invoked as a KiCad plugin
* native KiCad file compatibility as much as possible
* FreeCAD/system-Python integration for Kikakuka, using the native backend
  rather than KiCad's Python runtime
* Kikakuka headless mode for automation, CI, and non-GUI workflows
* ability to read/write KiCad board files offline
* source compatibility with common existing `pcbnew` scripts where practical

Therefore, Kikakuka will treat KiCad as a pinned native target engine when it
is driving board operations itself. KiKit compatibility remains a shim for code
that expects `pcbnew`, including code that may also run inside a normal
KiCad-hosted plugin environment.

## Proposed Architecture

```text
Application / FreeCAD / system Python control layer
        ↓
pybind11-kicad stable Python API
        ↓
optional pcbnew compatibility shim
        ↓
pybind11 native module
        ↓
pybind11-kicad C++ facade
        ↓
pinned target KiCad build artifacts
        ↓
.kicad_pcb / .kicad_mod / .kicad_sch / .kicad_sym
```

The pybind11 module must not expose KiCad’s raw C++ API directly. It should expose pybind11-kicad-owned wrapper classes.

Good API shape:

```python
import pybind11_kicad as kc

board = kc.Board.open("input.kicad_pcb")

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
import pybind11_kicad as kc

board = kc.BOARD()
track = kc.PCB_TRACK(board)
```

The goal is not to recreate KiCad's entire `pcbnew` surface. The goal is to
support the subset needed by headless board automation, with KiKit/Kikakuka
compatibility as a concrete source-derived target.

## Repository Layout

```text
pybind11-kicad/
  python/
    pcbnew.py
      # optional top-level compatibility module

    pybind11_kicad/
      __init__.py

      compat/
        __init__.py
        pcbnew.py

  native/
    CMakeLists.txt

    include/
      pybind11_kicad/
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
      kicad_discovery.cpp
      kicad_io.cpp
      error_map.cpp

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

pybind11-kicad should version the native backend by target KiCad major version.
The active target for this repository is KiCad 10.

```text
pybind11-kicad-native-10
  current package/backend name
  requires target KiCad 10.x source/build artifacts

pybind11-kicad-native-11
  future package/backend name
  requires target KiCad 11.x source/build artifacts

pybind11-kicad public API
  should stay stable above both
```

Example:

```python
import pybind11_kicad as kc

print(kc.backend_version())
# "kicad-10.0.4-native-pybind11-kicad-0.1"
```

The supported KiCad target should be pinned and validated exactly:

```text
KiCad source: official KiCad GitLab source tag or compatible source snapshot
KiCad target version: 10.0.4
KiCad major target: 10
pybind11-kicad native ABI: 0.x
```

To retarget this repository to a later KiCad major, treat the KiCad major as a
release-line decision rather than a casual build flag. Until the project grows a
single generated version source, update these values together:

* `native/CMakeLists.txt`: `PYBIND11_KICAD_TARGET_KICAD_MAJOR` and
  `PYBIND11_KICAD_TARGET_KICAD_VERSION`, plus the pinned KiCad Git tag and
  commit
* `pyproject.toml`: the distribution/backend package name, such as
  `pybind11-kicad-native-10`
* `python/pybind11_kicad/__init__.py`: `TARGET_KICAD_MAJOR`,
  `TARGET_KICAD_VERSION`, and native-backend metadata strings
* `native/src/bindings.cpp`: native-backend version metadata
* `tests/`: assertions that lock the active KiCad major and backend metadata
* `README.md`: examples, build paths, and the active target statement
* `deps/kicad-<version>/`: generated dependency/build helper scripts

After fetching or providing the new pinned KiCad checkout, regenerate the
dependency helper scripts instead of copying dependency names from the README:

```sh
PYBIND11_KICAD_TARGET_KICAD_MAJOR=10 \
PYBIND11_KICAD_TARGET_KICAD_VERSION=10.0.4 \
PYBIND11_KICAD_KICAD_GIT_COMMIT=f7414d419cae5df2d00e7eaacb16fc0e803799bc \
PYBIND11_KICAD_KICAD_SOURCE_DIR=$PWD/tmp/kicad \
PYBIND11_KICAD_DEPS_OUTPUT_DIR=deps/kicad-10.0.4 \
  scripts/update-kicad-deps.sh
```

For a future target, change only the values in that command and the project
version constants above; the generated scripts should be reviewed as normal
source changes.

The public Python import should remain `pybind11_kicad` across KiCad majors.
Only the native backend distribution name should carry the KiCad major, for
example `pybind11-kicad-native-10`.

Kikakuka-controlled headless mode should use the self-built backend produced
from pinned supported target KiCad source/build artifacts. KiKit's KiCad-hosted
plugin usage may still be hosted by the user's KiCad process, but that is a
separate integration mode from this native backend.

## API Layers

pybind11-kicad should provide two public Python layers.

### 1. Clean pybind11-kicad API

This is the preferred new API.

```python
import pybind11_kicad as kc

board = kc.Board.open("input.kicad_pcb")
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

This is a compatibility layer for existing scripts.

```python
import pcbnew

board = pcbnew.LoadBoard("input.kicad_pcb")
board.Save("output.kicad_pcb")
```

This API should be:

* best-effort source compatible
* intentionally partial
* clearly documented
* backed by the same pinned target KiCad backend
* implemented on top of the pybind11-kicad facade where possible

The compatibility layer should not become the main internal API.

## `pcbnew` Compatibility Layer

pybind11-kicad provides an optional `pcbnew` compatibility layer for scripts
that import KiCad's `pcbnew` Python module.

This layer is not the official KiCad `pcbnew` module. It is a
pybind11-kicad-maintained compatibility shim backed by the pinned target KiCad
backend.

The goal is to support common offline board file automation:

* open and save `.kicad_pcb`
* inspect footprints, pads, tracks, vias, zones, nets, and layers
* move footprints
* add or remove simple board items
* generate tracks, vias, and drawings
* run common workflows, including KiKit/Kikakuka workflows, that previously imported `pcbnew`

The goal is not to fully recreate KiCad's complete `pcbnew` API.

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
board.Zones()
board.GetNetsByName()
board.GetPads()
board.GetDesignSettings()
board.FindFootprintByReference("U1")
board.GetLayerName(layer_id)
board.GetLayerID("F.Cu")
board.Add(item)
board.Remove(item)

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
pcbnew.ZONE_FILLER(board).Fill([])
```

### Tier 2: Useful but Later

These can be implemented after the common flow works.

```python
board.GetBoardEdgesBoundingBox()
board.GetAreaCount()
pcbnew.ZONE_FILLER(board).Fill(non_empty_zones)

fp.GetFPID()
fp.GetPath()
fp.GetProperties()
fp.SetProperty(key, value)
fp.GetProperty(key)

zone.GetNetname()
zone.GetLayerSet()
zone.GetAssignedPriority()
```

`ZONE_FILLER` currently exists only as an empty-zone no-op so KiKit save flows
that construct a filler unconditionally can continue. Filling real/non-empty
zone collections must be backed by native KiCad zone filling or a delegated
`kicad-cli` flow before it is considered supported.

### Tier 3: Explicitly Unsupported or Stubbed

These should raise clear errors rather than silently doing the wrong thing.

```python
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
    "pcbnew.DRC is not supported by pybind11-kicad's native backend"
)
```

## Compatibility Metadata

The `pcbnew` shim should identify itself clearly.

```python
import pcbnew

print(pcbnew.GetBuildVersion())
# "pybind11-kicad pcbnew compatibility layer, target KiCad 10.0"

print(pcbnew.CompatibilityLevel())
# "partial-pcbnew-v10"
```

Optional:

```python
pcbnew.RequireCompatibility("partial-pcbnew-v10")
```

This helps existing scripts fail early when they rely on unsupported features.

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

Ownership model:

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

KiCad internal units should not leak into the clean pybind11-kicad API.

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
* remove generated items by marker/property

Clean API:

```python
fp.move_to((x, y))
board.add_track(...)
board.add_via(...)
board.add_edge_line(...)
board.remove_items_by_tag("pybind11-kicad-generated")
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
import pybind11_kicad as kc

kc.initialize(
    kicad_dir="/path/to/self-built/kicad",
    resource_dir="/path/to/self-built/kicad/resources",
    config_dir="/path/to/pybind11-kicad/config",
)
```

Or initialize lazily on first use.

Target KiCad source/build selection should be deterministic at build time:

1. repo-local `tmp/kicad` or CMake `PYBIND11_KICAD_KICAD_SOURCE_DIR`
2. script/CMake-managed shallow checkout of the pinned Git tag and commit
3. `PYBIND11_KICAD_KICAD_BUILD_DIR` when real board IO is enabled
4. explicit runtime resource/config paths through `kc.initialize(...)` when
   KiCad initialization needs them

The native backend must validate that its compiled KiCad major/version metadata
matches the supported target. It must not fall back to discovering or linking an
installed KiCad package.

Do not rely on the user’s KiCad config directory by default.

## Packaging Strategy

pybind11-kicad packages should use the self-built KiCad backend produced from
the pinned supported target KiCad source/build artifacts. Installed KiCad
applications or distro packages are not link targets for this backend. Runtime
validation should confirm that the loaded native backend matches the supported
target before board IO.

### Windows

Ship:

```text
pybind11_kicad_native.pyd
```

Key issues:

* matching the official target KiCad package ABI with MSVC
* DLL/search paths for bundled or build-produced KiCad runtime libraries
* runtime library compatibility
* Python ABI per version

### macOS

Ship:

```text
pybind11_kicad_native.so
```

Key issues:

* locating bundled or build-produced KiCad runtime libraries
* `@rpath` and loader-path handling for KiCad libraries
* code signing
* notarization
* universal2 or separate x86_64/arm64 builds
* Python ABI per version

### Linux

Ship:

```text
pybind11_kicad_native.so
```

Key issues:

* distro-specific native dependency locations
* manylinux compatibility may be difficult if bundling KiCad-linked libraries
* system wxWidgets/GTK conflicts
* glibc baseline
* OpenCascade and other large dependencies

For Linux, the supported KiCad target may need to be constrained by
distro/package family instead of only by KiCad version.

## Build Strategy

Use CMake for the native module.

High-level targets:

```text
kicad_board_io_adapter
  board IO adapter layer for pinned target KiCad build artifacts

pybind11_kicad_core
  C++ facade over KiCad internals

pybind11_kicad_native
  pybind11 Python extension
```

Avoid spreading KiCad includes throughout pybind11-kicad. Keep KiCad-specific code inside `native/src`.

Preferred layering:

```text
Python
  ↓
pybind11 bindings
  ↓
pybind11-kicad C++ facade
  ↓
KiCad adapter implementation
  ↓
KiCad internals
```

This makes future KiCad upgrades less painful.

## Build Configuration

Build options:

```text
PYBIND11_KICAD_TARGET_KICAD_MAJOR=10
PYBIND11_KICAD_TARGET_KICAD_VERSION=10.0.4
PYBIND11_KICAD_KICAD_GIT_REPOSITORY=https://gitlab.com/kicad/code/kicad.git
PYBIND11_KICAD_KICAD_GIT_TAG=10.0.4
PYBIND11_KICAD_KICAD_GIT_COMMIT=f7414d419cae5df2d00e7eaacb16fc0e803799bc
PYBIND11_KICAD_FETCH_KICAD_SOURCE=ON
PYBIND11_KICAD_KICAD_SOURCE_DIR=
PYBIND11_KICAD_KICAD_BUILD_DIR=
PYBIND11_KICAD_KICAD_LINK_LIBRARIES=
PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO=OFF
PYBIND11_KICAD_ENABLE_PCBNEW_COMPAT=ON
PYBIND11_KICAD_ENABLE_IPC=OFF
PYBIND11_KICAD_ENABLE_KICAD_CLI_VALIDATION=OPTIONAL
PYBIND11_KICAD_ENABLE_GUI=OFF
```

The first proof of concept should avoid GUI/editor functionality.

## Testing Plan

### Golden Roundtrip Tests

For each fixture:

1. open board
2. save to temp file
3. open saved file through the target KiCad backend
4. compare expected structural content
5. optionally compare normalized KiCad board text

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

For every supported KiCad target update:

* open files created by previous pybind11-kicad versions
* open files saved by official KiCad
* save files and verify official KiCad can open them
* compare generated diffs
* verify no unwanted deletion of unknown fields

### Python API Tests

Test the stable pybind11-kicad API, not KiCad internals.

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

Test common `pcbnew`-style scripts.

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

## `pcbnew` Migration Strategy

Existing scripts should be migrated in stages.

### Stage 1: Run Through Compatibility Shim

Existing script:

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
NotImplementedError: pcbnew.GetPcbFrame is not supported by pybind11-kicad's native backend.
This API requires KiCad GUI/editor state.
```

### Stage 3: Move to Clean API

New code should prefer:

```python
import pybind11_kicad as kc

board = kc.Board.open("input.kicad_pcb")
for fp in board.footprints():
    print(fp.reference)
board.save("output.kicad_pcb")
```

## Risk Register

### Risk: KiCad Internals Change

Mitigation:

* pin and validate the supported KiCad target version
* hide raw KiCad API behind the pybind11-kicad facade
* treat every KiCad major version as a porting task

### Risk: KiCad Is Not a Supported Library

Mitigation:

* keep native backend narrow
* avoid UI/editor objects
* focus on file IO and board object manipulation
* fail clearly when target KiCad initialization is unavailable

### Risk: Build and Packaging Complexity

Mitigation:

* start with one platform first
* automate full native build
* keep exact dependency versions pinned
* avoid arbitrary system KiCad dependencies

### Risk: Recreating Full `pcbnew`

Mitigation:

* do not mirror the entire KiCad `pcbnew` surface
* define compatibility tiers
* expose only commonly needed offline APIs first
* make unsupported calls fail clearly
* keep the clean pybind11-kicad API as the real core API

### Risk: Full KiCad Behavior Is Hard to Embed

Mitigation:

* do not require DRC/router/zone refill in phase 1
* use optional CLI/GUI validation for advanced features
* implement only needed subset first

## Non-goals

The native backend is not intended to:

* provide a full replacement for KiCad's complete `pcbnew` API
* make Kikakuka-controlled headless mode depend on unsupported or unpinned KiCad
  artifacts
* expose KiCad internals directly to Python
* become a general KiCad SDK
* replace KiCad IPC for normal KiCad plugins
* maintain an alternate direct `.kicad_pcb` parser or board-file backend
* reimplement the full KiCad editor
* support KiCad GUI action plugins in offline mode

## Milestone Status

### Implemented: Native Scaffold, Import Surface, and Minimal Board IO

The current repository has completed the initial scaffold and minimal board IO
milestones:

* Python package metadata exists for Python 3.14.
* The native C++ facade target builds as `pybind11_kicad_core`.
* The pybind11 extension target builds as `pybind11_kicad_native`.
* `import pybind11_kicad` works.
* top-level `import pcbnew` works through the compatibility shim.
* compatibility metadata and unit helpers are present.
* the build downloads or validates the pinned KiCad 10.0.4 source.
* `scripts/build.sh` reproducibly builds pinned KiCad and the KiCad-backed
  native module against KiCad 10.0.4 source and Python 3.14.
* the linked backend reports
  `kicad-10.0.4-native-pybind11-kicad-0.1`.
* `Board.open()`, `Board.save()`, `pcbnew.LoadBoard()`, and `BOARD.Save()` work
  for the current golden board fixture.
* scaffold builds without board IO fail clearly.
* there is no alternate direct `.kicad_pcb` parser or board-file backend.

Default standalone native builds still leave real board IO disabled.
`Board.open()` and `pcbnew.LoadBoard()` intentionally fail there. Use
`scripts/build.sh` for the reproducible KiCad-linked build.

### Next: Board IO Coverage and `pcbnew` Compatibility

Grow the current minimal proof of concept:

```python
import pybind11_kicad as kc

kc.initialize()

board = kc.Board.open("input.kicad_pcb")
print(board.footprints())
board.save("output.kicad_pcb")
```

Success criteria:

* native module imports in Python for each supported platform
* target KiCad backend initializes without GUI on each supported platform
* Kikakuka can use the backend in headless mode from normal Python
* more KiCad board fixtures can be opened, inspected, saved, and reopened
* saved board can be opened by official KiCad
* KiKit and Kikakuka tests identify and drive the next `pcbnew` compatibility
  methods
* Kikakuka outline discovery can use `BOARD.GetDrawings()` and drawing
  `GetLayer()`, `GetWidth()`, and `GetBoundingBox()` wrappers
* Kikakuka-controlled headless mode requires only the pinned target KiCad
  backend artifacts, not the KiCad GUI or KiCad's bundled Python

### Later: `pcbnew` Board Compatibility

Add `pcbnew` compatibility for open/save/list footprints:

```python
import pcbnew

board = pcbnew.LoadBoard("input.kicad_pcb")

for fp in board.GetFootprints():
    print(fp.GetReference())

board.Save("output.kicad_pcb")
```

Success criteria:

* `pcbnew` import works (already implemented)
* common existing scripts can open a board
* footprints can be inspected
* board can be saved
* unsupported GUI calls fail clearly

### Later: Generated Geometry

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

pybind11-kicad will use:

```text
pybind11 + pinned target KiCad build artifacts + pybind11-kicad-owned facade API
```

pybind11-kicad will also provide:

```text
optional pcbnew compatibility shim for common existing scripts
```

pybind11-kicad will avoid:

```text
pybind11 directly exposing KiCad's raw C++ API
making Kikakuka-controlled headless mode depend on a user-installed KiCad build
depending on IPC for normal offline file operations
trying to recreate the full pcbnew API
using the compatibility shim as the primary internal API
```

This is not KiCad’s officially supported integration model, but it is a
reasonable controlled architecture for Kikakuka-controlled headless mode because
the backend accepts only pinned supported target KiCad artifacts. KiKit's
KiCad-hosted plugin mode remains conceptually separate: it is hosted by KiCad
when invoked inside KiCad.
