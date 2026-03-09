#ifndef INCLUDE_BJAC_ANALYSIS_LIVENESS_HPP
#define INCLUDE_BJAC_ANALYSIS_LIVENESS_HPP

#include <memory>
#include <ranges>
#include <unordered_map>

#include "bjac/analysis/lifetime.hpp"

namespace bjac {

class Function;
class Instruction;

class LivenessAnalysis final : std::unordered_map<const Instruction *, Lifetime> {
    using base = std::unordered_map<const Instruction *, Lifetime>;

  public:
    explicit LivenessAnalysis(const Function &func);

    const Lifetime &at(const Instruction &instr) const { return base::at(std::addressof(instr)); }

    std::ranges::view auto lifetimes() const { return *this | std::views::values; }

    using base::begin;
    using base::cbegin;
    using base::cend;
    using base::end;
    using base::size;
};

} // namespace bjac

#endif // INCLUDE_BJAC_ANALYSIS_LIVENESS_HPP
