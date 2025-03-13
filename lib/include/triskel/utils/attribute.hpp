#pragma once

#include <fmt/printf.h>
#include <type_traits>
#include <vector>

#include "triskel/graph/igraph.hpp"

namespace triskel {

/// @brief A structure to add data to a graph.
/// Attributes function like maps taking advantage of the node / edge's ids.
/// They can be seen as a way of adding labels to an existing graph

template <typename Tag, typename T>
struct Attribute {
    // Bool needs special treatment because of how std::vector<bool> is
    // implemented
    using ConstRef =
        std::conditional_t<std::is_trivially_copyable_v<T>, T, const T&>;

    Attribute(size_t size, const T& v) : v_{v} { data_.resize(size, v); }

    virtual ~Attribute() = default;

    /// @brief Get by reference
    template <typename U = T>
    [[nodiscard]] auto get(const Identifiable<Tag>& n)
        -> T& requires(!std::is_same_v<U, bool>) { return get(n.id()); }

    /// @brief Get by reference
    template <typename U = T>
    [[nodiscard]] auto get(const ID<Tag>& id)
        -> T& requires(!std::is_same_v<U, bool>) {
        auto id_ = static_cast<size_t>(id);
        resize_if_necessary(id_);
        return data_[id_];
    }

    /// @brief Get by reference
    [[nodiscard]] auto get(const Identifiable<Tag>& n) ->
        typename std::vector<bool>::reference
        requires(std::is_same_v<T, bool>)
    {
        return get(n.id());
    }

    /// @brief Get by reference
    [[nodiscard]] auto get(const ID<Tag>& id) ->
        typename std::vector<bool>::reference
        requires(std::is_same_v<T, bool>)
    {
        auto id_ = static_cast<size_t>(id);
        resize_if_necessary(id_);
        return data_[id_];
    }

    /// @brief Get by const reference
    [[nodiscard]] auto get(const Identifiable<Tag>& n) const -> ConstRef {
        return get(n.id());
    }

    /// @brief Get by const reference
    [[nodiscard]] auto get(const ID<Tag>& id) const -> ConstRef {
        auto id_ = static_cast<size_t>(id);
        resize_if_necessary(id_);
        return data_[id_];
    }

    template <typename U = T>
    [[nodiscard]] auto operator[](const ID<Tag>& id)
        -> T& requires(!std::is_same_v<U, bool>) { return get(id); }

    [[nodiscard]] auto operator[](const ID<Tag>& id) ->
        typename std::vector<bool>::reference
        requires(std::is_same_v<T, bool>)
    {
        return get(id);
    }

    template <typename U = T>
    [[nodiscard]] auto operator[](const ID<Tag>& id) const -> ConstRef {
        return get(id);
    }

    void set(const ID<Tag>& id, T v) {
        auto id_ = static_cast<size_t>(id);
        resize_if_necessary(id_);
        data_[id_] = std::move(v);
    }

   private:
    /// @brief This is a const function thanks to mutable
    auto resize_if_necessary(size_t id) const {
        if (id >= data_.size()) {
            data_.resize(id + 1, v_);
        }
    }

    /// @brief This is mutable as the data_ vector might get resized to account
    /// for new nodes
    mutable std::vector<T> data_;

    /// @brief default value to add when the graph is resized
    T v_;
};

template <typename T>
struct NodeAttribute : public Attribute<NodeTag, T> {
    NodeAttribute(const IGraph& g, const T& v)
        : Attribute<NodeTag, T>{g.max_node_id(), v} {}

    /// @deprecated
    NodeAttribute(size_t size, const T& v) : Attribute<NodeTag, T>{size, v} {}

    [[nodiscard]] auto dump(IGraph& g) const -> std::string {
        auto s = std::string{};
        for (auto node : g.nodes()) {
            s += fmt::format("- {} -> {}\n", node, this->get(node));
        }
        return s;
    }
};

template <typename T>
struct EdgeAttribute : public Attribute<EdgeTag, T> {
    EdgeAttribute(const IGraph& g, const T& v)
        : Attribute<EdgeTag, T>{g.max_edge_id(), v} {}

    /// @deprecated
    EdgeAttribute(size_t size, const T& v) : Attribute<EdgeTag, T>{size, v} {}

    [[nodiscard]] auto dump(IGraph& g) const -> std::string {
        auto s = std::string{};
        for (auto edge : g.edges()) {
            s += fmt::format("- {} -> {}\n", edge, this->get(edge));
        }
        return s;
    }
};

};  // namespace triskel