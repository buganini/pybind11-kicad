#include "pybind11_kicad/board.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

#include "pybind11_kicad/errors.hpp"

#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
#include <memory>

#include <base_units.h>
#include <board.h>
#include <board_design_settings.h>
#include <board_item.h>
#include <footprint.h>
#include <kiid.h>
#include <layer_ids.h>
#include <lset.h>
#include <netinfo.h>
#include <pad.h>
#include <padstack.h>
#include <pcb_field.h>
#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_track.h>
#include <title_block.h>
#include <wx/string.h>
#include <zone.h>
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

VECTOR2I int_point_to_iu(const KkIntPoint& point)
{
    return VECTOR2I(point.x, point.y);
}

VECTOR2I local_int_point_to_board_iu(const FOOTPRINT& footprint, const KkIntPoint& point)
{
    return footprint.GetPosition() + int_point_to_iu(point);
}

VECTOR2I local_point_to_board_iu(const FOOTPRINT& footprint, const KkPoint& point)
{
    return footprint.GetPosition() + point_to_iu(point);
}

KkBox box_from_iu(const BOX2I& box)
{
    return KkBox{
        box.GetX(),
        box.GetY(),
        static_cast<int>(box.GetWidth()),
        static_cast<int>(box.GetHeight())
    };
}

KkIntPoint int_point_from_iu(const VECTOR2I& point)
{
    return KkIntPoint{ point.x, point.y };
}

bool int_points_equal(const VECTOR2I& lhs, const KkIntPoint& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

bool int_points_equal(const KkIntPoint& lhs, const KkIntPoint& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

void assign_uuid(EDA_ITEM& item, const std::string& uuid)
{
    if(!uuid.empty())
        const_cast<KIID&>( item.m_Uuid ) = KIID(uuid);
}

std::vector<int> layers_from_lset(const LSET& layer_set)
{
    std::vector<int> result;

    for(int layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer)
    {
        if(layer_set.Contains(static_cast<PCB_LAYER_ID>(layer)))
            result.push_back(layer);
    }

    return result;
}

std::vector<KkIntPoint> polygon_points_from_shape(const PCB_SHAPE& shape)
{
    std::vector<KkIntPoint> points;

    if(shape.GetShape() != SHAPE_T::POLY || !shape.IsPolyShapeValid())
        return points;

    const SHAPE_POLY_SET& poly = shape.GetPolyShape();

    if(poly.OutlineCount() == 0)
        return points;

    const SHAPE_LINE_CHAIN& outline = poly.Outline(0);
    points.reserve(outline.PointCount());

    for(int i = 0; i < outline.PointCount(); ++i)
        points.push_back(int_point_from_iu(outline.CPoint(i)));

    return points;
}

bool polygon_points_equal(const PCB_SHAPE& shape, const std::vector<KkIntPoint>& points)
{
    std::vector<KkIntPoint> existing = polygon_points_from_shape(shape);

    if(existing.size() != points.size())
        return false;

    for(std::size_t i = 0; i < points.size(); ++i)
    {
        if(existing[i].x != points[i].x || existing[i].y != points[i].y)
            return false;
    }

    return true;
}

std::vector<KkIntPoint> points_from_chain(const SHAPE_LINE_CHAIN& chain)
{
    std::vector<KkIntPoint> points;
    points.reserve(chain.PointCount());

    for(int i = 0; i < chain.PointCount(); ++i)
        points.push_back(int_point_from_iu(chain.CPoint(i)));

    return points;
}

SHAPE_LINE_CHAIN chain_from_points(const std::vector<KkIntPoint>& points)
{
    SHAPE_LINE_CHAIN chain;

    for(const KkIntPoint& point : points)
        chain.Append(int_point_to_iu(point));

    chain.SetClosed(true);
    return chain;
}

std::vector<KkPolygon> polygons_from_poly_set(const SHAPE_POLY_SET& poly)
{
    std::vector<KkPolygon> polygons;

    for(int outline_idx = 0; outline_idx < poly.OutlineCount(); ++outline_idx)
    {
        KkPolygon polygon;
        polygon.outline = points_from_chain(poly.Outline(outline_idx));

        for(int hole_idx = 0; hole_idx < poly.HoleCount(outline_idx); ++hole_idx)
            polygon.holes.push_back(points_from_chain(poly.CHole(outline_idx, hole_idx)));

        polygons.push_back(std::move(polygon));
    }

    return polygons;
}

bool point_vectors_equal(const std::vector<KkIntPoint>& lhs, const std::vector<KkIntPoint>& rhs)
{
    std::size_t lhs_size = lhs.size();
    std::size_t rhs_size = rhs.size();

    if(lhs_size > 1 && int_points_equal(lhs.front(), lhs.back()))
        --lhs_size;

    if(rhs_size > 1 && int_points_equal(rhs.front(), rhs.back()))
        --rhs_size;

    if(lhs_size != rhs_size)
        return false;

    for(std::size_t i = 0; i < lhs_size; ++i)
    {
        if(!int_points_equal(lhs[i], rhs[i]))
            return false;
    }

    return true;
}

bool polygons_equal(const std::vector<KkPolygon>& lhs, const std::vector<KkPolygon>& rhs)
{
    if(lhs.size() != rhs.size())
        return false;

    for(std::size_t i = 0; i < lhs.size(); ++i)
    {
        if(!point_vectors_equal(lhs[i].outline, rhs[i].outline))
            return false;

        if(lhs[i].holes.size() != rhs[i].holes.size())
            return false;

        for(std::size_t hole_idx = 0; hole_idx < lhs[i].holes.size(); ++hole_idx)
        {
            if(!point_vectors_equal(lhs[i].holes[hole_idx], rhs[i].holes[hole_idx]))
                return false;
        }
    }

    return true;
}

void add_polygons_to_poly_set(SHAPE_POLY_SET& poly, const std::vector<KkPolygon>& polygons)
{
    for(const KkPolygon& polygon : polygons)
    {
        if(polygon.outline.empty())
            continue;

        int outline_idx = poly.OutlineCount();
        poly.AddOutline(chain_from_points(polygon.outline));

        for(const std::vector<KkIntPoint>& hole : polygon.holes)
        {
            if(!hole.empty())
                poly.AddHole(chain_from_points(hole), outline_idx);
        }
    }
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

PCB_LAYER_ID layer_id_from_int_or_throw(int layer_id)
{
    if(layer_id < 0 || layer_id >= PCB_LAYER_ID_COUNT)
        throw std::runtime_error("KiCad board layer id is out of range: " + std::to_string(layer_id));

    return static_cast<PCB_LAYER_ID>(layer_id);
}

SHAPE_T shape_type_from_int_or_throw(int shape)
{
    if(shape < static_cast<int>(SHAPE_T::SEGMENT) || shape > static_cast<int>(SHAPE_T::BEZIER))
        throw std::runtime_error("KiCad PCB shape type is out of range: " + std::to_string(shape));

    return static_cast<SHAPE_T>(shape);
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

bool drawing_matches_shape(const KkDrawing& drawing, const PCB_SHAPE& shape)
{
    if(static_cast<int>(shape.GetLayer()) != drawing.layer)
        return false;

    if(static_cast<int>(shape.GetShape()) != drawing.shape)
        return false;

    if(shape.GetWidth() != drawing.width)
        return false;

    if(shape.IsSolidFill() != drawing.filled)
        return false;

    if(!int_points_equal(shape.GetStart(), drawing.start))
        return false;

    if(!int_points_equal(shape.GetEnd(), drawing.end))
        return false;

    if(shape.GetShape() == SHAPE_T::CIRCLE)
    {
        if(!int_points_equal(shape.GetCenter(), drawing.center))
            return false;

        if(shape.GetRadius() != drawing.radius)
            return false;
    }
    else if(shape.GetShape() == SHAPE_T::ARC)
    {
        if(!int_points_equal(shape.GetArcMid(), drawing.mid))
            return false;
    }
    else if(shape.GetShape() == SHAPE_T::POLY)
    {
        if(!polygon_points_equal(shape, drawing.polygon_points))
            return false;
    }

    return true;
}

KkDrawing drawing_from_shape(const PCB_SHAPE& shape)
{
    int shape_type = static_cast<int>(shape.GetShape());

    KkDrawing drawing{
        static_cast<int>(shape.GetLayer()),
        shape_type,
        shape.GetWidth(),
        shape.GetRadius(),
        shape.IsSolidFill(),
        int_point_from_iu(shape.GetStart()),
        int_point_from_iu(shape.GetEnd()),
        int_point_from_iu(shape.GetCenter()),
        shape_type == static_cast<int>(SHAPE_T::ARC)
            ? int_point_from_iu(shape.GetArcMid())
            : KkIntPoint{},
        polygon_points_from_shape(shape),
        box_from_iu(shape.GetBoundingBox())
    };
    drawing.uuid = shape.m_Uuid.AsStdString();
    return drawing;
}

KkTrackItem track_from_native(const PCB_TRACK& track)
{
    bool is_arc = track.Type() == PCB_ARC_T;
    KkIntPoint center;
    KkIntPoint mid;

    if(is_arc)
    {
        const PCB_ARC& arc = static_cast<const PCB_ARC&>(track);
        center = int_point_from_iu(arc.GetCenter());
        mid = int_point_from_iu(arc.GetMid());
    }

    KkTrackItem item{
        to_utf8_string(track.GetNetname()),
        track.GetNetCode(),
        static_cast<int>(track.GetLayer()),
        is_arc,
        int_point_from_iu(track.GetStart()),
        center,
        mid,
        int_point_from_iu(track.GetEnd()),
        track.GetWidth(),
        box_from_iu(track.GetBoundingBox())
    };
    item.uuid = track.m_Uuid.AsStdString();
    return item;
}

KkTextSpec text_spec_from_text(const PCB_TEXT& text)
{
    KkTextSpec spec{
        to_utf8_string(text.GetText()),
        static_cast<int>(text.GetLayer()),
        int_point_from_iu(text.GetPosition()),
        int_point_from_iu(text.GetTextSize()),
        text.GetTextThickness(),
        text.GetTextAngle().AsDegrees(),
        static_cast<int>(text.GetHorizJustify()),
        static_cast<int>(text.GetVertJustify()),
        text.IsMirrored()
    };
    spec.uuid = text.m_Uuid.AsStdString();
    return spec;
}

KkViaItem via_from_native(const PCB_VIA& via)
{
    KkViaItem item{
        to_utf8_string(via.GetNetname()),
        via.GetNetCode(),
        static_cast<int>(via.GetViaType()),
        int_point_from_iu(via.GetPosition()),
        via.GetDrillValue(),
        via.GetWidth(),
        layers_from_lset(via.GetLayerSet()),
        box_from_iu(via.GetBoundingBox())
    };
    item.uuid = via.m_Uuid.AsStdString();
    return item;
}

KkZoneItem zone_from_native(const ZONE& zone)
{
    KkZoneItem item;
    item.net = to_utf8_string(zone.GetNetname());
    item.net_code = zone.GetNetCode();
    item.layers = layers_from_lset(zone.GetLayerSet());
    item.priority = zone.GetAssignedPriority();
    item.name = to_utf8_string(zone.GetZoneName());
    item.fill_mode = static_cast<int>(zone.GetFillMode());
    item.is_rule_area = zone.GetIsRuleArea();
    item.is_filled = zone.IsFilled();
    item.bounding_box = box_from_iu(zone.GetBoundingBox());
    item.polygons = polygons_from_poly_set(*zone.Outline());
    item.uuid = zone.m_Uuid.AsStdString();

    for(int layer : item.layers)
    {
        PCB_LAYER_ID layer_id = layer_id_from_int_or_throw(layer);

        if(!zone.HasFilledPolysForLayer(layer_id))
            continue;

        std::shared_ptr<SHAPE_POLY_SET> fill = zone.GetFilledPolysList(layer_id);

        if(!fill || fill->OutlineCount() == 0)
            continue;

        KkZoneFill fill_spec;
        fill_spec.layer = layer;
        fill_spec.polygons = polygons_from_poly_set(*fill);
        item.fills.push_back(std::move(fill_spec));
    }

    return item;
}

bool zone_items_equal(const KkZoneItem& lhs, const KkZoneItem& rhs)
{
    return lhs.layers == rhs.layers
        && lhs.priority == rhs.priority
        && lhs.name == rhs.name
        && lhs.fill_mode == rhs.fill_mode
        && lhs.is_rule_area == rhs.is_rule_area
        && polygons_equal(lhs.polygons, rhs.polygons);
}

void apply_drawing_to_shape(FOOTPRINT& footprint, PCB_SHAPE& shape, const KkDrawing& drawing)
{
    assign_uuid(shape, drawing.uuid);
    shape.SetShape(shape_type_from_int_or_throw(drawing.shape));
    shape.SetLayer(layer_id_from_int_or_throw(drawing.layer));
    shape.SetWidth(drawing.width);
    shape.SetFilled(drawing.filled);

    if(drawing.shape == static_cast<int>(SHAPE_T::ARC))
    {
        shape.SetArcGeometry(
            int_point_to_iu(drawing.start),
            int_point_to_iu(drawing.mid),
            int_point_to_iu(drawing.end));
    }
    else
    {
        shape.SetStart(int_point_to_iu(drawing.start));
        shape.SetEnd(int_point_to_iu(drawing.end));
    }

    if(drawing.shape == static_cast<int>(SHAPE_T::CIRCLE))
    {
        shape.SetCenter(int_point_to_iu(drawing.center));
        shape.SetRadius(drawing.radius);
    }
    else if(drawing.shape == static_cast<int>(SHAPE_T::POLY))
    {
        SHAPE_POLY_SET& poly = shape.GetPolyShape();
        int outline = poly.NewOutline();

        for(const KkIntPoint& point : drawing.polygon_points)
            poly.Append(int_point_to_iu(point), outline);
    }
}

bool text_matches_spec(const KkTextSpec& spec, const PCB_TEXT& text)
{
    if(to_utf8_string(text.GetText()) != spec.text)
        return false;

    if(static_cast<int>(text.GetLayer()) != spec.layer)
        return false;

    if(!int_points_equal(text.GetPosition(), spec.position))
        return false;

    if(!int_points_equal(text.GetTextSize(), spec.size))
        return false;

    if(text.GetTextThickness() != spec.thickness)
        return false;

    if(text.GetHorizJustify() != static_cast<GR_TEXT_H_ALIGN_T>(spec.h_justify))
        return false;

    if(text.GetVertJustify() != static_cast<GR_TEXT_V_ALIGN_T>(spec.v_justify))
        return false;

    if(text.IsMirrored() != spec.mirrored)
        return false;

    return std::abs(text.GetTextAngle().AsDegrees() - spec.angle_degrees) < 0.001;
}

KkFootprintFieldSpec field_spec_from_field(const PCB_FIELD& field)
{
    KkFootprintFieldSpec spec{
        to_utf8_string(field.GetName()),
        to_utf8_string(field.GetText()),
        field.IsVisible(),
        int_point_from_iu(field.GetTextPos()),
        int_point_from_iu(field.GetTextSize()),
        field.GetTextThickness(),
        field.GetTextAngle().AsDegrees(),
        static_cast<int>(field.GetLayer()),
        static_cast<int>(field.GetHorizJustify()),
        static_cast<int>(field.GetVertJustify()),
        field.IsMirrored(),
        field.IsKeepUpright()
    };
    spec.uuid = field.m_Uuid.AsStdString();
    return spec;
}

KkFootprintFieldSpec field_spec_from_text(const std::string& name, const wxString& value)
{
    KkFootprintFieldSpec field;
    field.name = name;
    field.value = to_utf8_string(value);
    field.visible = false;
    return field;
}

void apply_field_spec(FOOTPRINT& footprint, const KkFootprintFieldSpec& field_spec)
{
    PCB_FIELD* field = nullptr;

    if(field_spec.name == "Reference")
        field = &footprint.Reference();
    else if(field_spec.name == "Value")
        field = &footprint.Value();
    else if(field_spec.name == "Sheet file")
    {
        footprint.SetSheetfile(to_wx_string(field_spec.value));
        return;
    }
    else if(field_spec.name == "Sheet name")
    {
        footprint.SetSheetname(to_wx_string(field_spec.value));
        return;
    }
    else
        field = footprint.GetField(to_wx_string(field_spec.name));

    if(!field)
    {
        field = new PCB_FIELD(&footprint, FIELD_T::USER, to_wx_string(field_spec.name));
        field->SetOrdinal(footprint.GetNextFieldOrdinal());
        footprint.Add(field);
    }

    assign_uuid(*field, field_spec.uuid);
    field->SetText(to_wx_string(field_spec.value));
    field->SetVisible(field_spec.visible);
    field->SetTextPos(int_point_to_iu(field_spec.position));

    if(field_spec.size.x != 0 || field_spec.size.y != 0)
        field->SetTextSize(int_point_to_iu(field_spec.size));

    if(field_spec.thickness > 0)
        field->SetTextThickness(field_spec.thickness);

    field->SetTextAngle(EDA_ANGLE(field_spec.angle_degrees, DEGREES_T));

    if(field_spec.layer >= 0)
        field->SetLayer(layer_id_from_int_or_throw(field_spec.layer));

    field->SetHorizJustify(static_cast<GR_TEXT_H_ALIGN_T>(field_spec.h_justify));
    field->SetVertJustify(static_cast<GR_TEXT_V_ALIGN_T>(field_spec.v_justify));
    field->SetMirrored(field_spec.mirrored);
    field->SetKeepUpright(field_spec.keep_upright);
}

std::vector<KkPolygon> custom_pad_polygons(const PAD& pad)
{
    if(pad.GetShape(F_Cu) != PAD_SHAPE::CUSTOM
        && pad.GetShape(PADSTACK::ALL_LAYERS) != PAD_SHAPE::CUSTOM)
    {
        return {};
    }

    SHAPE_POLY_SET polygon_set;
    pad.MergePrimitivesAsPolygon(F_Cu, &polygon_set);
    return polygons_from_poly_set(polygon_set);
}

void apply_footprint_attributes(FOOTPRINT& footprint, const KkFootprintSpec& spec)
{
    int attributes = footprint.GetAttributes();

    auto set_flag = [&](int flag, bool enabled) {
        if(enabled)
            attributes |= flag;
        else
            attributes &= ~flag;
    };

    set_flag(FP_EXCLUDE_FROM_POS_FILES, spec.excluded_from_pos);
    set_flag(FP_EXCLUDE_FROM_BOM, spec.excluded_from_bom);
    set_flag(FP_BOARD_ONLY, spec.board_only);
    set_flag(FP_DNP, spec.dnp);
    footprint.SetAttributes(attributes);
}

PAD_ATTRIB pad_attribute_from_int_or_throw(int attribute)
{
    if(attribute < static_cast<int>(PAD_ATTRIB::PTH)
        || attribute > static_cast<int>(PAD_ATTRIB::NPTH))
    {
        throw std::runtime_error("Unsupported KiCad pad attribute");
    }

    return static_cast<PAD_ATTRIB>(attribute);
}

void apply_pad_spec(BOARD& board, FOOTPRINT& footprint, PAD& pad, const KkPad& pad_spec)
{
    assign_uuid(pad, pad_spec.uuid);
    pad.SetNumber(to_wx_string(pad_spec.name));
    pad.SetNet(net_or_null(board, pad_spec.net));
    pad.SetAttribute(pad_attribute_from_int_or_throw(pad_spec.attribute));
    pad.SetPosition(point_to_iu(pad_spec.position));
    pad.SetSize(PADSTACK::ALL_LAYERS, point_to_iu(pad_spec.size));

    if(pad_spec.drill_size.size() >= 2)
    {
        pad.SetDrillSize(VECTOR2I(
            mm_to_iu(pad_spec.drill_size[0]),
            mm_to_iu(pad_spec.drill_size[1])));
    }

    pad.SetShape(PADSTACK::ALL_LAYERS, static_cast<PAD_SHAPE>(pad_spec.shape));
    pad.SetDrillShape(static_cast<PAD_DRILL_SHAPE>(pad_spec.drill_shape));

    if(!pad_spec.layers.empty())
    {
        LSET layer_set;

        for(int layer : pad_spec.layers)
            layer_set.set(static_cast<size_t>(layer_id_from_int_or_throw(layer)));

        pad.SetLayerSet(layer_set);
    }

    if(pad_spec.has_local_solder_mask_margin)
        pad.SetLocalSolderMaskMargin(pad_spec.local_solder_mask_margin);

    if(pad_spec.has_local_clearance)
        pad.SetLocalClearance(pad_spec.local_clearance);

    (void) footprint;
}

void apply_pad_specs(BOARD& board, FOOTPRINT& footprint, const std::vector<KkPad>& pads)
{
    auto footprint_pads = footprint.Pads();
    std::vector<PAD*> existing_pads(footprint_pads.begin(), footprint_pads.end());
    std::vector<bool> used(existing_pads.size(), false);

    for(const KkPad& pad_spec : pads)
    {
        PAD* pad = nullptr;

        for(std::size_t i = 0; i < existing_pads.size(); ++i)
        {
            if(used[i] || to_utf8_string(existing_pads[i]->GetNumber()) != pad_spec.name)
                continue;

            pad = existing_pads[i];
            used[i] = true;
            break;
        }

        if(!pad)
        {
            pad = new PAD(&footprint);
            footprint.Add(pad);
        }

        apply_pad_spec(board, footprint, *pad, pad_spec);
    }
}

void apply_footprint_spec(BOARD& board, FOOTPRINT& footprint, const KkFootprintSpec& spec)
{
    assign_uuid(footprint, spec.uuid);
    footprint.SetReference(to_wx_string(spec.reference.empty() ? "REF**" : spec.reference));
    footprint.SetValue(to_wx_string(spec.value));
    footprint.SetFPIDAsString(to_wx_string(spec.fpid));
    footprint.SetLayer(layer_id_from_int_or_throw(spec.layer));
    footprint.SetPosition(int_point_to_iu(spec.position));
    footprint.SetOrientation(EDA_ANGLE(spec.orientation_degrees, DEGREES_T));
    footprint.Reference().SetVisible(spec.reference_visible);
    footprint.Value().SetVisible(spec.value_visible);
    apply_footprint_attributes(footprint, spec);
    apply_pad_specs(board, footprint, spec.pads);

    for(const KkFootprintFieldSpec& field_spec : spec.fields)
        apply_field_spec(footprint, field_spec);

    std::vector<PCB_SHAPE*> existing_shapes;

    for(BOARD_ITEM* item : footprint.GraphicalItems())
    {
        if(PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>(item))
            existing_shapes.push_back(shape);
    }

    for(std::size_t i = 0; i < spec.drawings.size(); ++i)
    {
        if(i < existing_shapes.size())
        {
            apply_drawing_to_shape(footprint, *existing_shapes[i], spec.drawings[i]);
        }
        else
        {
            auto* shape = new PCB_SHAPE(&footprint, shape_type_from_int_or_throw(spec.drawings[i].shape));
            apply_drawing_to_shape(footprint, *shape, spec.drawings[i]);
            footprint.Add(shape);
        }
    }
}

std::shared_ptr<void> owned_footprint(FOOTPRINT* footprint)
{
    if(!footprint)
        return nullptr;

    footprint->SetParent(nullptr);
    return std::shared_ptr<void>(
        footprint,
        [](void* value) {
            delete static_cast<FOOTPRINT*>(value);
        });
}

std::shared_ptr<void> cloned_footprint(const FOOTPRINT& footprint)
{
    auto* native_clone = static_cast<FOOTPRINT*>(footprint.Clone());
    return owned_footprint(native_clone);
}

KkFootprint footprint_from_native(const FOOTPRINT& footprint, std::shared_ptr<void> native_footprint)
{
    std::vector<KkFootprintFieldSpec> fields;
    std::vector<KkPad> pads;
    std::vector<KkDrawing> drawings;

    for(const PCB_FIELD* field : footprint.GetFields())
        fields.push_back(field_spec_from_field(*field));

    fields.push_back(field_spec_from_text("Sheet file", footprint.GetSheetfile()));
    fields.push_back(field_spec_from_text("Sheet name", footprint.GetSheetname()));

    for(const BOARD_ITEM* item : footprint.GraphicalItems())
    {
        const PCB_SHAPE* shape = dynamic_cast<const PCB_SHAPE*>(item);

        if(shape)
            drawings.push_back(drawing_from_shape(*shape));
    }

    for(const PAD* pad : footprint.Pads())
    {
        KkPoint drill_size = point_from_iu(pad->GetDrillSize());
        KkPad pad_spec{
            to_utf8_string(pad->GetNumber()),
            to_utf8_string(pad->GetNetname()),
            pad->GetNetCode(),
            static_cast<int>(pad->GetAttribute()),
            point_from_iu(pad->GetPosition()),
            point_from_iu(pad->GetSize(PADSTACK::ALL_LAYERS)),
            { drill_size.x_mm, drill_size.y_mm },
            static_cast<int>(pad->GetShape(PADSTACK::ALL_LAYERS)),
            static_cast<int>(pad->GetDrillShape()),
            layers_from_lset(pad->GetLayerSet()),
            pad->GetLocalSolderMaskMargin().has_value(),
            pad->GetLocalSolderMaskMargin().value_or(0),
            pad->GetLocalClearance().has_value(),
            pad->GetLocalClearance().value_or(0),
            custom_pad_polygons(*pad)
        };
        pad_spec.uuid = pad->m_Uuid.AsStdString();
        pads.push_back(std::move(pad_spec));
    }

    if(!native_footprint)
        native_footprint = cloned_footprint(footprint);

    return KkFootprint(
        to_utf8_string(footprint.GetReference()),
        to_utf8_string(footprint.GetValue()),
        to_utf8_string(footprint.GetFPIDAsString()),
        point_from_iu(footprint.GetPosition()),
        static_cast<int>(footprint.GetLayer()),
        footprint.GetOrientation().AsDegrees(),
        footprint.IsExcludedFromPosFiles(),
        footprint.IsExcludedFromBOM(),
        footprint.GetAttributes() & FP_BOARD_ONLY,
        footprint.GetAttributes() & FP_DNP,
        std::move(fields),
        std::move(pads),
        std::move(drawings),
        footprint.m_Uuid.AsStdString(),
        std::move(native_footprint));
}

#else

[[noreturn]] void native_backend_unavailable()
{
    throw BackendUnavailableError(
        "The pybind11_kicad_native extension was built without KiCad board IO. "
        "Build with scripts/run.sh build-pybind11-kicad or configure with "
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

KkFootprint::KkFootprint(
    std::string reference,
    std::string value,
    std::string fpid,
    KkPoint position,
    int layer,
    double orientation_degrees,
    bool excluded_from_pos,
    bool excluded_from_bom,
    bool board_only,
    bool dnp,
    std::vector<KkFootprintFieldSpec> fields,
    std::vector<KkPad> pads,
    std::vector<KkDrawing> drawings,
    std::string uuid,
    std::shared_ptr<void> native_footprint)
    : reference_(std::move(reference)),
      value_(std::move(value)),
      fpid_(std::move(fpid)),
      position_(position),
      layer_(layer),
      orientation_degrees_(orientation_degrees),
      excluded_from_pos_(excluded_from_pos),
      excluded_from_bom_(excluded_from_bom),
      board_only_(board_only),
      dnp_(dnp),
      fields_(std::move(fields)),
      pads_(std::move(pads)),
      drawings_(std::move(drawings)),
      uuid_(std::move(uuid)),
      native_footprint_(std::move(native_footprint))
{
}

const std::string& KkFootprint::reference() const
{
    return reference_;
}

const std::string& KkFootprint::value() const
{
    return value_;
}

const std::string& KkFootprint::fpid() const
{
    return fpid_;
}

KkPoint KkFootprint::position() const
{
    return position_;
}

int KkFootprint::layer() const
{
    return layer_;
}

double KkFootprint::orientation_degrees() const
{
    return orientation_degrees_;
}

bool KkFootprint::excluded_from_pos() const
{
    return excluded_from_pos_;
}

bool KkFootprint::excluded_from_bom() const
{
    return excluded_from_bom_;
}

bool KkFootprint::board_only() const
{
    return board_only_;
}

bool KkFootprint::dnp() const
{
    return dnp_;
}

const std::vector<KkFootprintFieldSpec>& KkFootprint::fields() const
{
    return fields_;
}

const std::vector<KkPad>& KkFootprint::pads() const
{
    return pads_;
}

const std::vector<KkDrawing>& KkFootprint::drawings() const
{
    return drawings_;
}

const std::string& KkFootprint::uuid() const
{
    return uuid_;
}

bool KkFootprint::has_native_footprint() const
{
    return static_cast<bool>(native_footprint_);
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

KkBoard KkBoard::create(const std::string& path)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    auto board = std::make_unique<BOARD>();
    board->SetFileName(to_wx_string(path));

    auto impl = std::make_unique<Impl>();
    impl->board = std::move(board);
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

KkDesignSettings KkBoard::design_settings() const
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot inspect an uninitialized KiCad board");

    return KkDesignSettings{
        impl_->board->GetDesignSettings().GetBoardThickness(),
        int_point_from_iu(impl_->board->GetDesignSettings().GetAuxOrigin())
    };
#else
    native_backend_unavailable();
#endif
}

void KkBoard::set_board_thickness(int thickness)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot edit an uninitialized KiCad board");

    impl_->board->GetDesignSettings().SetBoardThickness(thickness);
#else
    (void) thickness;
    native_backend_unavailable();
#endif
}

void KkBoard::set_aux_origin(const KkIntPoint& origin)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot edit an uninitialized KiCad board");

    impl_->board->GetDesignSettings().SetAuxOrigin(int_point_to_iu(origin));
#else
    (void) origin;
    native_backend_unavailable();
#endif
}

KkTitleBlock KkBoard::title_block() const
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot inspect an uninitialized KiCad board");

    const TITLE_BLOCK& native_title_block = impl_->board->GetTitleBlock();
    KkTitleBlock result;
    result.title = to_utf8_string(native_title_block.GetTitle());

    for(int i = 0; i < 9; ++i)
        result.comments.push_back(to_utf8_string(native_title_block.GetComment(i)));

    while(!result.comments.empty() && result.comments.back().empty())
        result.comments.pop_back();

    return result;
#else
    native_backend_unavailable();
#endif
}

void KkBoard::set_title_block(const KkTitleBlock& title_block)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot edit an uninitialized KiCad board");

    TITLE_BLOCK native_title_block = impl_->board->GetTitleBlock();
    native_title_block.SetTitle(to_wx_string(title_block.title));

    for(std::size_t i = 0; i < title_block.comments.size(); ++i)
        native_title_block.SetComment(static_cast<int>(i), to_wx_string(title_block.comments[i]));

    impl_->board->SetTitleBlock(native_title_block);
#else
    (void) title_block;
    native_backend_unavailable();
#endif
}

int KkBoard::copper_layer_count() const
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot inspect an uninitialized KiCad board");

    return impl_->board->GetCopperLayerCount();
#else
    native_backend_unavailable();
#endif
}

void KkBoard::set_copper_layer_count(int count)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot edit an uninitialized KiCad board");

    impl_->board->SetCopperLayerCount(count);
#else
    (void) count;
    native_backend_unavailable();
#endif
}

std::vector<int> KkBoard::enabled_layers() const
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot inspect an uninitialized KiCad board");

    std::vector<int> result;
    const LSET& layers = impl_->board->GetEnabledLayers();

    for(int layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer)
    {
        if(layers.Contains(static_cast<PCB_LAYER_ID>(layer)))
            result.push_back(layer);
    }

    return result;
#else
    native_backend_unavailable();
#endif
}

void KkBoard::set_enabled_layers(const std::vector<int>& layers)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot edit an uninitialized KiCad board");

    LSET layer_set;

    for(int layer : layers)
        layer_set.set(static_cast<size_t>(layer_id_from_int_or_throw(layer)));

    impl_->board->SetEnabledLayers(layer_set);
#else
    (void) layers;
    native_backend_unavailable();
#endif
}

std::string KkBoard::get_layer_name(int layer_id) const
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot inspect an uninitialized KiCad board");

    return to_utf8_string(impl_->board->GetLayerName(layer_id_from_int_or_throw(layer_id)));
#else
    (void) layer_id;
    native_backend_unavailable();
#endif
}

int KkBoard::get_layer_id(const std::string& name) const
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot inspect an uninitialized KiCad board");

    return static_cast<int>(impl_->board->GetLayerID(to_wx_string(name)));
#else
    (void) name;
    native_backend_unavailable();
#endif
}

bool KkBoard::set_layer_name(int layer_id, const std::string& name)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot edit an uninitialized KiCad board");

    return impl_->board->SetLayerName(layer_id_from_int_or_throw(layer_id), to_wx_string(name));
#else
    (void) layer_id;
    (void) name;
    native_backend_unavailable();
#endif
}

std::vector<KkNetInfo> KkBoard::nets() const
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot inspect an uninitialized KiCad board");

    std::vector<KkNetInfo> result;

    for(const NETINFO_ITEM* net : impl_->board->GetNetInfo())
    {
        if(!net)
            continue;

        result.push_back(KkNetInfo{ to_utf8_string(net->GetNetname()), net->GetNetCode() });
    }

    return result;
#else
    native_backend_unavailable();
#endif
}

std::vector<KkDrawing> KkBoard::drawings() const
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot inspect an uninitialized KiCad board");

    std::vector<KkDrawing> result;

    for(const BOARD_ITEM* item : impl_->board->Drawings())
    {
        const PCB_SHAPE* shape = dynamic_cast<const PCB_SHAPE*>(item);

        if(!shape)
            continue;

        result.push_back(drawing_from_shape(*shape));
    }

    return result;
#else
    native_backend_unavailable();
#endif
}

std::vector<KkTextSpec> KkBoard::texts() const
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot inspect an uninitialized KiCad board");

    std::vector<KkTextSpec> result;

    for(const BOARD_ITEM* item : impl_->board->Drawings())
    {
        const PCB_TEXT* text = dynamic_cast<const PCB_TEXT*>(item);

        if(text)
            result.push_back(text_spec_from_text(*text));
    }

    return result;
#else
    native_backend_unavailable();
#endif
}

std::vector<KkZoneItem> KkBoard::zones() const
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot inspect an uninitialized KiCad board");

    std::vector<KkZoneItem> result;

    for(const ZONE* zone : impl_->board->Zones())
        result.push_back(zone_from_native(*zone));

    return result;
#else
    native_backend_unavailable();
#endif
}

std::vector<KkTrackItem> KkBoard::tracks() const
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot inspect an uninitialized KiCad board");

    std::vector<KkTrackItem> result;

    for(const PCB_TRACK* track : impl_->board->Tracks())
    {
        if(track->Type() == PCB_VIA_T)
            continue;

        result.push_back(track_from_native(*track));
    }

    return result;
#else
    native_backend_unavailable();
#endif
}

std::vector<KkViaItem> KkBoard::vias() const
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot inspect an uninitialized KiCad board");

    std::vector<KkViaItem> result;

    for(const PCB_TRACK* track : impl_->board->Tracks())
    {
        if(track->Type() != PCB_VIA_T)
            continue;

        result.push_back(via_from_native(static_cast<const PCB_VIA&>(*track)));
    }

    return result;
#else
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
        result.push_back(footprint_from_native(*footprint, nullptr));

    return result;
#else
    native_backend_unavailable();
#endif
}

void KkBoard::add_drawing(const KkDrawing& drawing)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot edit an uninitialized KiCad board");

    auto* shape = new PCB_SHAPE(impl_->board.get(), shape_type_from_int_or_throw(drawing.shape));
    assign_uuid(*shape, drawing.uuid);
    shape->SetLayer(layer_id_from_int_or_throw(drawing.layer));
    shape->SetWidth(drawing.width);
    shape->SetFilled(drawing.filled);

    if(drawing.shape == static_cast<int>(SHAPE_T::ARC))
    {
        shape->SetArcGeometry(
            int_point_to_iu(drawing.start),
            int_point_to_iu(drawing.mid),
            int_point_to_iu(drawing.end));
    }
    else
    {
        shape->SetStart(int_point_to_iu(drawing.start));
        shape->SetEnd(int_point_to_iu(drawing.end));
    }

    if(drawing.shape == static_cast<int>(SHAPE_T::CIRCLE))
    {
        shape->SetCenter(int_point_to_iu(drawing.center));
        shape->SetRadius(drawing.radius);
    }
    else if(drawing.shape == static_cast<int>(SHAPE_T::POLY))
    {
        SHAPE_POLY_SET& poly = shape->GetPolyShape();
        int outline = poly.NewOutline();

        for(const KkIntPoint& point : drawing.polygon_points)
            poly.Append(int_point_to_iu(point), outline);
    }

    impl_->board->Add(shape);
#else
    (void) drawing;
    native_backend_unavailable();
#endif
}

bool KkBoard::remove_drawing(const KkDrawing& drawing)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot edit an uninitialized KiCad board");

    BOARD_ITEM* match = nullptr;

    for(BOARD_ITEM* item : impl_->board->Drawings())
    {
        PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>(item);

        if(shape && drawing_matches_shape(drawing, *shape))
        {
            match = item;
            break;
        }
    }

    if(!match)
        return false;

    impl_->board->Delete(match);
    return true;
#else
    (void) drawing;
    native_backend_unavailable();
#endif
}

void KkBoard::add_npth_hole(const KkNpthSpec& spec)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot edit an uninitialized KiCad board");

    VECTOR2I position = int_point_to_iu(spec.position);
    VECTOR2I drill_size = int_point_to_iu(spec.drill_size);
    VECTOR2I pad_size = int_point_to_iu(spec.size);
    bool is_slot = drill_size.x != drill_size.y || pad_size.x != pad_size.y;

    auto* footprint = new FOOTPRINT(impl_->board.get());
    assign_uuid(*footprint, spec.uuid);
    footprint->SetReference(to_wx_string(spec.reference.empty() ? "NPTH" : spec.reference));
    footprint->SetValue(wxS("NPTH"));
    footprint->Reference().SetVisible(false);
    footprint->Value().SetVisible(false);
    footprint->SetPosition(position);

    auto* pad = new PAD(footprint);
    assign_uuid(*pad, spec.pad_uuid);
    pad->SetAttribute(PAD_ATTRIB::NPTH);
    pad->SetLayerSet(PAD::UnplatedHoleMask());
    pad->SetShape(PADSTACK::ALL_LAYERS, is_slot ? PAD_SHAPE::OVAL : PAD_SHAPE::CIRCLE);
    pad->SetDrillShape(is_slot ? PAD_DRILL_SHAPE::OBLONG : PAD_DRILL_SHAPE::CIRCLE);
    pad->SetSize(PADSTACK::ALL_LAYERS, pad_size);
    pad->SetDrillSize(drill_size);
    pad->SetPosition(position);
    footprint->Add(pad);
    footprint->SetOrientation(EDA_ANGLE(spec.orientation_degrees, DEGREES_T));
    impl_->board->Add(footprint);
#else
    (void) spec;
    native_backend_unavailable();
#endif
}

void KkBoard::add_footprint(const KkFootprintSpec& spec)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot edit an uninitialized KiCad board");

    auto* footprint = new FOOTPRINT(impl_->board.get());
    apply_footprint_spec(*impl_->board, *footprint, spec);
    impl_->board->Add(footprint);
#else
    (void) spec;
    native_backend_unavailable();
#endif
}

void KkBoard::add_footprint_clone(const KkFootprint& source, const KkFootprintSpec& spec)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot edit an uninitialized KiCad board");

    if(!source.native_footprint_)
    {
        add_footprint(spec);
        return;
    }

    const FOOTPRINT* source_footprint =
        static_cast<const FOOTPRINT*>(source.native_footprint_.get());
    auto* footprint = static_cast<FOOTPRINT*>(source_footprint->Duplicate(false, nullptr));
    apply_footprint_spec(*impl_->board, *footprint, spec);
    impl_->board->Add(footprint);
#else
    (void) source;
    (void) spec;
    native_backend_unavailable();
#endif
}

void KkBoard::add_text(const KkTextSpec& spec)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot edit an uninitialized KiCad board");

    auto* text = new PCB_TEXT(impl_->board.get());
    assign_uuid(*text, spec.uuid);
    text->SetText(to_wx_string(spec.text));
    text->SetLayer(layer_id_from_int_or_throw(spec.layer));
    text->SetPosition(int_point_to_iu(spec.position));
    text->SetTextSize(int_point_to_iu(spec.size));
    text->SetTextThickness(spec.thickness);
    text->SetTextAngle(EDA_ANGLE(spec.angle_degrees, DEGREES_T));
    text->SetHorizJustify(static_cast<GR_TEXT_H_ALIGN_T>(spec.h_justify));
    text->SetVertJustify(static_cast<GR_TEXT_V_ALIGN_T>(spec.v_justify));
    text->SetMirrored(spec.mirrored);
    impl_->board->Add(text);
#else
    (void) spec;
    native_backend_unavailable();
#endif
}

bool KkBoard::remove_text(const KkTextSpec& spec)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot edit an uninitialized KiCad board");

    BOARD_ITEM* match = nullptr;

    for(BOARD_ITEM* item : impl_->board->Drawings())
    {
        PCB_TEXT* text = dynamic_cast<PCB_TEXT*>(item);

        if(text && text_matches_spec(spec, *text))
        {
            match = item;
            break;
        }
    }

    if(!match)
        return false;

    impl_->board->Delete(match);
    return true;
#else
    (void) spec;
    native_backend_unavailable();
#endif
}

void KkBoard::add_track(const KkTrackSpec& spec)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot edit an uninitialized KiCad board");

    auto* track = new PCB_TRACK(impl_->board.get());
    assign_uuid(*track, spec.uuid);
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
    assign_uuid(*via, spec.uuid);
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

void KkBoard::add_track_item(const KkTrackItem& spec)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot edit an uninitialized KiCad board");

    PCB_TRACK* track = nullptr;

    if(spec.is_arc)
    {
        auto* arc = new PCB_ARC(impl_->board.get());
        assign_uuid(*arc, spec.uuid);
        arc->SetStart(int_point_to_iu(spec.start));
        arc->SetMid(int_point_to_iu(spec.mid));
        arc->SetEnd(int_point_to_iu(spec.end));
        track = arc;
    }
    else
    {
        track = new PCB_TRACK(impl_->board.get());
        assign_uuid(*track, spec.uuid);
        track->SetStart(int_point_to_iu(spec.start));
        track->SetEnd(int_point_to_iu(spec.end));
    }

    track->SetWidth(spec.width);
    track->SetLayer(layer_id_from_int_or_throw(spec.layer));
    track->SetNet(net_or_null(*impl_->board, spec.net));
    impl_->board->Add(track);
#else
    (void) spec;
    native_backend_unavailable();
#endif
}

void KkBoard::add_via_item(const KkViaItem& spec)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot edit an uninitialized KiCad board");

    auto* via = new PCB_VIA(impl_->board.get());
    assign_uuid(*via, spec.uuid);
    via->SetViaType(static_cast<VIATYPE>(spec.via_type));
    via->SetPosition(int_point_to_iu(spec.position));
    via->SetWidth(spec.diameter);
    via->SetDrill(spec.drill);

    if(!spec.layers.empty())
    {
        LSET layer_set;

        for(int layer : spec.layers)
            layer_set.set(static_cast<size_t>(layer_id_from_int_or_throw(layer)));

        via->SetLayerSet(layer_set);
    }
    else
    {
        via->SetLayerPair(F_Cu, B_Cu);
    }

    via->SetNet(net_or_null(*impl_->board, spec.net));
    impl_->board->Add(via);
#else
    (void) spec;
    native_backend_unavailable();
#endif
}

void KkBoard::add_zone_item(const KkZoneItem& spec)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot edit an uninitialized KiCad board");

    auto* zone = new ZONE(impl_->board.get());
    assign_uuid(*zone, spec.uuid);
    zone->SetAssignedPriority(spec.priority);
    zone->SetZoneName(to_wx_string(spec.name));
    zone->SetFillMode(static_cast<ZONE_FILL_MODE>(spec.fill_mode));
    zone->SetIsRuleArea(spec.is_rule_area);

    if(!spec.layers.empty())
    {
        LSET layer_set;

        for(int layer : spec.layers)
            layer_set.set(static_cast<size_t>(layer_id_from_int_or_throw(layer)));

        zone->SetLayerSet(layer_set);
    }

    if(!spec.net.empty())
        zone->SetNet(net_or_null(*impl_->board, spec.net));

    SHAPE_POLY_SET* poly = zone->Outline();
    poly->RemoveAllContours();

    add_polygons_to_poly_set(*poly, spec.polygons);

    for(const KkZoneFill& fill : spec.fills)
    {
        if(fill.polygons.empty())
            continue;

        SHAPE_POLY_SET fill_poly;
        add_polygons_to_poly_set(fill_poly, fill.polygons);
        zone->SetFilledPolysList(layer_id_from_int_or_throw(fill.layer), fill_poly);
    }

    zone->SetIsFilled(spec.is_filled || !spec.fills.empty());
    zone->SetNeedRefill(false);

    impl_->board->Add(zone);
#else
    (void) spec;
    native_backend_unavailable();
#endif
}

bool KkBoard::remove_zone_item(const KkZoneItem& spec)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    if(!impl_ || !impl_->board)
        throw std::runtime_error("Cannot edit an uninitialized KiCad board");

    ZONE* match = nullptr;

    for(ZONE* zone : impl_->board->Zones())
    {
        if(zone && zone_items_equal(zone_from_native(*zone), spec))
        {
            match = zone;
            break;
        }
    }

    if(!match)
        return false;

    impl_->board->Delete(match);
    return true;
#else
    (void) spec;
    native_backend_unavailable();
#endif
}

KkFootprint load_footprint(
    const std::string& library_path,
    const std::string& footprint_name,
    bool preserve_uuid)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    wxString lib_path = to_wx_string(library_path);
    PCB_IO_MGR::PCB_FILE_T file_type = PCB_IO_MGR::GuessPluginTypeFromLibPath(lib_path);

    if(file_type == PCB_IO_MGR::FILE_TYPE_NONE)
        file_type = PCB_IO_MGR::KICAD_SEXP;

    IO_RELEASER<PCB_IO> plugin(PCB_IO_MGR::FindPlugin(file_type));

    if(!plugin)
        throw std::runtime_error("No KiCad PCB_IO plugin for footprint library: " + library_path);

    FOOTPRINT* footprint = plugin->FootprintLoad(
        lib_path,
        to_wx_string(footprint_name),
        preserve_uuid);

    if(!footprint)
    {
        throw std::runtime_error(
            "KiCad footprint not found: " + footprint_name + " in " + library_path);
    }

    std::shared_ptr<void> native_footprint = owned_footprint(footprint);
    return footprint_from_native(
        *static_cast<const FOOTPRINT*>(native_footprint.get()),
        std::move(native_footprint));
#else
    (void) library_path;
    (void) footprint_name;
    (void) preserve_uuid;
    native_backend_unavailable();
#endif
}

void seed_kiid_generator(unsigned int seed)
{
#if defined(PYBIND11_KICAD_ENABLE_KICAD_BOARD_IO)
    KIID::SeedGenerator(seed);
#else
    (void) seed;
    native_backend_unavailable();
#endif
}

}  // namespace pybind11_kicad
