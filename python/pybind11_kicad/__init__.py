"""Headless KiCad API backed by the pybind11 native extension."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any


try:
    import pybind11_kicad_native as _native
except ImportError as import_error:
    _native = None
    _native_import_error = import_error
else:
    _native_import_error = None


if _native is not None and hasattr(_native, "BackendUnavailableError"):
    BackendUnavailableError = _native.BackendUnavailableError
else:
    class BackendUnavailableError(RuntimeError):
        """Raised when the native KiCad backend cannot be used."""


@dataclass(frozen=True)
class RuntimeConfig:
    resource_dir: str | None = None
    config_dir: str | None = None


_runtime_config = RuntimeConfig()
TARGET_KICAD_MAJOR = 10
TARGET_KICAD_VERSION = "10.0.x"


def initialize(resource_dir: str | None = None, config_dir: str | None = None) -> RuntimeConfig:
    """Initialize the backend runtime.

    This records the requested paths for the Python API and forwards them to
    the native extension once the native initializer exists.
    """

    global _runtime_config
    _runtime_config = RuntimeConfig(resource_dir=resource_dir, config_dir=config_dir)
    if _native is not None and hasattr(_native, "initialize"):
        _native.initialize(resource_dir=resource_dir, config_dir=config_dir)
    return _runtime_config


def runtime_config() -> RuntimeConfig:
    return _runtime_config


def backend_version() -> str:
    if _native is None:
        return "kicad-10-native-extension-unavailable"
    return _native.backend_version()


def target_kicad_major() -> int:
    return TARGET_KICAD_MAJOR


def target_kicad_version() -> str:
    return TARGET_KICAD_VERSION


def native_available() -> bool:
    return _native is not None


def native_import_error() -> ImportError | None:
    return _native_import_error


class Board:
    """Python facade for the native KiCad board object."""

    def __init__(self, native_board: Any):
        self._native_board = native_board

    @classmethod
    def open(cls, path: str | Path) -> "Board":
        native = _require_native()
        return cls(native.Board.open(str(path)))

    def save(self, path: str | Path) -> None:
        self._native_board.save(str(path))

    def footprints(self) -> list[Any]:
        return self._native_board.footprints()

    def add_track(
        self,
        *,
        net: str,
        layer: str,
        start: tuple[float, float],
        end: tuple[float, float],
        width: float,
    ) -> None:
        native = _require_native()
        spec = native.TrackSpec()
        spec.net = net
        spec.layer = layer
        spec.start = _native_point(start)
        spec.end = _native_point(end)
        spec.width_mm = width
        self._native_board.add_track(spec)


def _native_point(position: tuple[float, float]) -> Any:
    native = _require_native()
    point = native.Point()
    point.x_mm = position[0]
    point.y_mm = position[1]
    return point


def _require_native() -> Any:
    if _native is not None:
        return _native
    raise BackendUnavailableError(
        "The pybind11_kicad_native extension is not built or is not on "
        "PYTHONPATH. Build the native pybind11 module and make it importable; "
        "there is intentionally no alternate board-file implementation."
    ) from _native_import_error


__all__ = [
    "Board",
    "BackendUnavailableError",
    "RuntimeConfig",
    "TARGET_KICAD_MAJOR",
    "TARGET_KICAD_VERSION",
    "backend_version",
    "initialize",
    "native_available",
    "native_import_error",
    "runtime_config",
    "target_kicad_major",
    "target_kicad_version",
]
