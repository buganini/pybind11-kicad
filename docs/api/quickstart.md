# Quickstart

Build the pinned KiCad tree and native module:

```sh
scripts/build.sh
```

For a faster rebuild after KiCad is already configured, build only the native
module:

```sh
scripts/build.sh pybind11-kicad
```

Run Python through the project environment:

```sh
scripts/build.sh python
```

The `python` subcommand creates or reuses `$PWD/env`, prepends `$PWD/env/bin` to
`PATH`, and sets `PYTHONPATH` so both `python/` and the native module directory
are importable.

Smoke check:

```sh
scripts/build.sh python -c 'import pybind11_kicad as kc; print(kc.backend_version())'
```

Open and save a board:

```sh
scripts/build.sh python -c 'import pybind11_kicad as kc; b = kc.Board.open("tests/golden/simple_board.kicad_pcb"); print(len(b.footprints())); b.save("tmp/quickstart.kicad_pcb")'
```

Run the repository tests:

```sh
scripts/test-kicad.sh
```

Run the Kikakuka compatibility panel comparison:

```sh
scripts/run-kikakuka-test.sh
```

The Kikakuka runner compares output from the local compatibility layer with a
golden Python runner. It currently covers all Kikakuka v6.6 test panels plus the
Gerber conversion sample.

## Environment Assumptions

The backend is linked against the self-built pinned KiCad tree, not against a
user-installed KiCad application. The default locations are:

```text
KiCad source:       tmp/kicad
KiCad build:        tmp/pybind11-kicad-10-build
Native module dir:  tmp/pybind11-kicad-10-build/pybind11-kicad-native
Python env:         env
```

Use the variables documented in the README to override these paths. Keep the
same Python 3.14 interpreter family for the build and runtime environment.
