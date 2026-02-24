#ifndef INCLUDE_BJAC_ANALYSIS_LIVENESS_HPP
#define INCLUDE_BJAC_ANALYSIS_LIVENESS_HPP

#include <memory>
#include <unordered_map>

#include "bjac/analysis/lifetime.hpp"

#include "bjac/IR/function.hpp"
#include "bjac/IR/instruction.hpp"

namespace bjac {

class LivenessAnalysis final {
  public:
    explicit LivenessAnalysis(const Function &func);

    const Lifetime &at(const Instruction &instr) const {
        return lifetimes_.at(std::addressof(instr));
    }

  private:
    std::unordered_map<const Instruction *, Lifetime> lifetimes_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_ANALYSIS_LIVENESS_HPP
