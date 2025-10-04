#include <format>
#include <iterator>
#include <ostream>
#include <ranges>

#include "bjac/IR/basic_block.hpp"
#include "bjac/IR/function.hpp"

namespace bjac {

BasicBlock::BasicBlock(Function &parent) noexcept
    : parent_{std::addressof(parent)}, id_{parent.get_next_bb_id()} {}

void BasicBlock::print(std::ostream &os) const {
    os << std::format("%bb{}:", get_id().value());

    if (!predecessors_.empty()) {
        os << " preds: ";
        auto prev_end = std::ranges::prev(predecessors_.end());
        for (const auto *pred : std::ranges::subrange(predecessors_.begin(), prev_end)) {
            os << std::format("%bb{}, ", pred->get_id().value());
        }
        os << std::format("%bb{}", (*prev_end)->get_id().value());
    }

    os << '\n';

    for (auto &instr : *this) {
        os << "    " << instr.to_string() << '\n';
    }
}

} // namespace bjac
