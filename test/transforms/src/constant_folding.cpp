#include <cstdint>
#include <format>
#include <iterator>

#include <gtest/gtest.h>

#include "bjac/IR/binary_operator.hpp"
#include "bjac/IR/constant_instruction.hpp"
#include "bjac/IR/function.hpp"
#include "bjac/IR/icmp_instruction.hpp"
#include "bjac/IR/ret_instruction.hpp"

#include "bjac/transforms/constant_folding.hpp"

struct BinOpParam {
    bjac::Instruction::Opcode opcode;
    std::uintmax_t lhs;
    std::uintmax_t rhs;
    std::uintmax_t res;

    friend std::ostream &operator<<(std::ostream &os, [[maybe_unused]] const BinOpParam &p) {
        return os << "<some-param>";
    }
};

class ConstantFoldingForBinaryOperation : public ::testing::TestWithParam<BinOpParam> {};

TEST_P(ConstantFoldingForBinaryOperation, /* no test name */) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kI64, {}};

    auto &bb = foo.emplace_back();
    auto &const_1 = bb.emplace_back<bjac::ConstInstruction>(bjac::Type::kI64, GetParam().lhs);
    auto &const_2 = bb.emplace_back<bjac::ConstInstruction>(bjac::Type::kI64, GetParam().rhs);
    auto &op = bb.emplace_back<bjac::BinaryOperator>(GetParam().opcode, const_1, const_2);
    bb.emplace_back<bjac::ReturnInstruction>(op);

    // Act
    bjac::ConstantFoldingPass{}.run(foo);

    // Assert
    EXPECT_EQ(bb.size(), 4) << foo;

    EXPECT_EQ(std::next(bb.begin(), 0)->get_type(), bjac::Type::kI64);
    ASSERT_EQ(std::next(bb.begin(), 0)->get_opcode(), bjac::Instruction::Opcode::kConst);
    EXPECT_EQ(static_cast<bjac::ConstInstruction &>(*std::next(bb.begin(), 0)).get_value(),
              GetParam().lhs);

    EXPECT_EQ(std::next(bb.begin(), 1)->get_type(), bjac::Type::kI64);
    ASSERT_EQ(std::next(bb.begin(), 1)->get_opcode(), bjac::Instruction::Opcode::kConst);
    EXPECT_EQ(static_cast<bjac::ConstInstruction &>(*std::next(bb.begin(), 1)).get_value(),
              GetParam().rhs);

    EXPECT_EQ(std::next(bb.begin(), 2)->get_type(), bjac::Type::kI64);
    ASSERT_EQ(std::next(bb.begin(), 2)->get_opcode(), bjac::Instruction::Opcode::kConst);
    EXPECT_EQ(static_cast<bjac::ConstInstruction &>(*std::next(bb.begin(), 2)).get_value(),
              GetParam().res);

    ASSERT_EQ(std::next(bb.begin(), 3)->get_opcode(), bjac::Instruction::Opcode::kRet);
    EXPECT_EQ(static_cast<bjac::ReturnInstruction &>(*std::next(bb.begin(), 3)).get_ret_value(),
              &static_cast<bjac::ConstInstruction &>(*std::next(bb.begin(), 2)));
}

INSTANTIATE_TEST_SUITE_P(
    /* no instantiation name */, ConstantFoldingForBinaryOperation,
    ::testing::Values(BinOpParam(bjac::Instruction::Opcode::kAdd, 42, 5, 47),
                      BinOpParam(bjac::Instruction::Opcode::kSub, 42, 5, 37),
                      BinOpParam(bjac::Instruction::Opcode::kMul, 42, 5, 210),
                      BinOpParam(bjac::Instruction::Opcode::kUDiv, 42, 5, 8),
                      BinOpParam(bjac::Instruction::Opcode::kSDiv, 42, 5, 8),
                      BinOpParam(bjac::Instruction::Opcode::kUDiv, -2, 2,
                                 std::numeric_limits<std::intmax_t>::max()),
                      BinOpParam(bjac::Instruction::Opcode::kSDiv, -2, 2, -1),
                      BinOpParam(bjac::Instruction::Opcode::kURem, 41, 2, 1),
                      BinOpParam(bjac::Instruction::Opcode::kSRem, 41, 2, 1),
                      BinOpParam(bjac::Instruction::Opcode::kURem, -41, 2, 1),
                      BinOpParam(bjac::Instruction::Opcode::kSRem, -41, 2, -1),
                      BinOpParam(bjac::Instruction::Opcode::kShl, 0xf, 4, 0xf0),
                      BinOpParam(bjac::Instruction::Opcode::kShrL, 0xf0, 4, 0xf),
                      BinOpParam(bjac::Instruction::Opcode::kShrA, 0xf0, 4, 0xf),
                      BinOpParam(bjac::Instruction::Opcode::kShrL, -1, 4, 0x0fffffffffffffff),
                      BinOpParam(bjac::Instruction::Opcode::kShrA, -1, 4, 0xffffffffffffffff),
                      BinOpParam(bjac::Instruction::Opcode::kAnd, 0xffff'ffff'0000'ffff,
                                 0x0000'ffff'ffff'0000, 0x0000'ffff'0000'0000),
                      BinOpParam(bjac::Instruction::Opcode::kOr, 0xffff'ffff'0000'ffff,
                                 0x0000'ffff'ffff'0000, 0xffff'ffff'ffff'ffff),
                      BinOpParam(bjac::Instruction::Opcode::kXor, 0xffff'ffff'0000'ffff,
                                 0x0000'ffff'ffff'0000, 0xffff'0000'ffff'ffff)),
    [](const ::testing::TestParamInfo<BinOpParam> &info) static {
        return std::format("{}_{}_{}", info.param.opcode, info.param.lhs, info.param.rhs);
    });

struct ICmpParam {
    bjac::ICmpInstruction::Kind kind;
    std::uintmax_t lhs;
    std::uintmax_t rhs;
    bool res;

    friend std::ostream &operator<<(std::ostream &os, [[maybe_unused]] const ICmpParam &p) {
        return os << "<some-param>";
    }
};

class ConstantFoldingForICmp : public ::testing::TestWithParam<ICmpParam> {};

TEST_P(ConstantFoldingForICmp, /* no test name */) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kI1, {}};

    auto &bb = foo.emplace_back();
    auto &const_1 = bb.emplace_back<bjac::ConstInstruction>(bjac::Type::kI64, GetParam().lhs);
    auto &const_2 = bb.emplace_back<bjac::ConstInstruction>(bjac::Type::kI64, GetParam().rhs);
    auto &icmp = bb.emplace_back<bjac::ICmpInstruction>(GetParam().kind, const_1, const_2);
    bb.emplace_back<bjac::ReturnInstruction>(icmp);

    // Act
    bjac::ConstantFoldingPass{}.run(foo);

    // Assert
    EXPECT_EQ(bb.size(), 4) << foo;

    EXPECT_EQ(std::next(bb.begin(), 0)->get_type(), bjac::Type::kI64) << foo;
    ASSERT_EQ(std::next(bb.begin(), 0)->get_opcode(), bjac::Instruction::Opcode::kConst);
    EXPECT_EQ(static_cast<bjac::ConstInstruction &>(*std::next(bb.begin(), 0)).get_value(),
              GetParam().lhs)
        << foo;

    EXPECT_EQ(std::next(bb.begin(), 1)->get_type(), bjac::Type::kI64) << foo;
    ASSERT_EQ(std::next(bb.begin(), 1)->get_opcode(), bjac::Instruction::Opcode::kConst);
    EXPECT_EQ(static_cast<bjac::ConstInstruction &>(*std::next(bb.begin(), 1)).get_value(),
              GetParam().rhs)
        << foo;

    EXPECT_EQ(std::next(bb.begin(), 2)->get_type(), bjac::Type::kI1) << foo;
    ASSERT_EQ(std::next(bb.begin(), 2)->get_opcode(), bjac::Instruction::Opcode::kConst);
    EXPECT_EQ(static_cast<bjac::ConstInstruction &>(*std::next(bb.begin(), 2)).get_value(),
              GetParam().res)
        << foo;

    ASSERT_EQ(std::next(bb.begin(), 3)->get_opcode(), bjac::Instruction::Opcode::kRet) << foo;
    EXPECT_EQ(static_cast<bjac::ReturnInstruction &>(*std::next(bb.begin(), 3)).get_ret_value(),
              &static_cast<bjac::ConstInstruction &>(*std::next(bb.begin(), 2)))
        << foo;
}

INSTANTIATE_TEST_SUITE_P(
    /* no instantiation name */, ConstantFoldingForICmp,
    ::testing::Values(ICmpParam(bjac::ICmpInstruction::Kind::eq, 1, 1, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::eq, 1, 2, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::eq, 2, 1, false),

                      ICmpParam(bjac::ICmpInstruction::Kind::ne, 1, 1, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::ne, 1, 2, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::ne, 2, 1, true),

                      ICmpParam(bjac::ICmpInstruction::Kind::ugt, 1, 1, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::ugt, 1, 2, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::ugt, 2, 1, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::ugt, 1, -1, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::ugt, -1, 1, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::ugt, -1, -2, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::ugt, -2, -1, false),

                      ICmpParam(bjac::ICmpInstruction::Kind::uge, 1, 1, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::uge, 1, 2, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::uge, 2, 1, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::uge, 1, -1, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::uge, -1, 1, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::uge, -1, -2, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::uge, -2, -1, false),

                      ICmpParam(bjac::ICmpInstruction::Kind::ult, 1, 1, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::ult, 1, 2, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::ult, 2, 1, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::ult, 1, -1, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::ult, -1, 1, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::ult, -1, -2, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::ult, -2, -1, true),

                      ICmpParam(bjac::ICmpInstruction::Kind::ule, 1, 1, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::ule, 2, 1, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::ule, 1, 2, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::ule, 1, -1, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::ule, -1, 1, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::ule, -1, -2, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::ule, -2, -1, true),

                      ICmpParam(bjac::ICmpInstruction::Kind::sgt, 1, 1, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::sgt, 1, 2, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::sgt, 2, 1, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::sgt, 1, -1, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::sgt, -1, 1, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::sgt, -1, -2, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::sgt, -2, -1, false),

                      ICmpParam(bjac::ICmpInstruction::Kind::sge, 1, 1, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::sge, 2, 1, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::sge, 1, 2, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::sge, 1, -1, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::sge, -1, 1, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::sge, -1, -2, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::sge, -2, -1, false),

                      ICmpParam(bjac::ICmpInstruction::Kind::slt, 1, 1, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::slt, 1, 2, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::slt, 2, 1, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::slt, 1, -1, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::slt, -1, 1, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::slt, -1, -2, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::slt, -2, -1, true),

                      ICmpParam(bjac::ICmpInstruction::Kind::sle, 1, 1, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::sle, 1, 2, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::sle, 2, 1, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::sle, 1, -1, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::sle, -1, 1, true),
                      ICmpParam(bjac::ICmpInstruction::Kind::sle, -1, -2, false),
                      ICmpParam(bjac::ICmpInstruction::Kind::sle, -2, -1, true)),
    [](const ::testing::TestParamInfo<ICmpParam> &info) static {
        return std::format("{}_{}_{}", info.param.kind, info.param.lhs, info.param.rhs);
    });

TEST(ConstantFolding, Combination) {
    // Assign
    bjac::Function foo{"foo", bjac::Type::kI64, {}};

    auto &bb = foo.emplace_back();
    auto &const_1 = bb.emplace_back<bjac::ConstInstruction>(bjac::Type::kI64, 1);
    auto &const_2 = bb.emplace_back<bjac::ConstInstruction>(bjac::Type::kI64, 2);
    auto &add =
        bb.emplace_back<bjac::BinaryOperator>(bjac::Instruction::Opcode::kAdd, const_1, const_2);
    auto &const_3 = bb.emplace_back<bjac::ConstInstruction>(bjac::Type::kI64, 8);
    auto &const_4 = bb.emplace_back<bjac::ConstInstruction>(bjac::Type::kI64, 3);
    auto &sub =
        bb.emplace_back<bjac::BinaryOperator>(bjac::Instruction::Opcode::kSub, const_3, const_4);
    auto &mul = bb.emplace_back<bjac::BinaryOperator>(bjac::Instruction::Opcode::kMul, add, sub);
    bb.emplace_back<bjac::ReturnInstruction>(mul);

    // Act
    bjac::ConstantFoldingPass{}.run(foo);

    // Assert
    EXPECT_EQ(bb.size(), 8) << foo;

    EXPECT_EQ(std::next(bb.begin(), 6)->get_type(), bjac::Type::kI64) << foo;
    ASSERT_EQ(std::next(bb.begin(), 6)->get_opcode(), bjac::Instruction::Opcode::kConst) << foo;
    EXPECT_EQ(static_cast<bjac::ConstInstruction &>(*std::next(bb.begin(), 6)).get_value(),
              (1 + 2) * (8 - 3))
        << foo;

    ASSERT_EQ(std::next(bb.begin(), 7)->get_opcode(), bjac::Instruction::Opcode::kRet) << foo;
    EXPECT_EQ(static_cast<bjac::ReturnInstruction &>(*std::next(bb.begin(), 7)).get_ret_value(),
              &static_cast<bjac::ConstInstruction &>(*std::next(bb.begin(), 6)))
        << foo;
}
