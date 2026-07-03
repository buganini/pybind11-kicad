# Compatibility Matrix

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

## Clean `pybind11_kicad` API

| Area | Status | Verification |
| --- | --- | --- |
| Runtime metadata | implemented, tested | `tests/test_clean_api.py` |
| Board open/save | implemented, tested | unit round trip |
| Board create | implemented, tested through `pcbnew.NewBoard` | `tests/test_pcbnew_compat.py` |
| Layers and enabled layer set | implemented | exercised by Kikakuka comparison |
| Nets | implemented | exercised by board reads/comparison |
| Drawings and text | implemented, tested | unit tests and Kikakuka panels |
| Footprints | implemented, tested | unit tests and Kikakuka panels |
| Pads and pad attributes | implemented, tested | NPTH metadata test and panels |
| Footprint fields | implemented, tested | unit tests and panels |
| Footprint metadata flags | implemented, tested | NPTH metadata test and panels |
| Tracks and vias | implemented, tested | semantic board comparison |
| Zones and zone fills | implemented, tested | zone-fill unit test and panels |
| UUID stability | unsupported as semantic guarantee | comparator ignores UUID text |

## `pcbnew` Compatibility

| Area | Status | Verification |
| --- | --- | --- |
| `LoadBoard`, `NewBoard`, `SaveBoard` | implemented, tested | unit tests and panels |
| Unit conversion helpers | implemented, tested | unit tests |
| Layer constants | implemented, tested | unit tests and panels |
| Shape, pad, via, zone constants | implemented, tested | unit tests |
| `VECTOR2I`, `BOX2I`, `EDA_ANGLE` | implemented, tested | unit tests |
| `BOARD.Add` / `BOARD.Remove` | implemented, tested | unit tests and panels |
| `PCB_SHAPE` line/arc/circle/rect/poly behavior | partial, tested for current needs | unit tests and panels |
| `PCB_TEXT` and `PCB_FIELD` | partial, tested for current needs | unit tests and panels |
| `FOOTPRINT`, `PAD`, `FootprintLoad` | partial, tested for current needs | unit tests and panels |
| NPTH mousebite footprint preservation | implemented, tested | unit tests and Kikakuka v6.6 panels |
| `ZONE` and `ZONE_FILLER` | partial, tested for current needs | unit tests and panels |
| Nets and net info | partial, tested through panels | Kikakuka comparison |
| Plot/export writers | partial or unsupported | not part of current golden pass |
| DRC report generation | unsupported | not implemented |
| GUI/editor state APIs | unsupported | unit tests check clear failure |
| KiCad action plugins | unsupported | headless backend boundary |

## KiKit And Kikakuka

| Target | Status | Verification |
| --- | --- | --- |
| KiKit pinned unit tests | implemented for current pin | `scripts/run-kikit-tests.sh unit` |
| KiKit Bats/system tests | available but separate | requires `bats` and broader setup |
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
