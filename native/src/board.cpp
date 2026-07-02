#include "pybind11_kicad/board.hpp"

#include <utility>

#include "pybind11_kicad/errors.hpp"

namespace pybind11_kicad {
namespace {

[[noreturn]] void native_backend_unavailable()
{
    throw BackendUnavailableError(
        "The target KiCad native backend is not wired yet. "
        "Install a supported target KiCad and wire the installed-KiCad adapter "
        "before enabling board IO.");
}

}  // namespace

struct KkBoard::Impl {
    std::string path;
};

KkFootprint::KkFootprint(std::string reference, KkPoint position, std::string layer)
    : reference_(std::move(reference)),
      position_(position),
      layer_(std::move(layer))
{
}

const std::string& KkFootprint::reference() const
{
    return reference_;
}

KkPoint KkFootprint::position() const
{
    return position_;
}

const std::string& KkFootprint::layer() const
{
    return layer_;
}

KkBoard KkBoard::open(const std::string& path)
{
    (void) path;
    native_backend_unavailable();
}

KkBoard::KkBoard(std::unique_ptr<Impl> impl)
    : impl_(std::move(impl))
{
}

KkBoard::~KkBoard() = default;

KkBoard::KkBoard(KkBoard&& other) noexcept = default;

KkBoard& KkBoard::operator=(KkBoard&& other) noexcept = default;

void KkBoard::save(const std::string& path) const
{
    (void) path;
    native_backend_unavailable();
}

std::vector<KkFootprint> KkBoard::footprints() const
{
    native_backend_unavailable();
}

void KkBoard::add_track(const KkTrackSpec& spec)
{
    (void) spec;
    native_backend_unavailable();
}

void KkBoard::add_via(const KkViaSpec& spec)
{
    (void) spec;
    native_backend_unavailable();
}

}  // namespace pybind11_kicad
