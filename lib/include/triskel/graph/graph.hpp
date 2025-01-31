#pragma once

#include <stack>
#include "triskel/graph/igraph.hpp"

namespace triskel {

struct Graph;

struct GraphEditor : public IGraphEditor {
    /// @brief A graph editor with source control
    explicit GraphEditor(Graph& g);

    /// @brief Debug tests
    ~GraphEditor() override;

    auto make_node() -> Node override;
    void remove_node(NodeId id) override;
    auto make_edge(NodeId from, NodeId to) -> Edge override;
    void edit_edge(EdgeId edge, NodeId new_from, NodeId new_to) override;
    void remove_edge(EdgeId edge) override;
    void push() override;
    void pop() override;
    void commit() override;

   private:
    struct Frame {
        size_t created_nodes_count;
        std::stack<NodeId> deleted_nodes;

        std::stack<EdgeId> created_edges;
        std::stack<EdgeId> deleted_edges;
        std::stack<EdgeData> modified_edges;
    };

    auto frame() -> Frame&;

    Graph& g_;

    std::stack<Frame> frames;
};

/// @brief A graph that owns its data
struct Graph : public IGraph {
    Graph();

    /// @brief The root of this graph
    [[nodiscard]] auto root() const -> Node override;

    /// @brief The nodes in this graph
    [[nodiscard]] auto nodes() const -> std::vector<Node> override;

    /// @brief The edges in this graph
    [[nodiscard]] auto edges() const -> std::vector<Edge> override;

    /// @brief Turns a NodeId into a Node
    [[nodiscard]] auto get_node(NodeId id) const -> Node override;

    /// @brief Turns an EdgeId into an Edge
    [[nodiscard]] auto get_edge(EdgeId id) const -> Edge override;

    /// @brief The greatest id in this graph
    [[nodiscard]] auto max_node_id() const -> size_t override;

    /// @brief The greatest id in this graph
    [[nodiscard]] auto max_edge_id() const -> size_t override;

    /// @brief The number of nodes in this graph
    [[nodiscard]] auto node_count() const -> size_t override;

    /// @brief The number of edges in this graph
    [[nodiscard]] auto edge_count() const -> size_t override;

    /// @brief Gets the editor attached to this graph
    [[nodiscard]] auto editor() -> GraphEditor& override;

   private:
    GraphEditor editor_;

    GraphData data_;

    /// Gets the data of a specific node
    [[nodiscard]] auto get_node_data(NodeId id) -> NodeData&;
    [[nodiscard]] auto get_node_data(NodeId id) const -> const NodeData&;

    /// Gets the data of a specific edge
    [[nodiscard]] auto get_edge_data(EdgeId id) -> EdgeData&;
    [[nodiscard]] auto get_edge_data(EdgeId id) const -> const EdgeData&;

    friend struct GraphEditor;
    friend struct SubGraph;
};

}  // namespace triskel