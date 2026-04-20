#include <algorithm>
#include <cassert>
#include <concepts>
#include <iterator>
#include <memory>
#include <ranges>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/container_hash/hash.hpp>

#include "bjac/IR/bounds_check.hpp"
#include "bjac/IR/null_check.hpp"

#include "bjac/transforms/check_elimination.hpp"

#include "bjac/graphs/dfs.hpp"
#include "bjac/graphs/dominator_tree.hpp"

namespace bjac {

namespace {

template <typename T>
concept CheckInstruction =
    std::same_as<T, NullCheckInstruction> || std::same_as<T, BoundsCheckInstruction>;

template <CheckInstruction InstrT>
struct CheckToInput;

template <>
struct CheckToInput<NullCheckInstruction> {
    using InputT = Instruction *;

    static InputT get_input(NullCheckInstruction *check) { return check->get_input(); }
};

template <>
struct CheckToInput<BoundsCheckInstruction> {
    using InputT = std::pair<Instruction *, Instruction *>;

    static InputT get_input(BoundsCheckInstruction *check) {
        return {check->get_array(), check->get_index()};
    }
};

template <CheckInstruction InstrT>
using input_type = typename CheckToInput<InstrT>::InputT;

template <CheckInstruction InstrT>
class CheckHandler {
  public:
    CheckHandler(const DominatorTree<MutFunctionGraphTraits> &dom_tree,
                 std::vector<BasicBlock::iterator> &to_remove)
        : dom_tree_{dom_tree}, to_remove_{to_remove} {}

    void operator()(std::bidirectional_iterator auto instr_it) {
        auto *check = std::addressof(static_cast<InstrT &>(*instr_it));
        auto input = CheckToInput<InstrT>::get_input(check);
        auto [begin, end] = input_to_check.equal_range(input);
        std::ranges::view auto bbs_with_null_checks =
            std::ranges::subrange{begin, end} | std::views::transform([](auto &pair) {
                return std::addressof(pair.second->get_parent());
            });
        const bool is_dominated = std::ranges::any_of(
            bbs_with_null_checks, [&dom_tree = dom_tree_, bb = std::addressof(check->get_parent())](
                                      BasicBlock *maybe_dom) {
                return bb == maybe_dom || dom_tree.is_dominator_of(bb, maybe_dom);
            });
        if (is_dominated) {
            to_remove_.push_back(instr_it);
        } else {
            input_to_check.emplace_hint(begin, std::move(input), check);
        }
    }

  private:
    const DominatorTree<MutFunctionGraphTraits> &dom_tree_;
    std::vector<BasicBlock::iterator> &to_remove_;

    using Hash = boost::hash<input_type<InstrT>>;
    std::unordered_multimap<input_type<InstrT>, Instruction *, Hash> input_to_check;
};

} // unnamed namespace

void CheckEliminationPass::run(Function &f) {
    const DFS<MutFunctionGraphTraits> dfs{f};
    const DominatorTree<MutFunctionGraphTraits> dom_tree{f, dfs};

    std::vector<BasicBlock::iterator> to_remove;

    CheckHandler<NullCheckInstruction> null_check_handler{dom_tree, to_remove};
    CheckHandler<BoundsCheckInstruction> bounds_check_handler{dom_tree, to_remove};

    for (auto *bb : dfs.post_order() | std::views::reverse) {
        for (auto instr_it = bb->begin(), instr_end = bb->end(); instr_it != instr_end;
             ++instr_it) {
            using enum Instruction::Opcode;
            switch (instr_it->get_opcode()) {
            case kNullCheck:
                null_check_handler(instr_it);
                break;
            case kBoundsCheck:
                bounds_check_handler(instr_it);
                break;
            default:
                break;
            }
        }
    }

    for (auto check_it : to_remove) {
        check_it->get_parent().remove_instruction(check_it);
    }
}

} // namespace bjac
