#include <triskel/layout/sugiyama/network_simplex.hpp>

#include <triskel/analysis/dfs.hpp>

#include <fmt/base.h>
#include <fmt/printf.h>
#include <gtest/gtest.h>

#include <triskel/graph/graph.hpp>

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

TEST(NetworkSimplex, CutValue1) {
    auto graph = Graph{};
    auto& ge   = graph.editor();
    ge.push();

    auto a = ge.make_node();
    auto b = ge.make_node();
    auto c = ge.make_node();
    auto d = ge.make_node();
    auto e = ge.make_node();
    auto f = ge.make_node();
    auto g = ge.make_node();
    auto h = ge.make_node();

    auto ea_e = ge.make_edge(a, e);
    auto ea_f = ge.make_edge(a, f);
    auto ea_b = ge.make_edge(a, b);

    auto eb_c = ge.make_edge(b, c);

    auto ec_d = ge.make_edge(c, d);

    auto ed_h = ge.make_edge(d, h);

    auto ee_g = ge.make_edge(e, g);

    auto ef_g = ge.make_edge(f, g);

    auto eg_h = ge.make_edge(g, h);
    ge.commit();

    auto st = SpanningTree(graph);

    st.e_in_tree_[ea_b] = true;
    st.e_in_tree_[eb_c] = true;
    st.e_in_tree_[ec_d] = true;
    st.e_in_tree_[ed_h] = true;
    st.e_in_tree_[eg_h] = true;
    st.e_in_tree_[ee_g] = true;
    st.e_in_tree_[ef_g] = true;

    st.init_cut_values();

    fmt::print("a -> b {}\n", st.cut_[ea_b]);
    fmt::print("b -> c {}\n", st.cut_[eb_c]);
    fmt::print("c -> d {}\n", st.cut_[ec_d]);
    fmt::print("d -> h {}\n", st.cut_[ed_h]);
    fmt::print("g -> h {}\n", st.cut_[eg_h]);
    fmt::print("e -> g {}\n", st.cut_[ee_g]);
    fmt::print("f -> g {}\n", st.cut_[ef_g]);

    ASSERT_EQ(st.cut_[ea_b], 3);
    ASSERT_EQ(st.cut_[eb_c], 3);
    ASSERT_EQ(st.cut_[ec_d], 3);
    ASSERT_EQ(st.cut_[ed_h], 3);
    ASSERT_EQ(st.cut_[eg_h], -1);
    ASSERT_EQ(st.cut_[ee_g], 0);
    ASSERT_EQ(st.cut_[ef_g], 0);
}

TEST(NetworkSimplex, CutValue2) {
    auto graph = Graph{};
    auto& ge   = graph.editor();
    ge.push();

    auto a = ge.make_node();
    auto b = ge.make_node();
    auto c = ge.make_node();
    auto d = ge.make_node();
    auto e = ge.make_node();
    auto f = ge.make_node();
    auto g = ge.make_node();
    auto h = ge.make_node();

    auto ea_e = ge.make_edge(a, e);
    auto ea_f = ge.make_edge(a, f);
    auto ea_b = ge.make_edge(a, b);

    auto eb_c = ge.make_edge(b, c);

    auto ec_d = ge.make_edge(c, d);

    auto ed_h = ge.make_edge(d, h);

    auto ee_g = ge.make_edge(e, g);

    auto ef_g = ge.make_edge(f, g);

    auto eg_h = ge.make_edge(g, h);
    ge.commit();

    auto st = SpanningTree(graph);

    st.e_in_tree_[ea_b] = true;
    st.e_in_tree_[eb_c] = true;
    st.e_in_tree_[ec_d] = true;
    st.e_in_tree_[ed_h] = true;
    st.e_in_tree_[ea_e] = true;
    st.e_in_tree_[ee_g] = true;
    st.e_in_tree_[ef_g] = true;

    st.init_cut_values();

    fmt::print("a -> b {}\n", st.cut_[ea_b]);
    fmt::print("b -> c {}\n", st.cut_[eb_c]);
    fmt::print("c -> d {}\n", st.cut_[ec_d]);
    fmt::print("d -> h {}\n", st.cut_[ed_h]);
    fmt::print("a -> e {}\n", st.cut_[ea_e]);
    fmt::print("e -> g {}\n", st.cut_[ee_g]);
    fmt::print("f -> g {}\n", st.cut_[ef_g]);

    ASSERT_EQ(st.cut_[ea_b], 2);
    ASSERT_EQ(st.cut_[eb_c], 2);
    ASSERT_EQ(st.cut_[ec_d], 2);
    ASSERT_EQ(st.cut_[ed_h], 2);
    ASSERT_EQ(st.cut_[ea_e], 1);
    ASSERT_EQ(st.cut_[ee_g], 1);
    ASSERT_EQ(st.cut_[ef_g], 0);
}
