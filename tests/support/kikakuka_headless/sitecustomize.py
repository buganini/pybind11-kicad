"""Headless import shim for Kikakuka panel export tests.

Kikakuka imports ``PUI.PySide6`` before it checks whether it is running in
headless export mode.  The comparison tests exercise only the headless path, so
loading Qt is unnecessary and can fail on machines where the GUI stack is not
usable from the selected Python runner.
"""

from __future__ import annotations

import os
import sys
import types


if os.environ.get("PYBIND11_KICAD_KIKAKUKA_HEADLESS_PUI") == "1":
    try:
        import PUI as _real_pui
    except ImportError:
        _real_pui = None

    if _real_pui is not None and "PUI.PySide6" not in sys.modules:
        _shim = types.ModuleType("PUI.PySide6")
        _shim.__doc__ = "Headless PUI.PySide6 shim for Kikakuka tests."

        for _name in dir(_real_pui):
            if not _name.startswith("_"):
                setattr(_shim, _name, getattr(_real_pui, _name))

        class Application(_real_pui.PUIView):
            def __init__(self, *args, **kwargs):
                super().__init__()
                self.args = args
                self.kwargs = kwargs

            def start(self):
                return None

            def quit(self):
                return None

        def PUI(func):
            return func

        def PUIApp(func):
            return func

        def OpenFile(*args, **kwargs):
            return None

        def SaveFile(*args, **kwargs):
            return None

        def OpenDirectory(*args, **kwargs):
            return None

        def Critical(message, title=None):
            if title:
                print(f"{title}: {message}", file=sys.stderr)
            else:
                print(message, file=sys.stderr)

        _shim.Application = Application
        _shim.PUIView = _real_pui.PUIView
        _shim.PUI = PUI
        _shim.PUIApp = PUIApp
        _shim.PUI_BACKEND = "Headless"
        _shim.OpenFile = OpenFile
        _shim.SaveFile = SaveFile
        _shim.OpenDirectory = OpenDirectory
        _shim.Critical = Critical
        _shim.__all__ = [name for name in vars(_shim) if not name.startswith("_")]

        sys.modules["PUI.PySide6"] = _shim
        setattr(_real_pui, "PySide6", _shim)

    try:
        import pcbnew as _pcbnew
    except ImportError:
        _pcbnew = None

    if _pcbnew is not None and not hasattr(_pcbnew.FOOTPRINT, "GetFieldByName"):
        def _get_field_by_name(self, name):
            if hasattr(self, "HasField") and not self.HasField(name):
                self.SetField(name, "")
            return self.GetField(name)

        _pcbnew.FOOTPRINT.GetFieldByName = _get_field_by_name
