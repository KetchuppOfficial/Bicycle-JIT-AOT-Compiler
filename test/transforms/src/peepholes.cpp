#include <memory>
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

TEST(Peepholes, AndValueWithZero) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kI64, {bjac::Type::kI64}};

    auto &bb = foo.emplace_back();

    { // Use scope to discourage usage of variables defined within after the optimization pass
        auto &arg = bb.emplace_back<bjac::ArgumentInstruction>(foo, 0);
        auto &constant = bb.emplace_back<bjac::ConstInstruction>(bjac::Type::kI64, 0);
        auto &op =
            bb.emplace_back<bjac::BinaryOperator>(bjac::Instruction::Opcode::kAnd, arg, constant);
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
              static_cast<const bjac::ConstInstruction *>(instrs.at(1)))
        << foo;
}

TEST(Peepholes, AndZeroWithValue) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kI64, {bjac::Type::kI64}};

    auto &bb = foo.emplace_back();

    { // Use scope to discourage usage of variables defined within after the optimization pass
        auto &arg = bb.emplace_back<bjac::ArgumentInstruction>(foo, 0);
        auto &constant = bb.emplace_back<bjac::ConstInstruction>(bjac::Type::kI64, 0);
        auto &op =
            bb.emplace_back<bjac::BinaryOperator>(bjac::Instruction::Opcode::kAnd, constant, arg);
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
              static_cast<const bjac::ConstInstruction *>(instrs.at(1)))
        << foo;
}
