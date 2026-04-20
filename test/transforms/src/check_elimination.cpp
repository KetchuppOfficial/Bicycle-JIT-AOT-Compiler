#include <gtest/gtest.h>

#include "bjac/transforms/check_elimination.hpp"

#include "bjac/IR/argument_instruction.hpp"
#include "bjac/IR/binary_operator.hpp"
#include "bjac/IR/bounds_check.hpp"
#include "bjac/IR/branch_instruction.hpp"
#include "bjac/IR/constant_instruction.hpp"
#include "bjac/IR/function.hpp"
#include "bjac/IR/load_instruction.hpp"
#include "bjac/IR/null_check.hpp"
#include "bjac/IR/phi_instruction.hpp"
#include "bjac/IR/ret_instruction.hpp"

#include "test/common.hpp"

using enum bjac::Type::ID;
using enum bjac::Instruction::Opcode;

TEST(NullCheck, CannotEliminateSingleCheck) {
    // Assign
    std::vector<std::unique_ptr<bjac::Type>> parameters;
    parameters.emplace_back(get_ptr(kI64));
    bjac::Function foo{"foo", get_i64(), std::move(parameters)};

    auto &bb = foo.emplace_back();

    auto &addr = bb.emplace_back<bjac::ArgumentInstruction>(0);
    bb.emplace_back<bjac::NullCheckInstruction>(addr);
    auto &load = bb.emplace_back<bjac::LoadInstruction>(get_i64(), addr);
    bb.emplace_back<bjac::ReturnInstruction>(load);

    // Act
    bjac::CheckEliminationPass{}.run(foo);

    // Assert
    EXPECT_EQ(to_string(foo), "i64 foo(ptr)\n"
                              "%bb0:\n"
                              "    %0.0 = ptr arg [0]\n"
                              "    %0.1 null_check ptr %0.0\n"
                              "    %0.2 = load i64, ptr %0.0 ; used by: %0.3\n"
                              "    %0.3 ret i64 %0.2\n");
}

TEST(BoundsCheck, CannotEliminateSingleCheck) {
    // Assign
    std::vector<std::unique_ptr<bjac::Type>> parameters;
    parameters.emplace_back(get_array(kI64, 42));
    parameters.emplace_back(get_i64());
    bjac::Function foo{"foo", get_void(), std::move(parameters)};

    auto &bb = foo.emplace_back();

    auto &arr = bb.emplace_back<bjac::ArgumentInstruction>(0);
    auto &index = bb.emplace_back<bjac::ArgumentInstruction>(1);
    bb.emplace_back<bjac::BoundsCheckInstruction>(arr, index);
    bb.emplace_back<bjac::ReturnInstruction>();

    // Act
    bjac::CheckEliminationPass{}.run(foo);

    // Assert
    EXPECT_EQ(to_string(foo), "void foo([42 x i64], i64)\n"
                              "%bb0:\n"
                              "    %0.0 = [42 x i64] arg [0]\n"
                              "    %0.1 = i64 arg [1]\n"
                              "    %0.2 bounds_check [42 x i64] %0.0, i64 %0.1\n"
                              "    %0.3 ret void\n");
}

TEST(NullCheck, EliminateSimple) {
    // Assign
    std::vector<std::unique_ptr<bjac::Type>> parameters;
    parameters.emplace_back(get_ptr(kI64));
    bjac::Function foo{"foo", get_i64(), std::move(parameters)};

    auto &bb = foo.emplace_back();

    auto &addr = bb.emplace_back<bjac::ArgumentInstruction>(0);
    bb.emplace_back<bjac::NullCheckInstruction>(addr);
    auto &load_1 = bb.emplace_back<bjac::LoadInstruction>(get_i64(), addr);
    bb.emplace_back<bjac::NullCheckInstruction>(addr);
    auto &load_2 = bb.emplace_back<bjac::LoadInstruction>(get_i64(), addr);
    auto &res = bb.emplace_back<bjac::BinaryOperator>(kAdd, load_1, load_2);
    bb.emplace_back<bjac::ReturnInstruction>(res);

    // Act
    bjac::CheckEliminationPass{}.run(foo);

    // Assert
    EXPECT_EQ(to_string(foo), "i64 foo(ptr)\n"
                              "%bb0:\n"
                              "    %0.0 = ptr arg [0]\n"
                              "    %0.1 null_check ptr %0.0\n"
                              "    %0.2 = load i64, ptr %0.0 ; used by: %0.5\n"
                              "    %0.4 = load i64, ptr %0.0 ; used by: %0.5\n"
                              "    %0.5 = i64 add %0.2, %0.4 ; used by: %0.6\n"
                              "    %0.6 ret i64 %0.5\n");
}

TEST(BoundsCheck, EliminateSimple) {
    // Assign
    std::vector<std::unique_ptr<bjac::Type>> parameters;
    parameters.emplace_back(get_array(kI64, 42));
    parameters.emplace_back(get_i64());
    bjac::Function foo{"foo", get_void(), std::move(parameters)};

    auto &bb = foo.emplace_back();

    auto &arr = bb.emplace_back<bjac::ArgumentInstruction>(0);
    auto &index = bb.emplace_back<bjac::ArgumentInstruction>(1);
    bb.emplace_back<bjac::BoundsCheckInstruction>(arr, index);
    bb.emplace_back<bjac::BoundsCheckInstruction>(arr, index);
    bb.emplace_back<bjac::ReturnInstruction>();

    // Act
    bjac::CheckEliminationPass{}.run(foo);

    // Assert
    EXPECT_EQ(to_string(foo), "void foo([42 x i64], i64)\n"
                              "%bb0:\n"
                              "    %0.0 = [42 x i64] arg [0]\n"
                              "    %0.1 = i64 arg [1]\n"
                              "    %0.2 bounds_check [42 x i64] %0.0, i64 %0.1\n"
                              "    %0.4 ret void\n");
}

TEST(NullCheck, EliminateFromDominatedBlock) {
    // Assign
    std::vector<std::unique_ptr<bjac::Type>> parameters;
    parameters.emplace_back(get_ptr(kI64));
    parameters.emplace_back(get_i1());
    bjac::Function foo{"foo", get_i64(), std::move(parameters)};

    auto &bb_1 = foo.emplace_back();
    auto &bb_2 = foo.emplace_back();
    auto &bb_3 = foo.emplace_back();

    auto &addr = bb_1.emplace_back<bjac::ArgumentInstruction>(0);
    bb_1.emplace_back<bjac::NullCheckInstruction>(addr);
    auto &load_1 = bb_1.emplace_back<bjac::LoadInstruction>(get_i64(), addr);
    auto &cond = bb_1.emplace_back<bjac::ArgumentInstruction>(1);
    bb_1.emplace_back<bjac::BranchInstruction>(cond, bb_2, bb_3);

    bb_2.emplace_back<bjac::NullCheckInstruction>(addr);
    auto &load_2 = bb_2.emplace_back<bjac::LoadInstruction>(get_i64(), addr);
    auto &sum = bb_2.emplace_back<bjac::BinaryOperator>(kAdd, load_1, load_2);
    bb_2.emplace_back<bjac::BranchInstruction>(bb_3);

    auto &res = bb_3.emplace_back<bjac::PHIInstruction>(get_i64());
    bb_3.emplace_back<bjac::ReturnInstruction>(res);

    res.add_path(bb_1, load_1);
    res.add_path(bb_2, sum);

    // Act
    bjac::CheckEliminationPass{}.run(foo);

    // Assert
    EXPECT_EQ(to_string(foo), "i64 foo(ptr, i1)\n"
                              "%bb0:\n"
                              "    %0.0 = ptr arg [0]\n"
                              "    %0.1 null_check ptr %0.0\n"
                              "    %0.2 = load i64, ptr %0.0 ; used by: %1.2, %2.0\n"
                              "    %0.3 = i1 arg [1] ; used by: %0.4\n"
                              "    %0.4 br i1 %0.3, label %bb1, label %bb2\n"
                              "%bb1: ; preds: %bb0\n"
                              "    %1.1 = load i64, ptr %0.0 ; used by: %1.2\n"
                              "    %1.2 = i64 add %0.2, %1.1 ; used by: %2.0\n"
                              "    %1.3 br label %bb2\n"
                              "%bb2: ; preds: %bb0, %bb1\n"
                              "    %2.0 = phi i64 [%0.2, %bb0], [%1.2, %bb1] ; used by: %2.1\n"
                              "    %2.1 ret i64 %2.0\n");
}

TEST(BoundsCheck, EliminateFromDominatedBlock) {
    // Assign
    std::vector<std::unique_ptr<bjac::Type>> parameters;
    parameters.emplace_back(get_array(kI64, 42));
    parameters.emplace_back(get_i64());
    parameters.emplace_back(get_i1());
    bjac::Function foo{"foo", get_void(), std::move(parameters)};

    auto &bb_1 = foo.emplace_back();
    auto &bb_2 = foo.emplace_back();
    auto &bb_3 = foo.emplace_back();

    auto &arr = bb_1.emplace_back<bjac::ArgumentInstruction>(0);
    auto &index = bb_1.emplace_back<bjac::ArgumentInstruction>(1);
    auto &cond = bb_1.emplace_back<bjac::ArgumentInstruction>(2);
    bb_1.emplace_back<bjac::BoundsCheckInstruction>(arr, index);
    bb_1.emplace_back<bjac::BranchInstruction>(cond, bb_2, bb_3);

    bb_2.emplace_back<bjac::BoundsCheckInstruction>(arr, index);
    bb_2.emplace_back<bjac::BranchInstruction>(bb_3);

    bb_3.emplace_back<bjac::ReturnInstruction>();

    // Act
    bjac::CheckEliminationPass{}.run(foo);

    // Assert
    EXPECT_EQ(to_string(foo), "void foo([42 x i64], i64, i1)\n"
                              "%bb0:\n"
                              "    %0.0 = [42 x i64] arg [0]\n"
                              "    %0.1 = i64 arg [1]\n"
                              "    %0.2 = i1 arg [2] ; used by: %0.4\n"
                              "    %0.3 bounds_check [42 x i64] %0.0, i64 %0.1\n"
                              "    %0.4 br i1 %0.2, label %bb1, label %bb2\n"
                              "%bb1: ; preds: %bb0\n"
                              "    %1.1 br label %bb2\n"
                              "%bb2: ; preds: %bb0, %bb1\n"
                              "    %2.0 ret void\n");
}

TEST(NullCheck, CannotEliminateUnrelatedChecks) {
    // Assign
    std::vector<std::unique_ptr<bjac::Type>> parameters;
    parameters.emplace_back(get_ptr(kI64));
    parameters.emplace_back(get_ptr(kI64));
    bjac::Function foo{"foo", get_i64(), std::move(parameters)};

    auto &bb = foo.emplace_back();

    auto &addr_0 = bb.emplace_back<bjac::ArgumentInstruction>(0);
    bb.emplace_back<bjac::NullCheckInstruction>(addr_0);
    auto &load_0 = bb.emplace_back<bjac::LoadInstruction>(get_i64(), addr_0);
    auto &addr_1 = bb.emplace_back<bjac::ArgumentInstruction>(1);
    bb.emplace_back<bjac::NullCheckInstruction>(addr_1);
    auto &load_1 = bb.emplace_back<bjac::LoadInstruction>(get_i64(), addr_1);
    auto &sum = bb.emplace_back<bjac::BinaryOperator>(kAdd, load_0, load_1);
    bb.emplace_back<bjac::ReturnInstruction>(sum);

    // Act
    bjac::CheckEliminationPass{}.run(foo);

    // Assert
    EXPECT_EQ(to_string(foo), "i64 foo(ptr, ptr)\n"
                              "%bb0:\n"
                              "    %0.0 = ptr arg [0]\n"
                              "    %0.1 null_check ptr %0.0\n"
                              "    %0.2 = load i64, ptr %0.0 ; used by: %0.6\n"
                              "    %0.3 = ptr arg [1]\n"
                              "    %0.4 null_check ptr %0.3\n"
                              "    %0.5 = load i64, ptr %0.3 ; used by: %0.6\n"
                              "    %0.6 = i64 add %0.2, %0.5 ; used by: %0.7\n"
                              "    %0.7 ret i64 %0.6\n");
}

TEST(BoundsCheck, CannotEliminateUnrelatedChecks) {
    // Assign
    std::vector<std::unique_ptr<bjac::Type>> parameters;
    parameters.emplace_back(get_array(kI64, 42));
    parameters.emplace_back(get_i64());
    parameters.emplace_back(get_i64());
    bjac::Function foo{"foo", get_void(), std::move(parameters)};

    auto &bb = foo.emplace_back();

    auto &arr = bb.emplace_back<bjac::ArgumentInstruction>(0);
    auto &index_1 = bb.emplace_back<bjac::ArgumentInstruction>(1);
    auto &index_2 = bb.emplace_back<bjac::ArgumentInstruction>(2);
    bb.emplace_back<bjac::BoundsCheckInstruction>(arr, index_1);
    bb.emplace_back<bjac::BoundsCheckInstruction>(arr, index_2);
    bb.emplace_back<bjac::ReturnInstruction>();

    // Act
    bjac::CheckEliminationPass{}.run(foo);

    // Assert
    EXPECT_EQ(to_string(foo), "void foo([42 x i64], i64, i64)\n"
                              "%bb0:\n"
                              "    %0.0 = [42 x i64] arg [0]\n"
                              "    %0.1 = i64 arg [1]\n"
                              "    %0.2 = i64 arg [2]\n"
                              "    %0.3 bounds_check [42 x i64] %0.0, i64 %0.1\n"
                              "    %0.4 bounds_check [42 x i64] %0.0, i64 %0.2\n"
                              "    %0.5 ret void\n");
}

TEST(NullCheck, CannotEliminateChecksFromUnrelatedBasicBlocks) {
    // Assign
    std::vector<std::unique_ptr<bjac::Type>> parameters;
    parameters.emplace_back(get_ptr(kI64));
    parameters.emplace_back(get_i1());
    bjac::Function foo{"foo", get_i64(), std::move(parameters)};

    auto &bb_1 = foo.emplace_back();
    auto &bb_2 = foo.emplace_back();
    auto &bb_3 = foo.emplace_back();

    auto &addr = bb_1.emplace_back<bjac::ArgumentInstruction>(0);
    auto &cond = bb_1.emplace_back<bjac::ArgumentInstruction>(1);
    auto &constant = bb_1.emplace_back<bjac::ConstInstruction>(get_i64(), 42);
    bb_1.emplace_back<bjac::BranchInstruction>(cond, bb_2, bb_3);

    bb_2.emplace_back<bjac::NullCheckInstruction>(addr);
    auto &load_1 = bb_2.emplace_back<bjac::LoadInstruction>(get_i64(), addr);
    bb_2.emplace_back<bjac::BranchInstruction>(bb_3);

    auto &first_arg = bb_3.emplace_back<bjac::PHIInstruction>(get_i64());
    bb_3.emplace_back<bjac::NullCheckInstruction>(addr);
    auto &load_2 = bb_3.emplace_back<bjac::LoadInstruction>(get_i64(), addr);
    auto &res = bb_3.emplace_back<bjac::BinaryOperator>(kAdd, first_arg, load_2);
    bb_3.emplace_back<bjac::ReturnInstruction>(res);

    first_arg.add_path(bb_1, constant);
    first_arg.add_path(bb_2, load_1);

    // Act
    bjac::CheckEliminationPass{}.run(foo);

    // Assert
    EXPECT_EQ(to_string(foo), "i64 foo(ptr, i1)\n"
                              "%bb0:\n"
                              "    %0.0 = ptr arg [0]\n"
                              "    %0.1 = i1 arg [1] ; used by: %0.3\n"
                              "    %0.2 = i64 constant 42 ; used by: %2.0\n"
                              "    %0.3 br i1 %0.1, label %bb1, label %bb2\n"
                              "%bb1: ; preds: %bb0\n"
                              "    %1.0 null_check ptr %0.0\n"
                              "    %1.1 = load i64, ptr %0.0 ; used by: %2.0\n"
                              "    %1.2 br label %bb2\n"
                              "%bb2: ; preds: %bb0, %bb1\n"
                              "    %2.0 = phi i64 [%0.2, %bb0], [%1.1, %bb1] ; used by: %2.3\n"
                              "    %2.1 null_check ptr %0.0\n"
                              "    %2.2 = load i64, ptr %0.0 ; used by: %2.3\n"
                              "    %2.3 = i64 add %2.0, %2.2 ; used by: %2.4\n"
                              "    %2.4 ret i64 %2.3\n");
}

TEST(BoundsCheck, CannotEliminateChecksFromUnrelatedBasicBlocks) {
    // Assign
    std::vector<std::unique_ptr<bjac::Type>> parameters;
    parameters.emplace_back(get_array(kI64, 42));
    parameters.emplace_back(get_i64());
    parameters.emplace_back(get_i1());
    bjac::Function foo{"foo", get_void(), std::move(parameters)};

    auto &bb_1 = foo.emplace_back();
    auto &bb_2 = foo.emplace_back();
    auto &bb_3 = foo.emplace_back();

    auto &arr = bb_1.emplace_back<bjac::ArgumentInstruction>(0);
    auto &index = bb_1.emplace_back<bjac::ArgumentInstruction>(1);
    auto &cond = bb_1.emplace_back<bjac::ArgumentInstruction>(2);
    bb_1.emplace_back<bjac::BranchInstruction>(cond, bb_2, bb_3);

    bb_2.emplace_back<bjac::BoundsCheckInstruction>(arr, index);
    bb_2.emplace_back<bjac::BranchInstruction>(bb_3);

    bb_3.emplace_back<bjac::BoundsCheckInstruction>(arr, index);
    bb_3.emplace_back<bjac::ReturnInstruction>();

    // Act
    bjac::CheckEliminationPass{}.run(foo);

    // Assert
    EXPECT_EQ(to_string(foo), "void foo([42 x i64], i64, i1)\n"
                              "%bb0:\n"
                              "    %0.0 = [42 x i64] arg [0]\n"
                              "    %0.1 = i64 arg [1]\n"
                              "    %0.2 = i1 arg [2] ; used by: %0.3\n"
                              "    %0.3 br i1 %0.2, label %bb1, label %bb2\n"
                              "%bb1: ; preds: %bb0\n"
                              "    %1.0 bounds_check [42 x i64] %0.0, i64 %0.1\n"
                              "    %1.1 br label %bb2\n"
                              "%bb2: ; preds: %bb0, %bb1\n"
                              "    %2.0 bounds_check [42 x i64] %0.0, i64 %0.1\n"
                              "    %2.1 ret void\n");
}
