#include <triskel/graph/graph.hpp>

#include <algorithm>

#include <fmt/base.h>
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

TEST(Attribute, addNode) {
    GRAPH1

    ge.push();

    size_t og_size = g.node_count();

    auto new_node = ge.make_node();

    ASSERT_EQ(g.node_count(), og_size + 1);

    ge.pop();

    ASSERT_EQ(g.node_count(), og_size);

    for (const auto& n : g.nodes()) {
        ASSERT_NE(n, new_node);
    }
}

TEST(Attribute, rmNode) {
    GRAPH1

    size_t og_size = g.node_count();

    ge.push();

    ge.remove_node(n3);

    ASSERT_EQ(g.node_count(), og_size - 1);

    for (const auto& n : g.nodes()) {
        ASSERT_NE(n, n3);
    }

    for (const auto& e : g.edges()) {
        ASSERT_NE(e.from(), n3);
        ASSERT_NE(e.to(), n3);
    }

    ge.pop();

    ASSERT_EQ(g.node_count(), og_size);

    ASSERT_TRUE(std::ranges::contains(g.edges(), e2_3));
}

TEST(Attribute, addEdge) {
    GRAPH1

    ge.push();

    size_t og_size = g.edge_count();

    auto new_edge = ge.make_edge(n1, n5);

    ASSERT_TRUE(std::ranges::contains(n1.edges(), new_edge));
    ASSERT_TRUE(std::ranges::contains(n5.edges(), new_edge));

    ASSERT_EQ(g.edge_count(), og_size + 1);

    ge.pop();

    ASSERT_EQ(g.edge_count(), og_size);

    for (const auto& e : g.edges()) {
        ASSERT_NE(e, new_edge);
    }
}

TEST(Attribute, rmEdge) {
    GRAPH1

    ge.push();

    size_t og_size = g.edge_count();

    ge.remove_edge(e3_4);

    ASSERT_EQ(g.edge_count(), og_size - 1);

    ASSERT_FALSE(std::ranges::contains(n3.edges(), e3_4));
    ASSERT_FALSE(std::ranges::contains(n4.edges(), e3_4));
    ASSERT_FALSE(std::ranges::contains(g.edges(), e3_4));

    ge.pop();

    ASSERT_EQ(g.edge_count(), og_size);

    ASSERT_TRUE(std::ranges::contains(n3.edges(), e3_4));
    ASSERT_TRUE(std::ranges::contains(n4.edges(), e3_4));
    ASSERT_TRUE(std::ranges::contains(g.edges(), e3_4));
}

TEST(Attribute, editEdge) {
    GRAPH1

    ge.push();

    ge.edit_edge(e3_4, n1, n5);

    ASSERT_EQ(e3_4.from(), n1);
    ASSERT_EQ(e3_4.to(), n5);

    ASSERT_TRUE(std::ranges::contains(n1.edges(), e3_4));
    ASSERT_TRUE(std::ranges::contains(n5.edges(), e3_4));

    ASSERT_FALSE(std::ranges::contains(n3.edges(), e3_4));
    ASSERT_FALSE(std::ranges::contains(n4.edges(), e3_4));

    ge.pop();

    ASSERT_EQ(e3_4.from(), n3);
    ASSERT_EQ(e3_4.to(), n4);

    ASSERT_FALSE(std::ranges::contains(n1.edges(), e3_4));
    ASSERT_FALSE(std::ranges::contains(n5.edges(), e3_4));

    ASSERT_TRUE(std::ranges::contains(n3.edges(), e3_4));
    ASSERT_TRUE(std::ranges::contains(n4.edges(), e3_4));
}

#undef GRAPH1
