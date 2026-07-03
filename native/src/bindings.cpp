#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "pybind11_kicad/board.hpp"
#include "pybind11_kicad/errors.hpp"

namespace py = pybind11;
using namespace pybind11_kicad;

PYBIND11_MODULE(pybind11_kicad_native, module)
{
    py::register_exception<BackendUnavailableError>(module, "BackendUnavailableError");

    module.def("backend_version", [] {
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
        return "kicad-10.0.4-native-pybind11-kicad-0.1";
#else
        return "kicad-10.0.4-native-scaffold-pybind11-kicad-0.1";
#endif
    });

    py::class_<KkPoint>(module, "Point")
        .def(py::init<>())
        .def_readwrite("x_mm", &KkPoint::x_mm)
        .def_readwrite("y_mm", &KkPoint::y_mm);

    py::class_<KkTrackSpec>(module, "TrackSpec")
        .def(py::init<>())
        .def_readwrite("net", &KkTrackSpec::net)
        .def_readwrite("layer", &KkTrackSpec::layer)
        .def_readwrite("start", &KkTrackSpec::start)
        .def_readwrite("end", &KkTrackSpec::end)
        .def_readwrite("width_mm", &KkTrackSpec::width_mm);

    py::class_<KkViaSpec>(module, "ViaSpec")
        .def(py::init<>())
        .def_readwrite("net", &KkViaSpec::net)
        .def_readwrite("position", &KkViaSpec::position)
        .def_readwrite("drill_mm", &KkViaSpec::drill_mm)
        .def_readwrite("diameter_mm", &KkViaSpec::diameter_mm);

    py::class_<KkFootprint>(module, "Footprint")
        .def_property_readonly("reference", &KkFootprint::reference)
        .def_property_readonly("position", &KkFootprint::position)
        .def_property_readonly("layer", &KkFootprint::layer);

    py::class_<KkBoard>(module, "Board")
        .def_static("open", &KkBoard::open)
        .def("save", &KkBoard::save)
        .def("footprints", &KkBoard::footprints)
        .def("add_track", &KkBoard::add_track)
        .def("add_via", &KkBoard::add_via);
}
