#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <string>

#include "triskel/graph/graph.hpp"
#include "triskel/triskel.hpp"
#include "triskel/utils/attribute.hpp"

using Binary = std::map<size_t, uint8_t>;

struct Instruction {
    /// @brief The address of the instruction
    size_t addr;

    /// @brief A string representing this instruction
    std::string repr;
};

/// @brief The CFG of a binary program
struct BinaryCFG {
    BinaryCFG();

    /// @brief The graph
    std::unique_ptr<triskel::Graph> graph;

    /// @brief The node information
    triskel::NodeAttribute<std::vector<Instruction>> instructions;

    /// @brief The edge type
    triskel::EdgeAttribute<triskel::LayoutBuilder::EdgeType> edge_types;
};

/// @brief Creates a CFG
auto make_binary_graph(size_t start_addr,
                       const Binary& binary) -> std::unique_ptr<BinaryCFG>;
