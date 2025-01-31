![](https://github.com/triskellib/triskel/raw/master/.github/assets/triskel_light.png)

Python bindings for the [triskel](https://github.com/triskellib/triskel) library.

## Getting started

```
$ pip install pytriskel
```

```python
from pytriskel.pytriskel import *

builder = make_layout_builder()

# Build the graph
n1 = builder.make_node("Hello")
n2 = builder.make_node("World")
builder.make_edge(n1, n2)

# Measure node size using font size
png_renderer = make_png_renderer()
builder.measure_nodes(png_renderer)

# Export an image
layout = builder.build()
layout.save(png_renderer, "out.png")
```
