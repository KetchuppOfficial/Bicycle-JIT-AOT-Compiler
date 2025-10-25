#include <unordered_map>

#include <gtest/gtest.h>

#include "bjac/graphs/dominator_tree.hpp"

#include "bjac/IR/branch_instruction.hpp"
#include "bjac/IR/constant_instruction.hpp"
#include "bjac/IR/function.hpp"

TEST(DominatorTree, Mandatory_1) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kVoid, {}};

    std::unordered_map<char, bjac::BasicBlock *> bb;
    for (char name : {'A', 'B', 'C', 'D', 'E', 'F', 'G'}) {
        bb.emplace(name, &foo.emplace_back());
    }

    auto &cond = bb.at('A')->emplace_back<bjac::ConstInstruction>(bjac::Type::kI1, 0);

    bb.at('A')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));
    bb.at('B')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('C'), *bb.at('F'));
    bb.at('C')->emplace_back<bjac::BranchInstruction>(*bb.at('D'));
    bb.at('E')->emplace_back<bjac::BranchInstruction>(*bb.at('D'));
    bb.at('F')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('E'), *bb.at('G'));
    bb.at('G')->emplace_back<bjac::BranchInstruction>(*bb.at('D'));

    // Act
    bjac::DominatorTree<bjac::Function, bjac::FunctionGraphTraits> dom_tree{foo};

    // Assert
    EXPECT_FALSE(dom_tree.contains(bb.at('A')));

    EXPECT_EQ(dom_tree.idom(bb.at('B')), bb.at('A'));
    EXPECT_EQ(dom_tree.idom(bb.at('C')), bb.at('B'));
    EXPECT_EQ(dom_tree.idom(bb.at('D')), bb.at('B'));
    EXPECT_EQ(dom_tree.idom(bb.at('E')), bb.at('F'));
    EXPECT_EQ(dom_tree.idom(bb.at('F')), bb.at('B'));
    EXPECT_EQ(dom_tree.idom(bb.at('G')), bb.at('F'));
}

TEST(DominatorTree, Mandatory_2) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kVoid, {}};

    std::unordered_map<char, bjac::BasicBlock *> bb;
    for (char name : {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K'}) {
        bb.emplace(name, &foo.emplace_back());
    }

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
    bjac::DominatorTree<bjac::Function, bjac::FunctionGraphTraits> dom_tree{foo};

    // Assert
    EXPECT_FALSE(dom_tree.contains(bb.at('A')));

    EXPECT_EQ(dom_tree.idom(bb.at('B')), bb.at('A'));
    EXPECT_EQ(dom_tree.idom(bb.at('C')), bb.at('B'));
    EXPECT_EQ(dom_tree.idom(bb.at('D')), bb.at('C'));
    EXPECT_EQ(dom_tree.idom(bb.at('E')), bb.at('D'));
    EXPECT_EQ(dom_tree.idom(bb.at('F')), bb.at('E'));
    EXPECT_EQ(dom_tree.idom(bb.at('G')), bb.at('F'));
    EXPECT_EQ(dom_tree.idom(bb.at('H')), bb.at('G'));
    EXPECT_EQ(dom_tree.idom(bb.at('I')), bb.at('G'));
    EXPECT_EQ(dom_tree.idom(bb.at('J')), bb.at('B'));
    EXPECT_EQ(dom_tree.idom(bb.at('K')), bb.at('I'));
}

TEST(DominatorTree, Mandatory_3) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kVoid, {}};

    std::unordered_map<char, bjac::BasicBlock *> bb;
    for (char name : {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I'}) {
        bb.emplace(name, &foo.emplace_back());
    }

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
    bjac::DominatorTree<bjac::Function, bjac::FunctionGraphTraits> dom_tree{foo};

    // Assert
    EXPECT_FALSE(dom_tree.contains(bb.at('A')));

    EXPECT_EQ(dom_tree.idom(bb.at('B')), bb.at('A'));
    EXPECT_EQ(dom_tree.idom(bb.at('C')), bb.at('B'));
    EXPECT_EQ(dom_tree.idom(bb.at('D')), bb.at('B'));
    EXPECT_EQ(dom_tree.idom(bb.at('E')), bb.at('B'));
    EXPECT_EQ(dom_tree.idom(bb.at('F')), bb.at('E'));
    EXPECT_EQ(dom_tree.idom(bb.at('G')), bb.at('B'));
    EXPECT_EQ(dom_tree.idom(bb.at('H')), bb.at('F'));
    EXPECT_EQ(dom_tree.idom(bb.at('I')), bb.at('B'));
}
