#include <array>
#include <ranges>

#include <gtest/gtest.h>

#include "bjac/graphs/loop_tree.hpp"

#include "bjac/IR/branch_instruction.hpp"
#include "bjac/IR/constant_instruction.hpp"
#include "bjac/IR/function.hpp"

#include "test/common.hpp"

TEST(LoopTree, Mandatory_1) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kVoid, {}};
    auto [bb, names] = setup(foo, {'A', 'B', 'C', 'D', 'E'});

    auto &cond = bb.at('A')->emplace_back<bjac::ConstInstruction>(bjac::Type::kI1, 0);

    bb.at('A')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));
    bb.at('B')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('C'), *bb.at('D'));
    bb.at('D')->emplace_back<bjac::BranchInstruction>(*bb.at('E'));
    bb.at('E')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));

    // Act
    const bjac::LoopTree<bjac::ConstFunctionGraphTraits> loop_tree{foo};

    // Assert
    EXPECT_EQ(loop_tree.loops_count(), 1);

    const auto &loop = loop_tree.get_loop(bb.at('B'));
    EXPECT_EQ(loop.get_header(), bb.at('B'));
    EXPECT_EQ(loop.vertices_count(), 3);
    EXPECT_EQ(loop.inner_loops_count(), 0);
    EXPECT_EQ(loop.get_parent_loop(), nullptr);
    EXPECT_TRUE(matches(loop.vertices(), {std::array{bb.at('B'), bb.at('D'), bb.at('E')}}, names));
}

TEST(LoopTree, Mandatory_2) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kVoid, {}};
    auto [bb, names] = setup(foo, {'A', 'B', 'C', 'D', 'E', 'F'});

    auto &cond = bb.at('A')->emplace_back<bjac::ConstInstruction>(bjac::Type::kI1, 0);

    bb.at('A')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));
    bb.at('B')->emplace_back<bjac::BranchInstruction>(*bb.at('C'));
    bb.at('C')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('F'), *bb.at('D'));
    bb.at('D')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('F'), *bb.at('E'));
    bb.at('E')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));

    // Act
    const bjac::LoopTree<bjac::ConstFunctionGraphTraits> loop_tree{foo};

    // Assert
    EXPECT_EQ(loop_tree.loops_count(), 1);

    const auto &loop = loop_tree.get_loop(bb.at('B'));
    EXPECT_EQ(loop.get_header(), bb.at('B'));
    EXPECT_EQ(loop.vertices_count(), 4);
    EXPECT_EQ(loop.inner_loops_count(), 0);
    EXPECT_EQ(loop.get_parent_loop(), nullptr);
    EXPECT_TRUE(matches(loop.vertices(),
                        {std::array{bb.at('B'), bb.at('C'), bb.at('D'), bb.at('E')}}, names));
}

TEST(LoopTree, Mandatory_3) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kVoid, {}};
    auto [bb, names] = setup(foo, {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'});

    auto &cond = bb.at('A')->emplace_back<bjac::ConstInstruction>(bjac::Type::kI1, 0);

    bb.at('A')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));
    bb.at('B')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('C'), *bb.at('D'));
    bb.at('C')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('E'), *bb.at('F'));
    bb.at('D')->emplace_back<bjac::BranchInstruction>(*bb.at('F'));
    bb.at('F')->emplace_back<bjac::BranchInstruction>(*bb.at('G'));
    bb.at('G')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('H'), *bb.at('B'));
    bb.at('H')->emplace_back<bjac::BranchInstruction>(*bb.at('A'));

    // Act
    const bjac::LoopTree<bjac::ConstFunctionGraphTraits> loop_tree{foo};

    // Assert
    EXPECT_EQ(loop_tree.loops_count(), 1);

    const auto &a_loop = loop_tree.get_loop(bb.at('A'));
    EXPECT_EQ(a_loop.get_header(), bb.at('A'));
    EXPECT_EQ(a_loop.vertices_count(), 7);
    EXPECT_EQ(a_loop.inner_loops_count(), 1);
    EXPECT_EQ(a_loop.get_parent_loop(), nullptr);
    EXPECT_TRUE(matches(a_loop.vertices(),
                        {std::array{bb.at('A'), bb.at('B'), bb.at('C'), bb.at('D'), bb.at('F'),
                                    bb.at('G'), bb.at('H')},
                         std::array{bb.at('A'), bb.at('B'), bb.at('D'), bb.at('C'), bb.at('F'),
                                    bb.at('G'), bb.at('H')}},
                        names));

    const auto &b_loop = a_loop.get_inner_loop(bb.at('B'));
    EXPECT_EQ(b_loop.get_header(), bb.at('B'));
    EXPECT_EQ(b_loop.vertices_count(), 5);
    EXPECT_EQ(b_loop.inner_loops_count(), 0);
    EXPECT_EQ(b_loop.get_parent_loop(), &a_loop);
    EXPECT_TRUE(matches(b_loop.vertices(),
                        {std::array{bb.at('B'), bb.at('C'), bb.at('D'), bb.at('F'), bb.at('G')},
                         std::array{bb.at('B'), bb.at('D'), bb.at('C'), bb.at('F'), bb.at('G')}},
                        names));
}

TEST(LoopTree, Mandatory_4) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kVoid, {}};
    auto [bb, names] = setup(foo, {'A', 'B', 'C', 'D', 'E', 'F', 'G'});

    auto &cond = bb.at('A')->emplace_back<bjac::ConstInstruction>(bjac::Type::kI1, 0);

    bb.at('A')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));
    bb.at('B')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('C'), *bb.at('F'));
    bb.at('C')->emplace_back<bjac::BranchInstruction>(*bb.at('D'));
    bb.at('E')->emplace_back<bjac::BranchInstruction>(*bb.at('D'));
    bb.at('F')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('E'), *bb.at('G'));
    bb.at('G')->emplace_back<bjac::BranchInstruction>(*bb.at('D'));

    // Act
    const bjac::LoopTree<bjac::ConstFunctionGraphTraits> loop_tree{foo};

    // Assert
    EXPECT_EQ(loop_tree.loops_count(), 0);
}

TEST(LoopTree, Mandatory_5) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kVoid, {}};
    auto [bb, names] = setup(foo, {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K'});

    auto &cond = bb.at('A')->emplace_back<bjac::ConstInstruction>(bjac::Type::kI1, 0);

    bb.at('A')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));
    bb.at('B')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('C'), *bb.at('J'));
    bb.at('C')->emplace_back<bjac::BranchInstruction>(*bb.at('D'));
    bb.at('D')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('C'), *bb.at('E'));
    bb.at('E')->emplace_back<bjac::BranchInstruction>(*bb.at('F'));
    bb.at('F')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('E'), *bb.at('G'));
    bb.at('G')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('H'), *bb.at('I'));
    bb.at('H')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));
    bb.at('I')->emplace_back<bjac::BranchInstruction>(*bb.at('K'));
    bb.at('J')->emplace_back<bjac::BranchInstruction>(*bb.at('C'));

    // Act
    const bjac::LoopTree<bjac::ConstFunctionGraphTraits> loop_tree{foo};

    // Assert
    EXPECT_EQ(loop_tree.loops_count(), 1);

    const auto &b_loop = loop_tree.get_loop(bb.at('B'));
    EXPECT_EQ(b_loop.get_header(), bb.at('B'));
    EXPECT_EQ(b_loop.vertices_count(), 8);
    EXPECT_EQ(b_loop.inner_loops_count(), 2);
    EXPECT_EQ(b_loop.get_parent_loop(), nullptr);
    EXPECT_TRUE(matches(b_loop.vertices(),
                        {std::array{bb.at('B'), bb.at('C'), bb.at('J'), bb.at('D'), bb.at('E'),
                                    bb.at('F'), bb.at('G'), bb.at('H')},
                         std::array{bb.at('B'), bb.at('J'), bb.at('C'), bb.at('D'), bb.at('E'),
                                    bb.at('F'), bb.at('G'), bb.at('H')}},
                        names));

    const auto &c_loop = b_loop.get_inner_loop(bb.at('C'));
    EXPECT_EQ(c_loop.get_header(), bb.at('C'));
    EXPECT_EQ(c_loop.vertices_count(), 2);
    EXPECT_EQ(c_loop.inner_loops_count(), 0);
    EXPECT_EQ(c_loop.get_parent_loop(), &b_loop);
    EXPECT_TRUE(matches(c_loop.vertices(), {std::array{bb.at('C'), bb.at('D')}}, names));

    const auto &e_loop = b_loop.get_inner_loop(bb.at('E'));
    EXPECT_EQ(e_loop.get_header(), bb.at('E'));
    EXPECT_EQ(e_loop.vertices_count(), 2);
    EXPECT_EQ(e_loop.inner_loops_count(), 0);
    EXPECT_EQ(e_loop.get_parent_loop(), &b_loop);
    EXPECT_TRUE(matches(e_loop.vertices(), {std::array{bb.at('E'), bb.at('F')}}, names));
}

TEST(LoopTree, Mandatory_6) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kVoid, {}};
    auto [bb, names] = setup(foo, {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I'});

    auto &cond = bb.at('A')->emplace_back<bjac::ConstInstruction>(bjac::Type::kI1, 0);

    bb.at('A')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));
    bb.at('B')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('E'), *bb.at('C'));
    bb.at('C')->emplace_back<bjac::BranchInstruction>(*bb.at('D'));
    bb.at('D')->emplace_back<bjac::BranchInstruction>(*bb.at('G'));
    bb.at('E')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('F'), *bb.at('D'));
    bb.at('F')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('B'), *bb.at('H'));
    bb.at('G')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('C'), *bb.at('I'));
    bb.at('H')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('G'), *bb.at('I'));

    // Act
    const bjac::LoopTree<bjac::ConstFunctionGraphTraits> loop_tree{foo};

    // Assert
    EXPECT_EQ(loop_tree.loops_count(), 1);

    const auto &loop = loop_tree.get_loop(bb.at('B'));
    EXPECT_EQ(loop.get_header(), bb.at('B'));
    EXPECT_EQ(loop.vertices_count(), 3);
    EXPECT_EQ(loop.inner_loops_count(), 0);
    EXPECT_EQ(loop.get_parent_loop(), nullptr);
    EXPECT_TRUE(matches(loop.vertices(), {std::array{bb.at('B'), bb.at('E'), bb.at('F')}}, names));
}

TEST(LoopTree, SingleBlockLoop) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kVoid, {}};
    auto [bb, names] = setup(foo, {'A', 'B'});

    auto &cond = bb.at('A')->emplace_back<bjac::ConstInstruction>(bjac::Type::kI1, 0);

    bb.at('A')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('A'), *bb.at('B'));

    // Act
    const bjac::LoopTree<bjac::ConstFunctionGraphTraits> loop_tree{foo};

    // Assert
    EXPECT_EQ(loop_tree.loops_count(), 1);

    const auto &loop = loop_tree.get_loop(bb.at('A'));
    EXPECT_EQ(loop.get_header(), bb.at('A'));
    EXPECT_EQ(loop.vertices_count(), 1);
    EXPECT_EQ(loop.inner_loops_count(), 0);
    EXPECT_EQ(loop.get_parent_loop(), nullptr);
    EXPECT_TRUE(matches(loop.vertices(), {std::array{bb.at('A')}}, names));
}
