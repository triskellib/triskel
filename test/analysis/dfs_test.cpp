#include <triskel/analysis/dfs.hpp>

#include <fmt/printf.h>
#include <gtest/gtest.h>

#include <triskel/graph/graph.hpp>

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

// The graph from the wikipedia example
// https://en.wikipedia.org/wiki/Depth-first_search#Output_of_a_depth-first_search
#define GRAPH1                        \
    auto g   = Graph{};               \
    auto& ge = g.editor();            \
    ge.push();                        \
                                      \
    auto n1 = ge.make_node();         \
    auto n2 = ge.make_node();         \
    auto n3 = ge.make_node();         \
    auto n4 = ge.make_node();         \
    auto n5 = ge.make_node();         \
    auto n6 = ge.make_node();         \
    auto n7 = ge.make_node();         \
    auto n8 = ge.make_node();         \
                                      \
    auto e1_2 = ge.make_edge(n1, n2); \
    auto e1_5 = ge.make_edge(n1, n5); \
    auto e1_8 = ge.make_edge(n1, n8); \
                                      \
    auto e2_3 = ge.make_edge(n2, n3); \
                                      \
    auto e3_4 = ge.make_edge(n3, n4); \
                                      \
    auto e4_2 = ge.make_edge(n4, n2); \
                                      \
    auto e5_6 = ge.make_edge(n5, n6); \
                                      \
    auto e6_3 = ge.make_edge(n6, n3); \
    auto e6_7 = ge.make_edge(n6, n7); \
    auto e6_8 = ge.make_edge(n6, n8); \
    ge.commit();

TEST(DFSAnalysis, Smoke) {
    GRAPH1;

    ASSERT_NO_THROW(auto dfs = DFSAnalysis(g));
}

TEST(DFSAnalysis, EdgeTypes) {
    GRAPH1;

    auto dfs = DFSAnalysis(g);

    fmt::print("Node order: \n");
    for (const auto& node : dfs.nodes()) {
        fmt::print("- {}\n", node);
    }

    ASSERT_TRUE(dfs.is_backedge(e4_2));

    ASSERT_TRUE(dfs.is_forward(e1_8));

    ASSERT_TRUE(dfs.is_cross(e6_3));

    for (const auto& e : {e1_2, e1_5, e2_3, e5_6, e3_4, e6_7, e6_8}) {
        fmt::print("Testing edge {}\n", e);
        ASSERT_TRUE(dfs.is_tree(e));
    }
}

#undef GRAPH1
