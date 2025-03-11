#pragma once
#include "rtree.hpp"

#include <fmt/ranges.h>

template <typename T>
auto starts_with(const std::span<T>& vec, const std::span<T>& prefix) -> bool {
    return vec.size() >= prefix.size() &&
           std::equal(prefix.begin(), prefix.end(), vec.begin());
}

/// @brief A radix tree
template <typename K, typename T>
void RTree<K, T>::RNode::dump() {
    fmt::print("n{} [label=\"{}\"]\n", id, radix);

    for (const auto& child : children) {
        fmt::print("n{} -> n{}\n", id, child.second->id);
        child.second->dump();
    }

    for (size_t i = 0; i < data.size(); ++i) {
        fmt::print("n{}_{} [label=\"{}\", shape=square]\n", id, i, data[i]);
        fmt::print("n{} -> n{}_{}\n", id, id, i);
    }
}

template <typename K, typename T>
void RTree<K, T>::RNode::new_child(RTree& tree,
                                   const T& value,
                                   const Key& key) {
    assert(key.size() > 0);

    auto node = std::make_unique<RNode>(tree);
    node->radix.assign(key.begin(), key.end());
    node->data = {value};

    children[key.front()] = std::move(node);
}

template <typename K, typename T>
void RTree<K, T>::RNode::split(RTree& tree, const T& value, const Key& key) {
    auto shared_prefix_len = 0;

    auto size = std::min(radix.size(), key.size());
    while (shared_prefix_len < size) {
        if (radix[shared_prefix_len] != key[shared_prefix_len]) {
            break;
        }

        shared_prefix_len++;
    }

    assert(shared_prefix_len > 0);

    auto start = Key(radix).first(shared_prefix_len);
    auto end   = Key(radix).subspan(shared_prefix_len);

    auto new_node = std::make_unique<RNode>(tree);

    new_node->children = std::move(children);
    children.clear();

    new_node->data = std::move(data);
    data.clear();

    radix.assign(start.begin(), start.end());
    new_node->radix.assign(end.begin(), end.end());

    children[radix[shared_prefix_len]] = std::move(new_node);

    if (shared_prefix_len == key.size()) {
        data.push_back(value);
    } else {
        new_child(tree, value, key.subspan(shared_prefix_len));
    }
}

template <typename K, typename T>
void RTree<K, T>::insert(const T& value, const Key& key) {
    auto* cursor = &root;

    size_t i = 0;

    while (i < key.size()) {
        auto k = key[i];

        if (!cursor->children.contains(k)) {
            // create a new node
            cursor->new_child(*this, value, key.subspan(i));
            return;
        }

        cursor = cursor->children[k].get();

        Key radix = cursor->radix;
        if (starts_with(key.subspan(i), radix)) {
            i += radix.size();
        } else {
            cursor->split(*this, value, key.subspan(i));
            return;
        }
    }

    // An element with this key is already in the tree
    cursor->data.push_back(value);
}

template <typename K, typename T>
auto RTree<K, T>::bfs() -> std::vector<RNode*> {
    auto rnodes = std::vector<RNode*>{};

    auto current_layer = std::make_unique<std::vector<RNode*>>();

    auto next_layer = std::make_unique<std::vector<RNode*>>();
    next_layer->push_back(&root);

    while (!next_layer->empty()) {
        std::swap(current_layer, next_layer);
        next_layer->clear();

        for (auto* rnode : *current_layer) {
            rnodes.push_back(rnode);
            for (auto& child : rnode->children) {
                next_layer->push_back(child.second.get());
            }
        }
    }

    return rnodes;
}

template <typename K, typename T>
void RTree<K, T>::dump() {
    fmt::print("digraph G {{\n");
    root.dump();
    fmt::print("}}\n");
}
