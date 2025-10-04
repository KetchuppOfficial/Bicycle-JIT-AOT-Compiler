#include <cassert>
#include <format>
#include <stdexcept>

#include "bjac/IR/basic_block.hpp"
#include "bjac/IR/function.hpp"
#include "bjac/IR/instruction.hpp"

namespace bjac {

std::string BranchInstruction::to_string() const {
    if (is_conditional()) {
        return std::format("{} {} %{}, label %bb{}, label %bb{}", opcode_, condition_->get_type(),
                           Value::to_void_ptr(condition_), true_path_->get_id().value(),
                           false_path_->get_id().value());
    } else {
        return std::format("{} label %bb{}", opcode_, true_path_->get_id().value());
    }
}

std::string PHIInstruction::to_string() const {
    if (records_.size() < 2) {
        throw std::invalid_argument{"phi instruction does not have enough records"};
    }

    std::string phi_list;

    auto prev_end = std::ranges::prev(records_.end());
    for (const auto &[bb, value] : std::ranges::subrange(records_.begin(), prev_end)) {
        phi_list += std::format("[%{}, %bb{}], ", Value::to_void_ptr(value), bb->get_id().value());
    }
    const auto &[bb, value] = *std::ranges::prev(records_.end());
    phi_list += std::format("[%{}, %bb{}]", Value::to_void_ptr(value), bb->get_id().value());

    return std::format("%{} = {} {} {}", Value::to_void_ptr(this), opcode_, type_, phi_list);
}

namespace {

Type get_arg_type(const Function &f, unsigned pos) {
    if (const auto args = f.arguments(); pos < args.size()) {
        return args[pos];
    }
    throw ArgOutOfRange{"function parameter index is out of range"};
}

} // unnamed namespace

ArgumentInstruction::ArgumentInstruction(const Function &f, unsigned pos)
    : Instruction(Opcode::kArg, get_arg_type(f, pos)), pos_{pos} {}

} // namespace bjac
