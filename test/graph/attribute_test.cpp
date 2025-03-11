#include <triskel/utils/attribute.hpp>

#include <gtest/gtest.h>
#include <triskel/graph/graph.hpp>

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

TEST(Attribute, Node) {
    GRAPH1

    auto test_attribute = NodeAttribute<int>{g.max_node_id(), 42};

    ASSERT_EQ(test_attribute.get(n7), 42);

    test_attribute.set(n7, 0);

    ASSERT_EQ(test_attribute.get(n7), 0);
}

TEST(Attribute, Edge) {
    GRAPH1

    auto test_attribute = EdgeAttribute<int>{g.max_edge_id(), 42};

    ASSERT_EQ(test_attribute.get(e6_3), 42);

    test_attribute.set(e6_3, 0);

    ASSERT_EQ(test_attribute.get(e6_3), 0);
}

TEST(Attribute, Resize) {
    GRAPH1

    auto test_attribute = NodeAttribute<int>{1, 42};

    ASSERT_EQ(test_attribute.get(n1), 42);

    fmt::print("Reading value for n2\n");
    ASSERT_EQ(test_attribute.get(n2), 42);

    fmt::print("Setting value for n4\n");
    test_attribute.set(n4, 0);

    ASSERT_EQ(test_attribute.get(n4), 0);
}

#undef GRAPH1
