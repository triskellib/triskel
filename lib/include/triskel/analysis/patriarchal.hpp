/// @file common operations for analysis that provide a "family" hierarchy with
/// nodes having parents, childrens, ancestors and descendants
#pragma once

#include "triskel/utils/attribute.hpp"

namespace triskel {

struct Patriarchal {
    explicit Patriarchal(const IGraph& g);

    virtual ~Patriarchal() = default;

    /// @brief Gets this node's parent
    [[nodiscard]] auto parents(const Node& n) -> std::vector<Node>;

    /// @brief Gets this node's only parent.
    /// If this node has multiple parents raises an error
    [[nodiscard]] auto parent(const Node& n) -> Node;

    /// @brief Gets this node's children
    [[nodiscard]] auto children(const Node& n) -> std::vector<Node>;

    /// @brief Gets this node's only child.
    /// If this node has multiple children raises an error
    [[nodiscard]] auto child(const Node& n) -> Node;

    /// @brief Does node n1 precede n2 ?
    /// Checks using a node's children
    [[nodiscard]] auto precedes(const Node& n1, const Node& n2) -> bool;

    /// @brief Does node n1 succeed n2 ?
    /// Checks using a node's parents
    [[nodiscard]] auto succeed(const Node& n1, const Node& n2) -> bool;

   protected:
    /// @brief Makes a node a parent of another node
    void add_parent(const Node& parent, const Node& child);

   private:
    const IGraph& g_;
    NodeAttribute<std::vector<NodeId>> parents_;
    NodeAttribute<std::vector<NodeId>> children_;
};
}  // namespace triskel