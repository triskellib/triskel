from __future__ import annotations
import typing
__all__ = ['CFGLayout', 'Default', 'EdgeType', 'ExportingRenderer', 'F', 'LayoutBuilder', 'Point', 'Renderer', 'T', 'make_layout_builder', 'make_png_renderer', 'make_svg_renderer']
class CFGLayout:
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def get_coords(self, arg0: int) -> Point:
        """
        Gets the x and y coordinate of a node
        """
    @typing.overload
    def get_height(self) -> float:
        """
        Gets height of the graph
        """
    @typing.overload
    def get_height(self) -> float:
        """
        Gets width of the graph
        """
    def get_waypoints(self, arg0: int) -> ...:
        """
        Gets the waypoints of an edge
        """
    def save(self, arg0: ExportingRenderer, arg1: str) -> None:
        """
        Generate an image of the graph
        """
class EdgeType:
    """
    Members:
    
      Default
    
      T
    
      F
    """
    Default: typing.ClassVar[EdgeType]  # value = <EdgeType.Default: 0>
    F: typing.ClassVar[EdgeType]  # value = <EdgeType.F: 2>
    T: typing.ClassVar[EdgeType]  # value = <EdgeType.T: 1>
    __members__: typing.ClassVar[dict[str, EdgeType]]  # value = {'Default': <EdgeType.Default: 0>, 'T': <EdgeType.T: 1>, 'F': <EdgeType.F: 2>}
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __eq__(self, other: typing.Any) -> bool:
        ...
    def __getstate__(self) -> int:
        ...
    def __hash__(self) -> int:
        ...
    def __index__(self) -> int:
        ...
    def __init__(self, value: int) -> None:
        ...
    def __int__(self) -> int:
        ...
    def __ne__(self, other: typing.Any) -> bool:
        ...
    def __repr__(self) -> str:
        ...
    def __setstate__(self, state: int) -> None:
        ...
    def __str__(self) -> str:
        ...
    @property
    def name(self) -> str:
        ...
    @property
    def value(self) -> int:
        ...
class ExportingRenderer(Renderer):
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
class LayoutBuilder:
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def build(self) -> CFGLayout:
        """
        Builds the layout
        """
    @typing.overload
    def make_edge(self, arg0: int, arg1: int) -> int:
        """
        to
        """
    @typing.overload
    def make_edge(self, arg0: int, arg1: int, arg2: EdgeType) -> int:
        """
        type
        """
    @typing.overload
    def make_node(self) -> int:
        """
        Creates a new node
        """
    @typing.overload
    def make_node(self, arg0: float, arg1: float) -> int:
        """
        Creates a new node with a width and height
        """
    @typing.overload
    def make_node(self, arg0: str) -> int:
        """
        Creates a new node with a label
        """
    @typing.overload
    def make_node(self, arg0: Renderer, arg1: str) -> int:
        """
        Creates a new node using a renderer to determine the size of labels
        """
    def measure_nodes(self, arg0: Renderer) -> None:
        """
        Calculates the dimension of each node using the renderer
        """
class Point:
    x: float
    y: float
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
    def __init__(self, arg0: int, arg1: int) -> None:
        ...
class Renderer:
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs):
        ...
def make_layout_builder() -> LayoutBuilder:
    ...
def make_png_renderer() -> ExportingRenderer:
    ...
def make_svg_renderer() -> ExportingRenderer:
    ...
Default: EdgeType  # value = <EdgeType.Default: 0>
F: EdgeType  # value = <EdgeType.F: 2>
T: EdgeType  # value = <EdgeType.T: 1>
