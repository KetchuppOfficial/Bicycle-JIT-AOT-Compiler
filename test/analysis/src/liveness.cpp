#include <gtest/gtest.h>

#include "bjac/analysis/liveness.hpp"

#include "bjac/IR/argument_instruction.hpp"
#include "bjac/IR/binary_operator.hpp"
#include "bjac/IR/branch_instruction.hpp"
#include "bjac/IR/constant_instruction.hpp"
#include "bjac/IR/icmp_instruction.hpp"
#include "bjac/IR/phi_instruction.hpp"
#include "bjac/IR/ret_instruction.hpp"

#include "test/common.hpp"

using enum bjac::Instruction::Opcode;
using enum bjac::Type::ID;
using Segment = bjac::Lifetime::Segment;

/*   i64 foo(i64)
 *   %bb0:
 * 0     %0.0 = i64 arg [0] ; used by: %1.1
 * 1     %0.1 = i64 constant 0 ; used by: %1.0
 * 2     %0.2 = i64 constant 1 ; used by: %2.0
 * 3     %0.3 br label %bb1
 *   %bb1: ; preds: %bb0, %bb2
 * 4     %1.0 = phi i64 [%0.1, %bb0], [%2.0, %bb2] ; used by: %1.1, %2.0, %3.0
 * 5     %1.1 = icmp ult i64 %1.0, %0.0 ; used by: %1.2
 * 6     %1.2 br i1 %1.1, label %bb2, label %bb3
 *   %bb2: ; preds: %bb1
 * 7     %2.0 = i64 add %1.0, %0.2 ; used by: %1.0
 * 8     %2.1 br label %bb1
 *   %bb3: ; preds: %bb1
 * 9     %3.0 ret i64 %1.0
 */
TEST(Liveness, Loop1) {
    // Assign
    bjac::Function foo = get_func("foo", kI64, {kI64});
    auto [bb, names] = setup(foo, {'A', 'B', 'C', 'D'});

    auto &arg = bb.at('A')->emplace_back<bjac::ArgumentInstruction>(0);
    auto &i = bb.at('A')->emplace_back<bjac::ConstInstruction>(get_i64(), 0);
    auto &one = bb.at('A')->emplace_back<bjac::ConstInstruction>(get_i64(), 1);
    bb.at('A')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));

    auto &phi = bb.at('B')->emplace_back<bjac::PHIInstruction>(get_i64());
    auto &cond =
        bb.at('B')->emplace_back<bjac::ICmpInstruction>(bjac::ICmpInstruction::Kind::ult, phi, arg);
    bb.at('B')->emplace_back<bjac::BranchInstruction>(cond, *bb.at('C'), *bb.at('D'));

    auto &add = bb.at('C')->emplace_back<bjac::BinaryOperator>(kAdd, phi, one);
    bb.at('C')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));

    bb.at('D')->emplace_back<bjac::ReturnInstruction>(phi);
    phi.add_path(*bb.at('A'), i);
    phi.add_path(*bb.at('C'), add);

    // Act
    const bjac::LivenessAnalysis lifetimes{foo};

    // Assert
    EXPECT_EQ(lifetimes.at(arg), bjac::Lifetime{Segment(0, 8)});
    EXPECT_EQ(lifetimes.at(i), bjac::Lifetime{Segment(1, 3)});
    EXPECT_EQ(lifetimes.at(one), bjac::Lifetime{Segment(2, 8)});
    EXPECT_EQ(lifetimes.at(phi), bjac::Lifetime({Segment(4, 7), Segment(9, 9)}));
    EXPECT_EQ(lifetimes.at(cond), bjac::Lifetime{Segment(5, 6)});
    EXPECT_EQ(lifetimes.at(add), bjac::Lifetime{Segment(7, 8)});
}

/*
 *    i64 foo(i64)
 *    %bb0:
 *  0    %0.0 = i64 constant 0 ; used by: %1.0
 *  1    %0.1 = i64 constant 1 ; used by: %2.0
 *  2    %0.2 = i64 constant 2 ; used by: %1.1, %3.0
 *  3    %0.3 = i64 arg [0] ; used by: %1.2, %5.0
 *  4    %0.4 br label %bb1
 *    %bb1: ; preds: %bb0, %bb5
 *  5     %1.0 = phi i64 [%0.0, %bb0], [%4.0, %bb5] ; used by: %1.1
 *  6     %1.1 = i64 mul %1.0, %0.2 ; used by: %1.2, %2.0, %3.0, %5.0
 *  7     %1.2 = icmp ult i64 %1.1, %0.3 ; used by: %1.3
 *  8     %1.3 br i1 %1.2, label %bb2, label %bb3
 *    %bb2: ; preds: %bb1
 *  9     %2.0 = i64 add %1.1, %0.1 ; used by: %4.0
 * 10     %2.1 br label %bb4
 *    %bb3: ; preds: %bb1
 * 11     %3.0 = i64 add %1.1, %0.2 ; used by: %4.0
 * 12     %3.1 br label %bb4
 *    %bb4: ; preds: %bb2, %bb3
 * 13     %4.0 = phi i64 [%2.0, %bb2], [%3.0, %bb3] ; used by: %1.0, %6.0
 * 14     %4.1 br label %bb5
 *    %bb5: ; preds: %bb4
 * 15     %5.0 = icmp ult i64 %1.1, %0.3 ; used by: %5.1
 * 16     %5.1 br i1 %5.0, label %bb6, label %bb1
 *    %bb6: ; preds: %bb5
 * 17     %6.0 ret i64 %4.0
 */
TEST(Liveness, Loop2) {
    // Assign
    bjac::Function foo = get_func("foo", kI64, {kI64});
    auto [bb, names] = setup(foo, {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'});

    auto &zero = bb.at('A')->emplace_back<bjac::ConstInstruction>(get_i64(), 0);
    auto &one = bb.at('A')->emplace_back<bjac::ConstInstruction>(get_i64(), 1);
    auto &two = bb.at('A')->emplace_back<bjac::ConstInstruction>(get_i64(), 2);
    auto &arg = bb.at('A')->emplace_back<bjac::ArgumentInstruction>(0);
    bb.at('A')->emplace_back<bjac::BranchInstruction>(*bb.at('B'));

    auto &b_phi = bb.at('B')->emplace_back<bjac::PHIInstruction>(get_i64());
    auto &prod = bb.at('B')->emplace_back<bjac::BinaryOperator>(kMul, b_phi, two);
    auto &b_cmp = bb.at('B')->emplace_back<bjac::ICmpInstruction>(bjac::ICmpInstruction::Kind::ult,
                                                                  prod, arg);
    bb.at('B')->emplace_back<bjac::BranchInstruction>(b_cmp, *bb.at('C'), *bb.at('D'));

    auto &c_prod = bb.at('C')->emplace_back<bjac::BinaryOperator>(kAdd, prod, one);
    bb.at('C')->emplace_back<bjac::BranchInstruction>(*bb.at('E'));

    auto &d_prod = bb.at('D')->emplace_back<bjac::BinaryOperator>(kAdd, prod, two);
    bb.at('D')->emplace_back<bjac::BranchInstruction>(*bb.at('E'));

    auto &e_phi = bb.at('E')->emplace_back<bjac::PHIInstruction>(get_i64());
    bb.at('E')->emplace_back<bjac::BranchInstruction>(*bb.at('F'));

    auto &f_cmp = bb.at('F')->emplace_back<bjac::ICmpInstruction>(bjac::ICmpInstruction::Kind::ult,
                                                                  prod, arg);
    bb.at('F')->emplace_back<bjac::BranchInstruction>(f_cmp, *bb.at('G'), *bb.at('B'));

    bb.at('G')->emplace_back<bjac::ReturnInstruction>(e_phi);

    b_phi.add_path(*bb.at('A'), zero);
    b_phi.add_path(*bb.at('F'), e_phi);

    e_phi.add_path(*bb.at('C'), c_prod);
    e_phi.add_path(*bb.at('D'), d_prod);

    // Act
    const bjac::LivenessAnalysis lifetimes{foo};

    // Assert
    EXPECT_EQ(lifetimes.at(zero), bjac::Lifetime{Segment(0, 4)});
    EXPECT_EQ(lifetimes.at(one), bjac::Lifetime{Segment(1, 16)});
    EXPECT_EQ(lifetimes.at(two), bjac::Lifetime{Segment(2, 16)});
    EXPECT_EQ(lifetimes.at(arg), bjac::Lifetime{Segment(3, 16)});
    EXPECT_EQ(lifetimes.at(b_phi), bjac::Lifetime{Segment(5, 6)});
    EXPECT_EQ(lifetimes.at(prod), bjac::Lifetime{Segment(6, 15)});
    EXPECT_EQ(lifetimes.at(b_cmp), bjac::Lifetime{Segment(7, 8)});
    EXPECT_EQ(lifetimes.at(c_prod), bjac::Lifetime{Segment(9, 10)});
    EXPECT_EQ(lifetimes.at(d_prod), bjac::Lifetime{Segment(11, 12)});
    EXPECT_EQ(lifetimes.at(e_phi), bjac::Lifetime{Segment(13, 17)});
    EXPECT_EQ(lifetimes.at(f_cmp), bjac::Lifetime{Segment(15, 16)});
}

/*
 *    %bb0:
 *  0     %0.0 = i64 arg [0] ; used by: %0.3
 *  1     %0.1 = i64 constant 0 ; used by: %0.3, %1.0, %2.0, %2.1
 *  2     %0.2 = i64 constant 1 ; used by: %2.3
 *  3     %0.3 = icmp eq i64 %0.0, %0.1 ; used by: %0.4
 *  4     %0.4 br i1 %0.3, label %bb1, label %bb2
 *    %bb2: ; preds: %bb0, %bb3
 *  5     %2.0 = phi i64 [%0.1, %bb0], [%2.2, %bb3] ; used by: %2.2
 *  5     %2.1 = phi i64 [%0.1, %bb0], [%2.3, %bb3] ; used by: %2.2, %2.3
 *  6     %2.2 = i64 add %2.0, %2.1 ; used by: %2.0
 *  7     %2.3 = i64 add %2.1, %0.2 ; used by: %2.1
 *  8     %2.4 br label %bb3
 *    %bb3: ; preds: %bb2
 *  9     %3.0 br label %bb2
 *    %bb1: ; preds: %bb0
 * 10     %1.0 ret i64 %0.1
 */
TEST(Liveness, Loop3) {
    // Assign
    bjac::Function foo = get_func("foo", kI64, {kI64});
    auto [bb, names] = setup(foo, {'A', 'B', 'C', 'D'});

    auto &arg = bb.at('A')->emplace_back<bjac::ArgumentInstruction>(0);
    auto &zero = bb.at('A')->emplace_back<bjac::ConstInstruction>(get_i64(), 0);
    auto &one = bb.at('A')->emplace_back<bjac::ConstInstruction>(get_i64(), 1);
    auto &a_cmp =
        bb.at('A')->emplace_back<bjac::ICmpInstruction>(bjac::ICmpInstruction::Kind::eq, arg, zero);
    bb.at('A')->emplace_back<bjac::BranchInstruction>(a_cmp, *bb.at('B'), *bb.at('C'));

    bb.at('B')->emplace_back<bjac::ReturnInstruction>(zero);

    auto &acc = bb.at('C')->emplace_back<bjac::PHIInstruction>(get_i64());
    auto &counter = bb.at('C')->emplace_back<bjac::PHIInstruction>(get_i64());
    auto &next_acc = bb.at('C')->emplace_back<bjac::BinaryOperator>(kAdd, acc, counter);
    auto &next_counter = bb.at('C')->emplace_back<bjac::BinaryOperator>(kAdd, counter, one);
    bb.at('C')->emplace_back<bjac::BranchInstruction>(*bb.at('D'));

    bb.at('D')->emplace_back<bjac::BranchInstruction>(*bb.at('C'));

    acc.add_path(*bb.at('A'), zero);
    acc.add_path(*bb.at('D'), next_acc);

    counter.add_path(*bb.at('A'), zero);
    counter.add_path(*bb.at('D'), next_counter);

    // Act
    const bjac::LivenessAnalysis lifetimes{foo};

    // Assert
    EXPECT_EQ(lifetimes.at(arg), bjac::Lifetime{Segment(0, 3)});
    EXPECT_EQ(lifetimes.at(zero), bjac::Lifetime({Segment(1, 4), Segment(10, 10)}));
    EXPECT_EQ(lifetimes.at(one), bjac::Lifetime{Segment(2, 9)});
    EXPECT_EQ(lifetimes.at(a_cmp), bjac::Lifetime{Segment(3, 4)});
    EXPECT_EQ(lifetimes.at(acc), bjac::Lifetime{Segment(5, 6)});
    EXPECT_EQ(lifetimes.at(counter), bjac::Lifetime{Segment(5, 7)});
    EXPECT_EQ(lifetimes.at(next_acc), bjac::Lifetime({Segment(6, 9)}));
    EXPECT_EQ(lifetimes.at(next_counter), bjac::Lifetime({Segment(7, 9)}));
}
