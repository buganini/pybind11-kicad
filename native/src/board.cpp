#include "pybind11_kicad/board.hpp"

#include <stdexcept>
#include <utility>

#include "pybind11_kicad/errors.hpp"

#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
#include <memory>

#include <base_units.h>
#include <board.h>
#include <footprint.h>
#include <layer_ids.h>
#include <lset.h>
#include <netinfo.h>
#include <pcb_io/pcb_io_mgr.h>
#include <pcb_track.h>
#include <wx/string.h>
#endif

namespace pybind11_kicad {
namespace {

#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)

wxString to_wx_string(const std::string& value)
{
    return wxString::FromUTF8(value.c_str());
}

std::string to_utf8_string(const wxString& value)
{
    return std::string(value.utf8_str());
}

int mm_to_iu(double value_mm)
{
    return pcbIUScale.mmToIU(value_mm);
}

KkPoint point_from_iu(const VECTOR2I& point)
{
    return KkPoint{ pcbIUScale.IUTomm(point.x), pcbIUScale.IUTomm(point.y) };
}

VECTOR2I point_to_iu(const KkPoint& point)
{
    return VECTOR2I(mm_to_iu(point.x_mm), mm_to_iu(point.y_mm));
}

PCB_LAYER_ID layer_id_or_throw(const BOARD& board, const std::string& layer_name)
{
    wxString layer = to_wx_string(layer_name);
    PCB_LAYER_ID layer_id = board.GetLayerID(layer);

    if(layer_id == UNDEFINED_LAYER)
    {
        int fixed_layer_id = LSET::NameToLayer(layer);

        if(fixed_layer_id >= 0)
            layer_id = static_cast<PCB_LAYER_ID>(fixed_layer_id);
    }

    if(layer_id == UNDEFINED_LAYER)
        throw std::runtime_error("Unknown KiCad board layer: " + layer_name);

    return layer_id;
}

NETINFO_ITEM* net_or_null(BOARD& board, const std::string& net_name)
{
    if(net_name.empty())
        return nullptr;

    wxString name = to_wx_string(net_name);
    NETINFO_ITEM* net = board.FindNet(name);

    if(net)
        return net;

    net = new NETINFO_ITEM(&board, name, static_cast<int>(board.GetNetCount()));
    board.Add(net);
    return net;
}

#else

[[noreturn]] void native_backend_unavailable()
{
    throw BackendUnavailableError(
        "The pybind11_kicad_native extension was built without KiCad board IO. "
        "Build with scripts/build.sh pybind11-kicad or configure with "
        "PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO=ON and a configured target KiCad "
        "build tree.");
}

#endif

}  // namespace

#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)

struct KkBoard::Impl {
    std::unique_ptr<BOARD> board;
    std::string path;
};

#else

struct KkBoard::Impl {
    std::string path;
};

#endif

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
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    BOARD* board = PCB_IO_MGR::Load(PCB_IO_MGR::KICAD_SEXP, to_wx_string(path));

    if(!board)
        throw std::runtime_error("KiCad PCB_IO_MGR returned no board for: " + path);

    auto impl = std::make_unique<Impl>();
    impl->board.reset(board);
    impl->path = path;
    return KkBoard(std::move(impl));
#else
    (void) path;
    native_backend_unavailable();
#endif
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
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot save an uninitialized KiCad board");

    PCB_IO_MGR::Save(PCB_IO_MGR::KICAD_SEXP, to_wx_string(path), impl_->board.get());
#else
    (void) path;
    native_backend_unavailable();
#endif
}

std::vector<KkFootprint> KkBoard::footprints() const
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot inspect an uninitialized KiCad board");

    std::vector<KkFootprint> result;

    for(const FOOTPRINT* footprint : impl_->board->Footprints())
    {
        result.emplace_back(
            to_utf8_string(footprint->GetReference()),
            point_from_iu(footprint->GetPosition()),
            to_utf8_string(LSET::Name(footprint->GetLayer()))
        );
    }

    return result;
#else
    native_backend_unavailable();
#endif
}

void KkBoard::add_track(const KkTrackSpec& spec)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot edit an uninitialized KiCad board");

    auto* track = new PCB_TRACK(impl_->board.get());
    track->SetStart(point_to_iu(spec.start));
    track->SetEnd(point_to_iu(spec.end));
    track->SetWidth(mm_to_iu(spec.width_mm));
    track->SetLayer(layer_id_or_throw(*impl_->board, spec.layer));
    track->SetNet(net_or_null(*impl_->board, spec.net));
    impl_->board->Add(track);
#else
    (void) spec;
    native_backend_unavailable();
#endif
}

void KkBoard::add_via(const KkViaSpec& spec)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot edit an uninitialized KiCad board");

    auto* via = new PCB_VIA(impl_->board.get());
    via->SetViaType(VIATYPE::THROUGH);
    via->SetPosition(point_to_iu(spec.position));
    via->SetWidth(mm_to_iu(spec.diameter_mm));
    via->SetDrill(mm_to_iu(spec.drill_mm));
    via->SetLayerPair(F_Cu, B_Cu);
    via->SetNet(net_or_null(*impl_->board, spec.net));
    impl_->board->Add(via);
#else
    (void) spec;
    native_backend_unavailable();
#endif
}

}  // namespace pybind11_kicad
