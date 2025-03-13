#include <triskel/analysis/lengauer_tarjan.hpp>

#include <fmt/printf.h>
#include <gtest/gtest.h>

#include <triskel/graph/graph.hpp>
#include "triskel/graph/igraph.hpp"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

// The graph from the wikipedia example
// https://en.wikipedia.org/wiki/Depth-first_search#Output_of_a_depth-first_search
#define GRAPH1                      \
    auto graph = Graph{};           \
    auto& ge   = graph.editor();    \
    ge.push();                      \
                                    \
    auto r = ge.make_node();        \
    auto a = ge.make_node();        \
    auto b = ge.make_node();        \
    auto c = ge.make_node();        \
    auto d = ge.make_node();        \
    auto e = ge.make_node();        \
    auto f = ge.make_node();        \
    auto g = ge.make_node();        \
    auto h = ge.make_node();        \
    auto i = ge.make_node();        \
    auto j = ge.make_node();        \
    auto k = ge.make_node();        \
    auto l = ge.make_node();        \
                                    \
    auto er_a = ge.make_edge(r, a); \
    auto er_b = ge.make_edge(r, b); \
    auto er_c = ge.make_edge(r, c); \
                                    \
    auto ea_d = ge.make_edge(a, d); \
                                    \
    auto eb_a = ge.make_edge(b, a); \
    auto eb_d = ge.make_edge(b, d); \
    auto eb_e = ge.make_edge(b, e); \
                                    \
    auto ec_f = ge.make_edge(c, f); \
    auto ec_g = ge.make_edge(c, g); \
                                    \
    auto ed_l = ge.make_edge(d, l); \
                                    \
    auto ee_h = ge.make_edge(e, h); \
                                    \
    auto ef_i = ge.make_edge(f, i); \
                                    \
    auto eg_i = ge.make_edge(g, i); \
    auto eg_j = ge.make_edge(g, j); \
                                    \
    auto eh_e = ge.make_edge(h, e); \
    auto eh_k = ge.make_edge(h, k); \
                                    \
    auto ei_k = ge.make_edge(i, k); \
                                    \
    auto ej_i = ge.make_edge(j, i); \
                                    \
    auto ek_i = ge.make_edge(k, i); \
    auto ek_r = ge.make_edge(k, r); \
                                    \
    auto el_h = ge.make_edge(l, h); \
                                    \
    ge.commit();

TEST(Domination, Smoke) {
    GRAPH1;

    auto idom = make_idoms(graph);

    ASSERT_EQ(idom[r], NodeId::InvalidID);

    ASSERT_EQ(idom[a], r);
    ASSERT_EQ(idom[b], r);
    ASSERT_EQ(idom[c], r);
    ASSERT_EQ(idom[d], r);
    ASSERT_EQ(idom[e], r);
    ASSERT_EQ(idom[f], c);
    ASSERT_EQ(idom[g], c);
    ASSERT_EQ(idom[h], r);
    ASSERT_EQ(idom[i], r);
    ASSERT_EQ(idom[j], g);
    ASSERT_EQ(idom[k], r);
    ASSERT_EQ(idom[l], d);
}

#undef GRAPH1
