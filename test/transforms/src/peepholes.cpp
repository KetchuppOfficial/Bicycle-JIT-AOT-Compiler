#include <cstdint>
#include <memory>
#include <ostream>
#include <ranges>
#include <vector>

#include <gtest/gtest.h>

#include "bjac/IR/argument_instruction.hpp"
#include "bjac/IR/binary_operator.hpp"
#include "bjac/IR/constant_instruction.hpp"
#include "bjac/IR/function.hpp"
#include "bjac/IR/ret_instruction.hpp"

#include "bjac/transforms/peepholes.hpp"

static auto get_instructions(const bjac::BasicBlock &bb) {
    return std::vector<const bjac::Instruction *>{
        std::from_range,
        bb | std::views::transform([](const auto &instr) { return std::addressof(instr); })};
}

enum class ArgsOrder : bool { kSelectedArgOnLeft, kSelectedArgOnRight };

std::ostream &operator<<(std::ostream &os, ArgsOrder order) {
    return os << (order == ArgsOrder::kSelectedArgOnLeft ? "const-on-left" : "const-on-right");
}

class PeepholesForAndWithConstant : public ::testing::TestWithParam<ArgsOrder> {};

/*
 * Before (arguments of <and> appear in both orders):
 * i64 foo(i64)
 * %bb0:
 *     %0.0 = i64 arg [0] ; used by: %0.2
 *     %0.1 = i64 constant 0 ; used by: %0.2
 *     %0.2 = i64 and %0.0, %0.1 ; used by: %0.3
 *     %0.3 ret i64 %0.2
 *
 * After:
 * i64 foo(i64)
 * %bb0:
 *     %0.0 = i64 arg [0]
 *     %0.1 = i64 constant 0 ; used by: %0.3
 *     %0.3 ret i64 %0.1
 */
TEST_P(PeepholesForAndWithConstant, ValueWithZero) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kI64, {bjac::Type::kI64}};

    auto &bb = foo.emplace_back();

    { // Use scope to discourage usage of variables defined within after the optimization pass
        auto &arg = bb.emplace_back<bjac::ArgumentInstruction>(foo, 0);
        auto &constant = bb.emplace_back<bjac::ConstInstruction>(bjac::Type::kI64, 0);
        auto &op = (GetParam() == ArgsOrder::kSelectedArgOnLeft)
                       ? bb.emplace_back<bjac::BinaryOperator>(bjac::Instruction::Opcode::kAnd,
                                                               constant, arg)
                       : bb.emplace_back<bjac::BinaryOperator>(bjac::Instruction::Opcode::kAnd, arg,
                                                               constant);
        bb.emplace_back<bjac::ReturnInstruction>(op);
    }

    // Act
    bjac::PeepholePass{}.run(foo);

    // Assert
    EXPECT_EQ(foo.size(), 1) << foo;
    EXPECT_EQ(bb.get_parent(), std::addressof(foo)) << foo;

    EXPECT_EQ(bb.size(), 3) << foo;

    const auto instrs = get_instructions(bb);

    EXPECT_EQ(instrs.at(0)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(0)->users_count(), 0) << foo;
    ASSERT_EQ(instrs.at(0)->get_opcode(), bjac::Instruction::Opcode::kArg) << foo;
    EXPECT_EQ(static_cast<const bjac::ArgumentInstruction *>(instrs.at(0))->get_position(), 0)
        << foo;

    EXPECT_EQ(instrs.at(1)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(1)->users_count(), 1) << foo;
    EXPECT_TRUE(instrs.at(1)->has_user(instrs.at(2))) << foo;
    ASSERT_EQ(instrs.at(1)->get_opcode(), bjac::Instruction::Opcode::kConst) << foo;
    EXPECT_EQ(static_cast<const bjac::ConstInstruction *>(instrs.at(1))->get_value(), 0) << foo;

    EXPECT_EQ(instrs.at(2)->users_count(), 0) << foo;
    ASSERT_EQ(instrs.at(2)->get_opcode(), bjac::Instruction::Opcode::kRet) << foo;
    EXPECT_EQ(static_cast<const bjac::ReturnInstruction *>(instrs.at(2))->get_ret_value(),
              instrs.at(1))
        << foo;
}

/*
 * Before (arguments of <and> appear in both orders):
 * i64 foo(i64)
 * %bb0:
 *     %0.0 = i64 arg [0] ; used by: %0.2
 *     %0.1 = i64 constant 0xffffffffffffffff ; used by: %0.2
 *     %0.2 = i64 and %0.0, %0.1 ; used by: %0.3
 *     %0.3 ret i64 %0.2
 *
 * After:
 * i64 foo(i64)
 * %bb0:
 *     %0.0 = i64 arg [0] ; used by: %0.3
 *     %0.1 = i64 constant 0xffffffffffffffff
 *     %0.3 ret i64 %0.0
 */
TEST_P(PeepholesForAndWithConstant, ValueWithAllOnes) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kI64, {bjac::Type::kI64}};

    auto &bb = foo.emplace_back();

    { // Use scope to discourage usage of variables defined within after the optimization pass
        auto &arg = bb.emplace_back<bjac::ArgumentInstruction>(foo, 0);
        auto &constant =
            bb.emplace_back<bjac::ConstInstruction>(bjac::Type::kI64, ~std::uint64_t{0});
        auto &op = (GetParam() == ArgsOrder::kSelectedArgOnLeft)
                       ? bb.emplace_back<bjac::BinaryOperator>(bjac::Instruction::Opcode::kAnd,
                                                               constant, arg)
                       : bb.emplace_back<bjac::BinaryOperator>(bjac::Instruction::Opcode::kAnd, arg,
                                                               constant);
        bb.emplace_back<bjac::ReturnInstruction>(op);
    }

    // Act
    bjac::PeepholePass{}.run(foo);

    // Assert
    EXPECT_EQ(foo.size(), 1) << foo;
    EXPECT_EQ(bb.get_parent(), std::addressof(foo)) << foo;

    EXPECT_EQ(bb.size(), 3) << foo;

    const auto instrs = get_instructions(bb);

    EXPECT_EQ(instrs.at(0)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(0)->users_count(), 1) << foo;
    EXPECT_TRUE(instrs.at(0)->has_user(instrs.at(2))) << foo;
    ASSERT_EQ(instrs.at(0)->get_opcode(), bjac::Instruction::Opcode::kArg) << foo;
    EXPECT_EQ(static_cast<const bjac::ArgumentInstruction *>(instrs.at(0))->get_position(), 0)
        << foo;

    EXPECT_EQ(instrs.at(1)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(1)->users_count(), 0) << foo;
    ASSERT_EQ(instrs.at(1)->get_opcode(), bjac::Instruction::Opcode::kConst) << foo;
    EXPECT_EQ(static_cast<const bjac::ConstInstruction *>(instrs.at(1))->get_value(), ~uint64_t{0})
        << foo;

    EXPECT_EQ(instrs.at(2)->users_count(), 0) << foo;
    ASSERT_EQ(instrs.at(2)->get_opcode(), bjac::Instruction::Opcode::kRet) << foo;
    EXPECT_EQ(static_cast<const bjac::ReturnInstruction *>(instrs.at(2))->get_ret_value(),
              instrs.at(0))
        << foo;
}

INSTANTIATE_TEST_SUITE_P(
    /* no instantiation name */, PeepholesForAndWithConstant,
    ::testing::Values(ArgsOrder::kSelectedArgOnLeft, ArgsOrder::kSelectedArgOnRight));

/*
 * Before:
 * i64 foo(i64)
 * %bb0:
 *     %0.0 = i64 arg [0] ; used by: %0.1, %0.1
 *     %0.1 = i64 and %0.0, %0.0 ; used by: %0.2
 *     %0.2 ret i64 %0.1
 *
 * After:
 * i64 foo(i64)
 * %bb0:
 *     %0.0 = i64 arg [0] ; used by: %0.2
 *     %0.2 ret i64 %0.0
 */
TEST(PeepholesForAnd, BothArgumentsAreTheSameInstruction) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kI64, {bjac::Type::kI64}};

    auto &bb = foo.emplace_back();

    { // Use scope to discourage usage of variables defined within after the optimization pass
        auto &arg = bb.emplace_back<bjac::ArgumentInstruction>(foo, 0);
        auto &op = bb.emplace_back<bjac::BinaryOperator>(bjac::Instruction::Opcode::kAnd, arg, arg);
        bb.emplace_back<bjac::ReturnInstruction>(op);
    }

    // Act
    bjac::PeepholePass{}.run(foo);

    // Assert
    EXPECT_EQ(foo.size(), 1) << foo;
    EXPECT_EQ(bb.get_parent(), std::addressof(foo)) << foo;

    EXPECT_EQ(bb.size(), 2) << foo;

    const auto instrs = get_instructions(bb);

    EXPECT_EQ(instrs.at(0)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(0)->users_count(), 1) << foo;
    EXPECT_TRUE(instrs.at(0)->has_user(instrs.at(1))) << foo;
    ASSERT_EQ(instrs.at(0)->get_opcode(), bjac::Instruction::Opcode::kArg) << foo;
    EXPECT_EQ(static_cast<const bjac::ArgumentInstruction *>(instrs.at(0))->get_position(), 0)
        << foo;

    EXPECT_EQ(instrs.at(1)->users_count(), 0) << foo;
    ASSERT_EQ(instrs.at(1)->get_opcode(), bjac::Instruction::Opcode::kRet) << foo;
    EXPECT_EQ(static_cast<const bjac::ReturnInstruction *>(instrs.at(1))->get_ret_value(),
              instrs.at(0))
        << foo;
}

/*
 * Before:
 * i64 foo(i64)
 * %bb0:
 *     %0.0 = i64 arg [0] ; used by: %0.2
 *     %0.1 = i64 constant 0x0ff ; used by: %0.2
 *     %0.2 = i64 and %0.1, %0.0 ; used by: %0.4
 *     %0.3 = i64 constant 0xff0 ; used by: %0.4
 *     %0.4 = i64 and %0.3, %0.2 ; used by: %0.5
 *     %0.5 ret i64 %0.4
 *
 * After:
 * i64 foo(i64)
 * %bb0:
 *     %0.0 = i64 arg [0] ; used by: %0.2, %0.4
 *     %0.1 = i64 constant 0x0ff ; used by: %0.2
 *     %0.2 = i64 and %0.1, %0.0
 *     %0.3 = i64 constant 0xff0
 *     %0.6 = i64 constant 0x0f0 ; used by: %0.4
 *     %0.4 = i64 and %0.0, %0.6 ; used by: %0.5
 *     %0.5 ret i64 %0.4
 */
TEST(PeepholesForAnd, ChainingConstantOnLeftLeft) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kI64, {bjac::Type::kI64}};

    auto &bb = foo.emplace_back();

    { // Use scope to discourage usage of variables defined within after the optimization pass
        auto &arg = bb.emplace_back<bjac::ArgumentInstruction>(foo, 0);
        auto &const_1 =
            bb.emplace_back<bjac::ConstInstruction>(bjac::Type::kI64, std::uint64_t{0x0ff});
        auto &op_1 =
            bb.emplace_back<bjac::BinaryOperator>(bjac::Instruction::Opcode::kAnd, const_1, arg);
        auto &const_2 =
            bb.emplace_back<bjac::ConstInstruction>(bjac::Type::kI64, std::uint64_t{0xff0});
        auto &op_2 =
            bb.emplace_back<bjac::BinaryOperator>(bjac::Instruction::Opcode::kAnd, const_2, op_1);
        bb.emplace_back<bjac::ReturnInstruction>(op_2);
    }

    // Act
    bjac::PeepholePass{}.run(foo);

    // Assert
    EXPECT_EQ(foo.size(), 1) << foo;
    EXPECT_EQ(bb.get_parent(), std::addressof(foo)) << foo;

    EXPECT_EQ(bb.size(), 7) << foo;

    const auto instrs = get_instructions(bb);

    EXPECT_EQ(instrs.at(0)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(0)->users_count(), 2) << foo;
    EXPECT_TRUE(instrs.at(0)->has_user(instrs.at(2))) << foo;
    EXPECT_TRUE(instrs.at(0)->has_user(instrs.at(5))) << foo;
    ASSERT_EQ(instrs.at(0)->get_opcode(), bjac::Instruction::Opcode::kArg) << foo;
    EXPECT_EQ(static_cast<const bjac::ArgumentInstruction *>(instrs.at(0))->get_position(), 0)
        << foo;

    EXPECT_EQ(instrs.at(1)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(1)->users_count(), 1) << foo;
    EXPECT_TRUE(instrs.at(1)->has_user(instrs.at(2))) << foo;
    ASSERT_EQ(instrs.at(1)->get_opcode(), bjac::Instruction::Opcode::kConst) << foo;
    EXPECT_EQ(static_cast<const bjac::ConstInstruction *>(instrs.at(1))->get_value(), 0x0ff) << foo;

    EXPECT_EQ(instrs.at(2)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(2)->users_count(), 0) << foo;
    ASSERT_EQ(instrs.at(2)->get_opcode(), bjac::Instruction::Opcode::kAnd) << foo;
    EXPECT_EQ(static_cast<const bjac::BinaryOperator *>(instrs.at(2))->get_lhs(), instrs.at(1))
        << foo;
    EXPECT_EQ(static_cast<const bjac::BinaryOperator *>(instrs.at(2))->get_rhs(), instrs.at(0))
        << foo;

    EXPECT_EQ(instrs.at(3)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(3)->users_count(), 0) << foo;
    ASSERT_EQ(instrs.at(3)->get_opcode(), bjac::Instruction::Opcode::kConst) << foo;
    EXPECT_EQ(static_cast<const bjac::ConstInstruction *>(instrs.at(3))->get_value(), 0xff0) << foo;

    EXPECT_EQ(instrs.at(4)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(4)->users_count(), 1) << foo;
    EXPECT_TRUE(instrs.at(4)->has_user(instrs.at(5))) << foo;
    ASSERT_EQ(instrs.at(4)->get_opcode(), bjac::Instruction::Opcode::kConst) << foo;
    EXPECT_EQ(static_cast<const bjac::ConstInstruction *>(instrs.at(4))->get_value(), 0x0f0) << foo;

    EXPECT_EQ(instrs.at(5)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(5)->users_count(), 1) << foo;
    EXPECT_TRUE(instrs.at(5)->has_user(instrs.at(6))) << foo;
    ASSERT_EQ(instrs.at(5)->get_opcode(), bjac::Instruction::Opcode::kAnd) << foo;
    EXPECT_EQ(static_cast<const bjac::BinaryOperator *>(instrs.at(5))->get_lhs(), instrs.at(0))
        << foo;
    EXPECT_EQ(static_cast<const bjac::BinaryOperator *>(instrs.at(5))->get_rhs(), instrs.at(4))
        << foo;

    EXPECT_EQ(instrs.at(6)->users_count(), 0) << foo;
    ASSERT_EQ(instrs.at(6)->get_opcode(), bjac::Instruction::Opcode::kRet) << foo;
    EXPECT_EQ(static_cast<const bjac::ReturnInstruction *>(instrs.at(6))->get_ret_value(),
              instrs.at(5))
        << foo;
}

class PeepholesForAdd : public ::testing::TestWithParam<ArgsOrder> {};

TEST_P(PeepholesForAdd, OneOfArgumentsIsZero) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kI64, {bjac::Type::kI64}};

    auto &bb = foo.emplace_back();

    { // Use scope to discourage usage of variables defined within after the optimization pass
        auto &arg = bb.emplace_back<bjac::ArgumentInstruction>(foo, 0);
        auto &constant = bb.emplace_back<bjac::ConstInstruction>(bjac::Type::kI64, 0);
        auto &op = (GetParam() == ArgsOrder::kSelectedArgOnLeft)
                       ? bb.emplace_back<bjac::BinaryOperator>(bjac::Instruction::Opcode::kAdd,
                                                               constant, arg)
                       : bb.emplace_back<bjac::BinaryOperator>(bjac::Instruction::Opcode::kAdd, arg,
                                                               constant);
        bb.emplace_back<bjac::ReturnInstruction>(op);
    }

    // Act
    bjac::PeepholePass{}.run(foo);

    // Assert
    EXPECT_EQ(foo.size(), 1) << foo;
    EXPECT_EQ(bb.get_parent(), std::addressof(foo)) << foo;

    EXPECT_EQ(bb.size(), 3) << foo;

    const auto instrs = get_instructions(bb);

    EXPECT_EQ(instrs.at(0)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(0)->users_count(), 1) << foo;
    EXPECT_TRUE(instrs.at(0)->has_user(instrs.at(2))) << foo;
    ASSERT_EQ(instrs.at(0)->get_opcode(), bjac::Instruction::Opcode::kArg) << foo;
    EXPECT_EQ(static_cast<const bjac::ArgumentInstruction *>(instrs.at(0))->get_position(), 0)
        << foo;

    EXPECT_EQ(instrs.at(1)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(1)->users_count(), 0) << foo;
    ASSERT_EQ(instrs.at(1)->get_opcode(), bjac::Instruction::Opcode::kConst) << foo;
    EXPECT_EQ(static_cast<const bjac::ConstInstruction *>(instrs.at(1))->get_value(), 0) << foo;

    EXPECT_EQ(instrs.at(2)->users_count(), 0) << foo;
    ASSERT_EQ(instrs.at(2)->get_opcode(), bjac::Instruction::Opcode::kRet) << foo;
    EXPECT_EQ(static_cast<const bjac::ReturnInstruction *>(instrs.at(2))->get_ret_value(),
              instrs.at(0))
        << foo;
}

/*
 * Before (arguments of <and> appear in both orders):
 * i64 foo(i64, i64)
 * %bb0:
 *     %0.0 = i64 arg [0] ; used by: %0.2
 *     %0.1 = i64 constant 0 ; used by: %0.2
 *     %0.2 = i64 sub %0.1, %0.0 ; used by: %0.4
 *     %0.3 = i64 arg [1] ; used by: %0.4
 *     %0.4 = i64 add %0.3, %0.2 ; used by: %0.5
 *     %0.5 ret i64 %0.4
 *
 * After:
 * i64 foo(i64, i64)
 * %bb0:
 *     %0.0 = i64 arg [0] ; used by: %0.2, %0.6
 *     %0.1 = i64 constant 0 ; used by: %0.2
 *     %0.2 = i64 sub %0.1, %0.0
 *     %0.3 = i64 arg [1] ; used by: %0.6
 *     %0.6 = i64 sub %0.3, %0.0 ; used by: %0.5
 *     %0.5 ret i64 %0.6
 */
TEST_P(PeepholesForAdd, LeftArgumentIsSubFromZero) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kI64, {bjac::Type::kI64, bjac::Type::kI64}};

    auto &bb = foo.emplace_back();

    { // Use scope to discourage usage of variables defined within after the optimization pass
        auto &arg_1 = bb.emplace_back<bjac::ArgumentInstruction>(foo, 0);
        auto &const_1 = bb.emplace_back<bjac::ConstInstruction>(bjac::Type::kI64, 0);
        auto &sub =
            bb.emplace_back<bjac::BinaryOperator>(bjac::Instruction::Opcode::kSub, const_1, arg_1);
        auto &arg_2 = bb.emplace_back<bjac::ArgumentInstruction>(foo, 1);
        auto &add =
            (GetParam() == ArgsOrder::kSelectedArgOnLeft)
                ? bb.emplace_back<bjac::BinaryOperator>(bjac::Instruction::Opcode::kAdd, sub, arg_2)
                : bb.emplace_back<bjac::BinaryOperator>(bjac::Instruction::Opcode::kAdd, arg_2,
                                                        sub);
        bb.emplace_back<bjac::ReturnInstruction>(add);
    }

    // Act
    bjac::PeepholePass{}.run(foo);

    // Assert
    EXPECT_EQ(foo.size(), 1) << foo;
    EXPECT_EQ(bb.get_parent(), std::addressof(foo)) << foo;

    EXPECT_EQ(bb.size(), 6) << foo;

    const auto instrs = get_instructions(bb);

    EXPECT_EQ(instrs.at(0)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(0)->users_count(), 2) << foo;
    EXPECT_TRUE(instrs.at(0)->has_user(instrs.at(2))) << foo;
    EXPECT_TRUE(instrs.at(0)->has_user(instrs.at(4))) << foo;
    ASSERT_EQ(instrs.at(0)->get_opcode(), bjac::Instruction::Opcode::kArg) << foo;
    EXPECT_EQ(static_cast<const bjac::ArgumentInstruction *>(instrs.at(0))->get_position(), 0)
        << foo;

    EXPECT_EQ(instrs.at(1)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(1)->users_count(), 1) << foo;
    EXPECT_TRUE(instrs.at(1)->has_user(instrs.at(2))) << foo;
    ASSERT_EQ(instrs.at(1)->get_opcode(), bjac::Instruction::Opcode::kConst) << foo;
    EXPECT_EQ(static_cast<const bjac::ConstInstruction *>(instrs.at(1))->get_value(), 0) << foo;

    EXPECT_EQ(instrs.at(2)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(2)->users_count(), 0) << foo;
    ASSERT_EQ(instrs.at(2)->get_opcode(), bjac::Instruction::Opcode::kSub) << foo;
    EXPECT_EQ(static_cast<const bjac::BinaryOperator *>(instrs.at(2))->get_lhs(), instrs.at(1))
        << foo;
    EXPECT_EQ(static_cast<const bjac::BinaryOperator *>(instrs.at(2))->get_rhs(), instrs.at(0))
        << foo;

    EXPECT_EQ(instrs.at(3)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(3)->users_count(), 1) << foo;
    EXPECT_TRUE(instrs.at(3)->has_user(instrs.at(4))) << foo;
    ASSERT_EQ(instrs.at(3)->get_opcode(), bjac::Instruction::Opcode::kArg) << foo;
    EXPECT_EQ(static_cast<const bjac::ArgumentInstruction *>(instrs.at(3))->get_position(), 1)
        << foo;

    EXPECT_EQ(instrs.at(4)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(4)->users_count(), 1) << foo;
    EXPECT_TRUE(instrs.at(4)->has_user(instrs.at(5))) << foo;
    ASSERT_EQ(instrs.at(4)->get_opcode(), bjac::Instruction::Opcode::kSub) << foo;
    EXPECT_EQ(static_cast<const bjac::BinaryOperator *>(instrs.at(4))->get_lhs(), instrs.at(3))
        << foo;
    EXPECT_EQ(static_cast<const bjac::BinaryOperator *>(instrs.at(4))->get_rhs(), instrs.at(0))
        << foo;

    EXPECT_EQ(instrs.at(5)->users_count(), 0) << foo;
    ASSERT_EQ(instrs.at(5)->get_opcode(), bjac::Instruction::Opcode::kRet) << foo;
    EXPECT_EQ(static_cast<const bjac::ReturnInstruction *>(instrs.at(5))->get_ret_value(),
              instrs.at(4))
        << foo;
}

INSTANTIATE_TEST_SUITE_P(
    /* no instantiation name */, PeepholesForAdd,
    ::testing::Values(ArgsOrder::kSelectedArgOnLeft, ArgsOrder::kSelectedArgOnRight));

TEST(PeepholesForShrLWithZero, ShiftForZeroBits) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kI64, {bjac::Type::kI64}};

    auto &bb = foo.emplace_back();

    { // Use scope to discourage usage of variables defined within after the optimization pass
        auto &arg = bb.emplace_back<bjac::ArgumentInstruction>(foo, 0);
        auto &constant = bb.emplace_back<bjac::ConstInstruction>(bjac::Type::kI64, 0);
        auto &op =
            bb.emplace_back<bjac::BinaryOperator>(bjac::Instruction::Opcode::kShrL, arg, constant);
        bb.emplace_back<bjac::ReturnInstruction>(op);
    }

    // Act
    bjac::PeepholePass{}.run(foo);

    // Assert
    EXPECT_EQ(foo.size(), 1) << foo;
    EXPECT_EQ(bb.get_parent(), std::addressof(foo)) << foo;

    EXPECT_EQ(bb.size(), 3) << foo;

    const auto instrs = get_instructions(bb);

    EXPECT_EQ(instrs.at(0)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(0)->users_count(), 1) << foo;
    EXPECT_TRUE(instrs.at(0)->has_user(instrs.at(2))) << foo;
    ASSERT_EQ(instrs.at(0)->get_opcode(), bjac::Instruction::Opcode::kArg) << foo;
    EXPECT_EQ(static_cast<const bjac::ArgumentInstruction *>(instrs.at(0))->get_position(), 0)
        << foo;

    EXPECT_EQ(instrs.at(1)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(1)->users_count(), 0) << foo;
    ASSERT_EQ(instrs.at(1)->get_opcode(), bjac::Instruction::Opcode::kConst) << foo;
    EXPECT_EQ(static_cast<const bjac::ConstInstruction *>(instrs.at(1))->get_value(), 0) << foo;

    EXPECT_EQ(instrs.at(2)->users_count(), 0) << foo;
    ASSERT_EQ(instrs.at(2)->get_opcode(), bjac::Instruction::Opcode::kRet) << foo;
    EXPECT_EQ(static_cast<const bjac::ReturnInstruction *>(instrs.at(2))->get_ret_value(),
              instrs.at(0))
        << foo;
}

TEST(PeepholesForShrLWithZero, ShiftZeroForAnyNumberOfBits) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kI64, {bjac::Type::kI64}};

    auto &bb = foo.emplace_back();

    { // Use scope to discourage usage of variables defined within after the optimization pass
        auto &arg = bb.emplace_back<bjac::ArgumentInstruction>(foo, 0);
        auto &constant = bb.emplace_back<bjac::ConstInstruction>(bjac::Type::kI64, 0);
        auto &op =
            bb.emplace_back<bjac::BinaryOperator>(bjac::Instruction::Opcode::kShrL, constant, arg);
        bb.emplace_back<bjac::ReturnInstruction>(op);
    }

    // Act
    bjac::PeepholePass{}.run(foo);

    // Assert
    EXPECT_EQ(foo.size(), 1) << foo;
    EXPECT_EQ(bb.get_parent(), std::addressof(foo)) << foo;

    EXPECT_EQ(bb.size(), 3) << foo;

    const auto instrs = get_instructions(bb);

    EXPECT_EQ(instrs.at(0)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(0)->users_count(), 0) << foo;
    ASSERT_EQ(instrs.at(0)->get_opcode(), bjac::Instruction::Opcode::kArg) << foo;
    EXPECT_EQ(static_cast<const bjac::ArgumentInstruction *>(instrs.at(0))->get_position(), 0)
        << foo;

    EXPECT_EQ(instrs.at(1)->get_type(), bjac::Type::kI64) << foo;
    EXPECT_EQ(instrs.at(1)->users_count(), 1) << foo;
    EXPECT_TRUE(instrs.at(1)->has_user(instrs.at(2))) << foo;
    ASSERT_EQ(instrs.at(1)->get_opcode(), bjac::Instruction::Opcode::kConst) << foo;
    EXPECT_EQ(static_cast<const bjac::ConstInstruction *>(instrs.at(1))->get_value(), 0) << foo;

    EXPECT_EQ(instrs.at(2)->users_count(), 0) << foo;
    ASSERT_EQ(instrs.at(2)->get_opcode(), bjac::Instruction::Opcode::kRet) << foo;
    EXPECT_EQ(static_cast<const bjac::ReturnInstruction *>(instrs.at(2))->get_ret_value(),
              instrs.at(1))
        << foo;
}
