#pragma once

#include <memory>
#include <string>
#include <vector>

#include "pybind11_kicad/types.hpp"

namespace pybind11_kicad {

class KkFootprint {
public:
    KkFootprint(std::string reference, KkPoint position, std::string layer);

    const std::string& reference() const;
    KkPoint position() const;
    const std::string& layer() const;

private:
    std::string reference_;
    KkPoint position_;
    std::string layer_;
};

class KkBoard {
public:
    static KkBoard open(const std::string& path);

    ~KkBoard();
    KkBoard(KkBoard&& other) noexcept;
    KkBoard& operator=(KkBoard&& other) noexcept;
    KkBoard(const KkBoard& other) = delete;
    KkBoard& operator=(const KkBoard& other) = delete;

    void save(const std::string& path) const;
    std::vector<KkFootprint> footprints() const;
    void add_track(const KkTrackSpec& spec);
    void add_via(const KkViaSpec& spec);

private:
    struct Impl;

    explicit KkBoard(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> impl_;
};

}  // namespace pybind11_kicad
