#include "arch.hpp"

#include <cstddef>
#include <map>
#include <memory>

#include <LIEF/LIEF.h>
#include <fmt/format.h>
#include <LIEF/ELF.hpp>
#include <LIEF/ELF/Binary.hpp>
#include <LIEF/ELF/Parser.hpp>
#include <string>

#include "triskel/graph/igraph.hpp"
#include "triskel/layout/ilayout.hpp"

#include "../../disas/rec_descent.hpp"
#include "triskel/internal.hpp"
#include "triskel/triskel.hpp"
#include "triskel/utils/attribute.hpp"

using triskel::IGraph;
using triskel::ILayout;
using triskel::NodeAttribute;

namespace {

struct BinArch : public Arch {
    BinArch() : instructions{0, {}} {}

    std::unique_ptr<IGraph> graph;
    std::unique_ptr<ILayout> layout;

    std::map<std::string, size_t> functions;

    std::unique_ptr<LIEF::ELF::Binary> bin_;
    NodeAttribute<std::vector<Instruction>> instructions;

    std::map<size_t, uint8_t> binary;

    void load_binary_from_path(const std::string& path) {
        bin_ = LIEF::ELF::Parser::parse(path);

        if (bin_ == nullptr) {
            fmt::print("Error occurred while loading the binary\n");
        }

        for (auto& segment : bin_->segments()) {
            auto vaddr   = segment.virtual_address();
            auto content = segment.content();
            // auto content_size = segment.get_content_size();

            for (auto byte : content) {
                binary[vaddr] = byte;
                vaddr += 1;
            }
        }
    }

    auto select_function(const std::string& name, triskel::Renderer& renderer)
        -> std::unique_ptr<triskel::CFGLayout> override {
        auto cfg = make_binary_graph(functions[name], binary);

        auto labels  = NodeAttribute<std::string>{*cfg->graph, {}};
        auto widths  = NodeAttribute<float>{*cfg->graph, {}};
        auto heights = NodeAttribute<float>{*cfg->graph, {}};

        for (const auto& node : cfg->graph->nodes()) {
            auto label = std::string{};

            for (const auto& insn : cfg->instructions.get(node)) {
                label += fmt::format("{:x} {}\n", insn.addr, insn.repr);
            }

            auto bbox = renderer.measure_text(label, renderer.STYLE_TEXT);

            labels.set(node, label);
            widths.set(node, bbox.x);
            heights.set(node, bbox.y);
        }

        for (const auto& edge : cfg->graph->edges()) {
            const auto& from = edge.from();

            if (from.child_edges().size() == 2) {
                auto last_addr = cfg->instructions.get(from).back().addr;
            }
        }

        return triskel::make_layout(std::move(cfg->graph), widths, heights,
                                    labels, cfg->edge_types);
    }

    void start(const std::string& path) override {
        load_binary_from_path(path);
        assert(bin_ != nullptr);

        for (auto& f : bin_->functions()) {
            if (f.address() == 0) {
                continue;
            }

            auto name = f.name();
            if (name.empty()) {
                name = fmt::format("F_{:x}", f.address());
            }
            functions[name] = f.address();
            function_names.push_back(name);
        }

        std::ranges::sort(function_names);
    }
};

}  // namespace

auto make_bin_arch() -> std::unique_ptr<Arch> {
    return std::make_unique<BinArch>();
}