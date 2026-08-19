#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/vector.h>

#include <cstddef>
#include <filesystem>

#include "triskel/triskel.hpp"
#include "triskel/utils/point.hpp"

namespace nb = nanobind;

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace nb::literals;

using EdgeType = triskel::LayoutBuilder::EdgeType;

NB_MODULE(pytriskel, m) {
    m.doc() = "Python bindings for the triskel CFG layout library";

    nb::enum_<EdgeType>(m, "EdgeType")
        .value("Default", EdgeType::Default)
        .value("T", EdgeType::True)
        .value("F", EdgeType::False)
        .export_values();

    nb::class_<triskel::Renderer> Renderer(m, "Renderer");

    nb::class_<triskel::ExportingRenderer, triskel::Renderer> ExportingRenderer(
        m, "ExportingRenderer");

    nb::class_<triskel::Point>(m, "Point")
        .def(
            "__init__",
            [](triskel::Point* p, float x, float y) {
                new (p) triskel::Point{.x = x, .y = y};
            },
            "x"_a, "y"_a)
        .def_rw("x", &triskel::Point::x)
        .def_rw("y", &triskel::Point::y);

    nb::class_<triskel::CFGLayout>(m, "CFGLayout")
        .def("get_coords", &triskel::CFGLayout::get_coords, "node"_a,
             "Gets the x and y coordinate of a node")
        .def("get_waypoints", &triskel::CFGLayout::get_waypoints, "edge"_a,
             "Gets the waypoints of an edge")
        .def("get_height", &triskel::CFGLayout::get_height,
             "Gets height of the graph")
        .def("get_width", &triskel::CFGLayout::get_width,
             "Gets width of the graph")
        .def(
            "save",
            [](triskel::CFGLayout& layout, triskel::ExportingRenderer& renderer,
               const std::string& path) {
                layout.render_and_save(renderer, path);
            },
            "renderer"_a, "path"_a, "Generate an image of the graph");

    nb::class_<triskel::LayoutBuilder>(m, "LayoutBuilder")
        .def("make_node",
             nb::overload_cast<>(&triskel::LayoutBuilder::make_node),
             "Creates a new node")
        .def(
            "make_node",
            nb::overload_cast<float, float>(&triskel::LayoutBuilder::make_node),
            "height"_a, "width"_a,
            "Creates a new node with a width and height")
        .def("make_node",
             nb::overload_cast<const std::string&>(
                 &triskel::LayoutBuilder::make_node),
             "label"_a, "Creates a new node with a label")
        .def("make_node",
             nb::overload_cast<const triskel::Renderer&, const std::string&>(
                 &triskel::LayoutBuilder::make_node),
             "renderer"_a, "label"_a,
             "Creates a new node using a renderer to determine the size of "
             "labels")
        .def("make_edge",
             nb::overload_cast<size_t, size_t>(
                 &triskel::LayoutBuilder::make_edge),
             "from_"_a, "to"_a, "Creates a new edge")
        .def("make_edge",
             nb::overload_cast<size_t, size_t, EdgeType>(
                 &triskel::LayoutBuilder::make_edge),
             "from_"_a, "to"_a, "type"_a, "Creates a new edge")
        .def("measure_nodes", &triskel::LayoutBuilder::measure_nodes,
             "renderer"_a,
             "Calculates the dimension of each node using the renderer")
        .def("build", &triskel::LayoutBuilder::build, "Builds the layout");

    m.def("make_layout_builder", &triskel::make_layout_builder,
          "Creates a new layout builder");

    m.def("make_png_renderer", &triskel::make_png_renderer,
          "Creates a renderer that can export the layout to a PNG file");

    m.def("make_svg_renderer", &triskel::make_svg_renderer,
          "Creates a renderer that can export the layout to an SVG file");
}
