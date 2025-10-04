#include <iostream>
#include <print>
#include <cstdlib>

#include "bjac/IR/type.hpp"

#include "bjac/IR/basic_block.hpp"
#include "bjac/IR/function.hpp"

#include "bjac/IR/argument_instruction.hpp"
#include "bjac/IR/binary_operator.hpp"
#include "bjac/IR/branch_instruction.hpp"
#include "bjac/IR/constant_instruction.hpp"
#include "bjac/IR/icmp_instruction.hpp"
#include "bjac/IR/phi_instruction.hpp"
#include "bjac/IR/ret_instruction.hpp"

/*
 * i64 fibonacci(i64) {
 * %bb1:
 *     %arg = arg 0
 *     %two = const i64 2
 *     %1 = icmp ult i64 %arg, %two
 *     br %1, label %bb4, label %bb2
 * %bb2:
 *     %zero = const i64 0
 *     %one = const i64 1
 *     br %bb3
 * %bb3:
 *     %i = phi [%two, %bb2], [%next_i, %bb3]
 *     %second = phi [%one, %bb2], [%third, %bb3]
 *     %first = phi [%zero, %bb2], [%second, %bb3]
 *     %third = add_path i64 %first, %second
 *     %next_i = add_path i64 %i, %one
 *     %2 = icmp ule i64 %i, %arg
 *     br %2, label %bb3, label %bb4
 * %bb4:
 *     %ret_val = phi [%arg, %bb1], [%second, %bb3]
 *     ret i64 %ret_val
 * }
 */
int main() try {
    using Opcode = bjac::Instruction::Opcode;
    using enum bjac::Type;

    bjac::Function fibonacci{"fibonacci", kI64, {kI64}};

    auto &bb1 = fibonacci.emplace_back();
    auto &bb2 = fibonacci.emplace_back();
    auto &bb3 = fibonacci.emplace_back();
    auto &bb4 = fibonacci.emplace_back();

    auto &arg = bb1.push_back(bjac::ArgumentInstruction::create(fibonacci, 0));
    auto &two = bb1.push_back(bjac::ConstInstruction::create(kI64, 2));
    auto &bb1_cmp =
        bb1.push_back(bjac::ICmpInstruction::create(bjac::ICmpInstruction::Kind::ult, arg, two));
    bb1.push_back(bjac::BranchInstruction::create(bb1_cmp, bb4, bb2));

    auto &zero = bb2.push_back(bjac::ConstInstruction::create(kI64, 0));
    auto &one = bb2.push_back(bjac::ConstInstruction::create(kI64, 1));
    bb2.push_back(bjac::BranchInstruction::create(bb3));

    auto &i =
        static_cast<bjac::PHIInstruction &>(bb3.push_back(bjac::PHIInstruction::create(kI64)));
    auto &second =
        static_cast<bjac::PHIInstruction &>(bb3.push_back(bjac::PHIInstruction::create(kI64)));
    auto &first =
        static_cast<bjac::PHIInstruction &>(bb3.push_back(bjac::PHIInstruction::create(kI64)));
    auto &third = bb3.push_back(bjac::BinaryOperator::create(Opcode::kAdd, first, second));
    auto &next_i = bb3.push_back(bjac::BinaryOperator::create(Opcode::kAdd, i, one));
    auto &bb3_cmp =
        bb3.push_back(bjac::ICmpInstruction::create(bjac::ICmpInstruction::Kind::ule, i, arg));
    bb3.push_back(bjac::BranchInstruction::create(bb3_cmp, bb3, bb4));
    i.add_path(bb2, two);
    i.add_path(bb3, next_i);
    second.add_path(bb2, one);
    second.add_path(bb3, third);
    first.add_path(bb2, zero);
    first.add_path(bb3, second);

    auto &ret_val =
        static_cast<bjac::PHIInstruction &>(bb4.push_back(bjac::PHIInstruction::create(kI64)));
    bb4.push_back(bjac::ReturnInstruction::create(ret_val));
    ret_val.add_path(bb1, arg);
    ret_val.add_path(bb3, second);

    fibonacci.print(std::cout);

    return 0;
} catch (const std::exception &e) {
    std::println(stderr, "Caught an instance of {}.\nwhat(): {}", typeid(e).name(), e.what());
    return EXIT_FAILURE;
} catch (...) {
    std::println(stderr, "Caught an unknown exception");
    return EXIT_FAILURE;
}
