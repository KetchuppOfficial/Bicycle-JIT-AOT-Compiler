#include <format>

#include <gtest/gtest.h>

#include "bjac/analysis/lifetime.hpp"

using Segment = bjac::Lifetime::Segment;

TEST(Lifetime, Empty) {
    bjac::Lifetime lt;

    EXPECT_EQ("", std::format("{}", lt));
}

TEST(Lifetime, OneSegment) {
    bjac::Lifetime lt;

    lt.add(bjac::Lifetime::Segment{1, 5});

    EXPECT_TRUE(lt.contains(Segment{1, 5}));
    EXPECT_EQ("[1; 5]", std::format("{}", lt));
}

TEST(Lifetime, TwoNonOverlappingSegments) {
    // Assign
    bjac::Lifetime lt{};

    // Act
    lt.add(Segment{1, 5});
    lt.add(Segment{7, 9});

    // Assert
    EXPECT_TRUE(lt.contains(Segment{1, 5}));
    EXPECT_TRUE(lt.contains(Segment{7, 9}));
    EXPECT_EQ("[1; 5] U [7; 9]", std::format("{}", lt));
}

TEST(Lifetime, TwoOverlappingSegments) {
    // Assign
    bjac::Lifetime lt{Segment{1, 5}};

    // Act
    lt.add(Segment{4, 9});

    // Assert
    EXPECT_TRUE(lt.contains(Segment{1, 9}));
    EXPECT_EQ("[1; 9]", std::format("{}", lt));
}

TEST(Lifetime, TwoSegmentsOverlappingByASinglePoint) {
    // Assign
    bjac::Lifetime lt;

    // Act
    lt.add(Segment{1, 5});
    lt.add(Segment{5, 10});

    // Assert
    EXPECT_TRUE(lt.contains(Segment{1, 10}));
    EXPECT_EQ("[1; 10]", std::format("{}", lt));
}

TEST(Lifetime, ThreeOverlappingSegments) {
    // Assign
    bjac::Lifetime lt{Segment{1, 5}, Segment{6, 10}};

    // Act
    lt.add(Segment{4, 8});

    // Assert
    EXPECT_TRUE(lt.contains(Segment{1, 10}));
    EXPECT_EQ("[1; 10]", std::format("{}", lt));
}
