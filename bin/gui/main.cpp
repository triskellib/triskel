#include <gflags/gflags.h>

#include "gui/triskel.hpp"

auto main(int argc, char** argv) -> int {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    auto app = Triskel{};

    if (app.Create(argc, argv)) {
        return app.Run();
    }

    return 0;
}