#ifndef INCLUDE_BJAC_ANALYSIS_LIVENESS_HPP
#define INCLUDE_BJAC_ANALYSIS_LIVENESS_HPP

#include <memory>
#include <ranges>
#include <unordered_map>

#include "bjac/analysis/lifetime.hpp"

namespace bjac {

class Function;
class Instruction;

class LivenessAnalysis final {
  public:
    explicit LivenessAnalysis(const Function &func);

    const Lifetime &at(const Instruction &instr) const {
        return lifetimes_.at(std::addressof(instr));
    }

    std::ranges::forward_range auto lifetimes() const { return std::views::all(lifetimes_); }

  private:
    std::unordered_map<const Instruction *, Lifetime> lifetimes_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_ANALYSIS_LIVENESS_HPP
