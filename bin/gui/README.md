# Triskel GUI

A sample program using the triskel ImGui widget:

```cpp
#include <triskel/triskel.hpp>

imgui_renderer.Begin("##cfg", {42.0F, 10.0F});
layout->render(imgui_renderer);
imgui_renderer.End();
```

![](https://github.com/triskellib/triskel/blob/master/.github/assets/triskel-gui.png?raw=true)

## Usage

```
$ imgui-gui <filename>
```

The GUI works with LLVM bytecode (`.ll` and `.bc` files).
There is also limited support for `x64` binaries using a custom _recursive descent_ disassembler.

In the GUI, functions can be selected on the left.
To modify the canvas viewport
- use the scroll wheel for zoom
- use the right mouse button to pan
