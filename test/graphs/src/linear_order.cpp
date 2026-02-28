#include <gtest/gtest.h>

#include "bjac/graphs/linear_order.hpp"

#include "bjac/IR/branch_instruction.hpp"
#include "bjac/IR/constant_instruction.hpp"
#include "bjac/IR/function.hpp"
#include "bjac/IR/ret_instruction.hpp"

#include "test/common.hpp"

using enum bjac::Type;

TEST(LinearOrder, Mandatory_1) {
    // Assign
    bjac::Function foo{"foo", kVoid, {}};
    auto [bb, names] = setup(foo, {'A', 'B', 'C', 'D', 'E'});

    auto &cond = bb.at('A')->emplace_back<bjac::ConstInstruction>(kI1, 0);

    bb.at('A')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));
    bb.at('B')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('C'), *bb.at('D'));
    bb.at('D')->emplace_back<bjac::BranchInstruction>(*bb.at('E'));
    bb.at('E')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));

    // Act
    const bjac::LinearOrder<bjac::ConstFunctionGraphTraits> linear_order{foo};

    // Assert
    EXPECT_TRUE(matches(linear_order,
                        {std::array{bb.at('A'), bb.at('B'), bb.at('D'), bb.at('E'), bb.at('C')}},
                        names));
}

TEST(LinearOrder, Mandatory_2) {
    // Assign
    bjac::Function foo{"foo", kVoid, {}};
    auto [bb, names] = setup(foo, {'A', 'B', 'C', 'D', 'E', 'F'});

    auto &cond = bb.at('A')->emplace_back<bjac::ConstInstruction>(kI1, 0);

    bb.at('A')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));
    bb.at('B')->emplace_back<bjac::BranchInstruction>(*bb.at('C'));
    bb.at('C')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('F'), *bb.at('D'));
    bb.at('D')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('F'), *bb.at('E'));
    bb.at('E')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));

    // Act
    const bjac::LinearOrder<bjac::ConstFunctionGraphTraits> linear_order{foo};

    // Assert
    EXPECT_TRUE(matches(
        linear_order,
        {std::array{bb.at('A'), bb.at('B'), bb.at('C'), bb.at('D'), bb.at('E'), bb.at('F')}},
        names));
}

TEST(LinearOrder, Mandatory_3) {
    // Assign
    bjac::Function foo{"foo", kVoid, {}};
    auto [bb, names] = setup(foo, {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'});

    auto &cond = bb.at('A')->emplace_back<bjac::ConstInstruction>(kI1, 0);

    bb.at('A')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));
    bb.at('B')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('C'), *bb.at('D'));
    bb.at('C')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('E'), *bb.at('F'));
    bb.at('D')->emplace_back<bjac::BranchInstruction>(*bb.at('F'));
    bb.at('F')->emplace_back<bjac::BranchInstruction>(*bb.at('G'));
    bb.at('G')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('H'), *bb.at('B'));
    bb.at('H')->emplace_back<bjac::BranchInstruction>(*bb.at('A'));

    // Act
    const bjac::LinearOrder<bjac::ConstFunctionGraphTraits> linear_order{foo};

    // Assert
    EXPECT_TRUE(matches(linear_order,
                        {std::array{bb.at('A'), bb.at('B'), bb.at('C'), bb.at('D'), bb.at('F'),
                                    bb.at('G'), bb.at('H'), bb.at('E')},
                         std::array{bb.at('A'), bb.at('B'), bb.at('D'), bb.at('C'), bb.at('F'),
                                    bb.at('G'), bb.at('G'), bb.at('E')}},
                        names));
}

TEST(LinearOrder, Mandatory_5) {
    // Assign
    bjac::Function foo{"foo", kVoid, {}};
    auto [bb, names] = setup(foo, {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K'});

    auto &cond = bb.at('A')->emplace_back<bjac::ConstInstruction>(kI1, 0);

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
    const bjac::LinearOrder<bjac::ConstFunctionGraphTraits> linear_order{foo};

    // Assert
    EXPECT_TRUE(
        matches(linear_order,
                {std::array{bb.at('A'), bb.at('B'), bb.at('C'), bb.at('J'), bb.at('D'), bb.at('E'),
                            bb.at('F'), bb.at('G'), bb.at('H'), bb.at('I'), bb.at('K')},
                 std::array{bb.at('A'), bb.at('B'), bb.at('J'), bb.at('C'), bb.at('D'), bb.at('E'),
                            bb.at('F'), bb.at('G'), bb.at('H'), bb.at('I'), bb.at('K')}},
                names));
}

/*
 *    A
 *   / \
 *  B   C <--+
 *      |    |
 *      D ---+
 */
TEST(LinearOrder, IssueWithIfBeforeLoop) {
    using enum bjac::Instruction::Opcode;

    // Assign
    bjac::Function foo{"foo", bjac::Type::kVoid, {}};
    auto [bb, names] = setup(foo, {'A', 'B', 'C', 'D'});

    auto &cond = bb.at('A')->emplace_back<bjac::ConstInstruction>(bjac::Type::kI1, 0);

    bb.at('A')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('B'), *bb.at('C'));
    bb.at('B')->emplace_back<bjac::ReturnInstruction>();
    bb.at('C')->emplace_back<bjac::BranchInstruction>(*bb.at('D'));
    bb.at('D')->emplace_back<bjac::BranchInstruction>(*bb.at('C'));

    // Act
    const bjac::LinearOrder<bjac::ConstFunctionGraphTraits> linear_order{foo};

    // Assert
    EXPECT_TRUE(matches(linear_order,
                        {std::array{bb.at('A'), bb.at('B'), bb.at('C'), bb.at('D')},
                         std::array{bb.at('A'), bb.at('C'), bb.at('D'), bb.at('B')}},
                        names));
}
