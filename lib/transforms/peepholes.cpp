#include <cassert>
#include <functional>
#include <iterator>
#include <ranges>
#include <utility>

#include "bjac/graphs/dfs.hpp"

#include "bjac/IR/basic_block.hpp"
#include "bjac/IR/binary_operator.hpp"
#include "bjac/IR/constant_instruction.hpp"
#include "bjac/IR/function.hpp"

#include "bjac/transforms/peepholes.hpp"

namespace bjac {

namespace {

/*
 * Case 1:
 * ----------------------------------------------
 * Before:
 * %0.0 = <some-instruction> <---- rhs_rhs
 * %0.1 = i64 constant 42    <---- rhs_lhs
 * %0.2 = i64 and %0.1, %0.0 <---- rhs
 * %0.3 = i64 constant 43    <---- lhs
 * %0.4 = i64 and %0.3, %0.2 <---- bit_instr
 *
 * After:
 * %0.0 = <some-instruction>
 * %0.1 = i64 constant 42
 * %0.2 = i64 and %0.1, %0.0
 * %0.3 = i64 constant 43
 * %0.5 = i64 constant (42 & 43)
 * %0.4 = i64 and %0.0, %0.5
 * ----------------------------------------------
 *
 * Case 2:
 * ----------------------------------------------
 * Before:
 * %0.0 = <some-instruction> <---- rhs_lhs
 * %0.1 = i64 constant 42    <---- rhs_rhs
 * %0.2 = i64 and %0.0, %0.1 <---- rhs
 * %0.3 = i64 constant 43    <---- lhs
 * %0.4 = i64 and %0.3, %0.2 <---- bit_instr
 *
 * After:
 * %0.0 = <some-instruction>
 * %0.1 = i64 constant 42
 * %0.2 = i64 and %0.0, %0.1
 * %0.3 = i64 constant 43
 * %0.5 = i64 constant (42 & 43)
 * %0.4 = i64 and %0.0, %0.5
 * ----------------------------------------------
 *
 * Case 3:
 * ----------------------------------------------
 * Before:
 * %0.0 = <some-instruction> <---- lhs_rhs
 * %0.1 = i64 constant 42    <---- lhs_lhs
 * %0.2 = i64 and %0.1, %0.0 <---- lhs
 * %0.3 = i64 constant 43    <---- rhs
 * %0.4 = i64 and %0.2, %0.3 <---- bit_instr
 *
 * After:
 * %0.0 = <some-instruction>
 * %0.1 = i64 constant 42
 * %0.2 = i64 and %0.1, %0.0
 * %0.3 = i64 constant 43
 * %0.5 = i64 constant (42 & 43)
 * %0.4 = i64 and %0.0, %0.5
 * ----------------------------------------------
 *
 * Case 4:
 * ----------------------------------------------
 * Before:
 * %0.0 = <some-instruction> <---- lhs_lhs
 * %0.1 = i64 constant 42    <---- lhs_rhs
 * %0.2 = i64 and %0.0, %0.1 <---- lhs
 * %0.3 = i64 constant 43    <---- rhs
 * %0.4 = i64 and %0.2, %0.3 <---- bit_instr
 *
 * After:
 * %0.0 = <some-instruction>
 * %0.1 = i64 constant 42
 * %0.2 = i64 and %0.0, %0.1
 * %0.3 = i64 constant 43
 * %0.5 = i64 constant (42 & 43)
 * %0.4 = i64 and %0.0, %0.5
 * ----------------------------------------------
 */
void bitwise_instr_chaining(std::bidirectional_iterator auto it, auto op) {
    using enum Instruction::Opcode;

    assert(it->get_opcode() == kAnd || it->get_opcode() == kOr || it->get_opcode() == kXor);

    auto *bit_instr = static_cast<BinaryOperator *>(std::addressof(*it));
    auto *lhs = bit_instr->get_lhs();
    auto *rhs = bit_instr->get_rhs();

    auto [instr, constant] = [lhs, rhs, op] -> std::pair<Instruction *, std::uintmax_t> {
        if (lhs->get_opcode() == kConst) {
            if (rhs->get_opcode() == kAnd) {
                auto *rhs_lhs = static_cast<BinaryOperator *>(rhs)->get_lhs();
                auto *rhs_rhs = static_cast<BinaryOperator *>(rhs)->get_rhs();
                if (rhs_lhs->get_opcode() == kConst) {
                    return {rhs_rhs, op(static_cast<ConstInstruction *>(lhs)->get_value(),
                                        static_cast<ConstInstruction *>(rhs_lhs)->get_value())};
                } else if (rhs_rhs->get_opcode() == kConst) {
                    return {rhs_lhs, op(static_cast<ConstInstruction *>(lhs)->get_value(),
                                        static_cast<ConstInstruction *>(rhs_rhs)->get_value())};
                }
            }
        } else if (rhs->get_opcode() == kConst) {
            if (lhs->get_opcode() == kAnd) {
                auto *lhs_lhs = static_cast<BinaryOperator *>(lhs)->get_lhs();
                auto *lhs_rhs = static_cast<BinaryOperator *>(lhs)->get_rhs();
                if (lhs_lhs->get_opcode() == kConst) {
                    return {lhs_rhs, op(static_cast<ConstInstruction *>(rhs)->get_value(),
                                        static_cast<ConstInstruction *>(lhs_lhs)->get_value())};
                } else if (lhs_rhs->get_opcode() == kConst) {
                    return {lhs_lhs, op(static_cast<ConstInstruction *>(rhs)->get_value(),
                                        static_cast<ConstInstruction *>(lhs_rhs)->get_value())};
                }
            }
        }

        return {nullptr, 0};
    }();

    if (instr != nullptr) {
        auto *bb = it->get_parent();
        assert(it != bb->begin());
        auto new_it =
            bb->template emplace<ConstInstruction>(std::prev(it), bit_instr->get_type(), constant);
        bit_instr->set_lhs(*instr);
        bit_instr->set_rhs(*new_it);
    }
}

auto process_and(std::bidirectional_iterator auto it) -> decltype(it) {
    using enum Instruction::Opcode;

    auto &and_instr = static_cast<BinaryOperator &>(*it);
    assert(and_instr.get_opcode() == kAnd);

    auto *bb = it->get_parent();
    auto next_it = std::next(it);

    auto *lhs = and_instr.get_lhs();
    auto *rhs = and_instr.get_rhs();

    if (lhs->get_opcode() == kConst) {
        auto *const_lhs = static_cast<ConstInstruction *>(lhs);
        if (const auto value = const_lhs->get_value(); value == 0) { // 0 & x -> 0
            bb->replace_instruction(it, *lhs);
        } else if (value == const_lhs->max_value()) { // 1..1 & x -> x
            bb->replace_instruction(it, *rhs);
        } else {
            bitwise_instr_chaining(it, std::bit_and{});
        }
    } else if (rhs->get_opcode() == kConst) {
        auto *const_rhs = static_cast<ConstInstruction *>(rhs);
        if (const auto value = const_rhs->get_value(); value == 0) { // x & 0 -> 0
            bb->replace_instruction(it, *rhs);
        } else if (value == const_rhs->max_value()) { // x & 1..1 -> x
            bb->replace_instruction(it, *lhs);
        } else {
            bitwise_instr_chaining(it, std::bit_and{});
        }
    } else if (lhs == rhs) { // x & x -> x
        bb->replace_instruction(it, *lhs);
    }

    return next_it;
}

auto process_or(std::bidirectional_iterator auto it) -> decltype(it) {
    using enum Instruction::Opcode;

    auto &or_instr = static_cast<BinaryOperator &>(*it);
    assert(or_instr.get_opcode() == kOr);

    auto *bb = it->get_parent();
    auto next_it = std::next(it);

    auto *lhs = or_instr.get_lhs();
    auto *rhs = or_instr.get_rhs();

    if (lhs->get_opcode() == kConst) {
        auto *const_lhs = static_cast<ConstInstruction *>(lhs);
        if (const auto value = const_lhs->get_value(); value == 0) { // 0 | x -> x
            bb->replace_instruction(it, *rhs);
        } else if (value == const_lhs->max_value()) { // 1..1 | x -> 1..1
            bb->replace_instruction(it, *lhs);
        } else {
            bitwise_instr_chaining(it, std::bit_or{});
        }
    } else if (rhs->get_opcode() == kConst) {
        auto *const_rhs = static_cast<ConstInstruction *>(rhs);
        if (const auto value = const_rhs->get_value(); value == 0) { // x | 0 -> x
            bb->replace_instruction(it, *lhs);
        } else if (value == const_rhs->max_value()) { // x | 1..1 -> 1..1
            bb->replace_instruction(it, *rhs);
        } else {
            bitwise_instr_chaining(it, std::bit_or{});
        }
    } else if (lhs == rhs) { // x | x -> x
        bb->replace_instruction(it, *lhs);
    }

    return next_it;
}

auto process_xor(std::bidirectional_iterator auto it) -> decltype(it) {
    using enum Instruction::Opcode;

    auto &xor_instr = static_cast<BinaryOperator &>(*it);
    assert(xor_instr.get_opcode() == kOr);

    auto *bb = it->get_parent();
    auto next_it = std::next(it);

    auto *lhs = xor_instr.get_lhs();
    auto *rhs = xor_instr.get_rhs();

    if (lhs->get_opcode() == kConst) {
        const auto value = static_cast<ConstInstruction *>(lhs)->get_value();
        if (value == 0) { // 0 ^ x -> x
            bb->replace_instruction(it, *rhs);
        } else {
            bitwise_instr_chaining(it, std::bit_or{});
        }
    } else if (rhs->get_opcode() == kConst) {
        const auto value = static_cast<ConstInstruction *>(rhs)->get_value();
        if (value == 0) { // x ^ 0 -> x
            bb->replace_instruction(it, *lhs);
        } else {
            bitwise_instr_chaining(it, std::bit_or{});
        }
    } else if (lhs == rhs) { // x ^ x -> 0
        auto new_it = bb->template emplace<ConstInstruction>(it, xor_instr.get_type(), 0);
        bb->replace_instruction(it, *new_it);
        return new_it;
    }

    return next_it;
}

auto process_instruction(std::bidirectional_iterator auto it) -> decltype(it) {
    using enum Instruction::Opcode;
    switch (it->get_opcode()) {
    case kAdd:
        return std::next(it);
    case kShrL:
        return std::next(it);
    case kAnd:
        return process_and(it);
    case kOr:
        return process_or(it);
    case kXor:
        return process_xor(it);
    default:
        return std::next(it);
    }
}

} // unnamed namespace

void PeepholePass::run(Function &f) {
    const DFS<MutFunctionGraphTraits> dfs{f};
    for (auto *bb : dfs.post_order() | std::views::reverse) {
        for (auto it = bb->begin(), ite = bb->end(); it != ite;) {
            it = process_instruction(it);
        }
    }
}

} // namespace bjac
