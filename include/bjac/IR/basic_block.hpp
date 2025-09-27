#ifndef INCLUDE_BJAC_IR_BASIC_BLOCK_HPP
#define INCLUDE_BJAC_IR_BASIC_BLOCK_HPP

#include <cstddef>
#include <memory>
#include <optional>

#include "bjac/IR/instruction.hpp"
#include "bjac/utils/ilist.hpp"
#include "bjac/utils/ilist_node.hpp"

namespace bjac {

class Function;

class BasicBlock final : public ilist_node<BasicBlock>, private ilist<Instruction> {
    using instructions = ilist<Instruction>;

  public:
    using instructions::const_iterator;
    using instructions::difference_type;
    using instructions::iterator;
    using instructions::size_type;

    BasicBlock() = default;
    BasicBlock(Function *parent) noexcept : parent_{parent} {}

    template <typename Self>
    auto *get_parent(this Self &&self) noexcept {
        return self.parent_;
    }

    // TODO: add setting id
    void set_parent(Function *parent) noexcept { parent_ = parent; }

    template <typename Self>
    auto *get_terminator(this Self &&self) {
        if (!self.empty() && self.back().is_terminator()) {
            return std::addressof(self.back());
        }
        return nullptr;
    }

    std::optional<unsigned> get_id() const { return parent_ ? std::optional{id_} : std::nullopt; }

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

    using instructions::emplace;
    using instructions::emplace_back;
    using instructions::emplace_front;
    using instructions::insert;
    using instructions::push_back;
    using instructions::push_front;

    using instructions::erase;
    using instructions::pop_back;
    using instructions::pop_front;

  private:
    Function *parent_ = nullptr;
    unsigned id_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_BASIC_BLOCK_HPP
