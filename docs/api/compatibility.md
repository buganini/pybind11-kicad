# Compatibility

This matrix documents the current API areas and their verification status. It
should be updated whenever a new KiCad target, Python target, or compatibility
test target is added.

Status labels:

* `implemented`: present in the current API surface
* `tested`: covered by repository unit tests or semantic golden comparison
* `partial`: present but intentionally incomplete
* `unsupported`: intentionally unavailable in the headless backend

## Build And Runtime

| Area | Status | Verification |
| --- | --- | --- |
| KiCad target 10.0.4 | implemented, tested | `scripts/build.sh`, `scripts/test-kicad.sh` |
| Python target 3.14 | implemented, tested | build script version check |
| Self-built KiCad backend | implemented, tested | native module links against pinned build tree |
| User-installed KiCad lookup | unsupported | design decision in README |
| Missing native backend failure | implemented, tested | unit tests check clear native failure |
| KiCad 10 legacy SWIG test suite | implemented, tested | `scripts/run-pcbnewswig-tests.sh`: 22 passed |
| Plain KiCad 10 SWIG constants | implemented, tested | 949 constants synced through `SWIG_CONSTANTS` |
| Broad SWIG API surface parity | partial | SWIG exposes roughly 1332 public names; shim currently exposes roughly 1018 |

## Clean `pybind11_kicad` API

| Area | Status | Verification |
| --- | --- | --- |
| Runtime metadata | implemented, tested | `tests/test_clean_api.py` |
| Board open/save | implemented, tested | unit round trip |
| Board create | implemented, tested through `pcbnew.NewBoard` | `tests/test_pcbnew_compat.py` |
| Layers and enabled layer set | implemented | exercised by Kikakuka comparison |
| Title block | implemented, tested | vendored SWIG tests |
| Nets | implemented | exercised by board reads/comparison |
| Drawings and text | implemented, tested | unit tests, SWIG tests, KiKit, and Kikakuka panels |
| Footprints | implemented, tested | unit tests and Kikakuka panels |
| Pads and pad attributes | implemented, tested | NPTH metadata test and panels |
| Footprint fields | implemented, tested | unit tests and panels |
| Footprint metadata flags | implemented, tested | NPTH metadata test and panels |
| Tracks and vias | implemented, tested | semantic board comparison |
| Zones and zone fills | implemented, tested | zone-fill unit test and panels |
| Deterministic UUID forwarding for copies | implemented, tested | KiKit preset dump tests |
| UUID ordering stability | unsupported as semantic guarantee | tests compare semantic board content |

## `pcbnew` Compatibility Status

Detailed compatibility behavior is documented in
[pcbnew Compatibility](pcbnew.md).

| Area | Status | Verification |
| --- | --- | --- |
| `LoadBoard`, `NewBoard`, `SaveBoard` | implemented, tested | unit tests and panels |
| Unit conversion helpers | implemented, tested | unit tests |
| Layer constants | implemented, tested | included in synced SWIG constants |
| Shape, pad, via, zone constants | implemented, tested | included in synced SWIG constants |
| `VECTOR2I`, `BOX2I`, `EDA_ANGLE` | implemented, tested | unit tests |
| `BOARD.Add` / `BOARD.Remove` | implemented, tested | unit tests and panels |
| `PCB_SHAPE` line/arc/circle/rect/poly behavior | partial, tested for current needs | unit tests and panels |
| `PCB_TEXT` and `PCB_FIELD` | partial, tested for current needs | unit tests and panels |
| `FOOTPRINT`, `PAD`, `FootprintLoad` | partial, tested for current needs | unit tests and panels |
| NPTH mousebite footprint preservation | implemented, tested | unit tests and Kikakuka v6.6 panels |
| `ZONE` and `ZONE_FILLER` | partial, tested for current needs | unit tests and panels |
| Nets and net info | partial, tested through panels | Kikakuka comparison |
| Connectivity queries | partial, tested | vendored SWIG tests |
| Dimensions | partial, tested for render path | KiKit system tests |
| Plot/export writers | partial facade, tested subset | KiKit system tests; full SWIG plotting semantics remain a gap |
| DRC report generation | partial facade, tested subset | KiKit system tests; full SWIG DRC semantics remain a gap |
| Settings manager/project helpers | partial facade | import and consumer smoke coverage |
| Project-only compatibility diagnostics | implemented, not SWIG parity | `CompatibilityLevel`, `RequireCompatibility` |
| GUI/editor state APIs | unsupported | unit tests check clear failure |
| KiCad action plugins | unsupported | headless backend boundary |

## Compatibility Test Suites

| Target | Status | Verification |
| --- | --- | --- |
| KiCad 10 legacy SWIG `pcbnew` tests | primary compatibility contract | `scripts/run-pcbnewswig-tests.sh`: 22 passed |
| KiKit pinned unit tests | implemented for current pin | `scripts/run-kikit-tests.sh unit`: 31 passed |
| KiKit configured Bats/system tests | implemented for current pin | `scripts/run-kikit-tests.sh system`: 38 passed, 1 skipped |
| KiKit upstream stencil tests | out of scope | `stencil.bats` excluded; 3 out-of-scope cases |
| Kikakuka v6.6 panels | implemented, tested | `scripts/run-kikakuka-test.sh` |
| Kikakuka Gerber conversion sample | implemented, tested | `scripts/run-kikakuka-test.sh` |

The Kikakuka comparison is semantic, not byte-for-byte. It compares geometry,
fields, pad attributes, footprint metadata, zone fills, tracks, vias, and
generated mousebite metadata. It normalizes generated mousebite reference
ordering where Shapely/Python version differences can assign equivalent labels
to equivalent cuts in a different order.

## Update Rules

Update this matrix when:

* adding a public method or compatibility shim class
* changing the target KiCad or Python version
* adding a new compatibility target
* changing the semantic comparator
* moving an API from partial to tested

# SWIG Alignment

The `pcbnew` compatibility layer is judged against KiCad 10's legacy SWIG
`pcbnew` API. KiKit and Kikakuka are important consumers, but they are not the
API specification.

## Reference Sources

Use the pinned KiCad source tree as the reproducible SWIG reference:

* `tmp/kicad/pcbnew/python/swig/pcbnew.i`
* `tmp/kicad/pcbnew/python/swig/*.i`
* `tmp/kicad/common/swig/*.i`
* the C++ headers included by those SWIG interface files
* `tests/upstream/pcbnewswig`

The vendored SWIG tests are the primary executable contract currently in this
repo. A local KiCad SWIG Python runner can be used for broader comparison, but
the pinned KiCad 10 source files above are the source of truth.

## Alignment Rule

For every `pcbnew` name documented as supported, pybind11-kicad should match
KiCad 10 SWIG semantics:

* same public name and object model
* same units and coordinate conventions
* same visible mutation and save behavior
* same return-value shape where Python scripts can observe it
* same exception/failure meaning where possible in a headless backend

If a method is only partially implemented, the docs must say which SWIG behavior
is missing. A compatibility facade is a current gap, not the final API
semantics.

Project-only helpers may exist, but they must be labeled as project-only and
must not be counted as SWIG parity.

## Current Broad Comparison

A mechanical import comparison against the local SWIG Python runner shows the
scale of the remaining surface gap:

| Area | KiCad 10 SWIG | pybind11-kicad shim | Current meaning |
| --- | ---: | ---: | --- |
| Public top-level `pcbnew` names | 1332 | 1018 | all plain constants plus partial callable/object surface |
| Plain top-level constants | 949 | 949 | synced from KiCad 10 SWIG |
| `BOARD` public attributes/methods | 320 | 37 | common board IO subset |
| `FOOTPRINT` public attributes/methods | 313 | 46 | placement, fields, pads, flags subset |
| `PAD` public attributes/methods | 381 | 32 | KiKit/Kikakuka pad subset |
| `PCB_SHAPE` public attributes/methods | 260 | 39 | basic geometry subset |
| `PCB_TEXT` public attributes/methods | 220 | 39 | text geometry/field subset |
| `PCB_TRACK` public attributes/methods | 186 | 28 | track subset; `PCB_ARC` semantics remain a gap |
| `PCB_VIA` public attributes/methods | 304 | 24 | through-via subset |
| `ZONE` public attributes/methods | 280 | 32 | outline/fill preservation subset |
| `PCB_PLOT_PARAMS` public attributes/methods | 105 | 21 | KiKit facade subset |

These counts include inherited SWIG methods and implementation details such as
`thisown`, so they are a coarse progress signal rather than a method-by-method
todo list.

## Current Non-SWIG Names

These names are currently project-only and should not be described as SWIG API
parity:

```python
pcbnew.CompatibilityLevel()
pcbnew.RequireCompatibility("partial-pcbnew-v10")
```

Some extra aliases also exist for consumer convenience, such as
`EDA_UNITS_MILLIMETRES`, `EDA_UNITS_INCHES`, `DXF_UNITS_MILLIMETERS`,
`DIM_UNITS_MODE_MILLIMETRES`, `IU_PER_MM`, `IU_PER_MIL`, and
`SHAPE_T_POLYGON`. They should not be used as evidence that the SWIG API has
those names.

## Documentation Rule

The compatibility docs should describe SWIG-shaped behavior first:

* Use SWIG method names in examples.
* Prefer `board.Zones()` over project-only aliases such as `board.GetZones()`.
* Do not document pybind11-kicad helper methods as compatibility methods unless
  they exist in KiCad 10 SWIG.
* Mark GUI/editor state APIs as unsupported headless boundaries, not as
  alternate pybind11-kicad semantics.
* When a behavior is implemented only for KiKit/Kikakuka tests, document it as
  the tested subset of the SWIG behavior.

The final compatibility goal is for code behavior and documentation semantics to
match KiCad 10 SWIG for every supported `pcbnew` name.
