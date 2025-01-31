#pragma once

#include <cassert>
#include <compare>
#include <cstddef>
#include <deque>
#include <ranges>
#include <string>
#include <vector>

namespace triskel {

template <typename Tag>
struct ID {
    ID() : value(InvalidID.value) {}

    explicit ID(size_t value) : value{value} {}
    explicit operator size_t() const { return value; }

    static const ID InvalidID;

    auto operator==(ID other) const -> bool { return other.value == value; }

    friend auto operator<=>(ID lhs, ID rhs) -> std::strong_ordering {
        return lhs.value <=> rhs.value;
    }

    [[nodiscard]] auto is_valid() { return *this != InvalidID; }
    [[nodiscard]] auto is_invalid() { return *this == InvalidID; }

   private:
    size_t value;
};

template <typename Tag>
const ID<Tag> ID<Tag>::InvalidID{static_cast<size_t>(-1)};

template <typename Tag>
auto format_as(const ID<Tag>& id) -> std::string {
    return std::to_string(static_cast<size_t>(id));
}

/// @brief A struct with and id
template <typename Tag>
struct Identifiable {
    virtual ~Identifiable()                          = default;
    [[nodiscard]] virtual auto id() const -> ID<Tag> = 0;

    auto operator==(const Identifiable& other) const -> bool {
        return id() == other.id();
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator ID<Tag>() const { return id(); }
};

struct NodeTag {};
using NodeId = ID<NodeTag>;
static_assert(std::is_trivially_copyable_v<NodeId>);
static_assert(std::strict_weak_order<std::ranges::less, NodeId, NodeId>);

struct EdgeTag {};
using EdgeId = ID<EdgeTag>;
static_assert(std::is_trivially_copyable_v<EdgeId>);

struct GraphData;

struct NodeData {
    NodeId id;
    std::vector<EdgeId> edges;
    bool deleted;
};

struct EdgeData {
    EdgeId id;
    NodeId from;
    NodeId to;
    bool deleted;
};

struct GraphData {
    NodeId root;

    std::deque<NodeData> nodes;
    std::deque<EdgeData> edges;
};

struct Edge;
struct IGraph;
struct IGraphEditor;

struct Node : public Identifiable<NodeTag> {
    Node(const IGraph& g, const NodeData& n) : g_{g}, n_{&n} {}
    ~Node() override = default;

    auto operator=(const Node& node) -> Node&;

    [[nodiscard]] auto id() const -> NodeId final;
    [[nodiscard]] auto edges() const -> std::vector<Edge>;

    [[nodiscard]] auto child_edges() const -> std::vector<Edge>;
    [[nodiscard]] auto parent_edges() const -> std::vector<Edge>;

    [[nodiscard]] auto child_nodes() const -> std::vector<Node>;
    [[nodiscard]] auto parent_nodes() const -> std::vector<Node>;
    [[nodiscard]] auto neighbors() const -> std::vector<Node>;

    [[nodiscard]] auto is_root() const -> bool;

   private:
    const IGraph& g_;
    const NodeData* n_;
};

struct Edge : public Identifiable<EdgeTag> {
    Edge(const IGraph& g, const EdgeData& e);
    ~Edge() override = default;

    auto operator=(const Edge& other) -> Edge&;

    [[nodiscard]] auto id() const -> EdgeId final;
    [[nodiscard]] auto from() const -> Node;
    [[nodiscard]] auto to() const -> Node;

    /// @brief Returns the other side of the edge
    [[nodiscard]] auto other(NodeId n) const -> Node;

   private:
    const IGraph& g_;
    const EdgeData* e_;
};

/// @brief An interface for a graph
struct IGraph {
    virtual ~IGraph() = default;

    /// @brief The root of this graph
    [[nodiscard]] virtual auto root() const -> Node = 0;

    /// @brief The nodes in this graph
    [[nodiscard]] virtual auto nodes() const -> std::vector<Node> = 0;

    /// @brief The edges in this graph
    [[nodiscard]] virtual auto edges() const -> std::vector<Edge> = 0;

    /// @brief Turns a NodeId into a Node
    [[nodiscard]] virtual auto get_node(NodeId id) const -> Node = 0;

    /// @brief Turns an EdgeId into an Edge
    [[nodiscard]] virtual auto get_edge(EdgeId id) const -> Edge = 0;

    /// @brief The greatest id in this graph
    [[nodiscard]] virtual auto max_node_id() const -> size_t = 0;

    /// @brief The greatest id in this graph
    [[nodiscard]] virtual auto max_edge_id() const -> size_t = 0;

    /// @brief The number of nodes in this graph
    [[nodiscard]] virtual auto node_count() const -> size_t = 0;

    /// @brief The number of edges in this graph
    [[nodiscard]] virtual auto edge_count() const -> size_t = 0;

    /// @brief Gets the editor attached to this graph
    [[nodiscard]] virtual auto editor() -> IGraphEditor& = 0;

    /// @brief Turns a NodeId into a Node
    [[nodiscard]] virtual auto get_nodes(
        const std::span<const NodeId>& ids) const -> std::vector<Node>;

    /// @brief Turns an EdgeId into an Edge
    [[nodiscard]] virtual auto get_edges(
        const std::span<const EdgeId>& ids) const -> std::vector<Edge>;

   protected:
    /// @brief Maps a range of NodeID's to a range of Nodes
    [[nodiscard]] auto node_view() const {
        return std::views::transform(
                   [&](NodeId id) { return get_node(id); })  //
               | std::ranges::to<std::vector<Node>>();
    }

    /// @brief Maps a range of EdgeID's to a range of Edges
    [[nodiscard]] auto edge_view() const {
        return std::views::transform(
                   [&](EdgeId id) { return get_edge(id); })  //
               | std::ranges::to<std::vector<Edge>>();
    }
};

auto format_as(const Node& n) -> std::string;
auto format_as(const Edge& e) -> std::string;
auto format_as(const IGraph& g) -> std::string;

struct IGraphEditor {
    virtual ~IGraphEditor() = default;

    // ==========
    // Nodes
    // ==========
    /// @brief Adds a node to the graph
    virtual auto make_node() -> Node = 0;

    /// @brief Removes a node from the graph
    /// This will also remove all the edges that contain this node
    virtual void remove_node(NodeId node) = 0;

    // ==========
    // Edges
    // ==========
    /// @brief Create a new edge between two nodes
    virtual auto make_edge(NodeId from, NodeId to) -> Edge = 0;

    /// @brief Modifies the start and end point of an edge
    virtual void edit_edge(EdgeId edge, NodeId new_from, NodeId new_to) = 0;

    /// @brief Removes an edge from the graph
    virtual void remove_edge(EdgeId edge) = 0;

    // ==========
    // Version Control
    // ==========
    /// @brief Creates a new edit frame
    virtual void push() = 0;

    /// @brief Removes all changes from the current frame
    virtual void pop() = 0;

    /// @brief Writes all changes to the graph, erasing the modification frames
    virtual void commit() = 0;
};
}  // namespace triskel