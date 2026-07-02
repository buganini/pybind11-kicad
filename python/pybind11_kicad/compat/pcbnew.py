"""Partial legacy ``pcbnew`` compatibility shim over the native backend."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable

import pybind11_kicad as kk


IU_PER_MM = 1_000_000


def LoadBoard(path: str | Path) -> "BOARD":
    return BOARD(kk.Board.open(path), filename=str(path))


def SaveBoard(path: str | Path, board: "BOARD") -> None:
    board.Save(path)


def GetBuildVersion() -> str:
    return "pybind11-kicad pcbnew compatibility layer, " + kk.backend_version()


def CompatibilityLevel() -> str:
    return "partial-pcbnew-v10"


def RequireCompatibility(level: str) -> None:
    if level != CompatibilityLevel():
        raise RuntimeError(f"required pcbnew compatibility {level!r}, have {CompatibilityLevel()!r}")


def FromMM(mm: float) -> int:
    return int(round(float(mm) * IU_PER_MM))


def ToMM(value: int | float) -> float:
    return float(value) / IU_PER_MM


@dataclass(frozen=True)
class VECTOR2I:
    x: int
    y: int


wxPoint = VECTOR2I


class BOARD:
    def __init__(self, board: kk.Board, filename: str = ""):
        self._board = board
        self._filename = filename

    def Save(self, path: str | Path | None = None) -> None:
        if path is None:
            if not self._filename:
                raise ValueError("board has no filename; pass an explicit save path")
            path = self._filename
        self._board.save(path)
        self._filename = str(path)

    def GetFileName(self) -> str:
        return self._filename

    def GetFootprints(self) -> list["FOOTPRINT"]:
        return [FOOTPRINT(footprint) for footprint in self._board.footprints()]

    def GetTracks(self) -> list["PCB_TRACK_VIEW"]:
        return [PCB_TRACK_VIEW(track) for track in _call_native_method(self._board, "tracks")]

    def GetDrawings(self) -> list[Any]:
        return _call_native_method(self._board, "drawings")

    def GetZones(self) -> list[Any]:
        return _call_native_method(self._board, "zones")

    def GetNetsByName(self) -> dict[str, "NETINFO_ITEM"]:
        return {net.name: NETINFO_ITEM(net) for net in _call_native_method(self._board, "nets")}

    def FindFootprintByReference(self, reference: str) -> "FOOTPRINT | None":
        footprint = _call_native_method(self._board, "find_footprint", reference)
        if footprint is None:
            return None
        return FOOTPRINT(footprint)

    def GetLayerName(self, layer_id: int) -> str:
        return _call_native_method(self._board, "get_layer_name", layer_id)

    def GetLayerID(self, name: str) -> int:
        return _call_native_method(self._board, "get_layer_id", name)

    def Add(self, item: Any) -> None:
        if isinstance(item, PCB_TRACK):
            item._add_to(self)
            return
        if isinstance(item, PCB_VIA):
            item._add_to(self)
            return
        raise NotImplementedError(f"board.Add does not support {type(item).__name__}")

    def Remove(self, item: Any) -> None:
        raise NotImplementedError("board.Remove is not supported by the current native compatibility layer")


class FOOTPRINT:
    def __init__(self, footprint: kk.Footprint):
        self._footprint = footprint

    def GetReference(self) -> str:
        return self._footprint.reference

    def SetReference(self, reference: str) -> None:
        raise NotImplementedError("footprint reference mutation is not implemented by the native compatibility layer")

    def GetValue(self) -> str:
        return self._footprint.value

    def SetValue(self, value: str) -> None:
        raise NotImplementedError("footprint value mutation is not implemented by the native compatibility layer")

    def GetPosition(self) -> VECTOR2I:
        x, y = self._footprint.position or (0.0, 0.0)
        return VECTOR2I(FromMM(x), FromMM(y))

    def SetPosition(self, position: VECTOR2I) -> None:
        raise NotImplementedError("footprint movement is not implemented by the native compatibility layer")

    def GetOrientation(self) -> float:
        return self._footprint.orientation

    def SetOrientation(self, angle: float) -> None:
        raise NotImplementedError("footprint rotation is not implemented by the native compatibility layer")

    def Pads(self) -> list["PAD"]:
        pads = self._footprint.pads() if callable(getattr(self._footprint, "pads", None)) else []
        return [PAD(pad) for pad in pads]


class PAD:
    def __init__(self, pad: kk.Pad):
        self._pad = pad

    def GetName(self) -> str:
        return self._pad.name

    def GetNetname(self) -> str:
        return self._pad.net

    def GetPosition(self) -> VECTOR2I:
        x, y = self._pad.position or (0.0, 0.0)
        return VECTOR2I(FromMM(x), FromMM(y))

    def GetSize(self) -> VECTOR2I:
        x, y = self._pad.size or (0.0, 0.0)
        return VECTOR2I(FromMM(x), FromMM(y))

    def GetDrillSize(self) -> VECTOR2I:
        drill = self._pad.drill_size or (0.0,)
        x = drill[0]
        y = drill[1] if len(drill) > 1 else x
        return VECTOR2I(FromMM(x), FromMM(y))

    def GetShape(self) -> str:
        return self._pad.shape


class PCB_TRACK_VIEW:
    def __init__(self, track: kk.Track):
        self._track = track

    def GetStart(self) -> VECTOR2I:
        return _vector_from_mm_pair(self._track.start)

    def GetEnd(self) -> VECTOR2I:
        return _vector_from_mm_pair(self._track.end)

    def GetWidth(self) -> int:
        return FromMM(self._track.width)

    def GetLayer(self) -> str:
        return self._track.layer

    def GetNetname(self) -> str:
        return self._track.net


class PCB_TRACK:
    def __init__(self, board: BOARD):
        self._board = board
        self._start = VECTOR2I(0, 0)
        self._end = VECTOR2I(0, 0)
        self._width = FromMM(0.25)
        self._layer: int | str = "F.Cu"
        self._netname = ""

    def SetStart(self, position: VECTOR2I) -> None:
        self._start = position

    def GetStart(self) -> VECTOR2I:
        return self._start

    def SetEnd(self, position: VECTOR2I) -> None:
        self._end = position

    def GetEnd(self) -> VECTOR2I:
        return self._end

    def SetWidth(self, width: int) -> None:
        self._width = width

    def GetWidth(self) -> int:
        return self._width

    def SetLayer(self, layer: int | str) -> None:
        self._layer = layer

    def GetLayer(self) -> int | str:
        return self._layer

    def SetNetname(self, netname: str) -> None:
        self._netname = netname

    def GetNetname(self) -> str:
        return self._netname

    def _add_to(self, board: BOARD) -> None:
        layer = board.GetLayerName(self._layer) if isinstance(self._layer, int) else self._layer
        board._board.add_track(
            net=self._netname,
            layer=layer,
            start=_mm_pair_from_vector(self._start),
            end=_mm_pair_from_vector(self._end),
            width=ToMM(self._width),
        )


class PCB_VIA:
    def __init__(self, board: BOARD):
        self._board = board
        self._position = VECTOR2I(0, 0)
        self._drill = FromMM(0.3)
        self._diameter = FromMM(0.6)
        self._layers: tuple[str, ...] = ("F.Cu", "B.Cu")
        self._netname = ""

    def SetPosition(self, position: VECTOR2I) -> None:
        self._position = position

    def GetPosition(self) -> VECTOR2I:
        return self._position

    def SetDrill(self, drill: int) -> None:
        self._drill = drill

    def GetDrill(self) -> int:
        return self._drill

    def SetWidth(self, diameter: int) -> None:
        self._diameter = diameter

    def GetWidth(self) -> int:
        return self._diameter

    def SetNetname(self, netname: str) -> None:
        self._netname = netname

    def GetNetname(self) -> str:
        return self._netname

    def _add_to(self, board: BOARD) -> None:
        if not hasattr(board._board, "add_via"):
            raise NotImplementedError("PCB_VIA requires native add_via support")
        board._board.add_via(
            net=self._netname,
            position=_mm_pair_from_vector(self._position),
            drill=ToMM(self._drill),
            diameter=ToMM(self._diameter),
            layers=self._layers,
        )


class NETINFO_ITEM:
    def __init__(self, net: kk.Net):
        self._net = net

    def GetNetname(self) -> str:
        return self._net.name

    def GetNetCode(self) -> int:
        return self._net.code


def GetBoard() -> None:
    _unsupported_gui("pcbnew.GetBoard")


def GetPcbFrame() -> None:
    _unsupported_gui("pcbnew.GetPcbFrame")


def Refresh() -> None:
    _unsupported_gui("pcbnew.Refresh")


class ActionPlugin:
    def __init__(self, *_args: Any, **_kwargs: Any):
        _unsupported_gui("pcbnew.ActionPlugin")


class ZONE_FILLER:
    def __init__(self, *_args: Any, **_kwargs: Any):
        raise NotImplementedError(
            "pcbnew.ZONE_FILLER is not supported by pybind11-kicad's native backend. "
            "Use KiCad CLI/GUI validation or the clean pybind11-kicad API instead."
        )


class DRC:
    def __init__(self, *_args: Any, **_kwargs: Any):
        raise NotImplementedError("pcbnew.DRC is not supported by pybind11-kicad's native backend")


class PLOT_CONTROLLER:
    def __init__(self, *_args: Any, **_kwargs: Any):
        raise NotImplementedError("pcbnew.PLOT_CONTROLLER is not supported by pybind11-kicad's native backend")


def _unsupported_gui(name: str) -> None:
    raise NotImplementedError(f"{name} is not supported by pybind11-kicad's native backend. This API requires KiCad GUI/editor state.")


def _call_native_method(target: Any, method: str, *args: Any) -> Any:
    function = getattr(target, method, None)
    if function is None:
        raise NotImplementedError(f"{method} requires native backend support")
    return function(*args)


def _mm_pair_from_vector(position: VECTOR2I) -> tuple[float, float]:
    return (ToMM(position.x), ToMM(position.y))


def _vector_from_mm_pair(position: Iterable[float]) -> VECTOR2I:
    x, y = position
    return VECTOR2I(FromMM(x), FromMM(y))


__all__ = [
    "ActionPlugin",
    "BOARD",
    "CompatibilityLevel",
    "DRC",
    "FromMM",
    "GetBoard",
    "GetBuildVersion",
    "GetPcbFrame",
    "IU_PER_MM",
    "LoadBoard",
    "PCB_TRACK",
    "PCB_VIA",
    "PLOT_CONTROLLER",
    "Refresh",
    "RequireCompatibility",
    "SaveBoard",
    "ToMM",
    "VECTOR2I",
    "ZONE_FILLER",
    "wxPoint",
]
