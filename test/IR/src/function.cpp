#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include "bjac/IR/argument_instruction.hpp"
#include "bjac/IR/binary_operator.hpp"
#include "bjac/IR/call_instruction.hpp"
#include "bjac/IR/constant_instruction.hpp"
#include "bjac/IR/function.hpp"
#include "bjac/IR/icmp_instruction.hpp"
#include "bjac/IR/ret_instruction.hpp"

#include "test/common.hpp"

using enum bjac::Type::ID;
using enum bjac::Instruction::Opcode;
using Kind = bjac::ICmpInstruction::Kind;

/*
 * Before
 * -------------------------
 * void foo()
 * %bb0:
 *     %0.0 ret void
 * -------------------------
 * void bar()
 * %bb0:
 *     %0.0 = call foo()
 *     %0.1 ret void
 * -------------------------
 */
TEST(Inline, BodylessFunction) {
    // Assign
    bjac::Function foo = get_func("foo", kVoid);
    auto &foo_bb = foo.emplace_back();
    foo_bb.emplace_back<bjac::ReturnInstruction>();

    bjac::Function bar = get_func("bar", kVoid);
    auto &bar_bb = bar.emplace_back();
    auto &call = bar_bb.emplace_back<bjac::CallInstruction>(foo);
    bar_bb.emplace_back<bjac::ReturnInstruction>();

    // Act
    bjac::Function::inline_at(call);

    // Assert
    EXPECT_EQ(to_string(foo), "void foo()\n"
                              "%bb0:\n"
                              "    %0.0 ret void\n");
    EXPECT_EQ(to_string(bar), "void bar()\n"
                              "%bb0:\n"
                              "    %0.2 br label %bb2\n"
                              "%bb2: ; preds: %bb0\n"
                              "    %2.0 br label %bb1\n"
                              "%bb1: ; preds: %bb2\n"
                              "    %0.1 ret void\n");
}

/*
 * Before
 * ---------------------------------------------
 *  i64 foo()
 * %bb0:
 *     %0.0 = i64 constant 42 ; used by: %0.1
 *     %0.1 ret i64 %0.0
 * ---------------------------------------------
 * i64 bar()
 * %bb0:
 *     %0.0 = i64 constant 1 ; used by: %0.2
 *     %0.1 = call foo()
 *     %0.2 = i64 add %0.0, %0.1 ; used by: %0.3
 *     %0.3 ret i64 %0.2
 * ---------------------------------------------
 */
TEST(Inline, ReturnConstant) {
    // Assign
    bjac::Function foo = get_func("foo", kI64);
    auto &foo_bb = foo.emplace_back();
    auto &foo_const = foo_bb.emplace_back<bjac::ConstInstruction>(get_i64(), 42);
    foo_bb.emplace_back<bjac::ReturnInstruction>(foo_const);

    bjac::Function bar = get_func("bar", kI64);
    auto &bar_bb = bar.emplace_back();
    auto &bar_const = bar_bb.emplace_back<bjac::ConstInstruction>(get_i64(), 1);
    auto &call = bar_bb.emplace_back<bjac::CallInstruction>(foo);
    auto &add = bar_bb.emplace_back<bjac::BinaryOperator>(kAdd, bar_const, call);
    bar_bb.emplace_back<bjac::ReturnInstruction>(add);

    // Act
    bjac::Function::inline_at(call);

    // Assert
    EXPECT_EQ(to_string(foo), "i64 foo()\n"
                              "%bb0:\n"
                              "    %0.0 = i64 constant 42 ; used by: %0.1\n"
                              "    %0.1 ret i64 %0.0\n");
    EXPECT_EQ(to_string(bar), "i64 bar()\n"
                              "%bb0:\n"
                              "    %0.0 = i64 constant 1 ; used by: %0.2\n"
                              "    %0.4 br label %bb2\n"
                              "%bb2: ; preds: %bb0\n"
                              "    %2.0 = i64 constant 42 ; used by: %0.2\n"
                              "    %2.1 br label %bb1\n"
                              "%bb1: ; preds: %bb2\n"
                              "    %0.2 = i64 add %0.0, %2.0 ; used by: %0.3\n"
                              "    %0.3 ret i64 %0.2\n");
}

/*
 * Before
 * --------------------------------------
 * i64 foo(i64)
 * %bb0:
 *     %0.0 = i64 arg [0] ; used by: %0.1
 *     %0.1 ret i64 %0.0
 * --------------------------------------
 * i64 bar()
 * %bb0:
 *     %0.0 = i64 constant 1
 *     %0.1 = call foo(%0.0)
 *     %0.2 ret i64 %0.1
 * --------------------------------------
 */
TEST(Inline, ReturnArgument) {
    // Assign
    bjac::Function foo = get_func("foo", kI64, {kI64});
    auto &foo_bb = foo.emplace_back();
    auto &arg = foo_bb.emplace_back<bjac::ArgumentInstruction>(0);
    foo_bb.emplace_back<bjac::ReturnInstruction>(arg);

    bjac::Function bar = get_func("bar", kI64);
    auto &bar_bb = bar.emplace_back();
    auto &one = bar_bb.emplace_back<bjac::ConstInstruction>(get_i64(), 1);
    auto &call =
        bar_bb.emplace_back<bjac::CallInstruction>(foo, std::vector<bjac::Instruction *>{&one});
    bar_bb.emplace_back<bjac::ReturnInstruction>(call);

    // Act
    bjac::Function::inline_at(call);

    // Assert
    EXPECT_EQ(to_string(foo), "i64 foo(i64)\n"
                              "%bb0:\n"
                              "    %0.0 = i64 arg [0] ; used by: %0.1\n"
                              "    %0.1 ret i64 %0.0\n");
    EXPECT_EQ(to_string(bar), "i64 bar()\n"
                              "%bb0:\n"
                              "    %0.0 = i64 constant 1 ; used by: %0.2\n"
                              "    %0.3 br label %bb2\n"
                              "%bb2: ; preds: %bb0\n"
                              "    %2.0 br label %bb1\n"
                              "%bb1: ; preds: %bb2\n"
                              "    %0.2 ret i64 %0.0\n");
}

/*
 * Before
 * --------------------------------------------------
 * i64 max(i64, i64)
 * %bb0:
 *     %0.0 = i64 arg [0] ; used by: %0.2, %2.0
 *     %0.1 = i64 arg [1] ; used by: %0.2, %1.0
 *     %0.2 = icmp slt i64 %0.0, %0.1 ; used by: %0.3
 *     %0.3 br i1 %0.2, label %bb1, label %bb2
 * %bb1: ; preds: %bb0
 *     %1.0 ret i64 %0.1
 * %bb2: ; preds: %bb0
 *     %2.0 ret i64 %0.0
 * --------------------------------------------------
 * i64 foo()
 * %bb0:
 *     %0.0 = i64 constant 1
 *     %0.1 = i64 constant 2
 *     %0.2 = call max(%0.0, %0.1)
 *     %0.3 ret i64 %0.2
 * --------------------------------------------------
 */
TEST(Inline, Max) {
    // Assign
    // ---------------------------------------------------------------------------------------------
    bjac::Function max = get_func("max", kI64, {kI64, kI64});
    auto &first_bb = max.emplace_back();
    auto &second_bb = max.emplace_back();
    auto &third_bb = max.emplace_back();

    auto &x = first_bb.emplace_back<bjac::ArgumentInstruction>(0);
    auto &y = first_bb.emplace_back<bjac::ArgumentInstruction>(1);
    auto &cond = first_bb.emplace_back<bjac::ICmpInstruction>(Kind::slt, x, y);
    first_bb.emplace_back<bjac::BranchInstruction>(cond, second_bb, third_bb);

    second_bb.emplace_back<bjac::ReturnInstruction>(y);

    third_bb.emplace_back<bjac::ReturnInstruction>(x);
    // ---------------------------------------------------------------------------------------------
    bjac::Function foo = get_func("foo", kI64);
    auto &foo_bb = foo.emplace_back();
    auto &one = foo_bb.emplace_back<bjac::ConstInstruction>(get_i64(), 1);
    auto &two = foo_bb.emplace_back<bjac::ConstInstruction>(get_i64(), 2);
    auto &call = foo_bb.emplace_back<bjac::CallInstruction>(
        max, std::vector<bjac::Instruction *>{&one, &two});
    foo_bb.emplace_back<bjac::ReturnInstruction>(call);
    // ---------------------------------------------------------------------------------------------

    // Act
    bjac::Function::inline_at(call);

    // Assert
    EXPECT_EQ(to_string(max), "i64 max(i64, i64)\n"
                              "%bb0:\n"
                              "    %0.0 = i64 arg [0] ; used by: %0.2, %2.0\n"
                              "    %0.1 = i64 arg [1] ; used by: %0.2, %1.0\n"
                              "    %0.2 = icmp slt i64 %0.0, %0.1 ; used by: %0.3\n"
                              "    %0.3 br i1 %0.2, label %bb1, label %bb2\n"
                              "%bb1: ; preds: %bb0\n"
                              "    %1.0 ret i64 %0.1\n"
                              "%bb2: ; preds: %bb0\n"
                              "    %2.0 ret i64 %0.0\n");
    EXPECT_EQ(to_string(foo), "i64 foo()\n"
                              "%bb0:\n"
                              "    %0.0 = i64 constant 1 ; used by: %1.0, %2.0\n"
                              "    %0.1 = i64 constant 2 ; used by: %1.0, %2.0\n"
                              "    %0.4 br label %bb2\n"
                              "%bb2: ; preds: %bb0\n"
                              "    %2.0 = icmp slt i64 %0.0, %0.1 ; used by: %2.1\n"
                              "    %2.1 br i1 %2.0, label %bb3, label %bb4\n"
                              "%bb3: ; preds: %bb2\n"
                              "    %3.0 br label %bb1\n"
                              "%bb4: ; preds: %bb2\n"
                              "    %4.0 br label %bb1\n"
                              "%bb1: ; preds: %bb3, %bb4\n"
                              "    %1.0 = phi i64 [%0.1, %bb3], [%0.0, %bb4] ; used by: %0.3\n"
                              "    %0.3 ret i64 %1.0\n");
}

/*
 * Before
 * -------------------------------------------------------------------------
 * i64 foo()
 * %bb0:
 *     %0.0 = i64 constant 7
 *     %0.1 = call fibonacci(%0.0)
 *     %0.2 ret i64 %0.1
 * -------------------------------------------------------------------------
 * i64 fibonacci(i64)
 * %bb0:
 *     %0.0 = i64 arg [0] ; used by: %0.2, %2.5, %3.0
 *     %0.1 = i64 constant 2 ; used by: %0.2, %2.0
 *     %0.2 = icmp ult i64 %0.0, %0.1 ; used by: %0.3
 *     %0.3 br i1 %0.2, label %bb3, label %bb1
 * %bb1: ; preds: %bb0
 *     %1.0 = i64 constant 0 ; used by: %2.2
 *     %1.1 = i64 constant 1 ; used by: %2.1, %2.4
 *     %1.2 br label %bb2
 * %bb2: ; preds: %bb1, %bb2
 *     %2.0 = phi i64 [%0.1, %bb1], [%2.4, %bb2] ; used by: %2.4, %2.5
 *     %2.1 = phi i64 [%1.1, %bb1], [%2.3, %bb2] ; used by: %2.2, %2.3, %3.0
 *     %2.2 = phi i64 [%1.0, %bb1], [%2.1, %bb2] ; used by: %2.3
 *     %2.3 = i64 add %2.2, %2.1 ; used by: %2.1
 *     %2.4 = i64 add %2.0, %1.1 ; used by: %2.0
 *     %2.5 = icmp ule i64 %2.0, %0.0 ; used by: %2.6
 *     %2.6 br i1 %2.5, label %bb2, label %bb3
 * %bb3: ; preds: %bb0, %bb2
 *     %3.0 = phi i64 [%0.0, %bb0], [%2.1, %bb2] ; used by: %3.1
 *     %3.1 ret i64 %3.0
 * -------------------------------------------------------------------------
 */
TEST(Inline, Fibonacci) {
    // Assign
    // ---------------------------------------------------------------------------------------------
    bjac::Function fibonacci = get_func("fibonacci", kI64, {kI64});

    auto &bb1 = fibonacci.emplace_back();
    auto &bb2 = fibonacci.emplace_back();
    auto &bb3 = fibonacci.emplace_back();
    auto &bb4 = fibonacci.emplace_back();

    auto &arg = bb1.emplace_back<bjac::ArgumentInstruction>(0);
    auto &two = bb1.emplace_back<bjac::ConstInstruction>(get_i64(), 2);
    auto &bb1_cmp =
        bb1.emplace_back<bjac::ICmpInstruction>(bjac::ICmpInstruction::Kind::ult, arg, two);
    bb1.emplace_back<bjac::BranchInstruction>(bb1_cmp, bb4, bb2);

    auto &zero = bb2.emplace_back<bjac::ConstInstruction>(get_i64(), 0);
    auto &one = bb2.emplace_back<bjac::ConstInstruction>(get_i64(), 1);
    bb2.emplace_back<bjac::BranchInstruction>(bb3);

    auto &i = bb3.emplace_back<bjac::PHIInstruction>(get_i64());
    auto &second = bb3.emplace_back<bjac::PHIInstruction>(get_i64());
    auto &first = bb3.emplace_back<bjac::PHIInstruction>(get_i64());
    auto &third = bb3.emplace_back<bjac::BinaryOperator>(kAdd, first, second);
    auto &next_i = bb3.emplace_back<bjac::BinaryOperator>(kAdd, i, one);
    auto &bb3_cmp =
        bb3.emplace_back<bjac::ICmpInstruction>(bjac::ICmpInstruction::Kind::ule, i, arg);
    bb3.emplace_back<bjac::BranchInstruction>(bb3_cmp, bb3, bb4);
    i.add_path(bb2, two);
    i.add_path(bb3, next_i);
    second.add_path(bb2, one);
    second.add_path(bb3, third);
    first.add_path(bb2, zero);
    first.add_path(bb3, second);

    auto &ret_val = bb4.emplace_back<bjac::PHIInstruction>(get_i64());
    bb4.emplace_back<bjac::ReturnInstruction>(ret_val);
    ret_val.add_path(bb1, arg);
    ret_val.add_path(bb3, second);
    // ---------------------------------------------------------------------------------------------
    bjac::Function foo = get_func("foo", kI64);
    auto &bb = foo.emplace_back();
    auto &constant = bb.emplace_back<bjac::ConstInstruction>(get_i64(), 7);
    auto &call = bb.emplace_back<bjac::CallInstruction>(
        fibonacci, std::vector<bjac::Instruction *>{&constant});
    bb.emplace_back<bjac::ReturnInstruction>(call);
    // ---------------------------------------------------------------------------------------------

    // Act
    bjac::Function::inline_at(call);

    // Assert
    EXPECT_EQ(to_string(fibonacci),
              "i64 fibonacci(i64)\n"
              "%bb0:\n"
              "    %0.0 = i64 arg [0] ; used by: %0.2, %2.5, %3.0\n"
              "    %0.1 = i64 constant 2 ; used by: %0.2, %2.0\n"
              "    %0.2 = icmp ult i64 %0.0, %0.1 ; used by: %0.3\n"
              "    %0.3 br i1 %0.2, label %bb3, label %bb1\n"
              "%bb1: ; preds: %bb0\n"
              "    %1.0 = i64 constant 0 ; used by: %2.2\n"
              "    %1.1 = i64 constant 1 ; used by: %2.1, %2.4\n"
              "    %1.2 br label %bb2\n"
              "%bb2: ; preds: %bb1, %bb2\n"
              "    %2.0 = phi i64 [%0.1, %bb1], [%2.4, %bb2] ; used by: %2.4, %2.5\n"
              "    %2.1 = phi i64 [%1.1, %bb1], [%2.3, %bb2] ; used by: %2.2, %2.3, %3.0\n"
              "    %2.2 = phi i64 [%1.0, %bb1], [%2.1, %bb2] ; used by: %2.3\n"
              "    %2.3 = i64 add %2.2, %2.1 ; used by: %2.1\n"
              "    %2.4 = i64 add %2.0, %1.1 ; used by: %2.0\n"
              "    %2.5 = icmp ule i64 %2.0, %0.0 ; used by: %2.6\n"
              "    %2.6 br i1 %2.5, label %bb2, label %bb3\n"
              "%bb3: ; preds: %bb0, %bb2\n"
              "    %3.0 = phi i64 [%0.0, %bb0], [%2.1, %bb2] ; used by: %3.1\n"
              "    %3.1 ret i64 %3.0\n");

    EXPECT_EQ(to_string(foo),
              "i64 foo()\n"
              "%bb0:\n"
              "    %0.0 = i64 constant 7 ; used by: %2.1, %4.5, %5.0\n"
              "    %0.3 br label %bb2\n"
              "%bb2: ; preds: %bb0\n"
              "    %2.0 = i64 constant 2 ; used by: %2.1, %4.0\n"
              "    %2.1 = icmp ult i64 %0.0, %2.0 ; used by: %2.2\n"
              "    %2.2 br i1 %2.1, label %bb5, label %bb3\n"
              "%bb3: ; preds: %bb2\n"
              "    %3.0 = i64 constant 0 ; used by: %4.2\n"
              "    %3.1 = i64 constant 1 ; used by: %4.1, %4.4\n"
              "    %3.2 br label %bb4\n"
              "%bb4: ; preds: %bb3, %bb4\n"
              "    %4.0 = phi i64 [%2.0, %bb3], [%4.4, %bb4] ; used by: %4.4, %4.5\n"
              "    %4.1 = phi i64 [%3.1, %bb3], [%4.3, %bb4] ; used by: %4.2, %4.3, %5.0\n"
              "    %4.2 = phi i64 [%3.0, %bb3], [%4.1, %bb4] ; used by: %4.3\n"
              "    %4.3 = i64 add %4.2, %4.1 ; used by: %4.1\n"
              "    %4.4 = i64 add %4.0, %3.1 ; used by: %4.0\n"
              "    %4.5 = icmp ule i64 %4.0, %0.0 ; used by: %4.6\n"
              "    %4.6 br i1 %4.5, label %bb4, label %bb5\n"
              "%bb5: ; preds: %bb2, %bb4\n"
              "    %5.0 = phi i64 [%0.0, %bb2], [%4.1, %bb4] ; used by: %0.2\n"
              "    %5.1 br label %bb1\n"
              "%bb1: ; preds: %bb5\n"
              "    %0.2 ret i64 %5.0\n");
}

/*
 * Before
 * --------------------------------------------------
 * i64 factorial(i64)
 * %bb0:
 *     %0.0 = i64 arg [0] ; used by: %0.3, %2.0, %2.2
 *     %0.1 = i64 constant 1 ; used by: %1.0, %2.0
 *     %0.2 = i64 constant 2 ; used by: %0.3
 *     %0.3 = icmp ult i64 %0.0, %0.2 ; used by: %0.4
 *     %0.4 br i1 %0.3, label %bb1, label %bb2
 * %bb1: ; preds: %bb0
 *     %1.0 ret i64 %0.1
 * %bb2: ; preds: %bb0
 *     %2.0 = i64 sub %0.0, %0.1
 *     %2.1 = call factorial(%2.0)
 *     %2.2 = i64 mul %0.0, %2.1 ; used by: %2.3
 *     %2.3 ret i64 %2.2
 * --------------------------------------------------
 * i64 foo()
 * %bb0:
 *     %0.0 = i64 constant 7
 *     %0.1 = call factorial(%0.0)
 *     %0.2 ret i64 %0.1
 * --------------------------------------------------
 */
TEST(Inline, CannotInlineRecursiveFunction) {
    // Assign
    // ---------------------------------------------------------------------------------------------
    bjac::Function fact = get_func("factorial", kI64, {kI64});

    auto &first_bb = fact.emplace_back();
    auto &second_bb = fact.emplace_back();
    auto &third_bb = fact.emplace_back();

    auto &arg = first_bb.emplace_back<bjac::ArgumentInstruction>(0);
    auto &one = first_bb.emplace_back<bjac::ConstInstruction>(get_i64(), 1);
    auto &two = first_bb.emplace_back<bjac::ConstInstruction>(get_i64(), 2);
    auto &cond = first_bb.emplace_back<bjac::ICmpInstruction>(Kind::ult, arg, two);
    first_bb.emplace_back<bjac::BranchInstruction>(cond, second_bb, third_bb);

    second_bb.emplace_back<bjac::ReturnInstruction>(one);

    auto &arg_minus_one = third_bb.emplace_back<bjac::BinaryOperator>(kSub, arg, one);
    auto &next_fact = third_bb.emplace_back<bjac::CallInstruction>(
        fact, std::vector<bjac::Instruction *>{&arg_minus_one});
    auto &res = third_bb.emplace_back<bjac::BinaryOperator>(kMul, arg, next_fact);
    third_bb.emplace_back<bjac::ReturnInstruction>(res);
    // ---------------------------------------------------------------------------------------------
    bjac::Function foo = get_func("foo", kI64);
    auto &bb = foo.emplace_back();
    auto &constant = bb.emplace_back<bjac::ConstInstruction>(get_i64(), 7);
    auto &call =
        bb.emplace_back<bjac::CallInstruction>(fact, std::vector<bjac::Instruction *>{&constant});
    bb.emplace_back<bjac::ReturnInstruction>(call);
    // ---------------------------------------------------------------------------------------------

    // Act & Assert
    EXPECT_THROW(bjac::Function::inline_at(call), std::invalid_argument);
}
