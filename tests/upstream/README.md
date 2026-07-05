# Upstream KiCad 10 Legacy SWIG Tests

This directory preserves KiCad 10's legacy `pcbnew` SWIG Python QA tests from
the pinned KiCad 10.0.4 source tree. They are vendored here because future
KiCad versions may remove the KiCad 10 legacy SWIG test suite while this
project still needs a compatibility target for the `pcbnew` shim.

Source paths:

* `tmp/kicad/qa/tests/pcbnewswig`
* `tmp/kicad/qa/data/pcbnew`

Only the board fixtures referenced by the SWIG tests are copied. The tests keep
their original `../data/pcbnew/...` fixture paths, so the runner executes from
the `pcbnewswig` test directory:

```sh
scripts/run-pcbnewswig-tests.sh
```
