#pragma once

#include <cmath>
#include <cstddef>
#include <map>
#include <memory>
#include <vector>

#include "triskel/analysis/sese.hpp"
#include "triskel/graph/igraph.hpp"
#include "triskel/graph/subgraph.hpp"
#include "triskel/layout/ilayout.hpp"
#include "triskel/layout/sugiyama/sugiyama.hpp"
#include "triskel/utils/attribute.hpp"

namespace triskel {

struct Layout : public ILayout {
    Layout(Graph& g,
           const NodeAttribute<float>& heights,
           const NodeAttribute<float>& widths);
    explicit Layout(Graph& g);

    [[nodiscard]] auto get_x(NodeId node) const -> float override;
    [[nodiscard]] auto get_y(NodeId node) const -> float override;
    [[nodiscard]] auto get_waypoints(EdgeId edge) const
        -> const std::vector<Point>& override;

    [[nodiscard]] auto get_width(NodeId node) const -> float override;
    [[nodiscard]] auto get_height(NodeId node) const -> float override;

    [[nodiscard]] auto region_count() const -> size_t {
        return sese_->regions.nodes.size();
    }

   private:
    NodeAttribute<float> xs_;
    NodeAttribute<float> ys_;

    NodeAttribute<float> heights_;
    NodeAttribute<float> widths_;

    EdgeAttribute<std::vector<Point>> waypoints_;

    EdgeAttribute<float> start_x_offset_;
    EdgeAttribute<float> end_x_offset_;

    struct RegionData {
        explicit RegionData(Graph& g);

        SubGraph subgraph;
        NodeId node_id;

        std::vector<IOPair> entries;
        std::vector<IOPair> exits;

        std::map<Pair, std::vector<Point>> io_waypoints;

        bool was_layout = false;

        float width;
        float height;
    };

    /// @brief Remove SESE regions with a single node
    void remove_small_regions();

    std::vector<RegionData> regions_data_;

    /// @brief Edits the region subgraphs so that each region's subgraph
    /// contains its children region phony nodes
    /// Edges also need to be stitched accordingly
    void edit_region_subgraph();

    /// @brief Edit a region's entry edge
    void edit_region_entry(const SESE::SESERegion& r);

    /// @brief Edit a region's exit edge
    void edit_region_exit(const SESE::SESERegion& r);

    auto get_region_node(const SESE::SESERegion& r) const -> Node;

    auto get_editor(const SESE::SESERegion& r) -> SubGraphEditor&;

    void compute_layout(const SESE::SESERegion& r);

    void translate_region(const SESE::SESERegion& r, const Point& v);

    void translate_region(const SESE::SESERegion& r);

    /// @brief Initiates the regions
    void init_regions();

    /// @brief Creates a subgraph for each SESE region
    void create_region_subgraphs();

    /// @brief Creates a phony node for each SESE region
    void create_region_nodes();

    std::unique_ptr<SESE> sese_;
    Graph& g_;
};
}  // namespace triskel