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
import importlib.util
from collections import defaultdict


class _Prop:
    def __init__(self, value=None):
        self.value = value

    def set(self, value):
        changed = self.value != value
        self.value = value
        return changed


class _Binding:
    def __init__(self, owner, key):
        self.owner = owner
        self.key = key

    @property
    def value(self):
        if isinstance(self.owner, _StateObject):
            return getattr(self.owner, self.key)
        return self.owner[self.key]

    @value.setter
    def value(self, value):
        if isinstance(self.owner, _StateObject):
            setattr(self.owner, self.key, value)
        else:
            self.owner[self.key] = value

    def change(self, callback):
        return None

    def bind(self, getter, setter):
        self.value = getter()
        return None

    def emit(self):
        return None


class _StateObject:
    def __init__(self, values=None):
        object.__setattr__(self, "_values", types.SimpleNamespace())
        if values is not None:
            for key, value in vars(values).items():
                setattr(self, key, value)

    def __call__(self, key=None):
        if key is None:
            return None
        return _Binding(self, key)

    def __enter__(self):
        return self

    def __exit__(self, ex_type, value, traceback):
        if ex_type is None:
            return self
        return None

    def __getattr__(self, key):
        return getattr(self._values, key)

    def __setattr__(self, key, value):
        if key.startswith("_"):
            object.__setattr__(self, key, value)
        else:
            setattr(self._values, key, _state(value))

    def __eq__(self, other):
        return self._values == other

    def __repr__(self):
        return f"StateObject({self._values!r})"


class _StateList(list):
    def __call__(self, key=None):
        if key is None:
            return None
        return _Binding(self, key)

    def get(self, index, default=None):
        if 0 <= index < len(self):
            return self[index]
        return default

    def range(self):
        return range(len(self))


class _StateDict(dict):
    def __call__(self, key=None):
        if key is None:
            return None
        return _Binding(self, key)

    def __getattr__(self, key):
        return getattr(dict(self), key)


def _state(value):
    if isinstance(value, (dict, _StateDict)):
        return _StateDict(value)
    if isinstance(value, (list, _StateList)):
        return _StateList(value)
    return value


def _State(data=None, deep=False):
    if data is None:
        return _StateObject()
    if isinstance(data, list):
        return _StateList([_state(item) for item in data] if deep else data)
    if isinstance(data, dict):
        return _StateDict({key: _state(value) for key, value in data.items()} if deep else data)
    return _StateObject(data)


def _Unstate(data):
    if isinstance(data, _StateList):
        return [_Unstate(item) for item in data]
    if isinstance(data, _StateDict):
        return {key: _Unstate(value) for key, value in data.items()}
    return data


class _PUIView:
    def __init__(self, *args, **kwargs):
        self.args = args
        self.kwargs = kwargs
        self.frames = []

    def __enter__(self):
        return self

    def __exit__(self, ex_type, value, traceback):
        if ex_type is None:
            return self
        return None

    def content(self):
        return None

    def setup(self):
        return None

    def start(self):
        return None

    def quit(self):
        return None

    def close(self):
        return None

    def update(self, *args, **kwargs):
        return self.content()

    def sync(self):
        return self.update()

    def redraw(self):
        return self.sync()

    def run(self):
        self.sync()
        return self.start()

    def id(self, *args, **kwargs):
        return self

    def layout(self, *args, **kwargs):
        return self

    def style(self, *args, **kwargs):
        return self

    def grid(self, *args, **kwargs):
        return self

    def click(self, *args, **kwargs):
        return self

    def change(self, *args, **kwargs):
        return self

    def __getattr__(self, name):
        def _chain(*args, **kwargs):
            return self
        return _chain


class _Application(_PUIView):
    pass


class _Widget(_PUIView):
    pass


def _identity_decorator(func):
    return func


def _OpenFile(*args, **kwargs):
    return None


def _SaveFile(*args, **kwargs):
    return None


def _OpenDirectory(*args, **kwargs):
    return None


def _Critical(message, title=None):
    if title:
        print(f"{title}: {message}", file=sys.stderr)
    else:
        print(message, file=sys.stderr)


class _BaseTableAdapter:
    def clicked(self, *args, **kwargs):
        return None

    def dblclicked(self, *args, **kwargs):
        return None


class _BaseTreeAdapter:
    def clicked(self, *args, **kwargs):
        return None

    def dblclicked(self, *args, **kwargs):
        return None

    def expanded(self, *args, **kwargs):
        return None

    def collapsed(self, *args, **kwargs):
        return None


def _install_headless_pui_package():
    pui = types.ModuleType("PUI")
    pui.__doc__ = "Headless PUI shim for Kikakuka tests."
    pui.__version__ = "headless"

    interfaces = types.ModuleType("PUI.interfaces")
    interfaces.BaseTableAdapter = _BaseTableAdapter
    interfaces.BaseTreeAdapter = _BaseTreeAdapter
    interfaces.__all__ = ["BaseTableAdapter", "BaseTreeAdapter"]

    py_side = types.ModuleType("PUI.PySide6")
    py_side.__doc__ = "Headless PUI.PySide6 shim for Kikakuka tests."

    names = {
        "Application": _Application,
        "PUIView": _PUIView,
        "QtPUIView": _PUIView,
        "Prop": _Prop,
        "State": _State,
        "StateObject": _StateObject,
        "StateList": _StateList,
        "StateDict": _StateDict,
        "Unstate": _Unstate,
        "defaultdict": defaultdict,
        "PUI": _identity_decorator,
        "PUIApp": _identity_decorator,
        "PUI_BACKEND": "Headless",
        "OpenFile": _OpenFile,
        "SaveFile": _SaveFile,
        "OpenDirectory": _OpenDirectory,
        "Critical": _Critical,
        "Information": _Critical,
        "Warning": _Critical,
        "Confirm": lambda *args, **kwargs: False,
        "Prompt": lambda *args, **kwargs: None,
        "BaseTableAdapter": _BaseTableAdapter,
        "BaseTreeAdapter": _BaseTreeAdapter,
    }

    widget_names = [
        "Button",
        "Canvas",
        "Checkbox",
        "ComboBox",
        "ComboBoxItem",
        "Divider",
        "Grid",
        "HBox",
        "Image",
        "Label",
        "MDI",
        "Menu",
        "MenuItem",
        "Modal",
        "ProgressBar",
        "RadioButton",
        "Scroll",
        "Splitter",
        "Spacer",
        "Tab",
        "Table",
        "Text",
        "TextField",
        "ToolBar",
        "ToolButton",
        "Tree",
        "VBox",
        "Window",
    ]
    names.update({name: _Widget for name in widget_names})

    for name, value in names.items():
        setattr(pui, name, value)
        setattr(py_side, name, value)

    pui.interfaces = interfaces
    pui.PySide6 = py_side
    py_side.__all__ = [name for name in vars(py_side) if not name.startswith("_")]
    pui.__all__ = [name for name in vars(pui) if not name.startswith("_")]

    sys.modules["PUI"] = pui
    sys.modules["PUI.interfaces"] = interfaces
    sys.modules["PUI.PySide6"] = py_side
    return pui


def _install_optional_headless_modules():
    if "pypdfium2" not in sys.modules and importlib.util.find_spec("pypdfium2") is None:
        pdfium = types.ModuleType("pypdfium2")
        pdfium.version = types.SimpleNamespace(PYPDFIUM_INFO="headless")

        class _PdfDocument:
            def __init__(self, *args, **kwargs):
                raise RuntimeError("pypdfium2 is unavailable in the Kikakuka headless panel export test.")

        pdfium.PdfDocument = _PdfDocument
        sys.modules["pypdfium2"] = pdfium

    if "cv2" not in sys.modules and importlib.util.find_spec("cv2") is None:
        cv2 = types.ModuleType("cv2")
        cv2.__version__ = "headless"
        cv2.COLOR_RGBA2RGB = 0
        cv2.COLOR_BGR2BGRA = 1
        cv2.COLOR_GRAY2BGRA = 2
        cv2.COLOR_BGRA2GRAY = 3
        cv2.COLOR_BGR2GRAY = 4
        cv2.IMREAD_UNCHANGED = -1
        cv2.IMREAD_GRAYSCALE = 0
        cv2.THRESH_BINARY = 0

        def _unavailable(*args, **kwargs):
            raise RuntimeError("cv2 is unavailable in the Kikakuka headless panel export test.")

        cv2.imread = _unavailable
        cv2.imwrite = _unavailable
        cv2.cvtColor = _unavailable
        cv2.min = _unavailable
        cv2.max = _unavailable
        cv2.absdiff = _unavailable
        cv2.threshold = _unavailable
        cv2.GaussianBlur = _unavailable
        cv2.split = _unavailable
        cv2.merge = _unavailable
        sys.modules["cv2"] = cv2

    if "git" not in sys.modules:
        git = types.ModuleType("git")

        def _unavailable(*args, **kwargs):
            raise RuntimeError("git history access is unavailable in the Kikakuka headless panel export test.")

        git.repo = _unavailable
        git.log = _unavailable
        git.checkout = _unavailable
        sys.modules["git"] = git


if os.environ.get("PYBIND11_KICAD_KIKAKUKA_HEADLESS_PUI") == "1":
    _install_optional_headless_modules()

    try:
        import PUI as _real_pui
    except ImportError:
        _real_pui = None

    if _real_pui is None:
        _install_headless_pui_package()
    elif "PUI.PySide6" not in sys.modules:
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
