#pragma once

#include <memory>
#include <vector>
#include "triskel/triskel.hpp"

struct Arch {
    virtual ~Arch() = default;

    /// @brief Initialization
    virtual void start(const std::string& path) = 0;

    /// @brief The functions in the binary
    std::vector<std::string> function_names;

    /// @brief Selects a function
    [[nodiscard]] virtual auto select_function(const std::string& name,
                                               triskel::Renderer& renderer)
        -> std::unique_ptr<triskel::CFGLayout> = 0;
};

auto make_llvm_arch() -> std::unique_ptr<Arch>;
auto make_bin_arch() -> std::unique_ptr<Arch>;
