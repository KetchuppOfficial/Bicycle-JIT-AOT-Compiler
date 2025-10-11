#ifndef INCLUDE_BJAC_IR_FUNCTION_HPP
#define INCLUDE_BJAC_IR_FUNCTION_HPP

#include <concepts>
#include <initializer_list>
#include <iosfwd>
#include <iterator>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "bjac/IR/basic_block.hpp"
#include "bjac/IR/type.hpp"
#include "bjac/utils/ilist.hpp"

namespace bjac {

class Function final : public Value, private ilist<BasicBlock> {
    using basic_blocks = ilist<BasicBlock>;

  public:
    using basic_blocks::const_iterator;
    using basic_blocks::difference_type;
    using basic_blocks::iterator;
    using basic_blocks::size_type;

    Function(std::string_view name, Type return_type)
        : Value{Type::kNone}, name_(name), return_type_{return_type} {}

    template <std::input_iterator It>
        requires std::convertible_to<std::iter_value_t<It>, Type>
    Function(std::string_view name, Type return_type, It first, It last)
        : Value{Type::kNone}, name_(name), return_type_{return_type}, arguments_(first, last) {}

    Function(std::string_view name, Type return_type, std::initializer_list<Type> ilist)
        : Value{Type::kNone}, name_(name), return_type_{return_type}, arguments_(ilist) {}

    std::string_view name() const noexcept { return name_; }

    Type return_type() const noexcept { return return_type_; }
    auto arguments() const { return std::ranges::ref_view{arguments_}; }

    unsigned get_next_bb_id() const noexcept { return next_bb_id_; }

    template <typename... Args>
    iterator emplace(const_iterator pos, Args &&...args) {
        std::unique_ptr<BasicBlock> bb{new BasicBlock(*this, std::forward<Args>(args)...)};
        ++next_bb_id_;
        return basic_blocks::insert(pos, std::move(bb));
    }

    template <typename... Args>
    reference emplace_front(Args &&...args) {
        return *emplace(begin(), std::forward<Args>(args)...);
    }

    template <typename... Args>
    reference emplace_back(Args &&...args) {
        return *emplace(end(), std::forward<Args>(args)...);
    }

    iterator insert(const_iterator pos, std::unique_ptr<BasicBlock> bb) {
        bb->parent_ = this;
        bb->id_ = next_bb_id_;
        return basic_blocks::insert(pos, std::move(bb));
    }

    reference push_back(std::unique_ptr<BasicBlock> bb) { return *insert(end(), std::move(bb)); }
    reference push_front(std::unique_ptr<BasicBlock> bb) { return *insert(begin(), std::move(bb)); }

    void print(std::ostream &os) const;

    using basic_blocks::empty;
    using basic_blocks::size;

    using basic_blocks::back;
    using basic_blocks::front;

    using basic_blocks::begin;
    using basic_blocks::cbegin;
    using basic_blocks::cend;
    using basic_blocks::crbegin;
    using basic_blocks::crend;
    using basic_blocks::end;
    using basic_blocks::rbegin;
    using basic_blocks::rend;

    using basic_blocks::erase;
    using basic_blocks::pop_back;
    using basic_blocks::pop_front;

  private:
    std::string name_;

    Type return_type_;
    std::vector<Type> arguments_;

    unsigned next_bb_id_ = 0;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_FUNCTION_HPP
