# pybind11-kicad API Documentation

pybind11-kicad exposes a headless KiCad board library through one native
backend. The current target is KiCad 10.0.4 with Python 3.14.

There are two Python surfaces:

* `pybind11_kicad`: the clean API owned by this project.
* `pcbnew`: a compatibility shim for SWIG-era scripts such as KiKit and
  Kikakuka.

The clean API is the preferred surface for new code. The `pcbnew` module exists
to run existing scripts against the same native backend; it is not an
independent parser or board model.

## Documents

* [Quickstart](quickstart.md): build the backend and run Python with the right
  environment.
* [Board API](board.md): public `pybind11_kicad` objects, operations, units, and
  failure modes.
* [pcbnew Compatibility](pcbnew-compat.md): supported compatibility behavior and
  known boundaries.
* [Compatibility Matrix](compatibility-matrix.md): current implemented and
  tested API areas.

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

## Stability

The project is still in bring-up. APIs documented here describe the current
intended direction, but only tested behavior should be treated as a compatibility
promise.

The strongest current compatibility signal is:

* repository unit tests pass with the KiCad-backed native backend
* all Kikakuka v6.6 test panels pass the golden comparison
* the Kikakuka Gerber conversion sample passes the golden comparison

UUIDs and textual file ordering are not stable API guarantees. Tests compare
semantic board content instead: geometry, fields, pad attributes, footprint
metadata, zones/fills, tracks, vias, and selected compatibility metadata.
