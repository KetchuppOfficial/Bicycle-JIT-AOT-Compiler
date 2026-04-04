#include <format>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>

#include "bjac/IR/basic_block.hpp"
#include "bjac/IR/function.hpp"

#include "bjac/IR/argument_instruction.hpp"
#include "bjac/IR/binary_operator.hpp"
#include "bjac/IR/branch_instruction.hpp"
#include "bjac/IR/call_instruction.hpp"
#include "bjac/IR/constant_instruction.hpp"
#include "bjac/IR/icmp_instruction.hpp"
#include "bjac/IR/phi_instruction.hpp"
#include "bjac/IR/ret_instruction.hpp"

namespace bjac {

Instruction::Instruction(BasicBlock &parent, Opcode opcode, Type type)
    : Value{type}, opcode_{opcode}, parent_{std::addressof(parent)},
      id_{parent.get_next_instr_id()} {}

void Instruction::replace_with(Instruction &other) {
    for (auto it = users_.begin(), ite = users_.end(); it != ite;) {
        auto *user = *it;
        if (user->is_binary_op()) {
            if (auto *bin_op = static_cast<BinaryOperator *>(user); bin_op->get_lhs() == this) {
                if (bin_op->get_rhs() == this) {
                    it = std::next(it, 2);
                    bin_op->set_lhs(other);
                    bin_op->set_rhs(other);
                } else {
                    ++it;
                    bin_op->set_lhs(other);
                }
            } else if (bin_op->get_rhs() == this) {
                ++it;
                bin_op->set_rhs(other);
            } else {
                ++it;
            }
        } else {
            switch (user->get_opcode()) {
            case Opcode::kICmp:
                if (auto *icmp = static_cast<ICmpInstruction *>(user); icmp->get_lhs() == this) {
                    if (icmp->get_rhs() == this) {
                        it = std::next(it, 2);
                        icmp->set_lhs(other);
                        icmp->set_rhs(other);
                    } else {
                        ++it;
                        icmp->set_lhs(other);
                    }
                } else if (icmp->get_rhs() == this) {
                    ++it;
                    icmp->set_rhs(other);
                } else {
                    ++it;
                }
                break;
            case Opcode::kRet:
                ++it;
                static_cast<ReturnInstruction *>(user)->set_ret_value(other);
                break;
            case Opcode::kBr:
                ++it;
                static_cast<BranchInstruction *>(user)->set_condition(other);
                break;
            case Opcode::kPHI:
                ++it;
                static_cast<PHIInstruction *>(user)->replace_value(*this, other);
                break;
            default:
                ++it;
                break;
            }
        }
    }
}

namespace {

Type get_arg_type(const Function &f, unsigned pos) {
    if (const auto args = f.arguments(); pos < args.size()) {
        return args[pos];
    }
    throw ArgOutOfRange{"function parameter index is out of range"};
}

std::string ssa_value_to_string(const Instruction &instr) {
    return std::format("%{}.{}", instr.get_parent().get_id(), instr.get_id());
}

std::string users_to_string(const Instruction &instr) {
    std::ranges::bidirectional_range auto users = instr.get_users();
    if (users.empty()) {
        return {};
    }

    using namespace std::string_view_literals;
    auto values = users | std::views::transform(
                              [](const auto *user) static { return ssa_value_to_string(*user); });
    return std::format(" ; used by: {:s}", std::views::join_with(values, ", "sv));
}

} // unnamed namespace

ArgumentInstruction::ArgumentInstruction(BasicBlock &parent, unsigned pos)
    : Instruction(parent, Opcode::kArg, get_arg_type(parent.get_parent(), pos)), pos_{pos} {}

std::string ArgumentInstruction::to_string() const {
    return std::format("{} = {} {} [{}]{}", ssa_value_to_string(*this), type_, Opcode::kArg, pos_,
                       users_to_string(*this));
}

std::string BinaryOperator::to_string() const {
    return std::format("{} = {} {} {}, {}{}", ssa_value_to_string(*this), type_, opcode_,
                       ssa_value_to_string(*lhs_), ssa_value_to_string(*rhs_),
                       users_to_string(*this));
}

std::string BranchInstruction::to_string() const {
    if (is_conditional()) {
        return std::format("{} {} {} {}, label %bb{}, label %bb{}{}", ssa_value_to_string(*this),
                           Opcode::kBr, condition_->get_type(), ssa_value_to_string(*condition_),
                           get_true_path()->get_id(), get_false_path()->get_id(),
                           users_to_string(*this));
    } else {
        return std::format("{} {} label %bb{}{}", ssa_value_to_string(*this), Opcode::kBr,
                           get_true_path()->get_id(), users_to_string(*this));
    }
}

std::string ConstInstruction::to_string() const {
    return std::format("{} = {} {} {}{}", ssa_value_to_string(*this), type_, Opcode::kConst, value_,
                       users_to_string(*this));
}

std::string ICmpInstruction::to_string() const {
    return std::format("{} = {} {} {} {}, {}{}", ssa_value_to_string(*this), Opcode::kICmp, kind_,
                       lhs_->get_type(), ssa_value_to_string(*lhs_), ssa_value_to_string(*rhs_),
                       users_to_string(*this));
}

std::string PHIInstruction::to_string() const {
    if (records_.empty()) {
        return std::format("{} = {} {}{}", ssa_value_to_string(*this), Opcode::kPHI, type_,
                           users_to_string(*this));
    } else {
        auto phi_strings = records_ | std::views::transform([](const auto &r) static {
                               return std::format("[{}, %bb{}]", ssa_value_to_string(*r.second),
                                                  r.first->get_id());
                           });
        using namespace std::string_view_literals;
        return std::format("{} = {} {} {:s}{}", ssa_value_to_string(*this), Opcode::kPHI, type_,
                           std::views::join_with(phi_strings, ", "sv), users_to_string(*this));
    }
}

ReturnInstruction::ReturnInstruction(BasicBlock &parent)
    : Instruction(parent, Opcode::kRet, Type::kVoid), ret_val_{nullptr} {
    if (auto ret_type = parent.get_parent().return_type(); ret_type != Type::kVoid) {
        throw std::invalid_argument{std::format("trying to create {} {} in a function returning {}",
                                                Opcode::kRet, Type::kVoid, ret_type)};
    }
}

ReturnInstruction::ReturnInstruction(BasicBlock &parent, Instruction &ret_val)
    : Instruction(parent, Opcode::kRet, Type::kVoid), ret_val_{std::addressof(ret_val)} {
    auto &callee = parent.get_parent();
    auto &owner = ret_val.get_parent().get_parent();
    if (std::addressof(owner) != std::addressof(callee)) {
        throw std::invalid_argument{
            std::format("trying to return SSA value owned by function '{}' from function '{}'",
                        owner.name(), callee.name())};
    }

    if (auto ret_type = parent.get_parent().return_type(); ret_type != ret_val.get_type()) {
        throw std::invalid_argument{std::format("trying to create {} {} in a function returning {}",
                                                Opcode::kRet, ret_val.get_type(), ret_type)};
    }

    ret_val.add_user(this);
}

std::string ReturnInstruction::to_string() const {
    if (ret_val_) {
        return std::format("{} {} {} {}", ssa_value_to_string(*this), Opcode::kRet,
                           ret_val_->get_type(), ssa_value_to_string(*ret_val_));
    }
    return std::format("{} {} {}", ssa_value_to_string(*this), Opcode::kRet, Type::kVoid);
}

CallInstruction::CallInstruction(BasicBlock &parent, Function &callee,
                                 std::vector<Instruction *> args)
    : Instruction(parent, Opcode::kCall, callee.return_type()), callee_{std::addressof(callee)},
      args_(std::move(args)) {
    if (!std::ranges::equal(callee.arguments(),
                            args_ | std::views::transform(
                                        [](const auto *arg) static { return arg->get_type(); }))) {
        throw std::invalid_argument{std::format(
            "types of call arguments mismatch with parameters of function '{}'", callee.name())};
    }
}

std::string CallInstruction::to_string() const {
    std::ranges::view auto arg_string = args_ | std::views::transform([](const auto *arg) static {
                                            return ssa_value_to_string(*arg);
                                        });
    using namespace std::string_view_literals;
    return std::format("{} = {} {}({:s})", ssa_value_to_string(*this), Opcode::kCall,
                       callee_->name(), std::views::join_with(arg_string, ", "sv));
}

Function &CallInstruction::caller() noexcept { return get_parent().get_parent(); }
const Function &CallInstruction::caller() const noexcept { return get_parent().get_parent(); }

} // namespace bjac
