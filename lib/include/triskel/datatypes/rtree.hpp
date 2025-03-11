/// @file A radix tree
#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

#include <fmt/base.h>

template <typename T>
auto starts_with(const std::span<T>& vec, const std::span<T>& prefix) -> bool;

/// @brief A radix tree
template <typename K, typename T>
struct RTree {
    using Key = std::span<const K>;

    // A radix node ish
    struct RNode {
        std::vector<T> data;
        std::vector<K> radix;
        std::unordered_map<K, std::unique_ptr<RNode>> children;

        explicit RNode(RTree& tree) { id = tree.id++; }

        /// @brief Save the node to a Graphviz graph
        void dump();

        void new_child(RTree& tree, const T& value, const Key& key);

        void split(RTree& tree, const T& value, const Key& key);

        size_t id;
    };

    RTree() : root{*this} {}

    /// @brief Inserts a value into the radix tree
    void insert(const T& value, const Key& key);

    [[nodiscard]] auto bfs() -> std::vector<RNode*>;

    /// @brief Save the node to a Graphviz graph
    void dump();

    RNode root;

   private:
    size_t id;
};

#include "rtree.ipp"