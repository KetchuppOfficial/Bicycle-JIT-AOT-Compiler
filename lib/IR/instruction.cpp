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

} // unnamed namespace

ArgumentInstruction::ArgumentInstruction(BasicBlock &parent, const Function &f, unsigned pos)
    : Instruction(parent, Opcode::kArg, get_arg_type(f, pos)), pos_{pos} {}

std::string ArgumentInstruction::to_string() const {
    return std::format("%{}.{} = {} {} [{}]", parent_->get_id(), get_id(), type_, Opcode::kArg,
                       pos_);
}

std::string BinaryOperator::to_string() const {
    return std::format("%{}.{} = {} {} %{}.{}, %{}.{}", parent_->get_id(), get_id(), type_, opcode_,
                       lhs_->get_parent()->get_id(), lhs_->get_id(), rhs_->get_parent()->get_id(),
                       rhs_->get_id());
}

std::string BranchInstruction::to_string() const {
    if (is_conditional()) {
        return std::format("{} {} %{}.{}, label %bb{}, label %bb{}", Opcode::kBr,
                           condition_->get_type(), condition_->get_parent()->get_id(),
                           condition_->get_id(), get_true_path()->get_id(),
                           get_false_path()->get_id());
    } else {
        return std::format("{} label %bb{}", Opcode::kBr, get_true_path()->get_id());
    }
}

std::string ConstInstruction::to_string() const {
    return std::format("%{}.{} = {} {} {}", parent_->get_id(), get_id(), type_, Opcode::kConst,
                       value_);
}

std::string ICmpInstruction::to_string() const {
    return std::format("%{}.{} = {} {} {} %{}.{}, %{}.{}", parent_->get_id(), get_id(),
                       Opcode::kICmp, kind_, lhs_->get_type(), lhs_->get_parent()->get_id(),
                       lhs_->get_id(), rhs_->get_parent()->get_id(), rhs_->get_id());
}

std::string PHIInstruction::to_string() const {
    if (records_.size() < 2) {
        throw std::invalid_argument{"phi instruction does not have enough records"};
    }

    std::string phi_list;

    auto prev_end = std::ranges::prev(records_.end());
    for (const auto &[bb, instr] : std::ranges::subrange(records_.begin(), prev_end)) {
        phi_list += std::format("[%{}.{}, %bb{}], ", instr->get_parent()->get_id(), instr->get_id(),
                                bb->get_id());
    }
    const auto &[bb, instr] = *std::ranges::prev(records_.end());
    phi_list += std::format("[%{}.{}, %bb{}]", instr->get_parent()->get_id(), instr->get_id(),
                            bb->get_id());

    return std::format("%{}.{} = {} {} {}", parent_->get_id(), get_id(), Opcode::kPHI, type_,
                       phi_list);
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
        return std::format("{} {} %{}.{}", Opcode::kRet, ret_val_->get_type(),
                           ret_val_->get_parent()->get_id(), ret_val_->get_id());
    }
    return std::format("{} {}", Opcode::kRet, Type::kVoid);
}

} // namespace bjac
