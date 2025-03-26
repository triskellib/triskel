#include <algorithm>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <gflags/gflags.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "llvm/IR/ModuleSlotTracker.h"

#include "src/table.hpp"
#include "triskel/triskel.hpp"
#include "triskel/utils/point.hpp"

#include <locale>

using triskel::Point;

DEFINE_uint64(max_nodes,
              1000,
              "Functions with more than `max_nodes` nodes will be skipped");

DEFINE_string(out_dir, ".", "The location the csv will be saved at");

namespace {
struct ProgressBar {
    explicit ProgressBar(size_t size) : size_{size} {}

    void start() { start_ = std::chrono::steady_clock::now(); }

    void draw() {
        auto now = std::chrono::steady_clock::now();

        auto ratio = static_cast<float>(curr) / static_cast<float>(size_);

        auto fill_count =
            static_cast<size_t>(ratio * static_cast<float>(width));
        auto empty_count = width - fill_count;

        auto elapsed   = now - start_;
        auto remaining = elapsed / ratio - elapsed;

        fmt::print(
            "\r{3:>3.0f}%|{0:#^{1}}{0:-^{2}}| {4}/{5} "
            "[{6:.0%M:%S}<{7:.0%M:%S}]",
            "", fill_count, empty_count, ratio * 100.0F, curr, size_, elapsed,
            remaining);

        std::cout.flush();

        curr++;
    }

    void end() {
        draw();
        fmt::print("\n");
    }

   private:
    size_t size_;
    size_t curr = 0;
    std::chrono::steady_clock::time_point start_;

    size_t width = 50;
};

auto load_module_from_path(llvm::LLVMContext& ctx, const std::string& path)
    -> std::unique_ptr<llvm::Module> {
    llvm::SMDiagnostic err;
    fmt::print("Loading the ll file \"{}\"\n", path);

    auto m = llvm::parseIRFile(path, err, ctx);

    if (m == nullptr) {
        fmt::print("ERROR\n");

        err.print("triskel", ::llvm::errs());
        fmt::print("Error while attempting to read the ll file {}", path);
        return nullptr;
    }

    return m;
}

struct Segment {
    Point start;
    Point end;
};

struct SegmentMetrics {
    /// @brief The number of segments
    size_t count;

    /// @brief The number of intersections
    size_t intersections;

    /// @brief The number of segments overlapping
    size_t overlaps;
};

auto count_intersections(const std::vector<Segment>& verticals,
                         const std::vector<Segment>& horizontals) -> size_t {
    size_t intersections = 0;

    for (const auto& segment : verticals) {
        auto it = std::ranges::upper_bound(
            horizontals, segment, [](const Segment& a, const Segment& b) {
                return a.start.y < b.start.y;
            });

        for (; it != horizontals.end(); it++) {
            const auto& horizontal = *it;

            if (horizontal.start.y <= segment.start.y) {
                fmt::print("horizontal: {} vs vertical: {}", horizontal.start.y,
                           segment.start.y);
                throw std::invalid_argument("WEIRD");
            }

            if (horizontal.start.y >= segment.end.y) {
                break;
            }

            // we have the horizontal y which is in ]start.y, end.y[
            if (horizontal.start.x < segment.start.x &&
                horizontal.end.x > segment.end.x) {
                intersections += 1;
            }
        }
    }

    return intersections;
}

// Is small in big
auto vertical_overlap(Segment big, Segment small) -> bool {
    return big.start.y <= small.start.y && small.end.y <= big.end.y;
}

// Is small in big
auto horizontal_overlap(Segment big, Segment small) -> bool {
    return big.start.x <= small.start.x && small.end.x <= big.end.x;
}

auto count_overlaps(const std::vector<Segment>& verticals,
                    const std::vector<Segment>& horizontals) -> size_t {
    auto overlaps = 0;

    for (size_t i = 0; i < verticals.size(); ++i) {
        auto segment = verticals[i];

        const auto x = segment.start.x;

        for (size_t j = i + 1; j < verticals.size(); ++j) {
            auto other = verticals[j];

            // The list is sorted
            if (other.end.x != x) {
                break;
            }

            if (vertical_overlap(segment, other) ||
                vertical_overlap(other, segment)) {
                overlaps += 1;
            }
        }
    }

    for (size_t i = 0; i < horizontals.size(); ++i) {
        auto segment = horizontals[i];

        const auto y = segment.start.y;

        for (size_t j = i + 1; j < horizontals.size(); ++j) {
            auto other = horizontals[j];

            // The list is sorted
            if (other.end.y != y) {
                break;
            }

            if (horizontal_overlap(segment, other) ||
                horizontal_overlap(other, segment)) {
                overlaps += 1;
            }
        }
    }

    return overlaps;
}

auto measure_segments(triskel::CFGLayout& layout) -> SegmentMetrics {
    auto verticals   = std::vector<Segment>{};
    auto horizontals = std::vector<Segment>{};

    // An edge has at least 3 segments: 2 vertical and 1 horizontal
    verticals.reserve(2 * layout.edge_count());
    horizontals.reserve(layout.edge_count());

    for (size_t edge_id = 0; edge_id < layout.edge_count(); ++edge_id) {
        const auto& waypoints = layout.get_waypoints(edge_id);

        if (waypoints.empty()) {
            throw std::invalid_argument("Edge was not laid out");
        }

        auto start = waypoints[0];

        for (size_t i = 1; i < waypoints.size(); i++) {
            auto end = waypoints[i];
            if (start.x == end.x) {
                // This is a vertical segment
                if (start.y == end.y) {
                    // This is a point, we can ignore it
                    continue;
                }
                auto top    = start.y < end.y ? start : end;
                auto bottom = start.y < end.y ? end : start;
                verticals.push_back({top, bottom});
            } else if (start.y == end.y) {
                // This is a horizontal segment
                auto left  = start.y < end.y ? start : end;
                auto right = start.y < end.y ? end : start;
                horizontals.push_back({left, right});
            } else {
                // This should not happen
                for (const auto& waypoint : waypoints) {
                    fmt::print("({}, {})\n", waypoint.x, waypoint.y);
                }

                fmt::print("({},{}) ({},{}) on i={}, and edge={}\n", start.x,
                           start.y, end.x, end.y, i, edge_id);
                throw std::invalid_argument("Non orthogonal line");
            }

            start = end;
        }
    }

    // Sort the horizontal lines by y coordinate
    std::ranges::sort(horizontals, [](const Segment& a, const Segment& b) {
        // We have start.y == end.y
        return a.start.y < b.start.y;
    });

    // Sort the vertical lines by x coordinate
    std::ranges::sort(verticals, [](const Segment& a, const Segment& b) {
        // We have start.x == end.x
        return a.start.x < b.start.x;
    });

    return {.count         = verticals.size() + horizontals.size(),
            .intersections = count_intersections(verticals, horizontals),
            .overlaps      = count_overlaps(verticals, horizontals)};
}

struct Stats {
    std::string function_name;
    size_t nb_nodes;
    size_t nb_edges;
    float height;
    float width;
    size_t layout_time;  // in ms
    size_t nb_intersections;
    size_t nb_overlaps;
    size_t nb_segments;
};

auto stats_to_csv(const std::vector<Stats>& stats) {
    std::string s;

    // header
    s = "function_name,nb_nodes,nb_edges,height,width,layout_time,nb_"
        "intersections,nb_overlaps,nb_segments\n";

    // body
    for (const auto& stat : stats) {
        s += fmt::format("{},{},{},{},{},{},{},{},{}\n", stat.function_name,
                         stat.nb_nodes, stat.nb_edges, stat.height, stat.width,
                         stat.layout_time, stat.nb_intersections,
                         stat.nb_overlaps, stat.nb_segments);
    }

    return s;
}

auto write_str_to_file(const std::string& path, const std::string& data) {
    std::ofstream file{path};

    if (!file) {
        fmt::print("Could not open file \"{}\"\n", path);
        throw std::invalid_argument("Could not opent path");
    }

    file << data;
    file.close();

    return 0;
}

auto run_on_function(llvm::Function& function,
                     size_t& errors,
                     size_t& skipped,
                     llvm::ModuleSlotTracker& MST) -> std::optional<Stats> {
    if (function.isDeclaration()) {
        return {};
    }

    // Ignore functions with less then 3 blocks
    if (function.size() <= 2) {
        return {};
    }

    // Skip long functions
    if (FLAGS_max_nodes != 0 && function.size() > FLAGS_max_nodes) {
        skipped += 1;
        return {};
    }

    std::unique_ptr<triskel::CFGLayout> layout;

    // NOLINTNEXTLINE(google-build-using-namespace)
    using namespace std::chrono;

    const auto start = high_resolution_clock::now();

    layout = triskel::make_layout(&function, nullptr, &MST);

    const auto elapsed    = high_resolution_clock::now() - start;
    const auto elapsed_ms = duration_cast<milliseconds>(elapsed).count();

    const auto segment_stats = measure_segments(*layout);

    return Stats{
        .function_name    = function.getName().str(),
        .nb_nodes         = layout->node_count(),
        .nb_edges         = layout->edge_count(),
        .height           = layout->get_height(),
        .width            = layout->get_width(),
        .layout_time      = static_cast<size_t>(elapsed_ms),
        .nb_intersections = segment_stats.intersections,
        .nb_overlaps      = segment_stats.overlaps,
        .nb_segments      = segment_stats.count,
    };
}

auto run_on_module(llvm::Module& module,
                   llvm::ModuleSlotTracker& MST) -> std::vector<Stats> {
    fmt::print("Running on module: {}\n", module.getName().str());

    auto stats = std::vector<Stats>{};
    stats.reserve(module.size());

    size_t errors        = 0;
    size_t skipped       = 0;
    size_t intersections = 0;
    size_t overlaps      = 0;
    size_t segments      = 0;
    size_t heights       = 0;

    auto start = std::chrono::high_resolution_clock::now();

    auto bar = ProgressBar(module.size());
    bar.start();
    for (auto& function : module) {
        bar.draw();
        auto stat = run_on_function(function, errors, skipped, MST);
        if (stat.has_value()) {
            intersections += stat->nb_intersections;
            overlaps += stat->nb_overlaps;
            segments += stat->nb_segments;
            heights += stat->height;
            stats.push_back(*stat);
        };
    }
    bar.end();

    auto elapsed = std::chrono::high_resolution_clock::now() - start;

    std::locale::global(std::locale("en_US.UTF-8"));

    fmt::print("\n\nSummary:\n\n");
    print_table(Entry("Functions", stats.size(), "{:L}"),        //
                Entry("Skipped", skipped, "{:L}"),               //
                Entry("Errors", errors, "{:L}"),                 //
                Entry("Time", elapsed, "{:.0%M:%S}"),            //
                Entry("Segments", segments, "{:L}"),             //
                Entry("Intersections", intersections, "{:L}"),   //
                Entry("Overlaps", overlaps, "{:L}"),             //
                Entry("Height", heights / stats.size(), "{:L}")  //
    );

    return stats;
}

}  // namespace

auto main(int argc, char** argv) -> int {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (argc < 2 || argc > 3) {
        fmt::print(
            "No module specified. Usage:\n"
            "{} <path to llvm bc> [function name]\n",
            argv[0]);
        return 1;
    }

    llvm::LLVMContext ctx;

    auto module = load_module_from_path(ctx, argv[1]);
    auto MST    = llvm::ModuleSlotTracker{module.get()};

    if (argc == 3) {
        auto* function = module->getFunction(argv[2]);
        size_t errors  = 0;
        size_t skipped = 0;

        auto stat = run_on_function(*function, errors, skipped, MST);

        if (stat.has_value()) {
            fmt::print("{}\n", stats_to_csv({*stat}));
        }

        return 0;
    }

    auto stats = run_on_module(*module, MST);

    auto path = std::filesystem::path{argv[1]};
    auto ext  = std::filesystem::path{"csv"};
    path.replace_extension(ext);

    auto csv_path = std::filesystem::path{FLAGS_out_dir};
    csv_path      = csv_path / path.filename();

    write_str_to_file(csv_path, stats_to_csv(stats));

    fmt::print("Full report available at {}\n", csv_path.string());

    return 0;
}