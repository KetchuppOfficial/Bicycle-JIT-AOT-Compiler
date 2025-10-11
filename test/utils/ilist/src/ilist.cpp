#include <gtest/gtest.h>

#include "bjac/utils/ilist.hpp"

TEST(IList, EmptyList) {
    struct S : public bjac::ilist_node<S> {};

    bjac::ilist<S> list;

    EXPECT_TRUE(list.empty());
}
