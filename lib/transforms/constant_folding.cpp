#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <utility>

#include "bjac/transforms/constant_folding.hpp"

#include "bjac/IR/binary_operator.hpp"
#include "bjac/IR/branch_instruction.hpp"
#include "bjac/IR/constant_instruction.hpp"
#include "bjac/IR/icmp_instruction.hpp"
#include "bjac/IR/phi_instruction.hpp"
#include "bjac/IR/ret_instruction.hpp"

#include "bjac/graphs/dfs.hpp"

namespace bjac {

namespace {

template <typename F>
static std::uintmax_t apply_bin_op(F op, Type type, std::uintmax_t lhs, std::uintmax_t rhs) {
    using enum Type;
    switch (type) {
    case kI1:
        return op(static_cast<bool>(lhs), static_cast<bool>(rhs));
    case kI8:
        return op(static_cast<std::uint8_t>(lhs), static_cast<std::uint8_t>(rhs));
    case kI16:
        return op(static_cast<std::uint16_t>(lhs), static_cast<std::uint16_t>(rhs));
    case kI32:
        return op(static_cast<std::uint32_t>(lhs), static_cast<std::uint32_t>(rhs));
    case kI64:
        return op(static_cast<std::uint64_t>(lhs), static_cast<std::uint64_t>(rhs));
    default:
        std::unreachable();
    }
}

static std::uintmax_t fold_binary_operator(const BinaryOperator &bin_op) {
    assert(bin_op.get_lhs()->get_opcode() == Instruction::Opcode::kConst);
    assert(bin_op.get_rhs()->get_opcode() == Instruction::Opcode::kConst);

    Type type = bin_op.get_type();
    auto lhs = static_cast<const ConstInstruction *>(bin_op.get_lhs())->get_value();
    auto rhs = static_cast<const ConstInstruction *>(bin_op.get_rhs())->get_value();

    using enum Instruction::Opcode;
    switch (bin_op.get_opcode()) {
    case kAdd:
        return apply_bin_op(std::plus{}, type, lhs, rhs);
    case kSub:
        return apply_bin_op(std::minus{}, type, lhs, rhs);
    case kMul:
        return apply_bin_op(std::multiplies{}, type, lhs, rhs);
    case kUDiv:
        return apply_bin_op(std::divides{}, type, lhs, rhs);
    case kSDiv:
        return apply_bin_op(
            [](std::uintmax_t l, std::uintmax_t r) -> std::uintmax_t {
                return static_cast<std::intmax_t>(l) / static_cast<std::intmax_t>(r);
            },
            type, lhs, rhs);

    case kURem:
        return apply_bin_op(std::modulus{}, type, lhs, rhs);
    case kSRem:
        return apply_bin_op(
            [](std::uintmax_t l, std::uintmax_t r) -> std::uintmax_t {
                return static_cast<std::intmax_t>(l) % static_cast<std::intmax_t>(r);
            },
            type, lhs, rhs);
    case kShl:
        return apply_bin_op([](std::uintmax_t l, std::uintmax_t r) { return l << r; }, type, lhs,
                            rhs);
    case kShrA:
        return apply_bin_op(
            [](std::uintmax_t l, std::uintmax_t r) -> std::uintmax_t {
                return static_cast<std::intmax_t>(l) >> r;
            },
            type, lhs, rhs);
    case kShrL:
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

void replace_uses(Instruction &from, Instruction &to) {
    for (auto *user : from.get_users()) {
        to.add_user(user);
        if (user->is_binary_op()) {
            auto &bin_op = static_cast<BinaryOperator &>(*user);
            if (bin_op.get_lhs() == std::addressof(from)) {
                bin_op.set_lhs(to);
            }
            if (bin_op.get_rhs() == std::addressof(from)) {
                bin_op.set_rhs(to);
            }
        } else {
            using enum Instruction::Opcode;
            switch (user->get_opcode()) {
            case kRet:
                static_cast<ReturnInstruction *>(user)->set_ret_value(to);
                break;
            case kBr:
                static_cast<BranchInstruction *>(user)->set_condition(to);
                break;
            case kPHI:
                std::ranges::replace(static_cast<PHIInstruction *>(user)->get_values(),
                                     std::addressof(from), std::addressof(to));
            default:
                break;
            }
        }
    }
}

} // unnamed namespace

void ConstantFoldingPass::run(Function &f) {
    const DFS<MutFunctionGraphTraits> dfs{f};
    for (auto *bb : dfs.post_order() | std::views::reverse) {
        for (auto it = bb->begin(), ite = bb->end(); it != ite; ++it) {
            auto new_it = [it, bb] {
                using enum Instruction::Opcode;
                if (it->is_binary_op()) {
                    auto &bin_op = static_cast<BinaryOperator &>(*it);
                    auto *lhs = bin_op.get_lhs();
                    auto *rhs = bin_op.get_rhs();
                    if (lhs->get_opcode() == kConst && rhs->get_opcode() == kConst) {
                        const auto res = fold_binary_operator(bin_op);
                        lhs->remove_user(std::addressof(bin_op));
                        rhs->remove_user(std::addressof(bin_op));
                        return bb->emplace<ConstInstruction>(it, bin_op.get_type(), res);
                    }
                } else {
                    switch (it->get_opcode()) {
                    case kICmp: {
                        auto &icmp = static_cast<ICmpInstruction &>(*it);
                        auto *lhs = icmp.get_lhs();
                        auto *rhs = icmp.get_rhs();
                        if (lhs->get_opcode() == kConst && rhs->get_opcode() == kConst) {
                            const auto res = fold_icmp(icmp);
                            lhs->remove_user(std::addressof(icmp));
                            rhs->remove_user(std::addressof(icmp));
                            return bb->emplace<ConstInstruction>(it, Type::kI1, res);
                        }
                        break;
                    }
                    default:
                        break;
                    }
                }

                return it;
            }();

            if (new_it != it) {
                replace_uses(*it, *new_it);
                bb->erase(it);
                it = new_it;
            }
        }
    }
}

} // namespace bjac
