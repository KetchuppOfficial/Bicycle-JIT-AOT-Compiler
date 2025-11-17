#include <format>
#include <iterator>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>

#include "bjac/IR/basic_block.hpp"
#include "bjac/IR/function.hpp"

#include "bjac/IR/argument_instruction.hpp"
#include "bjac/IR/binary_operator.hpp"
#include "bjac/IR/branch_instruction.hpp"
#include "bjac/IR/constant_instruction.hpp"
#include "bjac/IR/icmp_instruction.hpp"
#include "bjac/IR/phi_instruction.hpp"
#include "bjac/IR/ret_instruction.hpp"

namespace bjac {

Instruction::Instruction(BasicBlock &parent, Opcode opcode, Type type)
    : Value{type}, opcode_{opcode}, parent_{std::addressof(parent)},
      id_{parent.get_next_instr_id()} {}

namespace {

Type get_arg_type(const Function &f, unsigned pos) {
    if (const auto args = f.arguments(); pos < args.size()) {
        return args[pos];
    }
    throw ArgOutOfRange{"function parameter index is out of range"};
}

std::string ssa_value_to_string(const Instruction &instr) {
    return std::format("%{}.{}", instr.get_parent()->get_id(), instr.get_id());
}

std::string users_to_string(const Instruction &instr) {
    std::ranges::bidirectional_range auto users = instr.get_users();
    if (users.empty()) {
        return {};
    }

    std::string res{" ; used by: "};
    for (auto *user : users | std::views::take(users.size() - 1)) {
        res += ssa_value_to_string(*user);
        res += ", ";
    }
    res += ssa_value_to_string(*users.back());

    return res;
}

} // unnamed namespace

ArgumentInstruction::ArgumentInstruction(BasicBlock &parent, const Function &f, unsigned pos)
    : Instruction(parent, Opcode::kArg, get_arg_type(f, pos)), pos_{pos} {}

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
    if (records_.size() < 2) {
        throw std::invalid_argument{"phi instruction does not have enough records"};
    }

    std::string phi_list;

    auto prev_end = std::ranges::prev(records_.end());
    for (const auto &[bb, instr] : std::ranges::subrange(records_.begin(), prev_end)) {
        phi_list += std::format("[{}, %bb{}], ", ssa_value_to_string(*instr), bb->get_id());
    }
    const auto &[bb, instr] = *prev_end;
    phi_list += std::format("[{}, %bb{}]", ssa_value_to_string(*instr), bb->get_id());

    return std::format("{} = {} {} {}{}", ssa_value_to_string(*this), Opcode::kPHI, type_, phi_list,
                       users_to_string(*this));
}

ReturnInstruction::ReturnInstruction(BasicBlock &parent)
    : Instruction(parent, Opcode::kRet, Type::kVoid), ret_val_{nullptr} {
    if (auto ret_type = parent.get_parent()->return_type(); ret_type != Type::kVoid) {
        throw std::invalid_argument{std::format("trying to create {} {} in a function returning {}",
                                                Opcode::kRet, Type::kVoid, ret_type)};
    }
}

ReturnInstruction::ReturnInstruction(BasicBlock &parent, Instruction &ret_val)
    : Instruction(parent, Opcode::kRet, Type::kVoid), ret_val_{std::addressof(ret_val)} {
    if (auto ret_type = parent.get_parent()->return_type(); ret_type != ret_val.get_type()) {
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

} // namespace bjac
