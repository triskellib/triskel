#include <memory>
#include <string>

#include <fmt/core.h>
#include <gflags/gflags.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include "triskel/triskel.hpp"

namespace {
auto load_module_from_path(llvm::LLVMContext& ctx, const std::string& path)
    -> std::unique_ptr<llvm::Module> {
    llvm::SMDiagnostic err;
    fmt::print("Loading the ll file \"{}\"\n", path);

    auto m = llvm::parseIRFile(path, err, ctx);

    if (m == nullptr) {
        fmt::print("ERROR\n");

        err.print("triskel", ::llvm::errs());
        fmt::print("Error while attempting to read the ll file {}", path);
        return nullptr;
    }

    return m;
}

auto draw_function(llvm::Function* f) {
    auto renderer = triskel::make_svg_renderer();

    auto layout = triskel::make_layout(f, renderer.get());

    layout->render_and_save(*renderer, "./out.svg");
}

};  // namespace

auto main(int argc, char** argv) -> int {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (argc != 3) {
        fmt::print(
            "No module specified. Usage:\n"
            "{} <path to llvm bc> <function name>\n",
            argv[0]);
        return 1;
    }

    llvm::LLVMContext ctx;

    auto module = load_module_from_path(ctx, argv[1]);

    auto* function = module->getFunction(argv[2]);

    if (function == nullptr) {
        fmt::print("No such function found: \"{}\"\n", argv[2]);
        return 1;
    }

    draw_function(function);

    return 0;
}