#include <iterator>
#include <memory>

#include "bjac/graphs/dfs.hpp"

#include "bjac/transforms/dce.hpp"

namespace bjac {

namespace {

void remove_unreachable_blocks(Function &f) {
    const DFS<MutFunctionGraphTraits> dfs{f};
    for (auto it = f.begin(), ite = f.end(); it != ite;) {
        if (dfs.contains(std::addressof(*it))) {
            ++it;
        } else {
            auto next_it = std::next(it);
            f.erase(it);
            it = next_it;
        }
    }
}

void remove_unused_instructions(Function &f) {
    for (auto &bb : f) {
        for (auto it = bb.begin(), ite = bb.end(); it != ite;) {
            if (it->get_type() != Type::kVoid && it->users_count() == 0) {
                auto next_it = std::next(it);
                bb.erase(it);
                it = next_it;
            } else {
                ++it;
            }
        }
    }
}

} // unnamed namespace

void DCE::run(Function &f) {
    remove_unreachable_blocks(f);
    remove_unused_instructions(f);
}

} // namespace bjac
