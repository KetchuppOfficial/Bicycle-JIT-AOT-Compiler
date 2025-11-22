#ifndef INCLUDE_BJAC_TRANSFORMS_CONSTANT_FOLDING_HPP
#define INCLUDE_BJAC_TRANSFORMS_CONSTANT_FOLDING_HPP

#include "bjac/transforms/pass.hpp"

namespace bjac {

class ConstantFoldingPass final : public PassMixin<ConstantFoldingPass> {
  public:
    ConstantFoldingPass() = default;

    void run(Function &f);
};

} // namespace bjac

#endif // INCLUDE_BJAC_TRANSFORMS_CONSTANT_FOLDING_HPP
