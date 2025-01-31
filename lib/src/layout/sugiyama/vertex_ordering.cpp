#include "triskel/layout/sugiyama/vertex_ordering.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <utility>
#include <vector>

#include "triskel/graph/graph_view.hpp"
#include "triskel/graph/igraph.hpp"
#include "triskel/utils/attribute.hpp"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

namespace {
[[nodiscard]] auto merge_and_count(std::vector<size_t>& lo,
                                   std::vector<size_t>& hi) -> size_t {
    size_t inversions = 0;

    size_t i = 0;
    size_t j = 0;

    while (i < lo.size() && j < hi.size()) {
        if (lo[i] > hi[j]) {
            j++;
            inversions += (lo.size() - i);
        } else {
            i++;
        }
    }

    return inversions;
}

[[nodiscard]] auto merge_and_count(std::vector<size_t>& arr,
                                   int64_t lo,
                                   int64_t mid,
                                   int64_t hi) -> size_t {
    auto lo_arr = std::vector<size_t>{arr.begin() + lo, arr.begin() + mid};

    auto hi_arr = std::vector<size_t>{arr.begin() + mid, arr.begin() + hi};

    auto lo_sz = mid - lo;
    auto hi_sz = hi - mid;

    assert(lo_sz == lo_arr.size());
    assert(hi_sz == hi_arr.size());
    assert(lo_sz + hi_sz == hi - lo);

    size_t i = 0;
    size_t j = 0;
    size_t k = lo;  // Merged index

    size_t inversions = 0;

    while (i < lo_sz && j < hi_sz) {
        if (lo_arr[i] <= hi_arr[j]) {
            arr[k++] = lo_arr[i++];
        } else {
            arr[k++] = hi_arr[j++];
            inversions += (lo_sz - i);
        }
    }

    while (i < lo_sz) {
        arr[k++] = lo_arr[i++];
    }

    while (j < hi_sz) {
        arr[k++] = hi_arr[j++];
    }

    return inversions;
}

// NOLINTNEXTLINE(misc-no-recursion)
[[nodiscard]] auto sort_and_count(std::vector<size_t>& arr,
                                  int64_t lo,
                                  int64_t hi) -> size_t {
    size_t inversions = 0;

    if (hi - lo > 1) {
        auto mid = lo + ((hi - lo) / 2);
        inversions += sort_and_count(arr, lo, mid);
        inversions += sort_and_count(arr, mid, hi);
        inversions += merge_and_count(arr, lo, mid, hi);
    }

    return inversions;
}
}  // namespace

VertexOrdering::VertexOrdering(const IGraph& g,
                               const NodeAttribute<size_t>& layers,
                               size_t layer_count_)
    : orders_(g, -1), g_{g}, layers_(layers) {
    node_layers_.resize(layer_count_);
    for (const auto& node : g_.nodes()) {
        node_layers_[layers_.get(node)].push_back(&node);
    }

    normalize_order();

    auto best        = orders_;
    size_t crossings = -1;

    // The 24 comes from
    // https://blog.disy.net/sugiyama-method/
    for (size_t i = 0; i < 24; ++i) {
        median(i);

        normalize_order();

        transpose();

        const auto new_crossings = count_crossings();
        if (new_crossings < crossings) {
            best      = orders_;
            crossings = new_crossings;

            if (new_crossings == 0) {
                break;
            }
        }
    }

    orders_ = best;
}

void VertexOrdering::get_neighbor_orders(
    const NodeView& n,
    std::vector<size_t>& orders_top,
    std::vector<size_t>& orders_bottom) const {
    for (const auto* child : n.child_nodes()) {
        orders_bottom.push_back(orders_.get(*child));
    }

    for (const auto* parent : n.parent_nodes()) {
        orders_top.push_back(orders_.get(*parent));
    }

    std::ranges::sort(orders_top);
    std::ranges::sort(orders_bottom);
}

auto VertexOrdering::count_crossings(const NodeView& node1,
                                     const NodeView& node2) const -> size_t {
    orders_top1.clear();
    orders_top2.clear();
    orders_bottom1.clear();
    orders_bottom2.clear();

    get_neighbor_orders(node1, orders_top1, orders_bottom1);
    get_neighbor_orders(node2, orders_top2, orders_bottom2);

    return merge_and_count(orders_top1, orders_top2) +
           merge_and_count(orders_bottom1, orders_bottom2);
}

auto VertexOrdering::count_crossings_with_layer(size_t l1,
                                                size_t l2) -> size_t {
    auto& layer = node_layers_[l1];

    if (layer.size() <= 1) {
        return 0;
    }

    assert(std::ranges::is_sorted(layer, [&](const auto* a, const auto* b) {
        return orders_.get(*a) < orders_.get(*b);
    }));

    // TODO: have a preallocated static vector
    auto orders = std::vector<size_t>{};
    orders.reserve(2 * node_layers_[l2].size());  // heuristically

    auto neighbors = std::vector<size_t>{};
    neighbors.reserve(node_layers_[l2].size());  // heuristically

    for (const auto* node : layer) {
        neighbors.clear();

        for (const auto* child : node->neighbors()) {
            if (layers_.get(*child) == l2) {
                neighbors.push_back(orders_.get(*child));
            }
        }

        std::ranges::sort(neighbors);
        orders.insert(orders.end(), neighbors.begin(), neighbors.end());
    }

    return sort_and_count(orders, 0, static_cast<int64_t>(orders.size()));
}

auto VertexOrdering::count_crossings() -> size_t {
    size_t crossings = 0;

    for (size_t l = 0; l < node_layers_.size() - 1; ++l) {
        crossings += count_crossings_with_layer(l, l + 1);
    }

    return crossings;
}

void VertexOrdering::normalize_order() {
    for (auto& nodes : node_layers_) {
        // THIS IS IMPORTANT
        std::ranges::shuffle(nodes, rng_);

        std::ranges::sort(nodes, [this](const auto* a, const auto* b) {
            return orders_.get(*a) < orders_.get(*b);
        });

        auto order = 0;

        for (const auto* node : nodes) {
            orders_.set(*node, order);
            order++;
        }
    }
}

// TODO: this sucks
void VertexOrdering::median(size_t iter) {
    if (iter % 2 == 0) {
        for (const auto& nodes : node_layers_) {
            for (const auto* node : nodes) {
                auto median_order = orders_.get(*node);

                auto children =
                    node->child_nodes() |
                    std::ranges::views::transform(
                        [&](const auto* node) { return orders_.get(*node); }) |
                    std::ranges::to<std::vector<size_t>>();

                std::ranges::sort(children);

                if (!children.empty()) {
                    median_order = children[children.size() / 2];
                }

                orders_.set(*node, median_order);
            }
        }
    } else {
        for (const auto& nodes : node_layers_) {
            for (const auto* node : nodes) {
                auto median_order = orders_.get(*node);

                auto parents =
                    node->parent_nodes() |
                    std::ranges::views::transform(
                        [&](const auto* node) { return orders_.get(*node); }) |
                    std::ranges::to<std::vector<size_t>>();

                std::ranges::sort(parents);

                if (!parents.empty()) {
                    median_order = parents[parents.size() / 2];
                }

                orders_.set(*node, median_order);
            }
        }
    }
}

void VertexOrdering::transpose() {
    auto improved = true;

    while (improved) {
        improved = false;
        for (auto& nodes : node_layers_) {
            if (nodes.empty()) {
                continue;
            }

            for (size_t i = 0; i < nodes.size() - 1; ++i) {
                const auto* v = nodes[i];
                const auto* w = nodes[i + 1];

                const auto crossings     = count_crossings(*v, *w);
                const auto new_crossings = count_crossings(*w, *v);

                if (new_crossings <= crossings) {
                    if (new_crossings < crossings) {
                        improved = true;
                    }

                    // Swap the node orders
                    orders_.set(*v, i + 1);
                    orders_.set(*w, i);

                    // Swap the nodes to ensure the array remains sorted
                    std::swap(nodes[i], nodes[i + 1]);
                }
            }
        }
    }
}