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
#include <unordered_set>
#include <utility>
#include <vector>

#include "bjac/IR/basic_block.hpp"
#include "bjac/IR/ret_instruction.hpp"
#include "bjac/IR/type.hpp"

#include "bjac/utils/ilist.hpp"

namespace bjac {

class CallInstruction;

class Function final : public Value, private ilist<BasicBlock> {
    using basic_blocks = ilist<BasicBlock>;

  public:
    using basic_blocks::const_iterator;
    using basic_blocks::difference_type;
    using basic_blocks::iterator;
    using basic_blocks::size_type;

    Function(std::string_view name, std::unique_ptr<Type> return_type)
        : Value{std::make_unique<NoneType>()}, name_(name), return_type_{std::move(return_type)} {}

    template <std::input_iterator It>
        requires std::convertible_to<std::iter_value_t<It>, Type>
    Function(std::string_view name, std::unique_ptr<Type> return_type, It first, It last)
        : Value{std::make_unique<NoneType>()}, name_(name), return_type_{std::move(return_type)},
          parameters_(first, last) {}

    Function(std::string_view name, std::unique_ptr<Type> return_type,
             std::vector<std::unique_ptr<Type>> parameters)
        : Value{std::make_unique<NoneType>()}, name_(name), return_type_{std::move(return_type)},
          parameters_(std::move(parameters)) {}

    Function(Function &&rhs) = delete("resetting parents of basic blocks would be slow");
    Function &operator=(Function &&rhs) = delete("resetting parents of basic blocks would be slow");

    std::string_view name() const noexcept { return name_; }

    const Type &return_type() const noexcept { return *return_type_; }
    Type::ID return_type_id() const noexcept { return return_type_->id(); }

    auto arguments() const { return std::ranges::ref_view{parameters_}; }

    bool is_recursive() const { return callees_.contains(this); }

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
        bb->id_ = next_bb_id_;
        return basic_blocks::insert(pos, std::move(bb));
    }

    reference push_back(std::unique_ptr<BasicBlock> bb) { return *insert(end(), std::move(bb)); }
    reference push_front(std::unique_ptr<BasicBlock> bb) { return *insert(begin(), std::move(bb)); }

    void add_ret(ReturnInstruction &ret) { rets_.insert(std::addressof(ret)); }
    void remove_ret(ReturnInstruction &ret) { rets_.erase(std::addressof(ret)); }
    std::unsigned_integral auto rets_count() const noexcept { return rets_.size(); }

    void add_callee(Function &callee) { callees_.insert(std::addressof(callee)); }
    void remove_callee(Function &callee) { callees_.erase(std::addressof(callee)); }

    void print(std::ostream &os) const;
    friend std::ostream &operator<<(std::ostream &os, const Function &f);

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

    static void inline_at(CallInstruction &call);
    iterator split_bb_at(Instruction &instr);

  private:
    std::string name_;

    std::unique_ptr<Type> return_type_;
    std::vector<std::unique_ptr<Type>> parameters_;

    std::unordered_set<ReturnInstruction *> rets_;

    struct ConstHash : public std::hash<const Function *> {
        using is_transparent = void;

        using std::hash<const Function *>::operator();

        std::size_t operator()(Function *f) const noexcept {
            return (*this)(static_cast<const Function *>(f));
        }
    };

    std::unordered_set<Function *, ConstHash, std::equal_to<>> callees_;

    unsigned next_bb_id_ = 0;
};

template <typename F>
    requires std::same_as<F, Function> || std::same_as<F, const Function>
struct FunctionGraphTraits {
    using graph_type = F;
    using size_type = typename F::size_type;
    using vertex_handler = std::conditional_t<std::is_const_v<F>, const BasicBlock *, BasicBlock *>;

    static size_type n_vertices(const F &g) { return g.size(); }
    static std::ranges::forward_range auto vertices(F &g) {
        return std::views::transform(g, [](auto &bb) static { return std::addressof(bb); });
    }

    static std::ranges::forward_range auto adjacent_vertices([[maybe_unused]] F &g,
                                                             vertex_handler v) {
        return v->successors();
    }

    static std::ranges::forward_range auto predecessors([[maybe_unused]] F &g, vertex_handler v) {
        return v->predecessors();
    }

    static vertex_handler source(F &g) { return std::addressof(g.front()); }
};

using ConstFunctionGraphTraits = FunctionGraphTraits<const Function>;
using MutFunctionGraphTraits = FunctionGraphTraits<Function>;

} // namespace bjac

#endif // INCLUDE_BJAC_IR_FUNCTION_HPP
