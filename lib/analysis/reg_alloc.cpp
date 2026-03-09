#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <ranges>
#include <vector>

#include "bjac/analysis/liveness.hpp"
#include "bjac/analysis/reg_alloc.hpp"

#include "bjac/IR/function.hpp"

namespace bjac {

RegAlloc::RegAlloc(const Function &func, std::size_t free_regs_count) {
    constexpr bool kFree = true;
    constexpr bool kOccupied = false;
    std::vector<bool> free_regs(free_regs_count, kFree);

    struct LTInfo {
        const Instruction *instr;
        const Lifetime *lifetime;
    };

    struct EndPointCompare {
        bool operator()(const LTInfo &lhs, const LTInfo &rhs) const {
            return lhs.lifetime->end_point() < rhs.lifetime->end_point();
        }
    };

    std::map<LTInfo, std::size_t, EndPointCompare> active;

    auto expire_old_intervals = [&active, &free_regs](std::size_t current_point) -> void {
        for (auto it = active.begin(), ite = active.end(); it != ite;) {
            auto active_it = it++;
            const auto [info, reg] = *active_it;
            if (info.lifetime->end_point() > current_point) {
                return;
            }
            active.erase(active_it);
            free_regs[reg] = kFree;
        }
    };

    auto spill_at_interval = [this, &active](LTInfo current_info) -> const Instruction * {
        assert(!active.empty());

        auto spill_it = std::prev(active.end());
        const auto [spill_info, spill_reg] = *spill_it;
        if (spill_info.lifetime->end_point() <= current_info.lifetime->end_point()) {
            return current_info.instr;
        }

        instr_to_reg_.try_emplace(current_info.instr, Storage::Kind::kRegister, spill_reg);

        auto node_handle = active.extract(spill_it);
        node_handle.key() = current_info;
        active.insert(std::move(node_handle));

        return spill_info.instr;
    };

    const auto sorted_lifetimes = [&func] {
        const LivenessAnalysis lifetimes{func};
        std::vector<LTInfo> lifetime_infos{
            std::from_range,
            lifetimes | std::views::transform([](const auto &pair) static -> LTInfo {
                return {.instr = pair.first, .lifetime = std::addressof(pair.second)};
            })};
        std::ranges::sort(lifetime_infos, {}, [](const LTInfo &info) static {
            const auto *lt = info.lifetime;
            assert(lt);
            assert(!lt->empty());
            return lt->start_point();
        });
        return lifetime_infos;
    }();

    for (std::size_t stack_loc = 0; const LTInfo &info : sorted_lifetimes) {
        expire_old_intervals(info.lifetime->start_point());

        if (active.size() == free_regs_count) {
            const auto *instr = spill_at_interval(info);
            instr_to_reg_[instr] = {.kind = Storage::Kind::kStackSlot, .index = stack_loc++};
        } else {
            auto free_reg_it = std::ranges::find(free_regs, kFree);
            assert(free_reg_it != free_regs.end());
            *free_reg_it = kOccupied;
            const auto free_reg = free_reg_it - free_regs.begin();

            instr_to_reg_.try_emplace(info.instr, Storage::Kind::kRegister, free_reg);

            active.emplace(info, free_reg);
        }
    }
}

} // namespace bjac
