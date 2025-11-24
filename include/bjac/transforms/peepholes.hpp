#ifndef INCLUDE_BJAC_TRANSFORMS_PEEPHOLES_HPP
#define INCLUDE_BJAC_TRANSFORMS_PEEPHOLES_HPP

#include "bjac/transforms/pass.hpp"

namespace bjac {

class PeepholePass final : public PassMixin<PeepholePass> {
  public:
    PeepholePass() = default;

    void run(Function &f);
};

} // namespace bjac

#endif // INCLUDE_BJAC_TRANSFORMS_PEEPHOLES_HPP
