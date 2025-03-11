#include <triskel/graph/subgraph.hpp>

#include <fmt/printf.h>
#include <gtest/gtest.h>

#include <triskel/graph/graph.hpp>

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

#define GRAPH1                         \
    auto gg  = Graph{};                \
    auto gge = gg.editor();            \
    gge.push();                        \
                                       \
    auto n1 = gge.make_node();         \
    auto n2 = gge.make_node();         \
    auto n3 = gge.make_node();         \
    auto n4 = gge.make_node();         \
    auto n5 = gge.make_node();         \
    auto n6 = gge.make_node();         \
    auto n7 = gge.make_node();         \
    auto n8 = gge.make_node();         \
                                       \
    auto e1_2 = gge.make_edge(n1, n2); \
    auto e1_5 = gge.make_edge(n1, n5); \
    auto e1_8 = gge.make_edge(n1, n8); \
                                       \
    auto e2_3 = gge.make_edge(n2, n3); \
                                       \
    auto e3_4 = gge.make_edge(n3, n4); \
                                       \
    auto e4_2 = gge.make_edge(n4, n2); \
                                       \
    auto e5_6 = gge.make_edge(n5, n6); \
                                       \
    auto e6_3 = gge.make_edge(n6, n3); \
    auto e6_7 = gge.make_edge(n6, n7); \
    auto e6_8 = gge.make_edge(n6, n8); \
    gge.commit();                      \
                                       \
    auto g  = SubGraph(gg);            \
    auto ge = g.editor();

TEST(SubGraph, Smoke) {
    GRAPH1;

    ge.select_node(n1);

    ASSERT_EQ(g.node_count(), 1);
    ASSERT_TRUE(std::ranges::contains(g.nodes(), n1));
}

TEST(SubGraph, AddEdge) {
    GRAPH1;

    ge.select_node(n1);
    ge.select_node(n2);

    ASSERT_EQ(g.edge_count(), 1);
    ASSERT_TRUE(std::ranges::contains(g.edges(), e1_2));
}

TEST(SubGraph, addNode) {
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

TEST(SubGraph, rmNode) {
    GRAPH1
    ge.select_node(n2);
    ge.select_node(n3);

    size_t og_size = g.node_count();

    ge.push();

    ge.remove_node(n3);

    ASSERT_EQ(g.node_count(), og_size - 1);
    ASSERT_FALSE(std::ranges::contains(g.edges(), e2_3));

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

TEST(SubGraph, addEdge) {
    GRAPH1
    ge.select_node(n1);
    ge.select_node(n5);

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

TEST(SubGraph, rmEdge) {
    GRAPH1

    ge.select_node(n3);
    ge.select_node(n4);

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

TEST(SubGraph, editEdge) {
    GRAPH1

    ge.select_node(n1);
    ge.select_node(n3);
    ge.select_node(n4);
    ge.select_node(n5);

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