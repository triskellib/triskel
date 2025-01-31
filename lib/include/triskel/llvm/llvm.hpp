#ifdef TRISKEL_LLVM
#pragma once

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

#include "triskel/graph/graph.hpp"
#include "triskel/utils/attribute.hpp"

namespace triskel {
auto to_string(const llvm::Value& v) -> std::string;

/// @brief LLVM support for libfmt
auto format_as(const llvm::Value& v) -> std::string;

/// @brief A cfg for LLVM functions
struct LLVMCFG {
    enum class EdgeType : uint8_t { Default, True, False };

    /// @brief Build a CFG from an LLVM function
    explicit LLVMCFG(llvm::Function* function);

    /// @brief The CFG's function
    llvm::Function* function;

    /// @brief The CFG
    Graph graph;

    /// @brief A map from the CFG node to LLVM basic blocks
    NodeAttribute<llvm::BasicBlock*> block_map;

    /// @brief The color of the edges
    EdgeAttribute<EdgeType> edge_types;
};

}  // namespace triskel
#endif