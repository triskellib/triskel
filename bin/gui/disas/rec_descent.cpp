#include "rec_descent.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <vector>

#include <capstone/capstone.h>
#include <fmt/format.h>

#include "triskel/graph/graph.hpp"
#include "triskel/triskel.hpp"

namespace {
using EdgeType = triskel::LayoutBuilder::EdgeType;

/// @brief An edge connecting a basic block to its children
struct ChildEdge {
    explicit ChildEdge(size_t addr) : addr{addr} {}

    size_t addr;
    EdgeType type = EdgeType::Default;
};

/// @brief A rudimentary basic block with helper methods
struct BasicBlock {
    /// @brief The instructions in this basic block, sorted by increasing
    /// addresses
    std::vector<Instruction> insns;

    /// @brief The children addresses of this block
    std::vector<ChildEdge> children;

    /// @brief The position of this block in the CFG's array of blocks
    size_t id;

    /// @brief The number of instructions in this block
    [[nodiscard]] auto size() const -> size_t { return insns.size(); }

    /// @brief The first addres of this block
    [[nodiscard]] auto start_addr() const -> size_t {
        return insns.front().addr;
    }

    /// @brief The last address of this block
    [[nodiscard]] auto end_addr() const -> size_t { return insns.back().addr; }
};

struct CSH {
    CSH() {
        auto is_open = cs_open(CS_ARCH_X86, CS_MODE_64, &handle);
        assert(is_open == CS_ERR_OK);
        cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);
    }

    ~CSH() { cs_close(&handle); }

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator csh&() { return handle; }

    csh handle;
};

struct RecursiveDescent {
    using InstructionPtr = std::shared_ptr<cs_insn>;

    RecursiveDescent(size_t start_addr, const Binary& binary) : binary{binary} {
        queue.push(start_addr);

        cfg = std::make_unique<BinaryCFG>();

        while (!queue.empty()) {
            auto addr = queue.front();
            queue.pop();

            explore_addr(addr);
        }

        // Link the blocks to one another
        auto& g  = cfg->graph;
        g        = std::make_unique<triskel::Graph>();
        auto& ge = g->editor();
        ge.push();

        for (size_t i = 0; i < blocks.size(); ++i) {
            ge.make_node();
        }

        auto& instructions = cfg->instructions;

        for (size_t i = 0; i < blocks.size(); ++i) {
            const auto& block = blocks[i];
            const auto& node  = g->get_node(triskel::NodeId{i});

            instructions.set(node, block.insns);

            for (const auto& edge : block.children) {
                const auto& child_node =
                    g->get_node(triskel::NodeId{addr2block[edge.addr]});

                auto e = ge.make_edge(node, child_node);
                cfg->edge_types.set(e, edge.type);
            }
        }

        ge.commit();
    }

    void explore_addr(size_t addr) {
        // Check if we have already visited this address
        if (addr2block.contains(addr)) {
            split_block(addr);
            return;
        }

        // Create a new block
        blocks.push_back({});
        auto& block = blocks.back();
        block.id    = blocks.size() - 1;

        auto next_addr = std::make_optional<size_t>(addr);
        while (next_addr.has_value()) {
            next_addr = add_instruction_to_block(block, *next_addr);
        }
    }

    void split_block(size_t addr) {
        const auto b = addr2block[addr];
        auto& block  = blocks[b];

        if (block.start_addr() == addr) {
            // We already visited this block from this point
        } else {
            // We found another entrypoint, we need to split the block
            size_t i = 0;
            for (; i < block.size(); ++i) {
                const auto& insn = block.insns[i];
                if (insn.addr == addr) {
                    break;
                }
            }
            assert(i != block.size());

            blocks.push_back(
                {.insns    = {block.insns.begin() + static_cast<int64_t>(i),
                              block.insns.end()},
                 .children = block.children});
            blocks.back().id = blocks.size() - 1;

            for (const auto& insn : blocks.back().insns) {
                addr2block[insn.addr] = blocks.size() - 1;
            }

            block.insns.resize(i);
            block.children = {ChildEdge{addr}};
        }
    }

    auto add_instruction_to_block(BasicBlock& block,
                                  size_t addr) -> std::optional<size_t> {
        if (addr2block.contains(addr)) {
            // The next address is in another basic block
            block.children.push_back(ChildEdge{addr});
            return {};
        }

        // Adds this instruction to the current block
        auto insn = get_instruction(addr);

        auto i = Instruction{
            .addr = addr,
            .repr = fmt::format("{}\t\t{}", insn->mnemonic, insn->op_str),
        };

        block.insns.push_back(i);
        addr2block[addr] = block.id;

        if (cs_insn_group(handle, insn.get(), CS_GRP_JUMP)) {
            handle_jump(block, addr, insn);
            return {};
        }

        if (cs_insn_group(handle, insn.get(), CS_GRP_RET)) {
            handle_return(insn);
            return {};
        }

        addr += insn->size;
        return {addr};
    }

    auto get_instruction(size_t addr) -> InstructionPtr {
        std::array<uint8_t, 16> bin;
        for (size_t i = 0; i < 16; ++i) {
            bin[i] = binary.at(addr + i);
        }

        cs_insn* i;
        auto count = cs_disasm(handle, bin.data(), bin.size(), addr, 1, &i);
        assert(count == 1);

        auto insn = std::shared_ptr<cs_insn>{
            i, [](cs_insn* insn) { cs_free(insn, 1); }};

        return insn;
    }

    // Both direct and indirect
    void handle_jump(BasicBlock& block, size_t addr, InstructionPtr& insn) {
        const auto* detail = insn->detail;
        assert(detail != nullptr);

        assert(detail->x86.op_count == 1);

        const auto& op = detail->x86.operands[0];
        if (op.type == X86_OP_IMM) {
            queue.push(op.imm);
            block.children.push_back(ChildEdge{static_cast<size_t>(op.imm)});
        } else {
            // We do not handle indirect jumps
            fmt::print("Found an indirect jump\n");
            // assert(false);
        }

        if (insn->id != X86_INS_JMP) {
            block.children.back().type = EdgeType::True;

            // This is a conditional, also add the next address
            queue.push(addr + insn->size);
            block.children.push_back(ChildEdge{addr + insn->size});

            block.children.back().type = EdgeType::False;
        }
    }

    void handle_return(InstructionPtr& insn) {}

    const Binary& binary;

    // Capstone handle
    CSH handle;

    // Addresses to explore for potential new blocks
    std::queue<size_t> queue;

    // Blocks we have explored
    std::vector<BasicBlock> blocks;

    // Maps addresses to visited blocks
    std::map<size_t, size_t> addr2block;

    std::unique_ptr<BinaryCFG> cfg;
};

}  // namespace

BinaryCFG::BinaryCFG()
    : instructions{0, {}}, edge_types{0, EdgeType::Default} {}

auto make_binary_graph(size_t start_addr,
                       const Binary& binary) -> std::unique_ptr<BinaryCFG> {
    auto r = RecursiveDescent(start_addr, binary);
    return std::move(r.cfg);
}