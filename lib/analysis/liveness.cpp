#include <cassert>
#include <cstddef>
#include <memory>
#include <ranges>
#include <unordered_map>
#include <unordered_set>

#include "bjac/analysis/liveness.hpp"

#include "bjac/graphs/linear_order.hpp"

#include "bjac/IR/function.hpp"
#include "bjac/IR/instruction.hpp"
#include "bjac/IR/phi_instruction.hpp"

namespace bjac {

LivenessAnalysis::LivenessAnalysis(const Function &func) {
    using LiveIn = std::unordered_set<const Instruction *>;
    using Segment = Lifetime::Segment;

    const DFS<ConstFunctionGraphTraits> dfs{func};
    const DominatorTree<ConstFunctionGraphTraits> dom_tree{func, dfs};
    const LoopTree<ConstFunctionGraphTraits> loop_tree{func, dfs, dom_tree};
    const LinearOrder<ConstFunctionGraphTraits> linear_order{func, dom_tree, loop_tree};

    std::unordered_map<const BasicBlock *, LiveIn> live_ins;
    live_ins.reserve(func.size());

    std::unordered_map<const Instruction *, std::size_t> numbering;
    for (std::size_t n = 0; const auto *bb : linear_order) {
        std::ranges::view auto phi_instructions = bb->phi_instructions();
        for (const auto &instr : phi_instructions) {
            numbering.emplace(std::addressof(instr), n);
        }

        n += !phi_instructions.empty();

        for (const auto &instr : bb->non_phi_instructions()) {
            numbering.emplace(std::addressof(instr), n++);
        }
    }

    for (const auto *bb : linear_order | std::views::reverse) {
        auto &live_in = live_ins[bb];

        for (const auto *succ : bb->successors()) {
            live_in.insert_range(live_ins[succ]);
            for (const auto &phi : succ->phi_instructions()) {
                // TODO: try to deal with constness issues and remove const_cast
                auto *input = static_cast<const PHIInstruction &>(phi).get_value(
                    const_cast<BasicBlock &>(*bb));
                assert(input);
                live_in.insert(input);
            }
        }

        const auto first_instr_n = numbering.at(std::addressof(bb->front()));
        const auto last_instr_n = numbering.at(std::addressof(bb->back()));

        for (const auto *instr : live_in) {
            assert(instr);
            (*this)[instr].add(Segment{first_instr_n, last_instr_n});
        }

        for (const auto &instr : bb->non_phi_instructions() | std::views::reverse) {
            const auto instr_n = numbering.at(std::addressof(instr));

            if (instr.get_type_id() != Type::ID::kVoid) {
                auto &lt = (*this)[std::addressof(instr)];
                auto seg = Segment{instr_n, last_instr_n};
                if (auto it = lt.find(seg); it == lt.end()) {
                    lt.add(seg);
                } else if (it->start() < instr_n) {
                    const auto old_seg_end = it->end();
                    lt.remove(it);
                    lt.add(Segment{instr_n, old_seg_end});
                } else {
                    assert(it->start() == instr_n);
                }

                live_in.erase(std::addressof(instr));
            }

            for (const auto *input : instr.inputs()) {
                assert(input);
                (*this)[input].add(Segment{first_instr_n, instr_n});
                live_in.insert(input);
            }
        }

        for (const auto &phi : bb->phi_instructions()) {
            live_in.erase(std::addressof(phi));
        }

        if (loop_tree.is_header(bb)) {
            const auto &last_loop_instr = loop_tree.get_loop(bb).vertices().back()->back();
            const auto loop_end = numbering.at(std::addressof(last_loop_instr));
            for (const auto *instr : live_in) {
                assert(instr);
                (*this)[instr].add(Segment{first_instr_n, loop_end});
            }
        }
    }
}

} // namespace bjac
