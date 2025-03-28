
#include <triskel/triskel.hpp>

#include <fmt/base.h>
#include <gtest/gtest.h>

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace triskel;

TEST(Triskel, Smoke1) {
    auto builder = make_layout_builder();

    const auto a = builder->make_node(100, 100);
    const auto b = builder->make_node(100, 100);
    const auto c = builder->make_node(100, 100);
    const auto d = builder->make_node(100, 100);
    const auto e = builder->make_node(100, 100);
    const auto f = builder->make_node(100, 100);
    const auto g = builder->make_node(100, 100);

    builder->make_edge(a, b);
    builder->make_edge(a, e);
    builder->make_edge(b, c);
    builder->make_edge(b, d);
    builder->make_edge(e, f);
    builder->make_edge(f, e);
    builder->make_edge(c, g);
    builder->make_edge(d, g);
    builder->make_edge(e, g);
    builder->make_edge(g, a);

    ASSERT_NO_THROW(const auto layout = builder->build());
}
