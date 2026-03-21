#include <algorithm>
#include <array>

#include <gtest/gtest.h>

#include "bjac/utils/ilist.hpp"

class S final : public bjac::ilist_node<S> {
  public:
    explicit S(int val) : val_{val} {}

    int value() const noexcept { return val_; }

    friend bool operator==(const S &lhs, const S &rhs) noexcept { return lhs.val_ == rhs.val_; }
    friend bool operator==(const S &lhs, int rhs) noexcept { return lhs.val_ == rhs; }
    friend bool operator==(int lhs, const S &rhs) noexcept { return rhs.val_ == lhs; }

  private:
    int val_;
};

TEST(IList, EmptyList) {
    // Act
    const bjac::ilist<S> list;

    // Assert
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0);
}

TEST(IList, EmplaceBack) {
    // Assign
    constexpr std::array<int, 5> kValues{1, 2, 3, 4, 5};
    bjac::ilist<S> list;

    // Act
    for (auto val : kValues) {
        list.emplace_back<S>(val);
    }

    // Assert
    EXPECT_FALSE(list.empty());
    EXPECT_EQ(list.size(), kValues.size());
    EXPECT_TRUE(std::ranges::equal(list, kValues, std::equal_to<>{}));
}

TEST(IList, SpliceWholeList) {
    // Assign
    constexpr std::array kValues1{1, 2, 3, 4, 5};
    bjac::ilist<S> list1;
    for (auto val : kValues1) {
        list1.emplace_back<S>(val);
    }

    constexpr std::array kValues2{42, 43, 44, 45};
    bjac::ilist<S> list2;
    for (auto val : kValues2) {
        list2.emplace_back<S>(val);
    }

    auto pos = std::next(list1.begin(), 2); // references 3

    // Act
    list1.splice(pos, list2);

    // Assert
    EXPECT_FALSE(list1.empty());
    EXPECT_EQ(list1.size(), kValues1.size() + kValues2.size());
    EXPECT_TRUE(
        std::ranges::equal(list1, std::array{1, 2, 42, 43, 44, 45, 3, 4, 5}, std::equal_to<>{}));

    EXPECT_TRUE(list2.empty());
    EXPECT_EQ(list2.size(), 0);
}

TEST(IList, SpliceNode) {
    // Assign
    constexpr std::array kValues1{1, 2, 3, 4, 5};
    bjac::ilist<S> list1;
    for (auto val : kValues1) {
        list1.emplace_back<S>(val);
    }

    constexpr std::array kValues2{42, 43, 44, 45};
    bjac::ilist<S> list2;
    for (auto val : kValues2) {
        list2.emplace_back<S>(val);
    }

    auto pos = std::next(list1.begin(), 2); // references 3
    auto it = std::next(list2.begin(), 1);  // references 43

    // Act
    list1.splice(pos, list2, it);

    // Assert
    EXPECT_FALSE(list1.empty());
    EXPECT_EQ(list1.size(), kValues1.size() + 1);
    EXPECT_TRUE(std::ranges::equal(list1, std::array{1, 2, 43, 3, 4, 5}, std::equal_to<>{}));

    EXPECT_FALSE(list2.empty());
    EXPECT_EQ(list2.size(), kValues2.size() - 1);
    EXPECT_TRUE(std::ranges::equal(list2, std::array{42, 44, 45}, std::equal_to<>{}));
}

TEST(IList, SpliceNodeToSelf_NoEffect_SplicedNodeAtPos) {
    // Assign
    constexpr std::array kValues{1, 2, 3, 4, 5};
    bjac::ilist<S> list;
    for (auto val : kValues) {
        list.emplace_back<S>(val);
    }

    auto pos = std::next(list.begin(), 2); // references 3
    auto it = pos;

    // Act
    list.splice(pos, list, it);

    // Assert
    EXPECT_FALSE(list.empty());
    EXPECT_EQ(list.size(), kValues.size());
    EXPECT_TRUE(std::ranges::equal(list, kValues, std::equal_to<>{}));
}

TEST(IList, SpliceNodeToSelf_NoEffect_SplicedNodeBeforePos) {
    // Assign
    constexpr std::array kValues{1, 2, 3, 4, 5};
    bjac::ilist<S> list;
    for (auto val : kValues) {
        list.emplace_back<S>(val);
    }

    auto pos = std::next(list.begin(), 2); // references 3
    auto it = std::prev(pos);

    // Act
    list.splice(pos, list, it);

    // Assert
    EXPECT_FALSE(list.empty());
    EXPECT_EQ(list.size(), kValues.size());
    EXPECT_TRUE(std::ranges::equal(list, kValues, std::equal_to<>{}));
}

TEST(IList, SpliceEmptyRange) {
    // Assign
    constexpr std::array kValues1{1, 2, 3, 4, 5};
    bjac::ilist<S> list1;
    for (auto val : kValues1) {
        list1.emplace_back<S>(val);
    }

    constexpr std::array kValues2{42, 43, 44, 45};
    bjac::ilist<S> list2;
    for (auto val : kValues2) {
        list2.emplace_back<S>(val);
    }

    auto pos = std::next(list1.begin(), 2); // references 3

    // Act
    list1.splice(pos, list2, list2.begin(), list2.begin());

    // Assert
    EXPECT_FALSE(list1.empty());
    EXPECT_EQ(list1.size(), kValues1.size());
    EXPECT_TRUE(std::ranges::equal(list1, kValues1, std::equal_to<>{}));

    EXPECT_FALSE(list2.empty());
    EXPECT_EQ(list2.size(), kValues2.size());
    EXPECT_TRUE(std::ranges::equal(list2, kValues2, std::equal_to<>{}));
}

TEST(IList, SpliceRange) {
    // Assign
    constexpr std::array kValues1{1, 2, 3, 4, 5};
    bjac::ilist<S> list1;
    for (auto val : kValues1) {
        list1.emplace_back<S>(val);
    }

    constexpr std::array kValues2{42, 43, 44, 45};
    bjac::ilist<S> list2;
    for (auto val : kValues2) {
        list2.emplace_back<S>(val);
    }

    auto pos = std::next(list1.begin(), 2);   // references 3
    auto first = std::next(list2.begin(), 1); // references 43
    auto last = std::next(list2.begin(), 3);  // references 45
    const auto count = std::distance(first, last);

    // Act
    list1.splice(pos, list2, first, last);

    // Assert
    EXPECT_FALSE(list1.empty());
    EXPECT_EQ(list1.size(), kValues1.size() + count);
    EXPECT_TRUE(std::ranges::equal(list1, std::array{1, 2, 43, 44, 3, 4, 5}, std::equal_to<>{}));

    EXPECT_FALSE(list2.empty());
    EXPECT_EQ(list2.size(), kValues2.size() - count);
    EXPECT_TRUE(std::ranges::equal(list2, std::array{42, 45}, std::equal_to<>{}));
}
