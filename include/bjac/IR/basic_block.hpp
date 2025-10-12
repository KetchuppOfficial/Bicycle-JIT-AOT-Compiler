#ifndef INCLUDE_BJAC_IR_BASIC_BLOCK_HPP
#define INCLUDE_BJAC_IR_BASIC_BLOCK_HPP

#include <iosfwd>
#include <memory>
#include <optional>
#include <ranges>
#include <set>
#include <type_traits>
#include <utility>

#include "bjac/IR/branch_instruction.hpp"
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

    template <typename Self>
    auto *get_parent(this Self &&self) noexcept {
        return std::addressof(std::forward_like<Self>(*self.parent_));
    }

    template <typename Self>
    auto *get_terminator(this Self &&self) {
        if (!self.empty() && self.back().is_terminator()) {
            return std::addressof(self.back());
        }
        return nullptr;
    }

    std::optional<unsigned> get_id() const { return parent_ ? std::optional{id_} : std::nullopt; }

    unsigned get_next_instr_id() const noexcept { return next_instr_id_; }

    auto predecessors() const { return std::ranges::ref_view(predecessors_); }
    void add_predecessor(const BasicBlock &bb) { predecessors_.insert(std::addressof(bb)); }
    void remove_predecessor(const BasicBlock &bb) { predecessors_.erase(std::addressof(bb)); }

    auto successors() const {
        using R = decltype(std::declval<const BranchInstruction &>().successors());
        if (empty()) {
            return R{nullptr, nullptr};
        }

        const Instruction &term = back();
        if (term.get_opcode() != Instruction::Opcode::kBr) {
            return R{nullptr, nullptr};
        }

        return static_cast<const BranchInstruction &>(term).successors();
    }

    template <typename T, typename... Args>
    iterator emplace(const_iterator pos, Args &&...args) {
        std::unique_ptr<T> instr{new T(*this, std::forward<Args>(args)...)};

        if constexpr (std::is_same_v<T, BranchInstruction>) {
            instr->get_true_path()->add_predecessor(*this);
            if (instr->is_conditional()) {
                instr->get_false_path()->add_predecessor(*this);
            }
        }

        instr->parent_ = this;
        ++next_instr_id_;

        return instructions::insert(pos, std::move(instr));
    }

    template <typename T, typename... Args>
    T &emplace_front(Args &&...args) {
        return static_cast<T &>(*emplace<T>(begin(), std::forward<Args>(args)...));
    }

    template <typename T, typename... Args>
    T &emplace_back(Args &&...args) {
        return static_cast<T &>(*emplace<T>(end(), std::forward<Args>(args)...));
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
    unsigned next_instr_id_ = 0;

    struct IDCompare {
        bool operator()(const BasicBlock *lhs, const BasicBlock *rhs) const {
            return lhs->get_id() < rhs->get_id();
        }
    };

    std::set<const BasicBlock *, IDCompare> predecessors_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_BASIC_BLOCK_HPP
