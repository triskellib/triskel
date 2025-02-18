#include <memory>
#include <stdexcept>

#include <fmt/core.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>

#include "arch.hpp"
#include "triskel/triskel.hpp"

namespace {

struct LLVMArch : public Arch {
    llvm::LLVMContext ctx;
    std::unique_ptr<llvm::Module> m;

    void load_module_from_path(const std::string& path) {
        llvm::SMDiagnostic err;
        fmt::print("Loading the ll file \"{}\"\n", path);

        m = llvm::parseIRFile(path, err, ctx);

        if (m == nullptr) {
            fmt::print("ERROR\n");

            err.print("leviathan", ::llvm::errs());
            fmt::print("Error while attempting to read the ll file {}", path);
            throw std::runtime_error("Error loading llvm module");
        }
    }

    auto select_function(const std::string& name, triskel::Renderer& renderer)
        -> std::unique_ptr<triskel::CFGLayout> override {
        auto* f = m->getFunction(name);
        fmt::print("Selecting function {} ({} blocks)\n", f->getName().str(),
                   f->size());
        return triskel::make_layout(f, &renderer);
    }

    void start(const std::string& path) override {
        load_module_from_path(path);
        assert(m != nullptr);

        for (auto& f : *m) {
            function_names.push_back(f.getName().str());
        }

        std::ranges::sort(function_names);
    }
};

}  // namespace

auto make_llvm_arch() -> std::unique_ptr<Arch> {
    return std::make_unique<LLVMArch>();
}