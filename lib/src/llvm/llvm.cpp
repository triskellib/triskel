#include "triskel/llvm/llvm.hpp"

#include <algorithm>
#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include <triskel/graph/igraph.hpp>

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

namespace {

auto truncate_str(const std::string& str,
                  const std::string& ellipsis = "(...)",
                  size_t max_length           = 80) -> std::string {
    const size_t ellipsisLength = ellipsis.length();

    if (str.length() <= max_length) {
        return str;
    }

    return str.substr(0, max_length - ellipsisLength) + ellipsis;
}

}  // namespace

auto triskel::to_string(const llvm::Value& v) -> std::string {
    std::string s;
    ::llvm::raw_string_ostream os{s};
    v.print(os);

    s = truncate_str(os.str());

    std::ranges::replace(s, '\n', ' ');
    return s;
}

auto triskel::format_as(const llvm::Value& v) -> std::string {
    return to_string(v);
}

LLVMCFG::LLVMCFG(llvm::Function* function)
    : function{function},
      block_map(function->size(), nullptr),
      edge_types(function->size(), EdgeType::Default) {
    // Important, otherwise the CFGs are not necessarily well defined
    llvm::EliminateUnreachableBlocks(*function);

    auto& editor = graph.editor();
    editor.push();

    std::map<llvm::BasicBlock*, NodeId> reverse_map;

    for (auto& block : *function) {
        auto node = editor.make_node();

        // Adds the block to the maps
        block_map.set(node, &block);
        reverse_map.insert_or_assign(&block, node.id());
    }

    for (auto& block : *function) {
        auto node = graph.get_node(reverse_map.at(&block));

        llvm::BasicBlock* true_edge  = nullptr;
        llvm::BasicBlock* false_edge = nullptr;

        const auto& insn = block.back();
        if (const auto* branch = llvm::dyn_cast<llvm::BranchInst>(&insn)) {
            if (branch->isConditional()) {
                true_edge  = branch->getSuccessor(0);
                false_edge = branch->getSuccessor(1);
            }
        }

        for (auto* child : llvm::successors(&block)) {
            auto child_node = reverse_map.at(child);

            auto edge = editor.make_edge(node, child_node);

            if (child == true_edge) {
                edge_types.set(edge, EdgeType::True);
            } else if (child == false_edge) {
                edge_types.set(edge, EdgeType::False);
            }
        }
    }

    editor.commit();
}