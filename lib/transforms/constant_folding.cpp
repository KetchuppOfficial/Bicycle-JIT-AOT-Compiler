#include <cassert>
#include <cstdint>
#include <functional>
#include <optional>
#include <utility>

#include "bjac/transforms/constant_folding.hpp"

#include "bjac/IR/binary_operator.hpp"
#include "bjac/IR/branch_instruction.hpp"
#include "bjac/IR/constant_instruction.hpp"
#include "bjac/IR/icmp_instruction.hpp"

#include "bjac/graphs/dfs.hpp"

namespace bjac {

namespace {

template <typename F>
static std::uintmax_t apply_bin_op(F op, Type type, std::uintmax_t lhs, std::uintmax_t rhs) {
    switch (type) {
    case Type::kI1:
        return op(static_cast<bool>(lhs), static_cast<bool>(rhs));
    case Type::kI8:
        return op(static_cast<std::uint8_t>(lhs), static_cast<std::uint8_t>(rhs));
    case Type::kI16:
        return op(static_cast<std::uint16_t>(lhs), static_cast<std::uint16_t>(rhs));
    case Type::kI32:
        return op(static_cast<std::uint32_t>(lhs), static_cast<std::uint32_t>(rhs));
    case Type::kI64:
        return op(static_cast<std::uint64_t>(lhs), static_cast<std::uint64_t>(rhs));
    default:
        std::unreachable();
    }
}

static std::optional<std::uintmax_t> fold_binary_operator(const BinaryOperator &bin_op) {
    using enum Instruction::Opcode;

    assert(bin_op.get_lhs()->get_opcode() == kConst);
    assert(bin_op.get_rhs()->get_opcode() == kConst);

    Type type = bin_op.get_type();
    auto lhs = static_cast<const ConstInstruction *>(bin_op.get_lhs())->get_value();
    auto rhs = static_cast<const ConstInstruction *>(bin_op.get_rhs())->get_value();

    switch (bin_op.get_opcode()) {
    case kAdd:
        return apply_bin_op(std::plus{}, type, lhs, rhs);
    case kSub:
        return apply_bin_op(std::minus{}, type, lhs, rhs);
    case kMul:
        return apply_bin_op(std::multiplies{}, type, lhs, rhs);
    case kUDiv:
        if (rhs == 0) { // do not optimize if encounter division by 0
            return std::nullopt;
        }
        return apply_bin_op(std::divides{}, type, lhs, rhs);
    case kSDiv:
        if (rhs == 0) { // do not optimize if encounter division by 0
            return std::nullopt;
        }
        return apply_bin_op(
            [](std::uintmax_t l, std::uintmax_t r) -> std::uintmax_t {
                return static_cast<std::intmax_t>(l) / static_cast<std::intmax_t>(r);
            },
            type, lhs, rhs);

    case kURem:
        if (rhs == 0) { // do not optimize if encounter division by 0
            return std::nullopt;
        }
        return apply_bin_op(std::modulus{}, type, lhs, rhs);
    case kSRem:
        if (rhs == 0) { // do not optimize if encounter division by 0
            return std::nullopt;
        }
        return apply_bin_op(
            [](std::uintmax_t l, std::uintmax_t r) -> std::uintmax_t {
                return static_cast<std::intmax_t>(l) % static_cast<std::intmax_t>(r);
            },
            type, lhs, rhs);
    case kShl:
        if (rhs >= width(bin_op.get_type())) { // do not optimize shifts for more than type's width
            return std::nullopt;
        }
        return apply_bin_op([](std::uintmax_t l, std::uintmax_t r) { return l << r; }, type, lhs,
                            rhs);
    case kShrA:
        if (rhs >= width(bin_op.get_type())) { // do not optimize shifts for more than type's width
            return std::nullopt;
        }
        return apply_bin_op(
            [](std::uintmax_t l, std::uintmax_t r) -> std::uintmax_t {
                return static_cast<std::intmax_t>(l) >> r;
            },
            type, lhs, rhs);
    case kShrL:
        if (rhs >= width(bin_op.get_type())) { // do not optimize shifts for more than type's width
            return std::nullopt;
        }
        return apply_bin_op([](std::uintmax_t l, std::uintmax_t r) { return l >> r; }, type, lhs,
                            rhs);
    case kAnd:
        return apply_bin_op(std::bit_and{}, type, lhs, rhs);
    case kOr:
        return apply_bin_op(std::bit_or{}, type, lhs, rhs);
    case kXor:
        return apply_bin_op(std::bit_xor{}, type, lhs, rhs);
    default:
        std::unreachable();
    }
}

static bool fold_icmp(const ICmpInstruction &icmp) {
    assert(icmp.get_lhs()->get_opcode() == Instruction::Opcode::kConst);
    assert(icmp.get_rhs()->get_opcode() == Instruction::Opcode::kConst);

    auto lhs = static_cast<const ConstInstruction *>(icmp.get_lhs())->get_value();
    auto rhs = static_cast<const ConstInstruction *>(icmp.get_rhs())->get_value();

    using enum ICmpInstruction::Kind;
    switch (icmp.get_kind()) {
    case eq:
        return lhs == rhs;
    case ne:
        return lhs != rhs;
    case ugt:
        return lhs > rhs;
    case uge:
        return lhs >= rhs;
    case ult:
        return lhs < rhs;
    case ule:
        return lhs <= rhs;
    case sgt:
        return static_cast<std::intmax_t>(lhs) > static_cast<std::intmax_t>(rhs);
    case sge:
        return static_cast<std::intmax_t>(lhs) >= static_cast<std::intmax_t>(rhs);
    case slt:
        return static_cast<std::intmax_t>(lhs) < static_cast<std::intmax_t>(rhs);
    case sle:
        return static_cast<std::intmax_t>(lhs) <= static_cast<std::intmax_t>(rhs);
    default:
        std::unreachable();
    }
}

} // unnamed namespace

void ConstantFoldingPass::run(Function &f) {
    const DFS<MutFunctionGraphTraits> dfs{f};
    for (auto *bb : dfs.post_order() | std::views::reverse) {
        for (auto it = bb->begin(), ite = bb->end(); it != ite; ++it) {
            auto maybe_constant = [&instr = std::as_const(*it)] -> std::optional<std::uintmax_t> {
                using enum Instruction::Opcode;
                if (instr.is_binary_op()) {
                    auto &bin_op = static_cast<const BinaryOperator &>(instr);
                    if (bin_op.get_lhs()->get_opcode() == kConst &&
                        bin_op.get_rhs()->get_opcode() == kConst) {
                        return fold_binary_operator(bin_op);
                    }
                } else {
                    switch (instr.get_opcode()) {
                    case kICmp: {
                        auto &icmp = static_cast<const ICmpInstruction &>(instr);
                        if (icmp.get_lhs()->get_opcode() == kConst &&
                            icmp.get_rhs()->get_opcode() == kConst) {
                            return fold_icmp(icmp);
                        }
                        break;
                    }
                    default:
                        break;
                    }
                }

                return std::nullopt;
            }();

            if (maybe_constant.has_value()) {
                auto new_it = bb->emplace<ConstInstruction>(it, it->get_type(), *maybe_constant);
                bb->replace_instruction(it, *new_it);
                it = new_it;
            }
        }
    }
}

} // namespace bjac
