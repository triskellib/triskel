#include "triskel/layout/layout.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <deque>
#include <memory>
#include <vector>

#include "triskel/analysis/sese.hpp"
#include "triskel/graph/graph.hpp"
#include "triskel/graph/igraph.hpp"
#include "triskel/graph/subgraph.hpp"
#include "triskel/layout/phantom_nodes.hpp"
#include "triskel/layout/sugiyama/sugiyama.hpp"
#include "triskel/utils/attribute.hpp"
#include "triskel/utils/constants.hpp"
#include "triskel/utils/point.hpp"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

Layout::Layout(Graph& g)
    : Layout(g, NodeAttribute<float>{g, 1.0F}, NodeAttribute<float>{g, 1.0F}) {}

Layout::Layout(Graph& g,
               const NodeAttribute<float>& heights,
               const NodeAttribute<float>& widths)
    : g_{g},
      xs_(g, 0.0F),
      ys_(g, 0),
      waypoints_(g, {}),
      start_x_offset_(g, -1),
      end_x_offset_(g, -1),
      heights_(heights),
      widths_(widths)

{
    // g.editor().push();

    // Add fake nodes to help sese
    // create_phantom_nodes(g);

    sese_ = std::make_unique<SESE>(g);
    // g.editor().pop();

    remove_small_regions();

    g.editor().push();
    init_regions();

    compute_layout(*sese_->regions.root);
    translate_region(*sese_->regions.root);

    g.editor().pop();

    // Tie loose ends
    for (const auto& region : sese_->regions.nodes) {
        auto& data = regions_data_[region->id];

        // Add the exit waypoints
        for (auto exit_pair : data.exits) {
            auto& exit_waypoints = data.io_waypoints[exit_pair];
            assert(!exit_waypoints.empty());
            auto& waypoints = waypoints_.get(exit_pair.edge);

            // Remove the nodes connecting it to the next layer
            waypoints.erase(waypoints.begin());
            waypoints.front().x = exit_waypoints.back().x;
            exit_waypoints.pop_back();

            waypoints.insert(waypoints.begin(), exit_waypoints.begin(),
                             exit_waypoints.end());
        }

        for (auto entry_pair : data.entries) {
            auto& entry_waypoints = data.io_waypoints[entry_pair];
            assert(!entry_waypoints.empty());
            auto& waypoints = waypoints_.get(entry_pair.edge);

            // Remove the nodes connecting it to the next layer
            waypoints.pop_back();
            waypoints.back().x = entry_waypoints.front().x;
            entry_waypoints.erase(entry_waypoints.begin());

            waypoints.insert(waypoints.end(), entry_waypoints.begin(),
                             entry_waypoints.end());
        }
    }
}

void Layout::remove_small_regions() {
    // Remove SESE regions with a single node
    std::deque<SESE::SESERegion*> small_regions;
    for (auto& region : sese_->regions.nodes) {
        if ((*region)->nodes.size() == 1 && region->children().empty() &&
            !region->is_root()) {
            small_regions.push_back(region.get());
        }
    }

    while (!small_regions.empty()) {
        auto& region = small_regions.back();
        small_regions.pop_back();

        auto node_id = (*region)->nodes.front();

        region->parent()->nodes.push_back(node_id);
        sese_->node_regions.set(node_id, &region->parent());

        sese_->regions.remove_node(region);
    }

    // fix the ids
    for (size_t i = 0; i < sese_->regions.nodes.size(); ++i) {
        sese_->regions.nodes[i]->id = i;
    }
}

void Layout::create_region_subgraphs() {
    for (const auto& node : g_.nodes()) {
        const auto& r = sese_->get_region(node);
        auto& editor  = get_editor(r);
        editor.select_node(node);
    }
}

void Layout::create_region_nodes() {
    auto& editor = g_.editor();

    for (const auto& r : sese_->regions.nodes) {
        auto& data   = regions_data_[r->id];
        data.node_id = editor.make_node().id();
    }

    // Adds the region nodes to the appropriate regions
    for (const auto& r : sese_->regions.nodes) {
        if (r->is_root()) {
            continue;
        }

        const auto& parent_region = r->parent();
        auto& editor = regions_data_[parent_region.id].subgraph.editor();
        editor.select_node(regions_data_[r->id].node_id);
    }
}

void Layout::edit_region_entry(const SESE::SESERegion& r) {
    auto entry_id = r->entry_edge;
    if (entry_id == EdgeId::InvalidID) {
        return;
    }

    const auto entry = g_.get_edge(entry_id);

    const auto& parent_region = r.parent();
    const auto& from_region   = sese_->get_region(entry.from());

    auto to_node = get_region_node(r);

    auto from_node = from_region == parent_region
                         ? entry.from()
                         : get_region_node(from_region);

    auto& ge = get_editor(parent_region);
    ge.select_node(to_node);
    ge.edit_edge(entry, from_node, to_node);
}

void Layout::edit_region_exit(const SESE::SESERegion& r) {
    auto exit_id = r->exit_edge;
    if (exit_id == EdgeId::InvalidID) {
        return;
    }

    const auto exit = g_.get_edge(exit_id);

    const auto& parent_region = r.parent();
    const auto& to_region     = sese_->get_region(exit.to());

    auto from_node = get_region_node(r);

    auto to_node =
        to_region == parent_region ? exit.to() : get_region_node(to_region);

    auto& ge = get_editor(parent_region);
    ge.select_node(to_node);
    ge.edit_edge(exit, from_node, to_node);
}

namespace {
// Is `successor` a successor of `region` that is, is `successor` contained
// within `region`
// NOLINTNEXTLINE(misc-no-recursion)
auto is_region_successor(const SESE::SESERegion& region,
                         const SESE::SESERegion& successor) -> bool {
    if (successor == region) {
        return true;
    }

    if (successor.depth <= region.depth) {
        return false;
    }

    return is_region_successor(region, successor.parent());
}

[[nodiscard]] auto get_closest_ancestor(const SESE::SESERegion& r1,
                                        const SESE::SESERegion& r2)
    -> const SESE::SESERegion& {
    auto depth = std::min(r1.depth, r2.depth);

    const auto* r1_ = &r1;
    const auto* r2_ = &r2;

    while (r1_->depth != depth) {
        r1_ = &r1_->parent();
    }

    while (r2_->depth != depth) {
        r2_ = &r2_->parent();
    }

    while (r1_ != r2_) {
        r1_ = &r1_->parent();
        r2_ = &r2_->parent();
    }

    assert(r1_ == r2_);

    return *r1_;
}

}  // namespace

void Layout::edit_region_subgraph() {
    // return;
    for (const auto& edge : g_.edges()) {
        const auto* from_region = &sese_->get_region(edge.from());
        const auto* to_region   = &sese_->get_region(edge.to());

        if (from_region != to_region) {
            if (is_region_successor(*from_region, *to_region)) {
                // This is an entry edge
                const auto* parent_region = from_region;
                const auto* child_region  = to_region;
                auto node_id              = edge.to().id();

                while (child_region != parent_region) {
                    // make everyone an entry node
                    regions_data_[child_region->id].entries.push_back(
                        {node_id, edge.id()});

                    node_id      = regions_data_[child_region->id].node_id;
                    child_region = &child_region->parent();
                }

                auto& ge = get_editor(*parent_region);
                ge.edit_edge(edge, edge.from(), node_id);
            } else if (is_region_successor(*to_region, *from_region)) {
                // This is an exit edge
                const auto* parent_region = to_region;
                const auto* child_region  = from_region;
                auto node_id              = edge.from().id();

                while (child_region != parent_region) {
                    // make everyone an exit node
                    regions_data_[child_region->id].exits.push_back(
                        {node_id, edge.id()});

                    node_id      = regions_data_[child_region->id].node_id;
                    child_region = &child_region->parent();
                }

                auto& ge = get_editor(*parent_region);
                ge.edit_edge(edge, node_id, edge.to());
            } else {
                // The nodes connect through their parent
                const auto& closest_ancestor =
                    get_closest_ancestor(*to_region, *from_region);

                auto from_id = edge.from().id();
                while (from_region != &closest_ancestor) {
                    // make everyone an exit node
                    regions_data_[from_region->id].exits.push_back(
                        {from_id, edge.id()});

                    from_id     = regions_data_[from_region->id].node_id;
                    from_region = &from_region->parent();
                }

                auto to_id = edge.to().id();
                while (to_region != &closest_ancestor) {
                    // make everyone an entry node
                    regions_data_[to_region->id].entries.push_back(
                        {to_id, edge.id()});

                    to_id     = regions_data_[to_region->id].node_id;
                    to_region = &to_region->parent();
                }

                auto& ge = get_editor(closest_ancestor);
                ge.edit_edge(edge, from_id, to_id);
            }
        }
    }
}

void Layout::init_regions() {
    // Creates the empty region structures
    regions_data_.reserve(sese_->regions.nodes.size());
    for (const auto& region : sese_->regions.nodes) {
        regions_data_.emplace_back(g_);
    }

    create_region_subgraphs();
    create_region_nodes();
    edit_region_subgraph();
}

auto Layout::get_x(NodeId node) const -> float {
    return xs_.get(node);
}

auto Layout::get_y(NodeId node) const -> float {
    return ys_.get(node);
}

auto Layout::get_width(NodeId node) const -> float {
    return widths_.get(node);
}

auto Layout::get_height(NodeId node) const -> float {
    return heights_.get(node);
}

auto Layout::get_waypoints(EdgeId edge) const -> const std::vector<Point>& {
    return waypoints_.get(edge);
}

auto Layout::get_region_node(const SESE::SESERegion& r) const -> Node {
    return g_.get_node(regions_data_[r.id].node_id);
}

auto Layout::get_editor(const SESE::SESERegion& r) -> SubGraphEditor& {
    return regions_data_[r.id].subgraph.editor();
}

// NOLINTNEXTLINE(misc-no-recursion)
void Layout::compute_layout(const SESE::SESERegion& r) {
    auto& region = regions_data_[r.id];
    if (region.was_layout) {
        return;
    }

    region.was_layout = true;

    for (const auto* child_region : r.children()) {
        auto node = get_region_node(*child_region);

        compute_layout(*child_region);
        widths_.set(node, regions_data_[child_region->id].width);
        heights_.set(node, regions_data_[child_region->id].height);
    }

    auto sugiyama =
        SugiyamaAnalysis(region.subgraph, heights_, widths_, start_x_offset_,
                         end_x_offset_, region.entries, region.exits);

    for (const auto& node : region.subgraph.nodes()) {
        auto x = sugiyama.xs_.get(node);
        auto y = sugiyama.ys_.get(node);

        xs_.set(node, x);

        ys_.set(node, y);
    }

    for (const auto& edge : region.subgraph.edges()) {
        waypoints_.set(edge, sugiyama.waypoints_.get(edge));
    }

    region.height = sugiyama.get_graph_height();
    region.width  = sugiyama.get_graph_width();

    region.io_waypoints = sugiyama.get_io_waypoints();

    for (auto entry_pair : region.entries) {
        end_x_offset_.set(entry_pair.edge,
                          region.io_waypoints[entry_pair].front().x);
    }

    for (auto exit_pair : region.exits) {
        start_x_offset_.set(exit_pair.edge,
                            region.io_waypoints[exit_pair].back().x);
    }

    heights_.set(region.node_id, region.height);
    widths_.set(region.node_id, region.width);

    if (sugiyama.has_top_loop_) {
        translate_region(r, {.x = 0, .y = -2 * Y_GUTTER});
    }
}

void Layout::translate_region(const SESE::SESERegion& r, const Point& v) {
    auto& region = regions_data_[r.id];

    for (const auto& node : region.subgraph.nodes()) {
        auto node_x = Layout::get_x(node);
        auto node_y = Layout::get_y(node);

        xs_.set(node, node_x + v.x);
        ys_.set(node, node_y + v.y);
    }

    for (const auto& edge : region.subgraph.edges()) {
        auto& waypoints = waypoints_.get(edge);

        for (auto& waypoint : waypoints) {
            waypoint += v;
        }
    }

    for (auto& kv : region.io_waypoints) {
        for (auto& waypoint : kv.second) {
            waypoint += v;
        }
    }
}

// NOLINTNEXTLINE (misc-no-recursion)
void Layout::translate_region(const SESE::SESERegion& r) {
    if (!r.is_root()) {
        auto node = get_region_node(r);

        auto v = Point{.x = Layout::get_x(node), .y = Layout::get_y(node)};
        translate_region(r, v);
    }

    // Propagate to children
    for (const auto* child_region : r.children()) {
        translate_region(*child_region);
    }
}

Layout::RegionData::RegionData(Graph& g)
    : subgraph{g}, node_id{NodeId::InvalidID} {}
