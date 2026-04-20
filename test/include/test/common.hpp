#ifndef TEST_INCLUDE_TEST_COMMON_HPP
#define TEST_INCLUDE_TEST_COMMON_HPP

#include <algorithm>
#include <format>
#include <initializer_list>
#include <ranges>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>

#include <gtest/gtest.h>

#include "bjac/IR/function.hpp"

inline auto get_i1() { return std::make_unique<bjac::IntegralType>(bjac::Type::ID::kI1); }
inline auto get_i64() { return std::make_unique<bjac::IntegralType>(bjac::Type::ID::kI64); }
inline auto get_ptr(bjac::Type::ID id) { return std::make_unique<bjac::PointerType>(id); }

inline auto get_func(std::string_view name, bjac::Type::ID ret,
                     std::initializer_list<bjac::Type::ID> params = {}) {
    std::vector<std::unique_ptr<bjac::Type>> parameters;
    for (auto id : params) {
        parameters.emplace_back(std::make_unique<bjac::IntegralType>(id));
    }

    if (ret == bjac::Type::ID::kVoid) {
        return bjac::Function{name, std::make_unique<bjac::VoidType>(), std::move(parameters)};
    } else {
        return bjac::Function{name, std::make_unique<bjac::IntegralType>(ret),
                              std::move(parameters)};
    }
}

inline auto setup(bjac::Function &foo, std::initializer_list<char> ilist) {
    std::unordered_map<char, bjac::BasicBlock *> bb;
    bb.reserve(foo.size());

    std::unordered_map<const bjac::BasicBlock *, char> names;
    names.reserve(foo.size());

    for (char c : ilist) {
        auto &BB = foo.emplace_back();
        bb.emplace(c, &BB);
        names.emplace(&BB, c);
    }

    return std::tuple{std::move(bb), std::move(names)};
}

inline std::string actual_names(const auto &names, std::ranges::forward_range auto vertices) {
    return std::format("{}",
                       vertices | std::views::transform([&names](auto v) { return names.at(v); }));
}

template <std::ranges::forward_range R1, std::ranges::forward_range R2>
::testing::AssertionResult matches(R1 &&range, std::initializer_list<R2> variants, auto &names) {
    const bool match = std::ranges::any_of(
        variants, [&range](const auto &variant) { return std::ranges::equal(range, variant); });

    if (match) {
        return ::testing::AssertionSuccess();
    }

    return ::testing::AssertionFailure() << actual_names(names, range);
}

template <std::ranges::forward_range R1>
::testing::AssertionResult empty(R1 &&range, auto &names) {
    if (range.empty()) {
        return ::testing::AssertionSuccess();
    }

    return ::testing::AssertionFailure() << actual_names(names, range);
}

inline std::string to_string(const bjac::Function &func) {
    std::ostringstream oss;
    oss << func;
    return oss.str();
}

#endif // TEST_INCLUDE_TEST_COMMON_HPP
