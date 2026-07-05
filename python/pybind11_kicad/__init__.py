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
    kicad_dir: str | None = None
    resource_dir: str | None = None
    config_dir: str | None = None


_runtime_config = RuntimeConfig()
TARGET_KICAD_MAJOR = 10
TARGET_KICAD_VERSION = "10.0.4"


def initialize(
    kicad_dir: str | None = None,
    resource_dir: str | None = None,
    config_dir: str | None = None,
) -> RuntimeConfig:
    """Initialize the backend runtime.

    This records the requested paths for the Python API and forwards them to
    the native extension once the native initializer exists.
    """

    global _runtime_config
    _runtime_config = RuntimeConfig(
        kicad_dir=kicad_dir,
        resource_dir=resource_dir,
        config_dir=config_dir,
    )
    if _native is not None and hasattr(_native, "initialize"):
        _native.initialize(
            kicad_dir=kicad_dir,
            resource_dir=resource_dir,
            config_dir=config_dir,
        )
    return _runtime_config


def runtime_config() -> RuntimeConfig:
    return _runtime_config


def backend_version() -> str:
    if _native is None:
        return "kicad-10.0.4-native-extension-unavailable"
    return _native.backend_version()


def target_kicad_major() -> int:
    return TARGET_KICAD_MAJOR


def target_kicad_version() -> str:
    return TARGET_KICAD_VERSION


def native_available() -> bool:
    return _native is not None


def native_import_error() -> ImportError | None:
    return _native_import_error


def load_footprint(
    library_path: str | Path,
    footprint_name: str,
    preserve_uuid: bool = False,
) -> Any:
    native = _require_native()
    return native.load_footprint(str(library_path), footprint_name, bool(preserve_uuid))


def seed_kiid_generator(seed: int) -> None:
    native = _require_native()
    native.seed_kiid_generator(int(seed))


class Board:
    """Python facade for the native KiCad board object."""

    def __init__(self, native_board: Any):
        self._native_board = native_board

    @classmethod
    def open(cls, path: str | Path) -> "Board":
        native = _require_native()
        return cls(native.Board.open(str(path)))

    @classmethod
    def create(cls, path: str | Path) -> "Board":
        native = _require_native()
        return cls(native.Board.create(str(path)))

    def save(self, path: str | Path) -> None:
        self._native_board.save(str(path))

    def design_settings(self) -> Any:
        return self._native_board.design_settings()

    def set_board_thickness(self, thickness: int) -> None:
        self._native_board.set_board_thickness(thickness)

    def set_aux_origin(self, origin: tuple[int, int]) -> None:
        self._native_board.set_aux_origin(_native_int_point(origin))

    def title_block(self) -> Any:
        return self._native_board.title_block()

    def set_title_block(self, title_block: Any) -> None:
        self._native_board.set_title_block(title_block)

    def copper_layer_count(self) -> int:
        return self._native_board.copper_layer_count()

    def set_copper_layer_count(self, count: int) -> None:
        self._native_board.set_copper_layer_count(count)

    def enabled_layers(self) -> list[int]:
        return list(self._native_board.enabled_layers())

    def set_enabled_layers(self, layers: list[int]) -> None:
        self._native_board.set_enabled_layers(layers)

    def get_layer_name(self, layer_id: int) -> str:
        return self._native_board.get_layer_name(layer_id)

    def get_layer_id(self, name: str) -> int:
        return self._native_board.get_layer_id(name)

    def set_layer_name(self, layer_id: int, name: str) -> bool:
        return self._native_board.set_layer_name(layer_id, name)

    def drawings(self) -> list[Any]:
        return self._native_board.drawings()

    def texts(self) -> list[Any]:
        return self._native_board.texts()

    def zones(self) -> list[Any]:
        return self._native_board.zones()

    def tracks(self) -> list[Any]:
        return self._native_board.tracks()

    def vias(self) -> list[Any]:
        return self._native_board.vias()

    def footprints(self) -> list[Any]:
        return self._native_board.footprints()

    def add_drawing(
        self,
        *,
        layer: int,
        shape: int,
        width: int,
        start: tuple[int, int],
        end: tuple[int, int],
        center: tuple[int, int] = (0, 0),
        mid: tuple[int, int] | None = None,
        radius: int = 0,
        filled: bool = False,
        polygon_points: list[tuple[int, int]] | None = None,
        uuid: str = "",
    ) -> None:
        native = _require_native()
        drawing = native.Drawing()
        drawing.layer = int(layer)
        drawing.shape = int(shape)
        drawing.width = int(width)
        drawing.start = _native_int_point(start)
        drawing.end = _native_int_point(end)
        drawing.center = _native_int_point(center)
        drawing.mid = _native_int_point(mid if mid is not None else center)
        drawing.radius = int(radius)
        drawing.filled = bool(filled)
        drawing.polygon_points = [_native_int_point(point) for point in (polygon_points or [])]
        drawing.uuid = uuid
        self._native_board.add_drawing(drawing)

    def remove_drawing(
        self,
        *,
        layer: int,
        shape: int,
        width: int,
        start: tuple[int, int],
        end: tuple[int, int],
        center: tuple[int, int] = (0, 0),
        mid: tuple[int, int] | None = None,
        radius: int = 0,
        filled: bool = False,
        polygon_points: list[tuple[int, int]] | None = None,
    ) -> bool:
        native = _require_native()
        drawing = native.Drawing()
        drawing.layer = int(layer)
        drawing.shape = int(shape)
        drawing.width = int(width)
        drawing.start = _native_int_point(start)
        drawing.end = _native_int_point(end)
        drawing.center = _native_int_point(center)
        drawing.mid = _native_int_point(mid if mid is not None else center)
        drawing.radius = int(radius)
        drawing.filled = bool(filled)
        drawing.polygon_points = [_native_int_point(point) for point in (polygon_points or [])]
        return bool(self._native_board.remove_drawing(drawing))

    def add_npth_hole(
        self,
        *,
        reference: str,
        position: tuple[int, int],
        drill_size: tuple[int, int],
        size: tuple[int, int],
        orientation_degrees: float = 0.0,
        uuid: str = "",
        pad_uuid: str = "",
    ) -> None:
        native = _require_native()
        spec = native.NpthSpec()
        spec.reference = reference
        spec.position = _native_int_point(position)
        spec.drill_size = _native_int_point(drill_size)
        spec.size = _native_int_point(size)
        spec.orientation_degrees = float(orientation_degrees)
        spec.uuid = uuid
        spec.pad_uuid = pad_uuid
        self._native_board.add_npth_hole(spec)

    def add_footprint(
        self,
        *,
        reference: str,
        value: str,
        fpid: str,
        layer: int,
        position: tuple[int, int],
        orientation_degrees: float,
        reference_visible: bool,
        value_visible: bool,
        fields: list[tuple[str, str, bool]] | None = None,
    ) -> None:
        native = _require_native()
        spec = native.FootprintSpec()
        spec.reference = reference
        spec.value = value
        spec.fpid = fpid
        spec.layer = int(layer)
        spec.position = _native_int_point(position)
        spec.orientation_degrees = float(orientation_degrees)
        spec.reference_visible = bool(reference_visible)
        spec.value_visible = bool(value_visible)
        spec.fields = [
            _native_footprint_field_spec(name, value, visible)
            for name, value, visible in (fields or [])
        ]
        self._native_board.add_footprint(spec)

    def add_footprint_native(self, spec: Any) -> None:
        self._native_board.add_footprint(spec)

    def add_footprint_clone(self, source: Any, spec: Any) -> None:
        self._native_board.add_footprint_clone(source, spec)

    def add_text(
        self,
        *,
        text: str,
        layer: int,
        position: tuple[int, int],
        size: tuple[int, int],
        thickness: int,
        angle_degrees: float,
        h_justify: int,
        v_justify: int,
        mirrored: bool,
        uuid: str = "",
    ) -> None:
        spec = _native_text_spec(
            text=text,
            layer=layer,
            position=position,
            size=size,
            thickness=thickness,
            angle_degrees=angle_degrees,
            h_justify=h_justify,
            v_justify=v_justify,
            mirrored=mirrored,
            uuid=uuid,
        )
        self._native_board.add_text(spec)

    def remove_text(
        self,
        *,
        text: str,
        layer: int,
        position: tuple[int, int],
        size: tuple[int, int],
        thickness: int,
        angle_degrees: float,
        h_justify: int,
        v_justify: int,
        mirrored: bool,
        uuid: str = "",
    ) -> bool:
        spec = _native_text_spec(
            text=text,
            layer=layer,
            position=position,
            size=size,
            thickness=thickness,
            angle_degrees=angle_degrees,
            h_justify=h_justify,
            v_justify=v_justify,
            mirrored=mirrored,
            uuid=uuid,
        )
        return bool(self._native_board.remove_text(spec))

    def add_track(
        self,
        *,
        net: str,
        layer: str,
        start: tuple[float, float],
        end: tuple[float, float],
        width: float,
        uuid: str = "",
    ) -> None:
        native = _require_native()
        spec = native.TrackSpec()
        spec.net = net
        spec.layer = layer
        spec.start = _native_point(start)
        spec.end = _native_point(end)
        spec.width_mm = width
        spec.uuid = uuid
        self._native_board.add_track(spec)

    def add_via(
        self,
        *,
        net: str,
        position: tuple[float, float],
        drill: float,
        diameter: float,
        layers: tuple[str, ...] = ("F.Cu", "B.Cu"),
        uuid: str = "",
    ) -> None:
        _ = layers
        native = _require_native()
        spec = native.ViaSpec()
        spec.net = net
        spec.position = _native_point(position)
        spec.drill_mm = drill
        spec.diameter_mm = diameter
        spec.uuid = uuid
        self._native_board.add_via(spec)

    def add_track_item(self, spec: Any) -> None:
        self._native_board.add_track_item(spec)

    def add_via_item(self, spec: Any) -> None:
        self._native_board.add_via_item(spec)

    def add_zone_item(self, spec: Any) -> None:
        self._native_board.add_zone_item(spec)

    def remove_zone_item(self, spec: Any) -> bool:
        return self._native_board.remove_zone_item(spec)

    def nets(self) -> Any:
        return self._native_board.nets()


def _native_point(position: tuple[float, float]) -> Any:
    native = _require_native()
    point = native.Point()
    point.x_mm = position[0]
    point.y_mm = position[1]
    return point


def _native_int_point(position: tuple[int, int]) -> Any:
    native = _require_native()
    point = native.IntPoint()
    point.x = int(position[0])
    point.y = int(position[1])
    return point


def _native_footprint_field_spec(name: str, value: str, visible: bool) -> Any:
    native = _require_native()
    spec = native.FootprintFieldSpec()
    spec.name = name
    spec.value = value
    spec.visible = bool(visible)
    return spec


def _native_text_spec(
    *,
    text: str,
    layer: int,
    position: tuple[int, int],
    size: tuple[int, int],
    thickness: int,
    angle_degrees: float,
    h_justify: int,
    v_justify: int,
    mirrored: bool,
    uuid: str = "",
) -> Any:
    native = _require_native()
    spec = native.TextSpec()
    spec.text = text
    spec.layer = int(layer)
    spec.position = _native_int_point(position)
    spec.size = _native_int_point(size)
    spec.thickness = int(thickness)
    spec.angle_degrees = float(angle_degrees)
    spec.h_justify = int(h_justify)
    spec.v_justify = int(v_justify)
    spec.mirrored = bool(mirrored)
    spec.uuid = uuid
    return spec


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
    "load_footprint",
    "native_available",
    "native_import_error",
    "runtime_config",
    "target_kicad_major",
    "target_kicad_version",
]
