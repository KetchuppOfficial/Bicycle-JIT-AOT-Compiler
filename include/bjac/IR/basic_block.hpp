#ifndef INCLUDE_BJAC_IR_BASIC_BLOCK_HPP
#define INCLUDE_BJAC_IR_BASIC_BLOCK_HPP

#include <memory>
#include <iosfwd>
#include <optional>
#include <ranges>
#include <set>
#include <utility>

#include "bjac/IR/instruction.hpp"
#include "bjac/utils/ilist.hpp"
#include "bjac/utils/ilist_node.hpp"

namespace bjac {

class Function;

class BasicBlock final : public Value, public ilist_node<BasicBlock>, private ilist<Instruction> {
    using instructions = ilist<Instruction>;

  public:
    using instructions::const_iterator;
    using instructions::difference_type;
    using instructions::iterator;
    using instructions::size_type;

    BasicBlock() = default;

    template <typename Self>
    auto *get_parent(this Self &&self) noexcept {
        return self.parent_;
    }

    template <typename Self>
    auto *get_terminator(this Self &&self) {
        if (!self.empty() && self.back().is_terminator()) {
            return std::addressof(self.back());
        }
        return nullptr;
    }

    std::optional<unsigned> get_id() const { return parent_ ? std::optional{id_} : std::nullopt; }

    auto predecessors() const { return std::ranges::ref_view(predecessors_); }
    void add_predecessor(BasicBlock &bb) { predecessors_.insert(std::addressof(bb)); }

    iterator insert(const_iterator pos, std::unique_ptr<Instruction> instr) {
        instr->parent_ = this;
        return instructions::insert(pos, std::move(instr));
    }

    iterator insert(const_iterator pos, std::unique_ptr<BranchInstruction> instr) {
        instr->get_true_path()->add_predecessor(*this);
        if (instr->is_conditional()) {
            instr->get_false_path()->add_predecessor(*this);
        }
        instr->parent_ = this;
        return instructions::insert(pos, std::move(instr));
    }

    reference push_back(std::unique_ptr<Instruction> instr) {
        return *insert(end(), std::move(instr));
    }
    reference push_back(std::unique_ptr<BranchInstruction> instr) {
        return *insert(end(), std::move(instr));
    }
    reference push_front(std::unique_ptr<Instruction> instr) {
        return *insert(begin(), std::move(instr));
    }

    void print(std::ostream &os) const;

    using instructions::empty;
    using instructions::size;

    using instructions::back;
    using instructions::front;

    using instructions::begin;
    using instructions::cbegin;
    using instructions::cend;
    using instructions::crbegin;
    using instructions::crend;
    using instructions::end;
    using instructions::rbegin;
    using instructions::rend;

    using instructions::erase;
    using instructions::pop_back;
    using instructions::pop_front;

  private:
    friend class Function;

    BasicBlock(Function &parent) noexcept;

    Function *parent_ = nullptr;
    unsigned id_ = 0;

    std::set<BasicBlock *> predecessors_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_BASIC_BLOCK_HPP
