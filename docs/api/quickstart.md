# Quickstart

This page is the command-level build and test guide. The README is the project
overview and status page.

## Prerequisites

Required for all platforms:

* Python 3.14
* Git and network access, unless `tmp/kicad` already contains the pinned KiCad
  checkout
* CMake 3.20 or newer
* Ninja, used as the single cross-platform CMake generator for this project
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

You do not have to manually install KiCad for this backend. The build fetches
or reuses the pinned KiCad source checkout in `tmp/kicad` and links against the
matching self-built KiCad artifacts. KiCad's bundled Python is not used.

## Windows Shell Setup

Install:

* Visual Studio 2022 Build Tools with the Desktop development with C++ workload
* Python 3.14 x64
* Git
* a POSIX shell with `sh` or `bash`, such as Git Bash, MSYS2, Cygwin, or
  another compatible shell
* CMake and Ninja
* vcpkg, with `VCPKG_ROOT` set
* the KiCad dependency set with
  `deps/kicad-10.0.4/install-windows-vcpkg.ps1`

Run the build from a Visual Studio developer environment so MSVC is active, and
use the POSIX shell only to execute the script:

```powershell
$env:PYBIND11_KICAD_PYTHON = "C:\Users\you\AppData\Local\Programs\Python\Python314\python.exe"
$env:VCPKG_ROOT = "C:\src\vcpkg"
$env:CMAKE_GENERATOR = "Ninja"
$env:CMAKE_BUILD_PARALLEL_LEVEL = "8"
```

```sh
bash scripts/run.sh
```

## Build

With no subcommand, `scripts/run.sh` builds pinned KiCad and the
pybind11-kicad native module:

```sh
scripts/run.sh
```

To build the pieces explicitly, use the same pinned KiCad build tree and choose
the named subcommand.

Build KiCad:

```sh
scripts/run.sh build-kicad
```

This fetches or reuses `tmp/kicad`, verifies the pinned KiCad commit,
configures KiCad, and builds the KiCad executable set, including the kifaces
that KiCad CLI loads for board and schematic-backed commands.

Invoke the self-built KiCad CLI through the build script so the executable path
is resolved for the current platform:

```sh
scripts/run.sh kicad-cli version
```

Build pybind11-kicad:

```sh
scripts/run.sh build-pybind11-kicad
```

This builds the native extension at
`tmp/pybind11-kicad-10-build/pybind11-kicad-native/pybind11_kicad_native.so`.
For checkout-based development, the Python package itself is used directly from
`python/`. The pybind11-kicad targets are explicit build targets, so a plain
KiCad build does not accidentally build the Python extension.

`scripts/run.sh` is the reproducible build entry point. It:

* fetches or reuses the pinned shallow KiCad 10.0.4 source checkout
* verifies commit `f7414d419cae5df2d00e7eaacb16fc0e803799bc`
* requires the active interpreter to be Python 3.14
* configures KiCad as the top-level CMake project
* adds `native/` at the end of that KiCad configure pass with
  `CMAKE_PROJECT_TOP_LEVEL_INCLUDES` and `cmake_language(DEFER)`
* builds KiCad from the pinned source tree, including the CLI-required
  `pcbnew` and `eeschema` kifaces
* configures KiCad's own `KICAD_IPC_API` option on so the self-built CLI and
  CLI-required kifaces match KiCad's expected link surface
* builds `pybind11_kicad_native` against KiCad's internal headless board/common
  IO targets instead of the full `pcbnew` editor kiface
* runs Python with the checkout package and built native module importable when
  invoked as `scripts/run.sh python`

`KICAD_IPC_API` is a KiCad build option needed by KiCad's own CLI/kiface
targets. pybind11-kicad still avoids IPC for normal board file IO.

Use configure-only mode when checking CMake changes:

```sh
scripts/run.sh configure-kicad
```

For a metadata-only native scaffold without KiCad-linked board IO:

```sh
cmake -S native -B $PWD/tmp/pybind11-kicad-native-10-check \
  -DPYBIND11_KICAD_KICAD_SOURCE_DIR=$PWD/tmp/kicad
cmake --build $PWD/tmp/pybind11-kicad-native-10-check \
  --target pybind11_kicad_native
```

## Python Environment

Run Python with the built backend:

```sh
scripts/run.sh python
```

Example smoke check:

```sh
scripts/run.sh python -c 'import pybind11_kicad as kc; print(kc.backend_version()); print(kc.native_available())'
```

Open and save a board:

```sh
scripts/run.sh python -c 'import pybind11_kicad as kc; b = kc.Board.open("tests/golden/simple_board.kicad_pcb"); print(len(b.footprints())); b.save("tmp/quickstart.kicad_pcb")'
```

The `python` subcommand uses `PYBIND11_KICAD_PYTHON` or `python3.14`, then
creates or reuses the ignored project virtual environment at `$PWD/env`. It
execs `$PWD/env/bin/python`, prepends `$PWD/env/bin` to `PATH`, and sets
`PYTHONPATH` to include this checkout's `python/` directory and the native
module directory from the pinned build tree. Pass `-m`, `-c`, or a script path
after `python` to run commands in that environment.

## Paths And Inputs

The default paths are:

```text
KiCad source:       $PWD/tmp/kicad
KiCad build:        $PWD/tmp/pybind11-kicad-10-build
Native module dir:  $PWD/tmp/pybind11-kicad-10-build/pybind11-kicad-native
KiCad CLI:          use scripts/run.sh kicad-cli [args...]
Python env:         $PWD/env
```

The pinned inputs are:

```text
KiCad:         10.0.4 (f7414d419cae5df2d00e7eaacb16fc0e803799bc)
Python:        3.14
```

Common overrides:

```sh
PYBIND11_KICAD_PYTHON=/opt/homebrew/bin/python3.14 \
PYBIND11_KICAD_BUILD_DIR=$PWD/tmp/pybind11-kicad-10-build \
  scripts/run.sh
```

## Dependency Helpers

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

## Tests

Run the repository tests:

```sh
scripts/test-kicad.sh
```

Run the self-built KiCad CLI smoke test. It checks the pinned CLI version and
exports a small board fixture to PDF:

```sh
scripts/test-kicad-cli.sh
```

Run the vendored KiCad 10 legacy SWIG `pcbnew` pytest suite:

```sh
scripts/run.sh python -m pip install -r compat/pcbnewswig-test-requirements.txt
scripts/run-pcbnewswig-tests.sh
```

The tests are copied from KiCad 10.0.4's `qa/tests/pcbnewswig` tree, with only
the referenced board fixtures copied into `tests/upstream/data/pcbnew`. They
are kept in this repository because future KiCad versions may remove the KiCad
10 legacy SWIG test suite while this project still needs those compatibility
checks for the `pcbnew` shim.

Run KiKit's upstream unit tests from the pinned compatibility source:

```sh
python3.14 -m venv $PWD/tmp/pybind11-kicad-kikit-test-venv
$PWD/tmp/pybind11-kicad-kikit-test-venv/bin/python -m pip install -r compat/kikit-test-requirements.txt
PATH=$PWD/tmp/pybind11-kicad-kikit-test-venv/bin:$PATH scripts/run-kikit-tests.sh unit
```

The wrapper reads `compat/kikit.lock`, fetches that exact KiKit commit into
`${PYBIND11_KICAD_COMPAT_DIR}` or the default ignored cache at `.cache/kikit`,
and reuses the cached checkout on later runs. It exports this checkout's
`python/` directory and the pinned KiKit source on `PYTHONPATH`, and then calls
KiKit's own test targets.

Use `KIKIT_SOURCE=/path/to/KiKit` to run against an existing checkout, but it
must already be at the pinned commit in `compat/kikit.lock`.

`compat/kikit-test-requirements.txt` includes `wxPython` because KiKit's CLI
imports `wx` even for headless command execution. That dependency is acceptable
for KiKit compatibility testing; it does not change the production board-file
decision that real board IO must be handled by the native KiCad-backed backend.

Run KiKit's configured Bats-based system tests:

```sh
PYBIND11_KICAD_KIKIT_PYTHON=$PWD/tmp/pybind11-kicad-kikit-test-venv/bin/python \
    PATH=$PWD/tmp/pybind11-kicad-kikit-test-venv/bin:$PATH \
    scripts/run-kikit-tests.sh system
```

Those tests require `bats` on `PATH`, the KiCad-backed board IO build, and real
board files. The wrapper creates temporary `kikit` and `kikit-info` launchers
bound to the active Python interpreter so the system tests do not accidentally
pick up a machine-global KiCad Python wrapper. It enumerates KiKit's upstream
system Bats files and intentionally skips `stencil.bats`; those cases require
OpenSCAD stencil generation and are outside this project's required `pcbnew`
compatibility target.

Set `PYBIND11_KICAD_KIKIT_PYTHON` to force a specific Python interpreter and
`PYBIND11_KICAD_NATIVE_BUILD_DIR` to force a specific native module directory.
The default native module path is
`$PWD/tmp/pybind11-kicad-10-build/pybind11-kicad-native`.

Run the Kikakuka compatibility panel comparison:

```sh
PYBIND11_KICAD_NATIVE_BUILD_DIR=$PWD/tmp/pybind11-kicad-10-build/pybind11-kicad-native \
PYBIND11_KICAD_KIKAKUKA_SOURCE=../kikakuka \
  scripts/run-kikakuka-test.sh
```

The script installs local test dependencies from the configured Kikakuka
checkout's `requirements.txt` through `scripts/run.sh python`, then installs
the Kikakuka test extra dependency `wxPython==4.2.5`. This project also declares
that pin in its `kikakuka-test` packaging extra. Kikakuka imports `wx` during
startup but does not list `wxPython` in its own requirements file. The script
then makes this repository's `python/` directory importable so Kikakuka can
import the local `pcbnew` compatibility module. Set
`PYBIND11_KICAD_NATIVE_BUILD_DIR` to the native module directory produced by
the board IO build instead of an installed `pybind11_kicad_native` extension.
Set `PYBIND11_KICAD_KIKAKUKA_INSTALL_DEPS=0` to skip the install step, or
`PYBIND11_KICAD_KIKAKUKA_EXTRA_REQUIREMENT=` to skip only the extra dependency.

`scripts/run-kikakuka-test.sh` enumerates all `.kikit_pnl` files under the
configured Kikakuka samples directory, also runs the `samples/gerber/export`
conversion case, generates golden outputs with the configured SWIG-wrapper
Python runner, and compares the pybind11-kicad outputs against those goldens.
