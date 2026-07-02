#pragma once

#include <string>

namespace pybind11_kicad {

struct KkPoint {
    double x_mm = 0.0;
    double y_mm = 0.0;
};

struct KkTrackSpec {
    std::string net;
    std::string layer;
    KkPoint start;
    KkPoint end;
    double width_mm = 0.0;
};

struct KkViaSpec {
    std::string net;
    KkPoint position;
    double drill_mm = 0.0;
    double diameter_mm = 0.0;
};

}  // namespace pybind11_kicad
