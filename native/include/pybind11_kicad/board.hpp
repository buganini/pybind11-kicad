#pragma once

#include <memory>
#include <string>
#include <vector>

#include "pybind11_kicad/types.hpp"

namespace pybind11_kicad {

class KkFootprint {
public:
    KkFootprint(
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
        std::shared_ptr<void> native_footprint = nullptr);

    const std::string& reference() const;
    const std::string& value() const;
    const std::string& fpid() const;
    KkPoint position() const;
    int layer() const;
    double orientation_degrees() const;
    bool excluded_from_pos() const;
    bool excluded_from_bom() const;
    bool board_only() const;
    bool dnp() const;
    const std::vector<KkFootprintFieldSpec>& fields() const;
    const std::vector<KkPad>& pads() const;
    const std::vector<KkDrawing>& drawings() const;
    const std::string& uuid() const;
    bool has_native_footprint() const;

private:
    friend class KkBoard;

    std::string reference_;
    std::string value_;
    std::string fpid_;
    KkPoint position_;
    int layer_ = 0;
    double orientation_degrees_ = 0.0;
    bool excluded_from_pos_ = false;
    bool excluded_from_bom_ = false;
    bool board_only_ = false;
    bool dnp_ = false;
    std::vector<KkFootprintFieldSpec> fields_;
    std::vector<KkPad> pads_;
    std::vector<KkDrawing> drawings_;
    std::string uuid_;
    std::shared_ptr<void> native_footprint_;
};

class KkBoard {
public:
    static KkBoard open(const std::string& path);
    static KkBoard create(const std::string& path);

    ~KkBoard();
    KkBoard(KkBoard&& other) noexcept;
    KkBoard& operator=(KkBoard&& other) noexcept;
    KkBoard(const KkBoard& other) = delete;
    KkBoard& operator=(const KkBoard& other) = delete;

    void save(const std::string& path) const;
    KkDesignSettings design_settings() const;
    void set_board_thickness(int thickness);
    void set_aux_origin(const KkIntPoint& origin);
    KkTitleBlock title_block() const;
    void set_title_block(const KkTitleBlock& title_block);
    int copper_layer_count() const;
    void set_copper_layer_count(int count);
    std::vector<int> enabled_layers() const;
    void set_enabled_layers(const std::vector<int>& layers);
    std::string get_layer_name(int layer_id) const;
    int get_layer_id(const std::string& name) const;
    bool set_layer_name(int layer_id, const std::string& name);
    std::vector<KkNetInfo> nets() const;
    std::vector<KkDrawing> drawings() const;
    std::vector<KkTextSpec> texts() const;
    std::vector<KkZoneItem> zones() const;
    std::vector<KkTrackItem> tracks() const;
    std::vector<KkViaItem> vias() const;
    std::vector<KkFootprint> footprints() const;
    void add_drawing(const KkDrawing& drawing);
    bool remove_drawing(const KkDrawing& drawing);
    void add_npth_hole(const KkNpthSpec& spec);
    void add_footprint(const KkFootprintSpec& spec);
    void add_footprint_clone(const KkFootprint& source, const KkFootprintSpec& spec);
    void add_text(const KkTextSpec& spec);
    bool remove_text(const KkTextSpec& spec);
    void add_track(const KkTrackSpec& spec);
    void add_via(const KkViaSpec& spec);
    void add_track_item(const KkTrackItem& spec);
    void add_via_item(const KkViaItem& spec);
    void add_zone_item(const KkZoneItem& spec);
    bool remove_zone_item(const KkZoneItem& spec);

private:
    struct Impl;

    explicit KkBoard(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> impl_;
};

KkFootprint load_footprint(
    const std::string& library_path,
    const std::string& footprint_name,
    bool preserve_uuid = false);

void seed_kiid_generator(unsigned int seed);

}  // namespace pybind11_kicad
