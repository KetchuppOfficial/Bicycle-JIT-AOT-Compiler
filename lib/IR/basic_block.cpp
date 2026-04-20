#include <cassert>
#include <format>
#include <iterator>
#include <ostream>
#include <ranges>

#include "bjac/IR/basic_block.hpp"
#include "bjac/IR/call_instruction.hpp"
#include "bjac/IR/function.hpp"

namespace bjac {

BasicBlock::BasicBlock(Function &parent)
    : Value{std::make_unique<NoneType>()}, first_non_phi_{end()}, parent_{std::addressof(parent)},
      id_{parent.get_next_bb_id()}, next_instr_id_{0} {}

void BasicBlock::replace_instruction(iterator from, Instruction &to) {
    assert(std::addressof(from->get_parent()) == this);

    from->replace_with(to);
    from->remove_as_user();
    erase(from);
}

void BasicBlock::remove_instruction(iterator it) {
    assert(std::addressof(it->get_parent()) == this);
    if (it->get_type_id() != Type::ID::kVoid) {
        throw std::invalid_argument{
            std::format("cannot remove '{}' from a basic block without replacing it with another "
                        "instruction for all users",
                        it->to_string())};
    }

    it->remove_as_user();
    erase(it);
}

void BasicBlock::add_ret_to_parent(ReturnInstruction &ret) { get_parent().add_ret(ret); }
void BasicBlock::remove_ret_from_parent(ReturnInstruction &ret) { get_parent().remove_ret(ret); }

void BasicBlock::add_callee_to_parent(CallInstruction &call) {
    get_parent().add_callee(call.callee());
}
void BasicBlock::remove_callee_from_parent(CallInstruction &call) {
    get_parent().remove_callee(call.callee());
}

void BasicBlock::print(std::ostream &os) const {
    os << std::format("%bb{}:", get_id());

    if (!predecessors_.empty()) {
        os << " ; preds: ";
        auto prev_end = std::ranges::prev(predecessors_.end());
        for (const auto *pred : std::ranges::subrange(predecessors_.begin(), prev_end)) {
            os << std::format("%bb{}, ", pred->get_id());
        }
        os << std::format("%bb{}", (*prev_end)->get_id());
    }

    os << '\n';

    for (auto &instr : *this) {
        os << "    " << instr.to_string() << '\n';
    }
}

std::ostream &operator<<(std::ostream &os, const BasicBlock &bb) {
    bb.print(os);
    return os;
}

} // namespace bjac
