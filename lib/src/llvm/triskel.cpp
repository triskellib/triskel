#include "triskel/triskel.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <string>

#include <llvm/IR/CFG.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Casting.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "llvm/IR/ModuleSlotTracker.h"

#include "triskel/llvm/llvm.hpp"

auto triskel::make_layout(llvm::Function* function,
                          Renderer* render,
                          llvm::ModuleSlotTracker* MST)
    -> std::unique_ptr<CFGLayout> {
    auto builder = make_layout_builder();

    // Important, otherwise the CFGs are not necessarily well defined
    llvm::EliminateUnreachableBlocks(*function);

    std::map<llvm::BasicBlock*, size_t> reverse_map;

    for (auto& block : *function) {
        auto content = std::string{};

        for (const auto& insn : block) {
            content += triskel::to_string(insn, MST) + "\n";
        }

        auto node = builder->make_node(content);

        // Adds the block to the maps
        reverse_map.insert_or_assign(&block, node);
    }

    for (auto& block : *function) {
        auto node = reverse_map.at(&block);

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

            auto type = LayoutBuilder::EdgeType::Default;

            if (child == true_edge) {
                type = LayoutBuilder::EdgeType::True;
            } else if (child == false_edge) {
                type = LayoutBuilder::EdgeType::False;
            }

            builder->make_edge(node, child_node, type);
        }
    }

    if (render != nullptr) {
        builder->measure_nodes(*render);
    }

    return builder->build();
}
