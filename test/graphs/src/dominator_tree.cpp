#include <array>
#include <optional>

#include <gtest/gtest.h>

#include "bjac/graphs/dominator_tree.hpp"

#include "bjac/IR/branch_instruction.hpp"
#include "bjac/IR/constant_instruction.hpp"
#include "bjac/IR/function.hpp"

#include "test/common.hpp"

TEST(DominatorTree, Mandatory_1) {
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
    const bjac::DominatorTree<bjac::ConstFunctionGraphTraits> dom_tree{foo};

    // Assert
    EXPECT_EQ(dom_tree.idom(bb.at('A')), std::nullopt);
    EXPECT_TRUE(matches(dom_tree.successors(bb.at('A')), {std::array{bb.at('B')}}, names));

    EXPECT_EQ(dom_tree.idom(bb.at('B')), bb.at('A'));
    EXPECT_TRUE(matches(dom_tree.successors(bb.at('B')),
                        {std::array{bb.at('C'), bb.at('F'), bb.at('D')},
                         std::array{bb.at('F'), bb.at('C'), bb.at('D')}},
                        names));

    EXPECT_EQ(dom_tree.idom(bb.at('C')), bb.at('B'));
    EXPECT_TRUE(empty(dom_tree.successors(bb.at('C')), names));

    EXPECT_EQ(dom_tree.idom(bb.at('D')), bb.at('B'));
    EXPECT_TRUE(empty(dom_tree.successors(bb.at('D')), names));

    EXPECT_EQ(dom_tree.idom(bb.at('E')), bb.at('F'));
    EXPECT_TRUE(empty(dom_tree.successors(bb.at('E')), names));

    EXPECT_EQ(dom_tree.idom(bb.at('F')), bb.at('B'));
    EXPECT_TRUE(matches(dom_tree.successors(bb.at('F')),
                        {std::array{bb.at('E'), bb.at('G')}, std::array{bb.at('G'), bb.at('E')}},
                        names));

    EXPECT_EQ(dom_tree.idom(bb.at('G')), bb.at('F'));
    EXPECT_TRUE(empty(dom_tree.successors(bb.at('G')), names));
}

TEST(DominatorTree, Mandatory_2) {
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
    const bjac::DominatorTree<bjac::ConstFunctionGraphTraits> dom_tree{foo};

    // Assert
    EXPECT_EQ(dom_tree.idom(bb.at('A')), std::nullopt);
    EXPECT_TRUE(matches(dom_tree.successors(bb.at('A')), {std::array{bb.at('B')}}, names));

    EXPECT_EQ(dom_tree.idom(bb.at('B')), bb.at('A'));
    EXPECT_TRUE(matches(dom_tree.successors(bb.at('B')),
                        {std::array{bb.at('C'), bb.at('J')}, std::array{bb.at('J'), bb.at('C')}},
                        names));

    EXPECT_EQ(dom_tree.idom(bb.at('C')), bb.at('B'));
    EXPECT_TRUE(matches(dom_tree.successors(bb.at('C')), {std::array{bb.at('D')}}, names));

    EXPECT_EQ(dom_tree.idom(bb.at('D')), bb.at('C'));
    EXPECT_TRUE(matches(dom_tree.successors(bb.at('D')), {std::array{bb.at('E')}}, names));

    EXPECT_EQ(dom_tree.idom(bb.at('E')), bb.at('D'));
    EXPECT_TRUE(matches(dom_tree.successors(bb.at('E')), {std::array{bb.at('F')}}, names));

    EXPECT_EQ(dom_tree.idom(bb.at('F')), bb.at('E'));
    EXPECT_TRUE(matches(dom_tree.successors(bb.at('F')), {std::array{bb.at('G')}}, names));

    EXPECT_EQ(dom_tree.idom(bb.at('G')), bb.at('F'));
    EXPECT_TRUE(matches(dom_tree.successors(bb.at('G')),
                        {std::array{bb.at('I'), bb.at('H')}, std::array{bb.at('H'), bb.at('I')}},
                        names));

    EXPECT_EQ(dom_tree.idom(bb.at('H')), bb.at('G'));
    EXPECT_TRUE(empty(dom_tree.successors(bb.at('H')), names));

    EXPECT_EQ(dom_tree.idom(bb.at('I')), bb.at('G'));
    EXPECT_TRUE(matches(dom_tree.successors(bb.at('I')), {std::array{bb.at('K')}}, names));

    EXPECT_EQ(dom_tree.idom(bb.at('J')), bb.at('B'));
    EXPECT_TRUE(empty(dom_tree.successors(bb.at('J')), names));

    EXPECT_EQ(dom_tree.idom(bb.at('K')), bb.at('I'));
    EXPECT_TRUE(empty(dom_tree.successors(bb.at('K')), names));
}

TEST(DominatorTree, Mandatory_3) {
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
    const bjac::DominatorTree<bjac::ConstFunctionGraphTraits> dom_tree{foo};

    // Assert
    EXPECT_EQ(dom_tree.idom(bb.at('A')), std::nullopt);
    EXPECT_TRUE(matches(dom_tree.successors(bb.at('A')), {std::array{bb.at('B')}}, names));

    EXPECT_EQ(dom_tree.idom(bb.at('B')), bb.at('A'));
    EXPECT_TRUE(matches(dom_tree.successors(bb.at('B')),
                        {std::array{bb.at('C'), bb.at('D'), bb.at('G'), bb.at('I'), bb.at('E')},
                         std::array{bb.at('E'), bb.at('D'), bb.at('G'), bb.at('I'), bb.at('C')},
                         std::array{bb.at('E'), bb.at('D'), bb.at('G'), bb.at('C'), bb.at('I')},
                         std::array{bb.at('E'), bb.at('I'), bb.at('G'), bb.at('C'), bb.at('D')},
                         std::array{bb.at('E'), bb.at('G'), bb.at('I'), bb.at('C'), bb.at('D')}},
                        names));

    EXPECT_EQ(dom_tree.idom(bb.at('C')), bb.at('B'));
    EXPECT_TRUE(empty(dom_tree.successors(bb.at('C')), names));

    EXPECT_EQ(dom_tree.idom(bb.at('D')), bb.at('B'));
    EXPECT_TRUE(empty(dom_tree.successors(bb.at('D')), names));

    EXPECT_EQ(dom_tree.idom(bb.at('E')), bb.at('B'));
    EXPECT_TRUE(matches(dom_tree.successors(bb.at('E')), {std::array{bb.at('F')}}, names));

    EXPECT_EQ(dom_tree.idom(bb.at('F')), bb.at('E'));
    EXPECT_TRUE(matches(dom_tree.successors(bb.at('F')), {std::array{bb.at('H')}}, names));

    EXPECT_EQ(dom_tree.idom(bb.at('G')), bb.at('B'));
    EXPECT_TRUE(empty(dom_tree.successors(bb.at('G')), names));

    EXPECT_EQ(dom_tree.idom(bb.at('H')), bb.at('F'));

    EXPECT_EQ(dom_tree.idom(bb.at('I')), bb.at('B'));
    EXPECT_TRUE(empty(dom_tree.successors(bb.at('I')), names));
}
