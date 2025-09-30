#include <cassert>
#include <format>

#include "bjac/IR/basic_block.hpp"
#include "bjac/IR/instruction.hpp"

namespace bjac {

std::string BranchInstruction::to_string() const {
    if (is_conditional()) {
        return std::format("{} %{}, label %bb{}, label %bb{}", get_name(),
                           Value::to_void_ptr(condition_), true_path_->get_id().value(),
                           false_path_->get_id().value());
    } else {
        return std::format("{} %bb{}", get_name(), true_path_->get_id().value());
    }
}

std::string PHIInstruction::to_string() const {
    if (records_.size() < 2) {
        throw std::invalid_argument{"phi instruction does not have enough records"};
    }

    std::string phi_list;
    for (const auto &[bb, value] : std::views::take(records_, records_.size() - 1)) {
        phi_list += std::format("[%{}, %bb{}], ", Value::to_void_ptr(value), bb->get_id().value());
    }
    const auto &[bb, value] = *std::ranges::prev(records_.end());
    phi_list += std::format("[%{}, %bb{}]", Value::to_void_ptr(value), bb->get_id().value());

    return std::format("%{} = {} {}", Value::to_void_ptr(this), get_name(), phi_list);
}

} // namespace bjac
