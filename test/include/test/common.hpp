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

#endif // TEST_INCLUDE_TEST_COMMON_HPP
