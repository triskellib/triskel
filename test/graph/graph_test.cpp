#include <triskel/graph/graph.hpp>

#include <gtest/gtest.h>

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

// The graph from the wikipedia example
// https://en.wikipedia.org/wiki/Depth-first_search#Output_of_a_depth-first_search
#define GRAPH1                        \
    auto g  = Graph{};                \
    auto ge = g.editor();             \
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

TEST(Graph, Smoke) {
    ASSERT_NO_THROW(GRAPH1);
}

#undef GRAPH1
