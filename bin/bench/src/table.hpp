#pragma once

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <fmt/format.h>

template <typename T>
struct Entry {
    Entry(std::string name, const T& value, fmt::format_string<T> fmt = "{}")
        : name{std::move(name)}, value{value}, fmt{fmt} {}

    std::string name;

    const T& value;

    fmt::format_string<T> fmt;
};

// NOLINTNEXTLINE(google-build-namespaces)
namespace {

struct Table {
    template <typename... Args>
    explicit Table(Args... args) {
        entries = std::vector<std::pair<std::string, std::string>>{
            {args.name,
             fmt::vformat(args.fmt, fmt::make_format_args(args.value))}...};

        column_width = std::ranges::max(
            entries | std::ranges::views::transform([](const auto& entry) {
                return std::max(entry.first.size(), entry.second.size());
            }));

        // For a space on either size
        column_width += 2;
    }

    auto print_separator() const -> void {
        for (size_t i = 0; i < entries.size(); ++i) {
            fmt::print("+{:->{}}", "", column_width);
        }

        fmt::print("+\n");
    }

    template <size_t i>
    auto print_content() const -> void {
        for (const auto& entry : entries) {
            fmt::print("| {:>{}} ", std::get<i>(entry), column_width - 2);
        }

        fmt::print("|\n");
    }

    auto print() const -> void {
        print_separator();
        print_content<0>();
        print_separator();
        print_content<1>();
        print_separator();
    }

    size_t column_width;

    std::vector<std::pair<std::string, std::string>> entries;
};

}  // namespace

template <typename... Args>
auto print_table(Args... args) -> void {
    const auto table = Table(args...);
    table.print();
}
