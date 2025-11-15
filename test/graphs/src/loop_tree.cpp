#include <unordered_map>

#include <gtest/gtest.h>

#include "bjac/graphs/loop_tree.hpp"

#include "bjac/IR/branch_instruction.hpp"
#include "bjac/IR/constant_instruction.hpp"
#include "bjac/IR/function.hpp"

TEST(LoopTree, Mandatory_1) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kVoid, {}};

    std::unordered_map<char, bjac::BasicBlock *> bb;
    for (char name : {'A', 'B', 'C', 'D', 'E'}) {
        auto &BB = foo.emplace_back();
        bb.emplace(name, &BB);
    }

    auto &cond = bb.at('A')->emplace_back<bjac::ConstInstruction>(bjac::Type::kI1, 0);

    bb.at('A')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));
    bb.at('B')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('C'), *bb.at('D'));
    bb.at('D')->emplace_back<bjac::BranchInstruction>(*bb.at('E'));
    bb.at('E')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));

    // Act
    bjac::LoopTree<bjac::Function, bjac::FunctionGraphTraits> loop_tree{foo};

    // Assert
    EXPECT_EQ(loop_tree.loops_count(), 1);

    const auto &loop = loop_tree.get_loop(bb.at('B'));
    EXPECT_EQ(loop.get_header(), bb.at('B'));
    EXPECT_EQ(loop.vertices_count(), 3);
    EXPECT_EQ(loop.inner_loops_count(), 0);
    EXPECT_EQ(loop.get_parent_loop(), nullptr);
    for (auto c : {'B', 'D', 'E'}) {
        EXPECT_TRUE(loop.contains_vertex(bb.at(c))) << "BB_" << c;
    }
}

TEST(LoopTree, Mandatory_2) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kVoid, {}};

    std::unordered_map<char, bjac::BasicBlock *> bb;
    for (char name : {'A', 'B', 'C', 'D', 'E', 'F'}) {
        auto &BB = foo.emplace_back();
        bb.emplace(name, &BB);
    }

    auto &cond = bb.at('A')->emplace_back<bjac::ConstInstruction>(bjac::Type::kI1, 0);

    bb.at('A')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));
    bb.at('B')->emplace_back<bjac::BranchInstruction>(*bb.at('C'));
    bb.at('C')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('F'), *bb.at('D'));
    bb.at('D')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('F'), *bb.at('E'));
    bb.at('E')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));

    // Act
    bjac::LoopTree<bjac::Function, bjac::FunctionGraphTraits> loop_tree{foo};

    // Assert
    EXPECT_EQ(loop_tree.loops_count(), 1);

    const auto &loop = loop_tree.get_loop(bb.at('B'));
    EXPECT_EQ(loop.get_header(), bb.at('B'));
    EXPECT_EQ(loop.vertices_count(), 4);
    EXPECT_EQ(loop.inner_loops_count(), 0);
    EXPECT_EQ(loop.get_parent_loop(), nullptr);
    for (auto c : {'B', 'C', 'D', 'E'}) {
        EXPECT_TRUE(loop.contains_vertex(bb.at(c))) << "BB_" << c;
    }
}

TEST(LoopTree, Mandatory_3) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kVoid, {}};

    std::unordered_map<char, bjac::BasicBlock *> bb;
    for (char name : {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'}) {
        auto &BB = foo.emplace_back();
        bb.emplace(name, &BB);
    }

    auto &cond = bb.at('A')->emplace_back<bjac::ConstInstruction>(bjac::Type::kI1, 0);

    bb.at('A')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));
    bb.at('B')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('C'), *bb.at('D'));
    bb.at('C')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('E'), *bb.at('F'));
    bb.at('D')->emplace_back<bjac::BranchInstruction>(*bb.at('F'));
    bb.at('F')->emplace_back<bjac::BranchInstruction>(*bb.at('G'));
    bb.at('G')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('H'), *bb.at('B'));
    bb.at('H')->emplace_back<bjac::BranchInstruction>(*bb.at('A'));

    // Act
    bjac::LoopTree<bjac::Function, bjac::FunctionGraphTraits> loop_tree{foo};

    // Assert
    EXPECT_EQ(loop_tree.loops_count(), 1);

    const auto &a_loop = loop_tree.get_loop(bb.at('A'));
    EXPECT_EQ(a_loop.get_header(), bb.at('A'));
    EXPECT_EQ(a_loop.vertices_count(), 7);
    EXPECT_EQ(a_loop.inner_loops_count(), 1);
    EXPECT_EQ(a_loop.get_parent_loop(), nullptr);
    for (auto c : {'A', 'B', 'C', 'D', 'F', 'G', 'H'}) {
        EXPECT_TRUE(a_loop.contains_vertex(bb.at(c))) << "BB_" << c;
    }

    const auto &b_loop = a_loop.get_inner_loop(bb.at('B'));
    EXPECT_EQ(b_loop.get_header(), bb.at('B'));
    EXPECT_EQ(b_loop.vertices_count(), 5);
    EXPECT_EQ(b_loop.inner_loops_count(), 0);
    EXPECT_EQ(b_loop.get_parent_loop(), &a_loop);
    for (auto c : {'B', 'C', 'D', 'F', 'G'}) {
        EXPECT_TRUE(b_loop.contains_vertex(bb.at(c))) << "BB_" << c;
    }
}

TEST(LoopTree, Mandatory_4) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kVoid, {}};

    std::unordered_map<char, bjac::BasicBlock *> bb;
    for (char name : {'A', 'B', 'C', 'D', 'E', 'F', 'G'}) {
        auto &BB = foo.emplace_back();
        bb.emplace(name, &BB);
    }

    auto &cond = bb.at('A')->emplace_back<bjac::ConstInstruction>(bjac::Type::kI1, 0);

    bb.at('A')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));
    bb.at('B')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('C'), *bb.at('F'));
    bb.at('C')->emplace_back<bjac::BranchInstruction>(*bb.at('D'));
    bb.at('E')->emplace_back<bjac::BranchInstruction>(*bb.at('D'));
    bb.at('F')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('E'), *bb.at('G'));
    bb.at('G')->emplace_back<bjac::BranchInstruction>(*bb.at('D'));

    // Act
    bjac::LoopTree<bjac::Function, bjac::FunctionGraphTraits> loop_tree{foo};

    // Assert
    EXPECT_EQ(loop_tree.loops_count(), 0);
}

TEST(LoopTree, Mandatory_5) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kVoid, {}};

    std::unordered_map<char, bjac::BasicBlock *> bb;
    for (char name : {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K'}) {
        auto &BB = foo.emplace_back();
        bb.emplace(name, &BB);
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
    bjac::LoopTree<bjac::Function, bjac::FunctionGraphTraits> loop_tree{foo};

    // Assert
    EXPECT_EQ(loop_tree.loops_count(), 1);

    const auto &b_loop = loop_tree.get_loop(bb.at('B'));
    EXPECT_EQ(b_loop.get_header(), bb.at('B'));
    EXPECT_EQ(b_loop.vertices_count(), 8);
    EXPECT_EQ(b_loop.inner_loops_count(), 2);
    EXPECT_EQ(b_loop.get_parent_loop(), nullptr);
    for (auto c : {'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J'}) {
        EXPECT_TRUE(b_loop.contains_vertex(bb.at(c))) << "BB_" << c;
    }

    const auto &c_loop = b_loop.get_inner_loop(bb.at('C'));
    EXPECT_EQ(c_loop.get_header(), bb.at('C'));
    EXPECT_EQ(c_loop.vertices_count(), 2);
    EXPECT_EQ(c_loop.inner_loops_count(), 0);
    EXPECT_EQ(c_loop.get_parent_loop(), &b_loop);
    for (auto c : {'C', 'D'}) {
        EXPECT_TRUE(c_loop.contains_vertex(bb.at(c))) << "BB_" << c;
    }

    const auto &e_loop = b_loop.get_inner_loop(bb.at('E'));
    EXPECT_EQ(e_loop.get_header(), bb.at('E'));
    EXPECT_EQ(e_loop.vertices_count(), 2);
    EXPECT_EQ(e_loop.inner_loops_count(), 0);
    EXPECT_EQ(e_loop.get_parent_loop(), &b_loop);
    for (auto c : {'E', 'F'}) {
        EXPECT_TRUE(e_loop.contains_vertex(bb.at(c))) << "BB_" << c;
    }
}

TEST(LoopTree, Mandatory_6) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kVoid, {}};

    std::unordered_map<char, bjac::BasicBlock *> bb;
    for (char name : {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I'}) {
        auto &BB = foo.emplace_back();
        bb.emplace(name, &BB);
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
    bjac::LoopTree<bjac::Function, bjac::FunctionGraphTraits> loop_tree{foo};

    // Assert
    EXPECT_EQ(loop_tree.loops_count(), 1);

    const auto &loop = loop_tree.get_loop(bb.at('B'));
    EXPECT_EQ(loop.get_header(), bb.at('B'));
    EXPECT_EQ(loop.vertices_count(), 3);
    EXPECT_EQ(loop.inner_loops_count(), 0);
    EXPECT_EQ(loop.get_parent_loop(), nullptr);
    for (auto c : {'B', 'E', 'F'}) {
        EXPECT_TRUE(loop.contains_vertex(bb.at(c))) << "BB_" << c;
    }
}
