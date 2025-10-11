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

Instruction::Instruction(BasicBlock &parent, Opcode opcode, Type type) noexcept
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
    return std::format("%{}.{} = {} {} [{}]", parent_->get_id().value(), get_id().value(), type_,
                       opcode_, pos_);
}

std::string BinaryOperator::to_string() const {
    return std::format("%{}.{} = {} {} %{}.{}, %{}.{}", parent_->get_id().value(), get_id().value(),
                       type_, opcode_, lhs_->get_parent()->get_id().value(), lhs_->get_id().value(),
                       rhs_->get_parent()->get_id().value(), rhs_->get_id().value());
}

std::string BranchInstruction::to_string() const {
    if (is_conditional()) {
        return std::format("{} {} %{}.{}, label %bb{}, label %bb{}", opcode_,
                           condition_->get_type(), condition_->get_parent()->get_id().value(),
                           condition_->get_id().value(), true_path_->get_id().value(),
                           false_path_->get_id().value());
    } else {
        return std::format("{} label %bb{}", opcode_, true_path_->get_id().value());
    }
}

std::string ConstInstruction::to_string() const {
    return std::format("%{}.{} = {} {} {}", parent_->get_id().value(), get_id().value(), type_,
                       opcode_, value_);
}

std::string ICmpInstruction::to_string() const {
    return std::format("%{}.{} = {} {} {} %{}.{}, %{}.{}", parent_->get_id().value(),
                       get_id().value(), opcode_, kind_, lhs_->get_type(),
                       lhs_->get_parent()->get_id().value(), lhs_->get_id().value(),
                       rhs_->get_parent()->get_id().value(), rhs_->get_id().value());
}

std::string PHIInstruction::to_string() const {
    if (records_.size() < 2) {
        throw std::invalid_argument{"phi instruction does not have enough records"};
    }

    std::string phi_list;

    auto prev_end = std::ranges::prev(records_.end());
    for (const auto &[bb, instr] : std::ranges::subrange(records_.begin(), prev_end)) {
        phi_list += std::format("[%{}.{}, %bb{}], ", instr->get_parent()->get_id().value(),
                                instr->get_id().value(), bb->get_id().value());
    }
    const auto &[bb, instr] = *std::ranges::prev(records_.end());
    phi_list += std::format("[%{}.{}, %bb{}]", instr->get_parent()->get_id().value(),
                            instr->get_id().value(), bb->get_id().value());

    return std::format("%{}.{} = {} {} {}", parent_->get_id().value(), get_id().value(), opcode_,
                       type_, phi_list);
}

std::string ReturnInstruction::to_string() const {
    if (ret_val_) {
        return std::format("{} {} %{}.{}", opcode_, ret_val_->get_type(),
                           ret_val_->get_parent()->get_id().value(), ret_val_->get_id().value());
    }
    return std::format("{} {}", opcode_, Type::kVoid);
}

} // namespace bjac
