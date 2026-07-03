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
    module.def(
        "load_footprint",
        &load_footprint,
        py::arg("library_path"),
        py::arg("footprint_name"),
        py::arg("preserve_uuid") = false);

    py::class_<KkPoint>(module, "Point")
        .def(py::init<>())
        .def_readwrite("x_mm", &KkPoint::x_mm)
        .def_readwrite("y_mm", &KkPoint::y_mm);

    py::class_<KkBox>(module, "Box")
        .def(py::init<>())
        .def_readwrite("x", &KkBox::x)
        .def_readwrite("y", &KkBox::y)
        .def_readwrite("width", &KkBox::width)
        .def_readwrite("height", &KkBox::height);

    py::class_<KkIntPoint>(module, "IntPoint")
        .def(py::init<>())
        .def_readwrite("x", &KkIntPoint::x)
        .def_readwrite("y", &KkIntPoint::y);

    py::class_<KkDesignSettings>(module, "DesignSettings")
        .def(py::init<>())
        .def_readwrite("board_thickness", &KkDesignSettings::board_thickness)
        .def_readwrite("aux_origin", &KkDesignSettings::aux_origin);

    py::class_<KkNetInfo>(module, "NetInfo")
        .def(py::init<>())
        .def_readwrite("name", &KkNetInfo::name)
        .def_readwrite("code", &KkNetInfo::code);

    py::class_<KkDrawing>(module, "Drawing")
        .def(py::init<>())
        .def_readwrite("layer", &KkDrawing::layer)
        .def_readwrite("shape", &KkDrawing::shape)
        .def_readwrite("width", &KkDrawing::width)
        .def_readwrite("radius", &KkDrawing::radius)
        .def_readwrite("filled", &KkDrawing::filled)
        .def_readwrite("start", &KkDrawing::start)
        .def_readwrite("end", &KkDrawing::end)
        .def_readwrite("center", &KkDrawing::center)
        .def_readwrite("mid", &KkDrawing::mid)
        .def_readwrite("polygon_points", &KkDrawing::polygon_points)
        .def_readwrite("bounding_box", &KkDrawing::bounding_box);

    py::class_<KkPolygon>(module, "Polygon")
        .def(py::init<>())
        .def_readwrite("outline", &KkPolygon::outline)
        .def_readwrite("holes", &KkPolygon::holes);

    py::class_<KkZoneFill>(module, "ZoneFill")
        .def(py::init<>())
        .def_readwrite("layer", &KkZoneFill::layer)
        .def_readwrite("polygons", &KkZoneFill::polygons);

    py::class_<KkZoneItem>(module, "ZoneItem")
        .def(py::init<>())
        .def_readwrite("net", &KkZoneItem::net)
        .def_readwrite("net_code", &KkZoneItem::net_code)
        .def_readwrite("layers", &KkZoneItem::layers)
        .def_readwrite("priority", &KkZoneItem::priority)
        .def_readwrite("name", &KkZoneItem::name)
        .def_readwrite("fill_mode", &KkZoneItem::fill_mode)
        .def_readwrite("is_rule_area", &KkZoneItem::is_rule_area)
        .def_readwrite("is_filled", &KkZoneItem::is_filled)
        .def_readwrite("polygons", &KkZoneItem::polygons)
        .def_readwrite("fills", &KkZoneItem::fills)
        .def_readwrite("bounding_box", &KkZoneItem::bounding_box);

    py::class_<KkNpthSpec>(module, "NpthSpec")
        .def(py::init<>())
        .def_readwrite("reference", &KkNpthSpec::reference)
        .def_readwrite("position", &KkNpthSpec::position)
        .def_readwrite("drill_size", &KkNpthSpec::drill_size)
        .def_readwrite("size", &KkNpthSpec::size)
        .def_readwrite("orientation_degrees", &KkNpthSpec::orientation_degrees);

    py::class_<KkFootprintFieldSpec>(module, "FootprintFieldSpec")
        .def(py::init<>())
        .def_readwrite("name", &KkFootprintFieldSpec::name)
        .def_readwrite("value", &KkFootprintFieldSpec::value)
        .def_readwrite("visible", &KkFootprintFieldSpec::visible)
        .def_readwrite("position", &KkFootprintFieldSpec::position)
        .def_readwrite("size", &KkFootprintFieldSpec::size)
        .def_readwrite("thickness", &KkFootprintFieldSpec::thickness)
        .def_readwrite("angle_degrees", &KkFootprintFieldSpec::angle_degrees)
        .def_readwrite("layer", &KkFootprintFieldSpec::layer)
        .def_readwrite("h_justify", &KkFootprintFieldSpec::h_justify)
        .def_readwrite("v_justify", &KkFootprintFieldSpec::v_justify)
        .def_readwrite("mirrored", &KkFootprintFieldSpec::mirrored)
        .def_readwrite("keep_upright", &KkFootprintFieldSpec::keep_upright);

    py::class_<KkPad>(module, "Pad")
        .def(py::init<>())
        .def_readwrite("name", &KkPad::name)
        .def_readwrite("net", &KkPad::net)
        .def_readwrite("attribute", &KkPad::attribute)
        .def_readwrite("position", &KkPad::position)
        .def_readwrite("size", &KkPad::size)
        .def_readwrite("drill_size", &KkPad::drill_size)
        .def_readwrite("shape", &KkPad::shape)
        .def_readwrite("drill_shape", &KkPad::drill_shape)
        .def_readwrite("layers", &KkPad::layers)
        .def_readwrite("has_local_solder_mask_margin", &KkPad::has_local_solder_mask_margin)
        .def_readwrite("local_solder_mask_margin", &KkPad::local_solder_mask_margin)
        .def_readwrite("has_local_clearance", &KkPad::has_local_clearance)
        .def_readwrite("local_clearance", &KkPad::local_clearance);

    py::class_<KkFootprintSpec>(module, "FootprintSpec")
        .def(py::init<>())
        .def_readwrite("reference", &KkFootprintSpec::reference)
        .def_readwrite("value", &KkFootprintSpec::value)
        .def_readwrite("fpid", &KkFootprintSpec::fpid)
        .def_readwrite("layer", &KkFootprintSpec::layer)
        .def_readwrite("position", &KkFootprintSpec::position)
        .def_readwrite("orientation_degrees", &KkFootprintSpec::orientation_degrees)
        .def_readwrite("excluded_from_pos", &KkFootprintSpec::excluded_from_pos)
        .def_readwrite("excluded_from_bom", &KkFootprintSpec::excluded_from_bom)
        .def_readwrite("board_only", &KkFootprintSpec::board_only)
        .def_readwrite("dnp", &KkFootprintSpec::dnp)
        .def_readwrite("reference_visible", &KkFootprintSpec::reference_visible)
        .def_readwrite("value_visible", &KkFootprintSpec::value_visible)
        .def_readwrite("fields", &KkFootprintSpec::fields)
        .def_readwrite("pads", &KkFootprintSpec::pads)
        .def_readwrite("drawings", &KkFootprintSpec::drawings);

    py::class_<KkTextSpec>(module, "TextSpec")
        .def(py::init<>())
        .def_readwrite("text", &KkTextSpec::text)
        .def_readwrite("layer", &KkTextSpec::layer)
        .def_readwrite("position", &KkTextSpec::position)
        .def_readwrite("size", &KkTextSpec::size)
        .def_readwrite("thickness", &KkTextSpec::thickness)
        .def_readwrite("angle_degrees", &KkTextSpec::angle_degrees)
        .def_readwrite("h_justify", &KkTextSpec::h_justify)
        .def_readwrite("v_justify", &KkTextSpec::v_justify)
        .def_readwrite("mirrored", &KkTextSpec::mirrored);

    py::class_<KkTrackSpec>(module, "TrackSpec")
        .def(py::init<>())
        .def_readwrite("net", &KkTrackSpec::net)
        .def_readwrite("layer", &KkTrackSpec::layer)
        .def_readwrite("start", &KkTrackSpec::start)
        .def_readwrite("end", &KkTrackSpec::end)
        .def_readwrite("width_mm", &KkTrackSpec::width_mm);

    py::class_<KkTrackItem>(module, "TrackItem")
        .def(py::init<>())
        .def_readwrite("net", &KkTrackItem::net)
        .def_readwrite("net_code", &KkTrackItem::net_code)
        .def_readwrite("layer", &KkTrackItem::layer)
        .def_readwrite("is_arc", &KkTrackItem::is_arc)
        .def_readwrite("start", &KkTrackItem::start)
        .def_readwrite("mid", &KkTrackItem::mid)
        .def_readwrite("end", &KkTrackItem::end)
        .def_readwrite("width", &KkTrackItem::width)
        .def_readwrite("bounding_box", &KkTrackItem::bounding_box);

    py::class_<KkViaSpec>(module, "ViaSpec")
        .def(py::init<>())
        .def_readwrite("net", &KkViaSpec::net)
        .def_readwrite("position", &KkViaSpec::position)
        .def_readwrite("drill_mm", &KkViaSpec::drill_mm)
        .def_readwrite("diameter_mm", &KkViaSpec::diameter_mm);

    py::class_<KkViaItem>(module, "ViaItem")
        .def(py::init<>())
        .def_readwrite("net", &KkViaItem::net)
        .def_readwrite("net_code", &KkViaItem::net_code)
        .def_readwrite("via_type", &KkViaItem::via_type)
        .def_readwrite("position", &KkViaItem::position)
        .def_readwrite("drill", &KkViaItem::drill)
        .def_readwrite("diameter", &KkViaItem::diameter)
        .def_readwrite("layers", &KkViaItem::layers)
        .def_readwrite("bounding_box", &KkViaItem::bounding_box);

    py::class_<KkFootprint>(module, "Footprint")
        .def_property_readonly("reference", &KkFootprint::reference)
        .def_property_readonly("value", &KkFootprint::value)
        .def_property_readonly("fpid", &KkFootprint::fpid)
        .def_property_readonly("position", &KkFootprint::position)
        .def_property_readonly("layer", &KkFootprint::layer)
        .def_property_readonly("orientation_degrees", &KkFootprint::orientation_degrees)
        .def_property_readonly("excluded_from_pos", &KkFootprint::excluded_from_pos)
        .def_property_readonly("excluded_from_bom", &KkFootprint::excluded_from_bom)
        .def_property_readonly("board_only", &KkFootprint::board_only)
        .def_property_readonly("dnp", &KkFootprint::dnp)
        .def_property_readonly("has_native_footprint", &KkFootprint::has_native_footprint)
        .def("fields", &KkFootprint::fields)
        .def("pads", &KkFootprint::pads)
        .def("drawings", &KkFootprint::drawings);

    py::class_<KkBoard>(module, "Board")
        .def_static("open", &KkBoard::open)
        .def_static("create", &KkBoard::create)
        .def("save", &KkBoard::save)
        .def("design_settings", &KkBoard::design_settings)
        .def("set_board_thickness", &KkBoard::set_board_thickness)
        .def("set_aux_origin", &KkBoard::set_aux_origin)
        .def("copper_layer_count", &KkBoard::copper_layer_count)
        .def("set_copper_layer_count", &KkBoard::set_copper_layer_count)
        .def("enabled_layers", &KkBoard::enabled_layers)
        .def("set_enabled_layers", &KkBoard::set_enabled_layers)
        .def("get_layer_name", &KkBoard::get_layer_name)
        .def("get_layer_id", &KkBoard::get_layer_id)
        .def("set_layer_name", &KkBoard::set_layer_name)
        .def("nets", &KkBoard::nets)
        .def("drawings", &KkBoard::drawings)
        .def("zones", &KkBoard::zones)
        .def("tracks", &KkBoard::tracks)
        .def("vias", &KkBoard::vias)
        .def("footprints", &KkBoard::footprints)
        .def("add_drawing", &KkBoard::add_drawing)
        .def("remove_drawing", &KkBoard::remove_drawing)
        .def("add_npth_hole", &KkBoard::add_npth_hole)
        .def("add_footprint", &KkBoard::add_footprint)
        .def("add_footprint_clone", &KkBoard::add_footprint_clone)
        .def("add_text", &KkBoard::add_text)
        .def("remove_text", &KkBoard::remove_text)
        .def("add_track", &KkBoard::add_track)
        .def("add_via", &KkBoard::add_via)
        .def("add_track_item", &KkBoard::add_track_item)
        .def("add_via_item", &KkBoard::add_via_item)
        .def("add_zone_item", &KkBoard::add_zone_item);
}
