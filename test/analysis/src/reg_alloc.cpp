#include <stdexcept>

#include <gtest/gtest.h>

#include "bjac/analysis/reg_alloc.hpp"

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
using Storage = bjac::RegAlloc::Storage;
using enum Storage::Kind;

TEST(RegAlloc, ThrowOneRequestForZeroRegisters) {
    // Assign
    const bjac::Function foo = get_func("foo", kVoid);

    // Act & Assert
    EXPECT_THROW(bjac::RegAlloc(foo, 0), std::invalid_argument);
}

/* i64 foo(i64)
 * %bb0:
 *     %0.0 = i64 arg [0]                        ; register[0]
 *     %0.1 = i64 constant 0                     ; register[1]
 *     %0.2 = i64 constant 1                     ; stack[0]
 *     %0.3 br label %bb1
 * %bb1: ; preds: %bb0, %bb2
 *     %1.0 = phi i64 [%0.1, %bb0], [%2.0, %bb2] ; stack[1]
 *     %1.1 = icmp ult i64 %1.0, %0.0            ; register[1]
 *     %1.2 br i1 %1.1, label %bb2, label %bb3
 * %bb2: ; preds: %bb1
 *     %2.0 = i64 add %1.0, %0.2                 ; register[1]
 *     %2.1 br label %bb1
 * %bb3: ; preds: %bb1
 *     %3.0 ret i64 %1.0
 */
TEST(RegAlloc, Loop1) {
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
    const bjac::RegAlloc allocator{foo, 2};

    // Assert
    EXPECT_EQ(allocator.at(arg), Storage(kRegister, 0));
    EXPECT_EQ(allocator.at(i), Storage(kRegister, 1));
    EXPECT_EQ(allocator.at(one), Storage(kStackSlot, 0));
    EXPECT_EQ(allocator.at(phi), Storage(kStackSlot, 1));
    EXPECT_EQ(allocator.at(cond), Storage(kRegister, 1));
    EXPECT_EQ(allocator.at(add), Storage(kRegister, 1));
}

/*
 * i64 foo(i64)
 * %bb0:
 *    %0.0 = i64 constant 0                      ; register[0]
 *    %0.1 = i64 constant 1                      ; stack[2]
 *    %0.2 = i64 constant 2                      ; stack[0]
 *    %0.3 = i64 arg [0]                         ; stack[1]
 *    %0.4 br label %bb1
 * %bb1: ; preds: %bb0, %bb5
 *     %1.0 = phi i64 [%0.0, %bb0], [%4.0, %bb5] ; register[0]
 *     %1.1 = i64 mul %1.0, %0.2                 ; register[0]
 *     %1.2 = icmp ult i64 %1.1, %0.3            ; register[1]
 *     %1.3 br i1 %1.2, label %bb2, label %bb3
 * %bb2: ; preds: %bb1
 *     %2.0 = i64 add %1.1, %0.1                 ; register[1]
 *     %2.1 br label %bb4
 * %bb3: ; preds: %bb1
 *     %3.0 = i64 add %1.1, %0.2                 ; register[1]
 *     %3.1 br label %bb4
 * %bb4: ; preds: %bb2, %bb3
 *     %4.0 = phi i64 [%2.0, %bb2], [%3.0, %bb3] ; register[1]
 *     %4.1 br label %bb5
 * %bb5: ; preds: %bb4
 *     %5.0 = icmp ult i64 %1.1, %0.3            ; register[0]
 *     %5.1 br i1 %5.0, label %bb6, label %bb1
 * %bb6: ; preds: %bb5
 *     %6.0 ret i64 %4.0
 */
TEST(RegAlloc, Loop2) {
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
    const bjac::RegAlloc allocator{foo, 2};

    // Assert
    EXPECT_EQ(allocator.at(zero), Storage(kRegister, 0));
    EXPECT_EQ(allocator.at(one), Storage(kStackSlot, 2));
    EXPECT_EQ(allocator.at(two), Storage(kStackSlot, 0));
    EXPECT_EQ(allocator.at(arg), Storage(kStackSlot, 1));
    EXPECT_EQ(allocator.at(b_phi), Storage(kRegister, 0));
    EXPECT_EQ(allocator.at(prod), Storage(kRegister, 0));
    EXPECT_EQ(allocator.at(b_cmp), Storage(kRegister, 1));
    EXPECT_EQ(allocator.at(c_prod), Storage(kRegister, 1));
    EXPECT_EQ(allocator.at(d_prod), Storage(kRegister, 1));
    EXPECT_EQ(allocator.at(e_phi), Storage(kRegister, 1));
    EXPECT_EQ(allocator.at(f_cmp), Storage(kRegister, 0));
}

/*
 * %bb0:
 *     %0.0 = i64 arg [0]                        ; register[0]
 *     %0.1 = i64 constant 0                     ; stack[0]
 *     %0.2 = i64 constant 1                     ; stack[1]
 *     %0.3 = icmp eq i64 %0.0, %0.1             ; register[0]
 *     %0.4 br i1 %0.3, label %bb1, label %bb2
 * %bb2: ; preds: %bb0, %bb3
 *     %2.0 = phi i64 [%0.1, %bb0], [%2.2, %bb3] ; register[0]
 *     %2.1 = phi i64 [%0.1, %bb0], [%2.3, %bb3] ; register[1]
 *     %2.2 = i64 add %2.0, %2.1                 ; register[0]
 *     %2.3 = i64 add %2.1, %0.2                 ; register[1]
 *     %2.4 br label %bb3
 * %bb3: ; preds: %bb2
 *     %3.0 br label %bb2
 * %bb1: ; preds: %bb0
 *     %1.0 ret i64 %0.1
 */
TEST(RegAlloc, Loop3) {
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
    const bjac::RegAlloc allocator{foo, 2};

    // Assert
    EXPECT_EQ(allocator.at(arg), Storage(kRegister, 0));
    EXPECT_EQ(allocator.at(zero), Storage(kStackSlot, 0));
    EXPECT_EQ(allocator.at(one), Storage(kStackSlot, 1));
    EXPECT_EQ(allocator.at(a_cmp), Storage(kRegister, 0));
    EXPECT_EQ(allocator.at(acc), Storage(kRegister, 0));
    EXPECT_EQ(allocator.at(counter), Storage(kRegister, 1));
    EXPECT_EQ(allocator.at(next_acc), Storage(kRegister, 0));
    EXPECT_EQ(allocator.at(next_counter), Storage(kRegister, 1));
}
