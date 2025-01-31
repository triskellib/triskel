#include <pybind11/pybind11.h>
#include <cstddef>
#include "triskel/triskel.hpp"
#include "triskel/utils/point.hpp"

namespace py = pybind11;

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace pybind11::literals;

using EdgeType = triskel::LayoutBuilder::EdgeType;

PYBIND11_MODULE(pytriskel, m) {
    py::enum_<EdgeType>(m, "EdgeType")
        .value("Default", EdgeType::Default)
        .value("T", EdgeType::True)
        .value("F", EdgeType::False)
        .export_values();

    py::class_<triskel::Renderer> Renderer(m, "Renderer");

    py::class_<triskel::ExportingRenderer, triskel::Renderer> ExportingRenderer(
        m, "ExportingRenderer");

    py::class_<triskel::Point>(m, "Point")
        .def(py::init<size_t, size_t>())
        .def_readwrite("x", &triskel::Point::x)
        .def_readwrite("y", &triskel::Point::y);

    py::class_<triskel::CFGLayout>(m, "CFGLayout")
        .def("get_coords", &triskel::CFGLayout::get_coords,
             "Gets the x and y coordinate of a node")
        .def("get_waypoints", &triskel::CFGLayout::get_waypoints,
             "Gets the waypoints of an edge")
        .def("get_height", &triskel::CFGLayout::get_height,
             "Gets height of the graph")
        .def("get_height", &triskel::CFGLayout::get_width,
             "Gets width of the graph")
        .def(
            "save",
            [](triskel::CFGLayout& layout, triskel::ExportingRenderer& renderer,
               const std::string& path) {
                layout.render_and_save(renderer, path);
            },
            "Generate an image of the graph");

    py::class_<triskel::LayoutBuilder>(m, "LayoutBuilder")
        .def("make_node",
             py::overload_cast<>(&triskel::LayoutBuilder::make_node),
             "Creates a new node")
        .def(
            "make_node",
            py::overload_cast<float, float>(&triskel::LayoutBuilder::make_node),
            "Creates a new node with a width and height")
        .def("make_node",
             py::overload_cast<const std::string&>(
                 &triskel::LayoutBuilder::make_node),
             "Creates a new node with a label")
        .def("make_node",
             py::overload_cast<const triskel::Renderer&, const std::string&>(
                 &triskel::LayoutBuilder::make_node),
             "Creates a new node using a renderer to determine the size of "
             "labels")
        .def("make_edge",
             py::overload_cast<size_t, size_t>(
                 &triskel::LayoutBuilder::make_edge),
             "Creates a new edge", "from", "to")
        .def("make_edge",
             py::overload_cast<size_t, size_t, EdgeType>(
                 &triskel::LayoutBuilder::make_edge),
             "Creates a new edge", "from", "to", "type")
        .def("measure_nodes", &triskel::LayoutBuilder::measure_nodes,
             "Calculates the dimension of each node using the renderer")
        .def("build", &triskel::LayoutBuilder::build, "Builds the layout");

    m.def("make_layout_builder", &triskel::make_layout_builder);

    m.def("make_png_renderer", &triskel::make_png_renderer);

    m.def("make_svg_renderer", &triskel::make_svg_renderer);
}