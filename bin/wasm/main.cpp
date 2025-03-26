#include <emscripten/bind.h>

#include <cstddef>

#include "triskel/triskel.hpp"
#include "triskel/utils/point.hpp"

// Binding the struct and function to WebAssembly
EMSCRIPTEN_BINDINGS(my_module) {
    emscripten::value_object<triskel::Point>("Point")
        .field("x", &triskel::Point::x)
        .field("y", &triskel::Point::y);

    emscripten::register_vector<triskel::Point>("PointVector");

    emscripten::class_<triskel::CFGLayout>("CFGLayout")
        .function("edge_count", &triskel::CFGLayout::edge_count)
        .function("node_count", &triskel::CFGLayout::node_count)
        .function("get_height", &triskel::CFGLayout::get_height)
        .function("get_width", &triskel::CFGLayout::get_width)
        .function("get_coords", &triskel::CFGLayout::get_coords)
        .function("get_waypoints", &triskel::CFGLayout::get_waypoints);

    emscripten::class_<triskel::LayoutBuilder>("LayoutBuilder")
        .function("build", &triskel::LayoutBuilder::build)
        .function("graphviz", &triskel::LayoutBuilder::graphviz)
        .function("make_node",
                  emscripten::select_overload<size_t(float, float)>(
                      &triskel::LayoutBuilder::make_node))
        .function("make_empty_node", emscripten::select_overload<size_t()>(
                                         &triskel::LayoutBuilder::make_node))
        .function("make_edge",
                  emscripten::select_overload<size_t(size_t, size_t)>(
                      &triskel::LayoutBuilder::make_edge));

    emscripten::function("make_layout_builder", &triskel::make_layout_builder);
}