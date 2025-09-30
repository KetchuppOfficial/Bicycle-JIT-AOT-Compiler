#include <cassert>
#include <format>
#include <sstream>

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
    assert(records_.size() >= 2);

    std::ostringstream phi_list;
    for (const auto &[bb, value] : records_) {
        phi_list << std::format("[%{}, %bb{}], ", Value::to_void_ptr(value), bb->get_id().value());
    }

    auto phi_list_sv = phi_list.view();
    phi_list_sv.remove_suffix(2);

    return std::format("%{} = {} {}", Value::to_void_ptr(this), get_name(), phi_list_sv);
}

} // namespace bjac
