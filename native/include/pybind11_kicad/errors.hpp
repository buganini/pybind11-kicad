#pragma once

#include <stdexcept>
#include <string>

namespace pybind11_kicad {

class BackendUnavailableError : public std::runtime_error {
public:
    explicit BackendUnavailableError(const std::string& message)
        : std::runtime_error(message) {}
};

}  // namespace pybind11_kicad
