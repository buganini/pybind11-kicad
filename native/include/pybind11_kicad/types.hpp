#pragma once

#include <string>
#include <vector>

namespace pybind11_kicad {

struct KkPoint {
    double x_mm = 0.0;
    double y_mm = 0.0;
};

struct KkBox {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct KkIntPoint {
    int x = 0;
    int y = 0;
};

struct KkDesignSettings {
    int board_thickness = 0;
    KkIntPoint aux_origin;
};

struct KkTitleBlock {
    std::string title;
    std::vector<std::string> comments;
};

struct KkNetInfo {
    std::string name;
    int code = 0;
};

struct KkDrawing {
    int layer = -1;
    int shape = -1;
    int width = 0;
    int radius = 0;
    bool filled = false;
    KkIntPoint start;
    KkIntPoint end;
    KkIntPoint center;
    KkIntPoint mid;
    std::vector<KkIntPoint> polygon_points;
    KkBox bounding_box;
    std::string uuid;
};

struct KkPolygon {
    std::vector<KkIntPoint> outline;
    std::vector<std::vector<KkIntPoint>> holes;
};

struct KkZoneFill {
    int layer = -1;
    std::vector<KkPolygon> polygons;
};

struct KkZoneItem {
    std::string net;
    int net_code = 0;
    std::vector<int> layers;
    unsigned priority = 0;
    std::string name;
    int fill_mode = 0;
    bool is_rule_area = false;
    bool is_filled = false;
    std::vector<KkPolygon> polygons;
    std::vector<KkZoneFill> fills;
    KkBox bounding_box;
    std::string uuid;
};

struct KkNpthSpec {
    std::string reference;
    KkIntPoint position;
    KkIntPoint drill_size;
    KkIntPoint size;
    double orientation_degrees = 0.0;
    std::string uuid;
    std::string pad_uuid;
};

struct KkFootprintFieldSpec {
    std::string name;
    std::string value;
    bool visible = true;
    KkIntPoint position;
    KkIntPoint size;
    int thickness = 0;
    double angle_degrees = 0.0;
    int layer = -1;
    int h_justify = 0;
    int v_justify = 0;
    bool mirrored = false;
    bool keep_upright = true;
    std::string uuid;
};

struct KkPad {
    std::string name;
    std::string net;
    int net_code = 0;
    int attribute = 0;
    KkPoint position;
    KkPoint size;
    std::vector<double> drill_size;
    int shape = 0;
    int drill_shape = 0;
    std::vector<int> layers;
    bool has_local_solder_mask_margin = false;
    int local_solder_mask_margin = 0;
    bool has_local_clearance = false;
    int local_clearance = 0;
    std::vector<KkPolygon> custom_polygons;
    std::string uuid;
};

struct KkFootprintSpec {
    std::string reference;
    std::string value;
    std::string fpid;
    int layer = 0;
    KkIntPoint position;
    double orientation_degrees = 0.0;
    bool excluded_from_pos = false;
    bool excluded_from_bom = false;
    bool board_only = false;
    bool dnp = false;
    bool reference_visible = true;
    bool value_visible = true;
    std::vector<KkFootprintFieldSpec> fields;
    std::vector<KkPad> pads;
    std::vector<KkDrawing> drawings;
    std::string uuid;
};

struct KkTextSpec {
    std::string text;
    int layer = -1;
    KkIntPoint position;
    KkIntPoint size;
    int thickness = 0;
    double angle_degrees = 0.0;
    int h_justify = 0;
    int v_justify = 0;
    bool mirrored = false;
    std::string uuid;
};

struct KkTrackSpec {
    std::string net;
    std::string layer;
    KkPoint start;
    KkPoint end;
    double width_mm = 0.0;
    std::string uuid;
};

struct KkTrackItem {
    std::string net;
    int net_code = 0;
    int layer = -1;
    bool is_arc = false;
    KkIntPoint start;
    KkIntPoint center;
    KkIntPoint mid;
    KkIntPoint end;
    int width = 0;
    KkBox bounding_box;
    std::string uuid;
};

struct KkViaSpec {
    std::string net;
    KkPoint position;
    double drill_mm = 0.0;
    double diameter_mm = 0.0;
    std::string uuid;
};

struct KkViaItem {
    std::string net;
    int net_code = 0;
    int via_type = 0;
    KkIntPoint position;
    int drill = 0;
    int diameter = 0;
    std::vector<int> layers;
    KkBox bounding_box;
    std::string uuid;
};

}  // namespace pybind11_kicad
