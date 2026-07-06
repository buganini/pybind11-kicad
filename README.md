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

pybind11-kicad exposes a headless KiCad board library through one native
backend. The current target is KiCad 10.0.4 with Python 3.14.

There are two Python surfaces:

* `pybind11_kicad`: the clean API owned by this project.
* `pcbnew`: a compatibility shim for SWIG-era scripts such as KiKit and
  Kikakuka.

The clean API is the preferred surface for new code. The `pcbnew` module exists
to run existing scripts against the same native backend; it is not an
independent parser or board model. For compatibility work, the vendored KiCad
10 legacy SWIG `pcbnew` tests are the primary contract because this project is
trying to replace that API surface.

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
* the current target is pinned to [KiCad 10.0.4](#version-targets), and the
  distribution/backend major-version name is `pybind11-kicad-native-10`.
* the current target Python version is [Python 3.14](#version-targets).

Current compatibility test status, last checked on 2026-07-06: the configured
compatibility suite passes.

| Suite | Passed | Failed | Skipped | Notes |
| --- | ---: | ---: | ---: | --- |
| Local unit tests | 9 | 0 | 0 | Python import surface, SWIG constants, and native board IO smoke coverage |
| KiCad 10 legacy SWIG `pcbnew` tests | 22 | 0 | 0 | Vendored from KiCad 10.0.4 |
| KiKit upstream unit tests | 31 | 0 | 0 | Pinned KiKit checkout |
| KiKit upstream system tests | 38 | 0 | 1 | Configured run excludes `stencil.bats`; one upstream PcbDraw case is skipped |
| Kikakuka golden comparisons | 9 | 0 | 0 | All `.kikit_pnl` samples plus Gerber conversion |
| Configured total | 109 | 0 | 1 | |

Out-of-scope coverage:

| Suite | Passed | Failed | Skipped | Notes |
| --- | ---: | ---: | ---: | --- |
| KiKit upstream `stencil.bats` | 0 | 0 | 3 | Not part of this project's required compatibility target; these cases exercise OpenSCAD stencil generation rather than `pcbnew` compatibility |

## Documentation

* [Quickstart](docs/api/quickstart.md): build the backend, run Python with the
  right environment, and run the compatibility suites.
* [pybind11-kicad API](docs/api/pybind11-kicad.md): public `pybind11_kicad`
  objects, operations, units, and failure modes.
* [pcbnew Compatibility](docs/api/pcbnew.md): supported legacy
  compatibility behavior and known boundaries.
* [Porting Notes](docs/api/porting-notes.md): maintainer notes for coordinate,
  angle, arc, and SWIG porting behavior.
* [Compatibility](docs/api/compatibility.md): current implemented
  and tested API areas plus SWIG alignment rules.

## Version Targets

Current pinned targets:

```text
KiCad version: 10.0.4
KiCad major:   10
Python:        3.14
Backend name:  pybind11-kicad-native-10
```

The target KiCad major is part of the backend package identity. Retargeting to a
future KiCad major should update the build variables, dependency scripts, docs,
and compatibility matrix together.

## Project Process

This project is human-directed and AI-coded. The human maintainer sets the
goals, technical direction, acceptance criteria, and final review decisions;
AI systems perform much of the implementation and documentation work under
that direction.

## License

pybind11-kicad is licensed under GPL-3.0-or-later, following KiCad's license.
See `LICENSE`.

## Build And Test

The executable build, environment, and compatibility-test commands live in
[Quickstart](docs/api/quickstart.md). Keep command-level changes there so the
README remains the project overview, status, and design record instead of a
second build manual.

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

## Compatibility Scope

The detailed `pcbnew` compatibility contract lives in
[pcbnew Compatibility](docs/api/pcbnew.md). Current status and SWIG
comparison rules live in [Compatibility](docs/api/compatibility.md).

The README should not duplicate method inventories, compatibility tiers, or
consumer-specific API audits. Its compatibility rule is only this: the `pcbnew`
shim is backed by the same pinned native backend, and every documented supported
`pcbnew` name should follow KiCad 10 SWIG behavior unless the compatibility
docs explicitly mark it partial, project-only, or unsupported.

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

Avoid exposing raw KiCad internals directly:

```python
# Avoid designing the new core API around this style.
import pybind11_kicad as kc

board = kc.BOARD()
track = kc.PCB_TRACK(board)
```

The goal is not to recreate KiCad's entire `pcbnew` surface with alternate
semantics. The compatibility surface is documented in
[pcbnew Compatibility](docs/api/pcbnew.md).

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

fp.reference
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

The `pcbnew` compatibility layer preserves SWIG unit helpers such as `FromMM()`
and `ToMM()` where supported; see
[pcbnew Compatibility](docs/api/pcbnew.md#units-and-values).

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

### Real KiCad Validation

Optional but useful:

* open output in official KiCad GUI
* run `kicad-cli` DRC if available
* export Gerbers
* compare with expected generated files

This can be CI optional because it may require a full KiCad runtime.

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

* do not create alternate semantics for SWIG names
* grow the SWIG-compatible surface deliberately through the compatibility docs
  and matrix
* expose only tested offline APIs first
* make unsupported calls fail clearly
* keep the clean pybind11-kicad API as the real core API

### Risk: Full KiCad Behavior Is Hard to Embed

Mitigation:

* do not require DRC/router/zone refill in phase 1
* use optional CLI/GUI validation for advanced features
* implement only needed subset first

## Non-goals

The native backend is not intended to:

* claim complete KiCad editor/GUI `pcbnew` parity before it is implemented and
  tested
* make Kikakuka-controlled headless mode depend on unsupported or unpinned KiCad
  artifacts
* expose KiCad internals directly to Python
* become a general KiCad SDK
* replace KiCad IPC for normal KiCad plugins
* maintain an alternate direct `.kicad_pcb` parser or board-file backend
* reimplement the full KiCad editor
* support KiCad GUI action plugins in offline mode

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
trying to recreate the full pcbnew API with alternate semantics
using the compatibility shim as the primary internal API
```

This is not KiCad’s officially supported integration model, but it is a
reasonable controlled architecture for Kikakuka-controlled headless mode because
the backend accepts only pinned supported target KiCad artifacts. KiKit's
KiCad-hosted plugin mode remains conceptually separate: it is hosted by KiCad
when invoked inside KiCad.
